#include "queryengine.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <QtGlobal>

QueryEngine::QueryEngine(QObject *parent) : QObject(parent) {}

void QueryEngine::setCurrentUser(const QString &user) { m_currentUser = user; }
void QueryEngine::setCurrentDatabase(const QString &db) { m_currentDb = db; }

bool QueryEngine::loadTableData(const QString &tableName, QList<Field> &fields, QJsonArray &records, Response &error) {
    Response res = m_schema.loadTableSchema(m_currentUser, m_currentDb, tableName);
    if (res.status != ResponseStatus::OK) {
        error = res;
        return false;
    }
    TableSchema schema = res.data.value<TableSchema>();
    fields = schema.fields;

    res = m_record.selectAllRecords(m_currentUser, m_currentDb, tableName);
    if (res.status != ResponseStatus::OK) {
        error = res;
        return false;
    }
    records = res.data.toJsonArray();
    return true;
}

Response QueryEngine::rewriteTable(const QString &tableName, const QJsonArray &records, const QList<Field> &fields) {
    Q_UNUSED(fields);
    return m_record.replaceAllRecords(m_currentUser, m_currentDb, tableName, records);
}

// 提取聚合函数名与字段名，支持 COUNT(*)
static QString extractAggFunc(const QString &colExpr, QString &funcName, QString &fieldName) {
    QRegularExpression re(R"(^(\w+)\s*\(\s*(\*|\w+)\s*\)$)");
    auto match = re.match(colExpr.trimmed());
    if (match.hasMatch()) {
        funcName = match.captured(1).toUpper();
        fieldName = match.captured(2);
        return funcName;
    }
    funcName.clear();
    fieldName = colExpr.trimmed();
    return QString();
}

QVariant QueryEngine::computeAggregate(const QString &funcName, const QJsonArray &groupRecords, const QString &fieldName) {
    if (funcName == "COUNT") {
        return groupRecords.size();
    }
    if (groupRecords.isEmpty()) return QVariant();

    if (fieldName == "*") {
        return QVariant();
    }

    QVector<double> values;
    for (const QJsonValue &v : groupRecords) {
        QJsonObject obj = v.toObject();
        if (obj.contains(fieldName)) {
            // 直接 toDouble()，QJsonValue 会自动将字符串或数字转换为 double
            double val = obj[fieldName].toDouble();
            values.append(val);
        }
    }
    if (values.isEmpty()) return QVariant();

    if (funcName == "SUM") {
        double sum = 0;
        for (double d : values) sum += d;
        return sum;
    } else if (funcName == "AVG") {
        double sum = 0;
        for (double d : values) sum += d;
        return sum / values.size();
    } else if (funcName == "MAX") {
        return *std::max_element(values.begin(), values.end());
    } else if (funcName == "MIN") {
        return *std::min_element(values.begin(), values.end());
    }
    return QVariant();
}

Response QueryEngine::executeSelect(const QString &tableName, const QStringList &columns,
                                    const QString &whereClause, const QString &orderBy,
                                    const QStringList &groupBy, const QString &having,
                                    int limit, int offset, bool distinct)
{
    QList<Field> fields;
    QJsonArray records;
    Response err;
    if (!loadTableData(tableName, fields, records, err)) return err;

    // 1. WHERE 过滤
    if (!whereClause.isEmpty()) {
        auto cond = ConditionParser::parse(whereClause);
        if (!cond) return {ResponseStatus::ERROR, "Failed to parse WHERE clause", QVariant()};
        QJsonArray filtered;
        for (const auto &val : records) {
            QJsonObject obj = val.toObject();
            if (cond->evaluate(obj, fields)) filtered.append(obj);
        }
        records = filtered;
    }

    // 2. 检查聚合函数
    bool hasAgg = false;
    for (const QString &col : columns) {
        if (col.trimmed().compare("*", Qt::CaseInsensitive) == 0) continue;
        QString func, field;
        extractAggFunc(col, func, field);
        if (!func.isEmpty()) { hasAgg = true; break; }
    }

    // GROUP BY / 聚合处理
    if (!groupBy.isEmpty() || hasAgg) {
        QMap<QString, QJsonArray> groups;
        for (const auto &val : records) {
            QJsonObject obj = val.toObject();
            QStringList keyParts;
            for (const QString &gb : groupBy) {
                QJsonValue gv = obj.value(gb);
                if (gv.isDouble()) {
                    double dv = gv.toDouble();
                    if (dv == (int)dv)
                        keyParts << QString::number((int)dv);
                    else
                        keyParts << QString::number(dv, 'f', 2);
                } else {
                    keyParts << gv.toString();
                }
            }
            QString key = keyParts.join("|");
            groups[key].append(obj);
        }
        QJsonArray result;
        for (auto it = groups.begin(); it != groups.end(); ++it) {
            QJsonArray groupRecs = it.value();
            QJsonObject out;
            if (!groupBy.isEmpty()) {
                QStringList keyParts = it.key().split('|');
                for (int i = 0; i < groupBy.size(); ++i) {
                    out[groupBy[i]] = groupRecs.first()[groupBy[i]];
                }
            }
            for (const QString &col : columns) {
                if (col.trimmed() == "*") {
                    QJsonObject first = groupRecs.first().toObject();
                    for (const QString &k : first.keys()) out[k] = first[k];
                } else {
                    QString func, field;
                    extractAggFunc(col, func, field);
                    if (!func.isEmpty()) {
                        QVariant aggVal = computeAggregate(func, groupRecs, field);
                        out[col] = aggVal.toDouble();
                    } else {
                        out[col] = groupRecs.first()[col];
                    }
                }
            }
            if (!having.isEmpty()) {
                auto havingCond = ConditionParser::parse(having);
                if (havingCond && !havingCond->evaluate(out, fields)) continue;
            }
            result.append(out);
        }
        records = result;
    }

    // 3. ORDER BY
    if (!orderBy.isEmpty()) {
        QStringList parts = orderBy.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QString sortField = parts.value(0);
        bool desc = (parts.size() > 1 && parts[1].toUpper() == "DESC");

        QVector<QJsonValue> vecRecords;
        for (const QJsonValue &v : records)
            vecRecords.append(v);

        std::sort(vecRecords.begin(), vecRecords.end(),
                  [&](const QJsonValue &a, const QJsonValue &b) {
                      QJsonObject oa = a.toObject(), ob = b.toObject();
                      QVariant va = oa.value(sortField).toVariant();
                      QVariant vb = ob.value(sortField).toVariant();
                      if (desc)
                          return va.toString() > vb.toString();
                      else
                          return va.toString() < vb.toString();
                  });

        records = QJsonArray();
        for (const QJsonValue &v : vecRecords)
            records.append(v);
    }

    // 4. 投影列
    if (!columns.isEmpty() && !(columns.size()==1 && columns[0].trimmed()=="*")) {
        QJsonArray projected;
        for (const auto &val : records) {
            QJsonObject obj = val.toObject();
            QJsonObject row;
            for (const QString &col : columns) {
                row[col] = obj.contains(col) ? obj[col] : obj[col];
            }
            projected.append(row);
        }
        records = projected;
    }

    // 5. DISTINCT
    if (distinct) {
        QJsonArray unique;
        QSet<QString> seen;
        for (const auto &val : records) {
            QJsonObject obj = val.toObject();
            QString key = QJsonDocument(obj).toJson(QJsonDocument::Compact);
            if (!seen.contains(key)) {
                seen.insert(key);
                unique.append(obj);
            }
        }
        records = unique;
    }

    // 6. LIMIT / OFFSET
    if (offset > 0 || limit >= 0) {
        QJsonArray sliced;
        int start = qMin(offset, records.size());
        int end = records.size();
        if (limit >= 0) {
            end = qMin(start + limit, records.size());
        }
        for (int i = start; i < end; ++i) {
            sliced.append(records[i]);
        }
        records = sliced;
    }

    return {ResponseStatus::OK, QString("Selected %1 rows").arg(records.size()), QVariant::fromValue(records)};
}

