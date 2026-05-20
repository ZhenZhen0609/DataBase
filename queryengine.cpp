#include "queryengine.h"
#include "sqlparser.h"
#include "constraintmanager.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <QtGlobal>
#include <QFile>
#include <QDir>

QueryEngine::QueryEngine(QObject *parent) : QObject(parent) {}

void QueryEngine::setCurrentUser(const QString &user) { m_currentUser = user; }
void QueryEngine::setCurrentDatabase(const QString &db) { m_currentDb = db; }
void QueryEngine::setParser(SQLParser *parser) { m_parser = parser; }

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
    if (fieldName == "*") return QVariant();

    QVector<double> values;
    for (const QJsonValue &v : groupRecords) {
        QJsonObject obj = v.toObject();
        if (obj.contains(fieldName)) {
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

// 原有的 executeSelect
Response QueryEngine::executeSelect(const QString &tableName, const QStringList &columns,
                                    const QString &whereClause, const QString &orderBy,
                                    const QStringList &groupBy, const QString &having,
                                    int limit, int offset, bool distinct)
{
    qDebug() << "[QueryEngine] executeSelect:" << tableName;
    
    QString expandedSql = expandView(tableName);
    if (!expandedSql.isEmpty()) {
        qDebug() << "[QueryEngine] View expanded to:" << expandedSql;
        return m_parser ? m_parser->parseSQL(expandedSql) : Response{ResponseStatus::ERROR, "No parser", QVariant()};
    }
    
    QList<Field> fields;
    QJsonArray records;
    Response err;
    if (!loadTableData(tableName, fields, records, err)) {
        return err;
    }

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

    bool hasAgg = false;
    for (const QString &col : columns) {
        if (col.trimmed().compare("*", Qt::CaseInsensitive) == 0) continue;
        QString func, field;
        extractAggFunc(col, func, field);
        if (!func.isEmpty()) { hasAgg = true; break; }
    }

    if (!groupBy.isEmpty() || hasAgg) {
        qDebug() << "[QueryEngine] GROUP BY:" << groupBy << "hasAgg:" << hasAgg << "columns:" << columns;
        QMap<QString, QJsonArray> groups;
        for (const auto &val : records) {
            QJsonObject obj = val.toObject();
            QStringList keyParts;
            for (const QString &gb : groupBy) {
                QJsonValue gv = obj.value(gb);
                if (gv.isDouble()) {
                    double dv = gv.toDouble();
                    if (dv == (int)dv) keyParts << QString::number((int)dv);
                    else keyParts << QString::number(dv, 'f', 2);
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
                qDebug() << "[QueryEngine] HAVING:" << having;
                auto havingCond = ConditionParser::parse(having);
                if (!havingCond) {
                    qDebug() << "[QueryEngine] HAVING parse failed";
                } else {
                    bool pass = havingCond->evaluate(out, fields);
                    qDebug() << "[QueryEngine] HAVING evaluate result:" << pass << "out:" << out;
                    if (!pass) continue;
                }
            }
            result.append(out);
        }
        records = result;
    }

    if (!orderBy.isEmpty()) {
        qDebug() << "[QueryEngine] Sorting by:" << orderBy;
        QStringList parts = orderBy.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QString sortField = parts.value(0);
        bool desc = (parts.size() > 1 && parts[1].toUpper() == "DESC");
        qDebug() << "[QueryEngine] sortField:" << sortField << "desc:" << desc;
        
        FieldType sortFieldType = FieldType::TEXT;
        for (const Field &f : fields) {
            if (f.name == sortField) {
                sortFieldType = f.type;
                break;
            }
        }
        qDebug() << "[QueryEngine] sortFieldType:" << (int)sortFieldType;
        
        QVector<QJsonValue> vecRecords;
        for (const QJsonValue &v : records) vecRecords.append(v);
        std::sort(vecRecords.begin(), vecRecords.end(),
                  [=](const QJsonValue &a, const QJsonValue &b) {
                      QJsonObject oa = a.toObject(), ob = b.toObject();
                      QJsonValue va = oa.value(sortField);
                      QJsonValue vb = ob.value(sortField);
                      
                      bool less = false;
                      if (sortFieldType == FieldType::INT || sortFieldType == FieldType::DOUBLE) {
                          double aVal = va.toDouble();
                          double bVal = vb.toDouble();
                          less = aVal < bVal;
                      } else if (sortFieldType == FieldType::BOOLEAN) {
                          bool aVal = va.toBool();
                          bool bVal = vb.toBool();
                          less = aVal < bVal;
                      } else {
                          less = va.toString() < vb.toString();
                      }
                      
                      return desc ? !less : less;
                  });
        records = QJsonArray();
        for (const QJsonValue &v : vecRecords) records.append(v);
        qDebug() << "[QueryEngine] After sort, first record:" << records.first().toObject();
    }

    if (!columns.isEmpty() && !(columns.size()==1 && columns[0].trimmed()=="*")) {
        QJsonArray projected;
        for (const auto &val : records) {
            QJsonObject obj = val.toObject();
            QJsonObject row;
            for (const QString &col : columns) row[col] = obj[col];
            projected.append(row);
        }
        records = projected;
    }

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

    if (offset > 0 || limit >= 0) {
        QJsonArray sliced;
        int start = qMin(offset, records.size());
        int end = records.size();
        if (limit >= 0) end = qMin(start + limit, records.size());
        for (int i = start; i < end; ++i) sliced.append(records[i]);
        records = sliced;
    }

    QJsonDocument resultDoc(records);
    return {ResponseStatus::OK, QString("Selected %1 rows").arg(records.size()), QVariant::fromValue(records)};
}

Response QueryEngine::executeInsert(const QString &tableName, const QStringList &colNames,
                                    const QList<QJsonArray> &rows)
{
    Response res = m_schema.loadTableSchema(m_currentUser, m_currentDb, tableName);
    if (res.status != ResponseStatus::OK) return res;
    TableSchema schema = res.data.value<TableSchema>();

    QStringList effectiveColNames = colNames;
    if (effectiveColNames.isEmpty()) {
        for (const Field &f : schema.fields) effectiveColNames.append(f.name);
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
    qDebug() << "[UPDATE] START: table=" << tableName;
    StorageManager storage;
    QList<Field> fields = storage.loadTableSchema(m_currentUser, m_currentDb, tableName);
    if (fields.isEmpty()) return {ResponseStatus::TABLE_NOT_FOUND, "Table not found: " + tableName, QVariant()};

    QString pkField = "id";
    for (const Field &f : fields) if (f.isPrimaryKey) { pkField = f.name; break; }

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
            for (const QString &key : assignments.keys()) obj[key] = assignments[key];
            updatedPks.append(obj[pkField].toVariant().toString());
            count++;
        }
        finalRecords.append(obj);
    }

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
    return {ResponseStatus::OK, QString("Updated %1 rows").arg(count), QVariant()};
}

Response QueryEngine::executeDelete(const QString &tableName, const QString &whereClause)
{
    qDebug() << "[DELETE] START: table=" << tableName;
    StorageManager storage;
    QList<Field> fields = storage.loadTableSchema(m_currentUser, m_currentDb, tableName);
    if (fields.isEmpty()) return {ResponseStatus::TABLE_NOT_FOUND, "Table not found: " + tableName, QVariant()};

    QString pkField = "id";
    for (const Field &f : fields) if (f.isPrimaryKey) { pkField = f.name; break; }

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
                if (f.isEncrypted && obj.contains(f.name))
                    obj[f.name] = ConstraintManager::decrypt(obj[f.name].toString());
            }
            allRecords.append(obj);
        }
    }

    QList<QJsonObject> remaining;
    QStringList deletedPks;
    if (whereClause.trimmed().isEmpty()) {
        for (const QJsonValue &v : allRecords) deletedPks.append(v.toObject()[pkField].toVariant().toString());
    } else {
        auto cond = ConditionParser::parse(whereClause.trimmed());
        if (!cond) return {ResponseStatus::ERROR, "WHERE parse failed: " + whereClause, QVariant()};
        for (const QJsonValue &v : allRecords) {
            QJsonObject obj = v.toObject();
            if (cond->evaluate(obj, fields)) deletedPks.append(obj[pkField].toVariant().toString());
            else remaining.append(obj);
        }
    }

    if (deletedPks.isEmpty()) return {ResponseStatus::OK, "No rows deleted", QVariant()};

    for (const QString &pk : deletedPks) {
        ConstraintManager::cascadeDelete(m_currentUser, m_currentDb, tableName, pk);
    }

    QByteArray newData;
    {
        QDataStream ds(&newData, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        for (const QJsonObject &obj : remaining) {
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
    if (!ok) return {ResponseStatus::ERROR, "Write failed", QVariant()};
    return {ResponseStatus::OK, QString("Deleted %1 rows").arg(deletedPks.size()), QVariant()};
}

// ========================= JOIN 实现（支持别名） =========================
Response QueryEngine::executeJoinSelect(const QString &sql)
{
    qDebug() << "[QueryEngine] executeJoinSelect input:" << sql;
    
    int onIdx = sql.indexOf(QRegularExpression("\\bON\\b", QRegularExpression::CaseInsensitiveOption));
    if (onIdx == -1) {
        return {ResponseStatus::ERROR, "JOIN missing ON clause", QVariant()};
    }
    
    QString beforeOn = sql.left(onIdx).trimmed();
    QString afterOn = sql.mid(onIdx + 2).trimmed();
    
    qDebug() << "[QueryEngine] beforeOn:" << beforeOn;
    qDebug() << "[QueryEngine] afterOn:" << afterOn;
    
    QRegularExpression beforeOnRe(R"(SELECT\s+(.*?)\s+FROM\s+(\w+)(?:\s+(\w+))?\s+JOIN\s+(\w+)(?:\s+(\w+))?)",
                                  QRegularExpression::CaseInsensitiveOption);
    auto beforeMatch = beforeOnRe.match(beforeOn);
    if (!beforeMatch.hasMatch()) {
        return {ResponseStatus::ERROR, "Invalid JOIN syntax", QVariant()};
    }
    
    QString columnsPart = beforeMatch.captured(1).trimmed();
    QString table1Name = beforeMatch.captured(2).trimmed();
    QString table1Alias = beforeMatch.captured(3).trimmed();
    QString table2Name = beforeMatch.captured(4).trimmed();
    QString table2Alias = beforeMatch.captured(5).trimmed();
    
    QString onCondition = afterOn;
    QString remaining;
    
    QRegularExpression remainingRe(R"((.+?)(?:\s+(?:WHERE|ORDER\s+BY|LIMIT)\b\s+(.*))?$)", QRegularExpression::CaseInsensitiveOption);
    auto remainingMatch = remainingRe.match(afterOn);
    if (remainingMatch.hasMatch()) {
        onCondition = remainingMatch.captured(1).trimmed();
        remaining = remainingMatch.captured(2).trimmed();
    }
    
    qDebug() << "[QueryEngine] JOIN: table1=" << table1Name << "alias1=" << table1Alias 
             << "table2=" << table2Name << "alias2=" << table2Alias;
    qDebug() << "[QueryEngine] ON condition:" << onCondition << "remaining:" << remaining;

    // 加载两个表的数据（使用真实表名）
    QList<Field> fields1, fields2;
    QJsonArray records1, records2;
    Response err;
    if (!loadTableData(table1Name, fields1, records1, err)) return err;
    if (!loadTableData(table2Name, fields2, records2, err)) return err;
    
    qDebug() << "[QueryEngine] Table1" << table1Name << "records:" << records1.size();
    qDebug() << "[QueryEngine] Table2" << table2Name << "records:" << records2.size();

    // 将 ON 条件中的别名替换为真实字段名
    QString actualOn = onCondition;
    // 处理表名.字段名格式
    actualOn.replace(QRegularExpression("\\b" + table1Name + "\\."), "");
    if (!table1Alias.isEmpty()) {
        actualOn.replace(QRegularExpression("\\b" + table1Alias + "\\."), "");
    }
    
    // 对于table2的字段，如果有重名，需要加上表名前缀
    QStringList table1FieldNames;
    for (const Field &f : fields1) table1FieldNames << f.name;
    
    QString table2Prefix = table2Name + ".";
    if (!table2Alias.isEmpty()) {
        actualOn.replace(QRegularExpression("\\b" + table2Alias + "\\."), table2Prefix);
    } else {
        actualOn.replace(QRegularExpression("\\b" + table2Name + "\\."), table2Prefix);
    }
    
    qDebug() << "[QueryEngine] actualOn after replace:" << actualOn;
    
    auto cond = ConditionParser::parse(actualOn);
    if (!cond) {
        return {ResponseStatus::ERROR, "Failed to parse JOIN ON condition: " + onCondition, QVariant()};
    }

    // 构建合并字段列表（处理重名）
    QList<Field> allFields = fields1;
    for (const Field &f : fields2) {
        bool dup = false;
        for (const Field &f1 : fields1) if (f1.name == f.name) { dup = true; break; }
        if (dup) {
            Field renamed = f;
            renamed.name = table2Name + "." + f.name;
            allFields.append(renamed);
        } else {
            allFields.append(f);
        }
    }
    
    qDebug() << "[QueryEngine] allFields count:" << allFields.size();
    for (const Field &f : allFields) qDebug() << "  field:" << f.name;

    // 笛卡尔积 + ON 过滤
    QJsonArray resultRecords;
    int matchCount = 0;
    int testCount = 0;
    for (const QJsonValue &v1 : records1) {
        QJsonObject obj1 = v1.toObject();
        for (const QJsonValue &v2 : records2) {
            QJsonObject obj2 = v2.toObject();
            QJsonObject merged;
            for (const Field &f : fields1) merged[f.name] = obj1[f.name];
            for (const Field &f : fields2) {
                QString key = f.name;
                for (const Field &f1 : fields1) if (f1.name == f.name) { key = table2Name + "." + f.name; break; }
                merged[key] = obj2[f.name];
            }
            
            if (testCount < 3) {
                qDebug() << "[QueryEngine] Test merge" << testCount << ": dept_id=" << merged["dept_id"] << "departments.id=" << merged["departments.id"];
                testCount++;
            }
            
            if (cond->evaluate(merged, allFields)) {
                resultRecords.append(merged);
                matchCount++;
            }
        }
    }
    
    qDebug() << "[QueryEngine] Cartesian product:" << (records1.size() * records2.size()) << "matches:" << matchCount;

    // 处理剩余的 WHERE, ORDER BY, LIMIT
    if (!remaining.isEmpty()) {
        QRegularExpression whereRe("\\bWHERE\\b\\s+(.+?)(?=\\b(ORDER\\s+BY|LIMIT)\\b|$)", QRegularExpression::CaseInsensitiveOption);
        auto whereMatch = whereRe.match(remaining);
        QString whereClause;
        if (whereMatch.hasMatch()) whereClause = whereMatch.captured(1).trimmed();

        QRegularExpression orderRe("\\bORDER\\s+BY\\b\\s+(.+?)(?=\\b(LIMIT)\\b|$)", QRegularExpression::CaseInsensitiveOption);
        auto orderMatch = orderRe.match(remaining);
        QString orderBy;
        if (orderMatch.hasMatch()) orderBy = orderMatch.captured(1).trimmed();

        QRegularExpression limitRe("\\bLIMIT\\b\\s+(\\d+)(?:\\s+OFFSET\\s+(\\d+))?", QRegularExpression::CaseInsensitiveOption);
        auto limitMatch = limitRe.match(remaining);
        int limit = -1, offset = 0;
        if (limitMatch.hasMatch()) {
            limit = limitMatch.captured(1).toInt();
            if (!limitMatch.captured(2).isEmpty()) offset = limitMatch.captured(2).toInt();
        }

        if (!whereClause.isEmpty()) {
            auto whereCond = ConditionParser::parse(whereClause);
            if (!whereCond) return {ResponseStatus::ERROR, "Failed to parse WHERE clause after JOIN", QVariant()};
            QJsonArray filtered;
            for (const auto &val : resultRecords) {
                QJsonObject obj = val.toObject();
                if (whereCond->evaluate(obj, allFields)) filtered.append(obj);
            }
            resultRecords = filtered;
        }

        if (!orderBy.isEmpty()) {
            QStringList parts = orderBy.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            QString sortField = parts.value(0);
            bool desc = (parts.size() > 1 && parts[1].toUpper() == "DESC");
            QVector<QJsonValue> vec;
            for (const QJsonValue &v : resultRecords) vec.append(v);
            std::sort(vec.begin(), vec.end(),
                      [&](const QJsonValue &a, const QJsonValue &b) {
                          QVariant va = a.toObject().value(sortField).toVariant();
                          QVariant vb = b.toObject().value(sortField).toVariant();
                          if (desc) return va.toString() > vb.toString();
                          else return va.toString() < vb.toString();
                      });
            resultRecords = QJsonArray();
            for (const QJsonValue &v : vec) resultRecords.append(v);
        }

        if (offset > 0 || limit >= 0) {
            QJsonArray sliced;
            int start = qMin(offset, resultRecords.size());
            int end = resultRecords.size();
            if (limit >= 0) end = qMin(start + limit, resultRecords.size());
            for (int i = start; i < end; ++i) sliced.append(resultRecords[i]);
            resultRecords = sliced;
        }
    }

    // 投影指定的列（支持表别名.列名）
    if (columnsPart != "*") {
        QStringList cols = columnsPart.split(',', Qt::SkipEmptyParts);
        for (QString &c : cols) c = c.trimmed();
        QJsonArray projected;
        for (const QJsonValue &val : resultRecords) {
            QJsonObject obj = val.toObject();
            QJsonObject row;
            for (const QString &c : cols) {
                // 处理形如 "e.name" 的列引用
                QString colName = c;
                if (colName.contains('.')) {
                    colName = colName.split('.').last();
                }
                row[c] = obj[colName];
            }
            projected.append(row);
        }
        resultRecords = projected;
    }

    QJsonDocument doc(resultRecords);
    return {ResponseStatus::OK, QString("JOIN returned %1 rows").arg(resultRecords.size()), QVariant::fromValue(resultRecords)};
}

// ========================= UNION 实现 =========================
Response QueryEngine::executeUnion(const QString &leftSql, const QString &rightSql, bool distinct)
{
    // 此函数保留给未来可能需要直接调用的场景，目前 UNION 由 SQLParser 处理并调用 mergeUnion
    return {ResponseStatus::ERROR, "executeUnion should not be called directly; use mergeUnion instead", QVariant()};
}

Response QueryEngine::mergeUnion(const QJsonArray &leftRows, const QJsonArray &rightRows, bool distinct)
{
    QJsonArray result = leftRows;
    for (const QJsonValue &v : rightRows) result.append(v);
    if (distinct) {
        QSet<QString> seen;
        QJsonArray unique;
        for (const QJsonValue &v : result) {
            QString key = QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact);
            if (!seen.contains(key)) {
                seen.insert(key);
                unique.append(v);
            }
        }
        result = unique;
    }
    QJsonDocument doc(result);
    return {ResponseStatus::OK, QString("UNION returned %1 rows").arg(result.size()), QVariant::fromValue(result)};
}

// ========================= 事务管理 =========================
Response QueryEngine::executeBeginTransaction()
{
    if (m_currentUser.isEmpty() || m_currentDb.isEmpty()) {
        return {ResponseStatus::ERROR, "请先选择数据库", QVariant()};
    }
    StorageManager storage;
    bool ok = storage.beginTransaction(m_currentUser, m_currentDb);
    if (ok) {
        return {ResponseStatus::OK, "事务已开始", QVariant()};
    }
    return {ResponseStatus::ERROR, "开始事务失败", QVariant()};
}

Response QueryEngine::executeCommit()
{
    if (m_currentUser.isEmpty() || m_currentDb.isEmpty()) {
        return {ResponseStatus::ERROR, "请先选择数据库", QVariant()};
    }
    StorageManager storage;
    bool ok = storage.commitTransaction(m_currentUser, m_currentDb);
    if (ok) {
        return {ResponseStatus::OK, "事务已提交", QVariant()};
    }
    return {ResponseStatus::ERROR, "提交事务失败", QVariant()};
}

Response QueryEngine::executeRollback()
{
    if (m_currentUser.isEmpty() || m_currentDb.isEmpty()) {
        return {ResponseStatus::ERROR, "请先选择数据库", QVariant()};
    }
    StorageManager storage;
    bool ok = storage.rollbackTransaction(m_currentUser, m_currentDb);
    if (ok) {
        return {ResponseStatus::OK, "事务已回滚", QVariant()};
    }
    return {ResponseStatus::ERROR, "回滚事务失败", QVariant()};
}

// ========================= 视图实现 =========================
Response QueryEngine::executeCreateView(const QString &viewName, const QString &selectSql)
{
    if (m_currentUser.isEmpty() || m_currentDb.isEmpty()) {
        return {ResponseStatus::ERROR, "Not logged in or no database selected", QVariant()};
    }
    QString viewsPath = Config::dataPath() + m_currentUser + "/" + m_currentDb + "/views.json";
    QFile file(viewsPath);
    QJsonObject views;
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly)) return {ResponseStatus::ERROR, "Cannot read views.json", QVariant()};
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) views = doc.object();
        file.close();
    }
    views[viewName] = selectSql;
    if (!file.open(QIODevice::WriteOnly)) return {ResponseStatus::ERROR, "Cannot write views.json", QVariant()};
    file.write(QJsonDocument(views).toJson(QJsonDocument::Indented));
    file.close();
    return {ResponseStatus::OK, QString("View '%1' created successfully").arg(viewName), QVariant()};
}

