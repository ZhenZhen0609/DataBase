#include "queryengine.h"
#include "constraintmanager.h"
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
    records = res.data.value<QJsonArray>();
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
    qDebug() << QString("[QueryEngine] executeSelect: table=%1 cols=%2 user=%3 db=%4")
                .arg(tableName).arg(columns.join(",")).arg(m_currentUser).arg(m_currentDb);

    QList<Field> fields;
    QJsonArray records;
    Response err;
    if (!loadTableData(tableName, fields, records, err)) {
        qDebug() << QString("[QueryEngine] executeSelect: loadTableData FAILED: %1").arg(err.message);
        return err;
    }

    qDebug() << QString("[QueryEngine] executeSelect: loaded %1 fields, %2 raw records").arg(fields.size()).arg(records.size());

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

    qDebug() << QString("[QueryEngine] executeSelect: returning %1 rows").arg(records.size());
    QJsonDocument resultDoc(records);
    QString jsonStr = QString::fromUtf8(resultDoc.toJson(QJsonDocument::Compact));
    return {ResponseStatus::OK, QString("Selected %1 rows").arg(records.size()), QVariant(jsonStr)};
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
    qDebug() << "[UPDATE] START: table=" << tableName << " user=" << m_currentUser << " db=" << m_currentDb;

    StorageManager storage;
    QList<Field> fields = storage.loadTableSchema(m_currentUser, m_currentDb, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::TABLE_NOT_FOUND, "Table not found: " + tableName, QVariant()};
    }

    QString pkField = "id";
    for (const Field &f : fields) {
        if (f.isPrimaryKey) { pkField = f.name; break; }
    }

    QByteArray raw = storage.readTableData(m_currentUser, m_currentDb, tableName);
    QJsonArray allRecords;
    {
        QDataStream ds(&raw, QIODevice::ReadOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        while (!ds.atEnd()) {
            qint64 sz = 0;
            if (ds.readRawData(reinterpret_cast<char*>(&sz), sizeof(qint64)) != sizeof(qint64)) break;
            QByteArray chunk(sz, 0);
            if (ds.readRawData(chunk.data(), sz) != sz) break;
            QDataStream rs(&chunk, QIODevice::ReadOnly);
            rs.setByteOrder(QDataStream::LittleEndian);
            int fc; rs >> fc;
            QJsonObject obj;
            for (const Field &f : fields) {
                switch (f.type) {
                case FieldType::INT:    { int v;    rs >> v; obj[f.name] = v; break; }
                case FieldType::DOUBLE: { double v; rs >> v; obj[f.name] = v; break; }
                case FieldType::BOOLEAN:{ bool v;   rs >> v; obj[f.name] = v; break; }
                case FieldType::TEXT:   { QString v;rs >> v; obj[f.name] = v; break; }
                }
            }
            QString ca; rs >> ca; obj["_created_at"] = ca;
            for (const Field &f : fields) {
                if (f.isEncrypted && obj.contains(f.name))
                    obj[f.name] = ConstraintManager::decrypt(obj[f.name].toString());
            }
            allRecords.append(obj);
        }
    }

    qDebug() << "[UPDATE] loaded" << allRecords.size() << "records";

    std::unique_ptr<ConditionNode> cond;
    if (!whereClause.trimmed().isEmpty()) {
        cond = ConditionParser::parse(whereClause.trimmed());
        if (!cond) return {ResponseStatus::ERROR, "WHERE parse failed: " + whereClause, QVariant()};
    }

    QList<QJsonObject> finalRecords;
    QStringList updatedPks;
    int count = 0;

    for (const QJsonValue &v : allRecords) {
        QJsonObject obj = v.toObject();
        if (!cond || cond->evaluate(obj, fields)) {
            for (const QString &key : assignments.keys()) {
                obj[key] = assignments[key];
            }
            updatedPks.append(obj[pkField].toVariant().toString());
            count++;
        }
        finalRecords.append(obj);
    }

    qDebug() << "[UPDATE] updated" << count << "records";

    if (count == 0) return {ResponseStatus::OK, "No rows updated", QVariant()};

    for (const QString &pk : updatedPks) {
        ConstraintManager::cascadeUpdate(m_currentUser, m_currentDb, tableName, pk, assignments);
    }

    QByteArray newData;
    {
        QDataStream ds(&newData, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        for (const QJsonObject &obj : finalRecords) {
            QJsonObject encObj = obj;
            for (const Field &f : fields) {
                if (f.isEncrypted && encObj.contains(f.name))
                    encObj[f.name] = ConstraintManager::encrypt(encObj[f.name].toString());
            }
            QByteArray chunk;
            QDataStream rs(&chunk, QIODevice::WriteOnly);
            rs.setByteOrder(QDataStream::LittleEndian);
            int fcount = fields.size();
            rs << fcount;
            for (const Field &f : fields) {
                QJsonValue val = encObj.value(f.name);
                switch (f.type) {
                case FieldType::INT:    rs << val.toInt(); break;
                case FieldType::DOUBLE: rs << val.toDouble(); break;
                case FieldType::BOOLEAN:rs << val.toBool(); break;
                case FieldType::TEXT:   rs << val.toString(); break;
                }
            }
            rs << obj["_created_at"].toString();
            qint64 s = chunk.size();
            ds.writeRawData(reinterpret_cast<const char*>(&s), sizeof(qint64));
            ds.writeRawData(chunk.data(), s);
        }
    }

    storage.writeTableData(m_currentUser, m_currentDb, tableName, newData);
    qDebug() << "[UPDATE] DONE, wrote" << finalRecords.size() << "records";
    return {ResponseStatus::OK, QString("Updated %1 rows").arg(count), QVariant()};
}

Response QueryEngine::executeDelete(const QString &tableName, const QString &whereClause)
{
    qDebug() << "========================================";
    qDebug() << "[DELETE] START: table=" << tableName << "user=" << m_currentUser << "db=" << m_currentDb;
    qDebug() << "[DELETE] WHERE raw=[" << whereClause << "]";

    StorageManager storage;

    QList<Field> fields = storage.loadTableSchema(m_currentUser, m_currentDb, tableName);
    if (fields.isEmpty()) {
        qDebug() << "[DELETE] FAIL: unknown table";
        return {ResponseStatus::TABLE_NOT_FOUND, "Table not found: " + tableName, QVariant()};
    }

    QString pkField = "id";
    for (const Field &f : fields) {
        if (f.isPrimaryKey) { pkField = f.name; break; }
    }
    qDebug() << "[DELETE] pkField=" << pkField << " fields=" << fields.size();

    QByteArray raw = storage.readTableData(m_currentUser, m_currentDb, tableName);
    QJsonArray allRecords;
    {
        QDataStream ds(&raw, QIODevice::ReadOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        while (!ds.atEnd()) {
            qint64 sz = 0;
            if (ds.readRawData(reinterpret_cast<char*>(&sz), sizeof(qint64)) != sizeof(qint64)) break;
            QByteArray chunk(sz, 0);
            if (ds.readRawData(chunk.data(), sz) != sz) break;
            QDataStream rs(&chunk, QIODevice::ReadOnly);
            rs.setByteOrder(QDataStream::LittleEndian);
            int fc; rs >> fc;
            QJsonObject obj;
            for (const Field &f : fields) {
                switch (f.type) {
                case FieldType::INT:    { int v;    rs >> v; obj[f.name] = v; break; }
                case FieldType::DOUBLE: { double v; rs >> v; obj[f.name] = v; break; }
                case FieldType::BOOLEAN:   { bool v;   rs >> v; obj[f.name] = v; break; }
                case FieldType::TEXT:   { QString v;rs >> v; obj[f.name] = v; break; }
                }
            }
            QString ca; rs >> ca; obj["_created_at"] = ca;

            for (const Field &f : fields) {
                if (f.isEncrypted && obj.contains(f.name)) {
                    obj[f.name] = ConstraintManager::decrypt(obj[f.name].toString());
                }
            }

            allRecords.append(obj);
        }
    }
    qDebug() << "[DELETE] loaded" << allRecords.size() << "records from disk";
    for (int i = 0; i < allRecords.size(); ++i) {
        QJsonObject r = allRecords[i].toObject();
        qDebug() << "[DELETE]   rec[" << i << "] pk=" << r[pkField].toVariant().toString();
    }

    QList<QJsonObject> remaining;
    QStringList deletedPks;

    if (whereClause.trimmed().isEmpty()) {
        qDebug() << "[DELETE] WARNING: no WHERE clause, deleting ALL";
        for (const QJsonValue &v : allRecords) {
            deletedPks.append(v.toObject()[pkField].toVariant().toString());
        }
    } else {
        auto cond = ConditionParser::parse(whereClause.trimmed());
        if (!cond) {
            qDebug() << "[DELETE] FAIL: WHERE parse error";
            return {ResponseStatus::ERROR, "WHERE parse failed: " + whereClause, QVariant()};
        }

        for (const QJsonValue &v : allRecords) {
            QJsonObject obj = v.toObject();
            bool hit = cond->evaluate(obj, fields);
            QString pk = obj[pkField].toVariant().toString();
            qDebug() << "[DELETE] eval pk=" << pk << " => match=" << hit;
            if (hit) {
                deletedPks.append(pk);
            } else {
                remaining.append(obj);
            }
        }
    }

    qDebug() << "[DELETE] toDelete=" << deletedPks.size() << " remaining=" << remaining.size();

    if (deletedPks.isEmpty()) {
        qDebug() << "[DELETE] DONE: nothing to delete";
        return {ResponseStatus::OK, "No rows deleted", QVariant()};
    }

    for (const QString &pk : deletedPks) {
        qDebug() << "[DELETE] cascading for pk=" << pk;
        ConstraintManager::cascadeDelete(m_currentUser, m_currentDb, tableName, pk);
    }

    QByteArray newData;
    {
        QDataStream ds(&newData, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        for (const QJsonObject &obj : remaining) {
            QJsonObject encObj = obj;
            for (const Field &f : fields) {
                if (f.isEncrypted && encObj.contains(f.name)) {
                    encObj[f.name] = ConstraintManager::encrypt(encObj[f.name].toString());
                }
            }

            QByteArray chunk;
            QDataStream rs(&chunk, QIODevice::WriteOnly);
            rs.setByteOrder(QDataStream::LittleEndian);
            int fcount = fields.size();
            rs << fcount;
            for (const Field &f : fields) {
                QJsonValue val = encObj.value(f.name);
                switch (f.type) {
                case FieldType::INT:    rs << val.toInt(); break;
                case FieldType::DOUBLE: rs << val.toDouble(); break;
                case FieldType::BOOLEAN:   rs << val.toBool(); break;
                case FieldType::TEXT:   rs << val.toString(); break;
                }
            }
            rs << obj["_created_at"].toString();
            qint64 s = chunk.size();
            ds.writeRawData(reinterpret_cast<const char*>(&s), sizeof(qint64));
            ds.writeRawData(chunk.data(), s);
        }
    }

    bool ok = storage.writeTableData(m_currentUser, m_currentDb, tableName, newData);
    qDebug() << "[DELETE] wrote" << remaining.size() << "records, ok=" << ok;
    qDebug() << "========================================";

    if (!ok) {
        return {ResponseStatus::ERROR, "Write failed", QVariant()};
    }
    return {ResponseStatus::OK, QString("Deleted %1 rows").arg(deletedPks.size()), QVariant()};
}