Response QueryEngine::executeInsert(const QString &tableName, const QStringList &colNames,
                                    const QList<QJsonArray> &rows)
{
    Response res = m_schema.loadTableSchema(m_currentUser, m_currentDb, tableName);
    if (res.status != ResponseStatus::OK) return res;
    TableSchema schema = res.data.value<TableSchema>();

    // 修复：未指定列名时自动按表结构顺序填充
    QStringList effectiveColNames = colNames;
    if (effectiveColNames.isEmpty()) {
        for (const Field &f : schema.fields) {
            effectiveColNames.append(f.name);
        }
    }

    int count = 0;
    for (const QJsonArray &row : rows) {
        QJsonObject obj;
        for (int i = 0; i < effectiveColNames.size() && i < row.size(); ++i) {
            obj[effectiveColNames[i]] = row[i];
        }
        Response ins = m_record.insertRecord(m_currentUser, m_currentDb, tableName, obj);
        if (ins.status != ResponseStatus::OK) return ins;
        count++;
    }
    return {ResponseStatus::OK, QString("Inserted %1 rows").arg(count), QVariant()};
}

Response QueryEngine::executeUpdate(const QString &tableName, const QJsonObject &assignments,
                                    const QString &whereClause)
{
    QList<Field> fields;
    QJsonArray records;
    Response err;
    if (!loadTableData(tableName, fields, records, err)) return err;

    std::unique_ptr<ConditionNode> cond;
    if (!whereClause.isEmpty()) {
        cond = ConditionParser::parse(whereClause);
        if (!cond) return {ResponseStatus::ERROR, "Failed to parse WHERE clause", QVariant()};
    }

    int count = 0;
    for (int i = 0; i < records.size(); ++i) {
        QJsonObject obj = records[i].toObject();
        if (!cond || cond->evaluate(obj, fields)) {
            for (const QString &key : assignments.keys()) {
                obj[key] = assignments[key];
            }
            records[i] = obj;
            count++;
        }
    }

    if (count == 0) return {ResponseStatus::OK, "No rows updated", QVariant()};

    Response wres = rewriteTable(tableName, records, fields);
    if (wres.status != ResponseStatus::OK) return wres;
    return {ResponseStatus::OK, QString("Updated %1 rows").arg(count), QVariant()};
}

Response QueryEngine::executeDelete(const QString &tableName, const QString &whereClause)
{
    QList<Field> fields;
    QJsonArray records;
    Response err;
    if (!loadTableData(tableName, fields, records, err)) return err;

    std::unique_ptr<ConditionNode> cond;
    if (!whereClause.isEmpty()) {
        cond = ConditionParser::parse(whereClause);
        if (!cond) return {ResponseStatus::ERROR, "Failed to parse WHERE clause", QVariant()};
    }

    QJsonArray remaining;
    int count = 0;
    for (const auto &val : records) {
        QJsonObject obj = val.toObject();
        if (cond && cond->evaluate(obj, fields)) {
            count++;
        } else {
            remaining.append(obj);
        }
    }

    if (count == 0) return {ResponseStatus::OK, "No rows deleted", QVariant()};

    Response wres = rewriteTable(tableName, remaining, fields);
    if (wres.status != ResponseStatus::OK) return wres;
    return {ResponseStatus::OK, QString("Deleted %1 rows").arg(count), QVariant()};
}