Response QueryEngine::executeDropView(const QString &viewName)
{
    QString viewsPath = Config::dataPath() + m_currentUser + "/" + m_currentDb + "/views.json";
    QFile file(viewsPath);
    if (!file.exists()) return {ResponseStatus::ERROR, "No views exist", QVariant()};
    if (!file.open(QIODevice::ReadOnly)) return {ResponseStatus::ERROR, "Cannot read views.json", QVariant()};
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return {ResponseStatus::ERROR, "Invalid views.json", QVariant()};
    QJsonObject views = doc.object();
    if (!views.contains(viewName)) return {ResponseStatus::ERROR, QString("View '%1' not found").arg(viewName), QVariant()};
    views.remove(viewName);
    if (!file.open(QIODevice::WriteOnly)) return {ResponseStatus::ERROR, "Cannot write views.json", QVariant()};
    file.write(QJsonDocument(views).toJson(QJsonDocument::Indented));
    file.close();
    return {ResponseStatus::OK, QString("View '%1' dropped").arg(viewName), QVariant()};
}

QString QueryEngine::expandView(const QString &viewName, int depth)
{
    if (depth > 5) return QString();
    QString viewsPath = Config::dataPath() + m_currentUser + "/" + m_currentDb + "/views.json";
    QFile file(viewsPath);
    if (!file.exists()) return QString();
    if (!file.open(QIODevice::ReadOnly)) return QString();
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return QString();
    QJsonObject views = doc.object();
    if (!views.contains(viewName)) return QString();
    QString selectSql = views[viewName].toString();
    QRegularExpression viewRefRe(R"(\bFROM\s+(\w+)\b)", QRegularExpression::CaseInsensitiveOption);
    auto it = viewRefRe.globalMatch(selectSql);
    while (it.hasNext()) {
        auto match = it.next();
        QString refView = match.captured(1);
        QString expanded = expandView(refView, depth+1);
        if (!expanded.isEmpty()) {
            selectSql.replace(match.capturedStart(), match.capturedLength(), "FROM (" + expanded + ") AS " + refView);
        }
    }
    return selectSql;
}
