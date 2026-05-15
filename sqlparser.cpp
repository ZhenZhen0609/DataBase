#include "sqlparser.h"
#include "storagemanager.h"
#include "queryengine.h"
#include <QStringList>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

SQLParser::SQLParser(QObject *parent) : QObject(parent) {}

void SQLParser::setStorageManager(StorageManager *storage) { m_storage = storage; }
void SQLParser::setCurrentUser(const QString &user) { m_currentUser = user; }
void SQLParser::setCurrentDatabase(const QString &db) { m_currentDB = db; }
void SQLParser::setQueryEngine(QueryEngine *engine) { m_engine = engine; }

// 解析入口，根据语句类型分发
Response SQLParser::parseSQL(const QString &sql)
{
    if (!m_storage) {
        return {ResponseStatus::ERROR, "[SQLParser] StorageManager 未设置", QVariant()};
    }
    if (!m_engine) {
        return {ResponseStatus::ERROR, "[SQLParser] QueryEngine 未设置", QVariant()};
    }

    QString input = sql.trimmed();
    
    // 检查是否有多条SQL语句（用分号分隔）
    if (input.contains(';')) {
        QStringList statements = input.split(';', Qt::SkipEmptyParts);
        QList<QString> stmtList;
        for (QString s : statements) { s = s.trimmed(); if (!s.isEmpty()) stmtList.append(s); }

        if (stmtList.size() == 1) {
            return parseSingleStatement(stmtList[0]);
        }

        QString resultMessage;
        int successCount = 0;
        int errorCount = 0;
        QVariant lastData;

        for (const QString &stmt : stmtList) {
            Response resp = parseSingleStatement(stmt);
            if (resp.status == ResponseStatus::OK) {
                successCount++;
                resultMessage += QString("语句 %1: %2\n").arg(successCount + errorCount).arg(resp.message);
                if (resp.data.isValid() && !resp.data.isNull())
                    lastData = resp.data;
            } else {
                errorCount++;
                resultMessage += QString("语句 %1: 错误 - %2\n").arg(successCount + errorCount).arg(resp.message);
            }
        }
        
        if (errorCount == 0) {
            return {ResponseStatus::OK, QString("成功执行 %1 条语句\n%2").arg(successCount).arg(resultMessage), lastData};
        } else {
            return {ResponseStatus::ERROR, QString("执行完成：成功 %1 条，失败 %2 条\n%3").arg(successCount).arg(errorCount).arg(resultMessage), QVariant()};
        }
    }
    
    // 处理单条语句
    return parseSingleStatement(input);
}

Response SQLParser::parseSingleStatement(const QString &sql)
{
    // 1. 去除首尾空白
    QString trimmed = sql.trimmed();

    // 2. 统一转大写用于关键字判断
    QString upper = trimmed.toUpper();
    QStringList tokens = trimmed.split(' ', Qt::SkipEmptyParts);
    QStringList upperTokens = upper.split(' ', Qt::SkipEmptyParts);

    if (tokens.isEmpty()) {
        return {ResponseStatus::ERROR, "语法错误：空语句", QVariant()};
    }

    // ---------- DML 分支 ----------
    if (upperTokens[0] == "SELECT") {
        return execSelect(trimmed);
    }
    if (upperTokens[0] == "INSERT") {
        return execInsert(trimmed);
    }
    if (upperTokens[0] == "UPDATE") {
        return execUpdate(trimmed);
    }
    if (upperTokens[0] == "DELETE") {
        return execDelete(trimmed);
    }

    // ---------- DDL 分支（原有代码） ----------
    if (tokens.size() < 2)
        return {ResponseStatus::ERROR, "语法错误：指令不完整", QVariant()};

    // 数据库操作
    if (upperTokens[0] == "CREATE" && upperTokens[1] == "DATABASE") {
        if (tokens.size() < 3)
            return {ResponseStatus::ERROR, "语法错误：缺少数据库名称", QVariant()};
        QString dbName = tokens[2];
        return execCreateDatabase(dbName);
    }

    if (upperTokens[0] == "DROP" && upperTokens[1] == "DATABASE") {
        if (tokens.size() < 3)
            return {ResponseStatus::ERROR, "语法错误：缺少数据库名称", QVariant()};
        return execDropDatabase(tokens[2]);
    }

    // 表操作
    if (upperTokens[0] == "CREATE" && upperTokens[1] == "TABLE") {
        // 先将多行SQL压缩成单行，再精确计数配对外括号
        QString compact = trimmed;
        compact.replace(QRegularExpression(R"(\s+)"), " ");
        int parenStart = compact.indexOf('(');
        if (parenStart == -1) {
            return {ResponseStatus::ERROR,
                    "语法错误：CREATE TABLE 格式应为 CREATE TABLE 表名 (字段名 类型, ...)",
                    QVariant()};
        }
        int braceCount = 0;
        int parenEnd = -1;
        for (int j = parenStart; j < compact.length(); ++j) {
            if (compact[j] == '(') braceCount++;
            else if (compact[j] == ')') {
                braceCount--;
                if (braceCount == 0) {
                    parenEnd = j;
                    break;
                }
            }
        }
        if (parenEnd == -1) {
            return {ResponseStatus::ERROR,
                    "语法错误：括号不匹配",
                    QVariant()};
        }
        QString tableName = tokens[2];
        QString fieldsStr = compact.mid(parenStart + 1, parenEnd - parenStart - 1).trimmed();
        return execCreateTable(tableName, fieldsStr);
    }

    if (upperTokens[0] == "DROP" && upperTokens[1] == "TABLE") {
        if (tokens.size() < 3)
            return {ResponseStatus::ERROR, "语法错误：缺少表名称", QVariant()};
        return execDropTable(tokens[2]);
    }

    // ALTER TABLE 操作
    if (upperTokens[0] == "ALTER" && upperTokens[1] == "TABLE") {
        if (tokens.size() < 4)
            return {ResponseStatus::ERROR, "语法错误：ALTER TABLE 格式应为 ALTER TABLE 表名 ADD/DROP/MODIFY 字段名 类型", QVariant()};

        QString tableName = tokens[2];
        QString alterType = upperTokens[3]; // ADD/DROP/MODIFY

        QString fieldStr = trimmed;
        int pos = fieldStr.indexOf(alterType, 0, Qt::CaseInsensitive);
        if (pos != -1) {
            fieldStr = fieldStr.mid(pos + alterType.length()).trimmed();
        }

        return execAlterTable(tableName, alterType, fieldStr);
    }

    return {ResponseStatus::ERROR, "不支持的SQL指令：" + trimmed, QVariant()};
}

// ==================== DML 执行函数 ====================

Response SQLParser::execSelect(const QString &sql) {
    QRegularExpression re(R"(SELECT\s+(.*?)\s+FROM\s+(\w+)(.*))",
                          QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(sql);
    if (!match.hasMatch())
        return {ResponseStatus::ERROR, "Invalid SELECT syntax", QVariant()};

    QString colsPart = match.captured(1).trimmed();
    QString tableName = match.captured(2).trimmed();
    QString rest = match.captured(3).trimmed();

    // 解析列名
    QStringList columns;
    if (colsPart != "*") {
        QStringList rawCols = colsPart.split(',', Qt::SkipEmptyParts);
        for (QString c : rawCols) {
            columns.append(c.trimmed());
        }
    } else {
        columns.append("*");
    }

    // 提取 WHERE 子句
    QString whereClause;
    QRegularExpression whereRe("\\bWHERE\\b\\s+(.+?)(?=\\b(ORDER\\s+BY|GROUP\\s+BY|HAVING|LIMIT)\\b|$)",
                               QRegularExpression::CaseInsensitiveOption);
    auto whereMatch = whereRe.match(rest);
    if (whereMatch.hasMatch()) {
        whereClause = whereMatch.captured(1).trimmed();
    }

    // 提取 ORDER BY
    QString orderBy;
    QRegularExpression orderRe("\\bORDER\\s+BY\\b\\s+(.+?)(?=\\b(WHERE|GROUP\\s+BY|HAVING|LIMIT)\\b|$)",
                               QRegularExpression::CaseInsensitiveOption);
    auto orderMatch = orderRe.match(rest);
    if (orderMatch.hasMatch()) {
        orderBy = orderMatch.captured(1).trimmed();
    }

    // 提取 GROUP BY
    QStringList groupBy;
    QRegularExpression groupRe("\\bGROUP\\s+BY\\b\\s+(.+?)(?=\\b(WHERE|ORDER\\s+BY|HAVING|LIMIT)\\b|$)",
                               QRegularExpression::CaseInsensitiveOption);
    auto groupMatch = groupRe.match(rest);
    if (groupMatch.hasMatch()) {
        QString gb = groupMatch.captured(1).trimmed();
        QStringList rawGb = gb.split(',', Qt::SkipEmptyParts);
        for (QString g : rawGb) {
            groupBy.append(g.trimmed());
        }
    }

    // 提取 HAVING
    QString having;
    QRegularExpression havingRe("\\bHAVING\\b\\s+(.+?)(?=\\b(WHERE|ORDER\\s+BY|GROUP\\s+BY|LIMIT)\\b|$)",
                                QRegularExpression::CaseInsensitiveOption);
    auto havingMatch = havingRe.match(rest);
    if (havingMatch.hasMatch()) {
        having = havingMatch.captured(1).trimmed();
    }

    // 提取 LIMIT / OFFSET
    int limit = -1;
    int offset = 0;
    QRegularExpression limitRe("\\bLIMIT\\b\\s+(\\d+)(?:\\s+OFFSET\\s+(\\d+))?",
                               QRegularExpression::CaseInsensitiveOption);
    auto limitMatch = limitRe.match(rest);
    if (limitMatch.hasMatch()) {
        limit = limitMatch.captured(1).toInt();
        if (!limitMatch.captured(2).isEmpty()) {
            offset = limitMatch.captured(2).toInt();
        }
    }

    // DISTINCT
    bool distinct = sql.toUpper().contains("DISTINCT");

    return m_engine->executeSelect(tableName, columns, whereClause, orderBy,
                                   groupBy, having, limit, offset, distinct);
}

Response SQLParser::execInsert(const QString &sql) {
    QRegularExpression re(R"(INSERT\s+INTO\s+(\w+)\s*(?:\(([^)]*)\))?\s*VALUES\s*(.+))",
                          QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(sql);
    if (!match.hasMatch())
        return {ResponseStatus::ERROR, "Invalid INSERT syntax", QVariant()};

    QString tableName = match.captured(1);
    QString colsPart = match.captured(2).trimmed();
    QString valsPart = match.captured(3).trimmed();

    QStringList colNames;
    if (!colsPart.isEmpty()) {
        QStringList rawCols = colsPart.split(',', Qt::SkipEmptyParts);
        for (QString c : rawCols) {
            colNames.append(c.trimmed());
        }
    }

    // 解析多行值
    QList<QJsonArray> rows;
    QRegularExpression valueRe(R"(\(([^)]+)\))");
    auto it = valueRe.globalMatch(valsPart);
    while (it.hasNext()) {
        auto vm = it.next();
        QStringList vals = vm.captured(1).split(',', Qt::SkipEmptyParts);
        QJsonArray row;
        for (const QString &v : vals) {
            QString trimmed = v.trimmed();
            bool isDouble;
            trimmed.toDouble(&isDouble);
            if (isDouble) {
                row.append(trimmed.toDouble());
            } else {
                // 去掉引号
                trimmed.remove('\'').remove('\"');
                row.append(trimmed);
            }
        }
        rows.append(row);
    }

    if (rows.isEmpty())
        return {ResponseStatus::ERROR, "No values specified", QVariant()};

    return m_engine->executeInsert(tableName, colNames, rows);
}

Response SQLParser::execUpdate(const QString &sql) {
    if (m_currentUser.isEmpty())
        return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty())
        return {ResponseStatus::ERROR, "请先选择或创建一个数据库", QVariant()};

    // 使用 indexOf 定位 SET 和 WHERE
    int setIdx = sql.indexOf(QRegularExpression("\\bSET\\b", QRegularExpression::CaseInsensitiveOption));
    if (setIdx == -1) return {ResponseStatus::ERROR, "Invalid UPDATE syntax", QVariant()};
    int whereIdx = sql.indexOf(QRegularExpression("\\bWHERE\\b", QRegularExpression::CaseInsensitiveOption), setIdx + 3);

    // 提取表名（UPDATE 关键字之后，SET 之前）
    QString prefix = sql.left(setIdx).trimmed();
    QStringList parts = prefix.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() < 2 || parts[0].toUpper() != "UPDATE")
        return {ResponseStatus::ERROR, "Invalid UPDATE syntax", QVariant()};
    QString tableName = parts[1];

    QString setPart;
    QString whereClause;
    if (whereIdx != -1) {
        setPart = sql.mid(setIdx + 3, whereIdx - setIdx - 3).trimmed();
        whereClause = sql.mid(whereIdx + 5).trimmed();
        if (whereClause.endsWith(';')) whereClause.chop(1);
    } else {
        setPart = sql.mid(setIdx + 3).trimmed();
        if (setPart.endsWith(';')) setPart.chop(1);
    }

    // 解析 SET 赋值
    QJsonObject assignments;
    QStringList setPairs = setPart.split(',', Qt::SkipEmptyParts);
    for (const QString &pair : setPairs) {
        int eqIdx = pair.indexOf('=');
        if (eqIdx == -1) continue;
        QString key = pair.left(eqIdx).trimmed();
        QString val = pair.mid(eqIdx + 1).trimmed();
        bool isDouble;
        val.toDouble(&isDouble);
        if (isDouble) {
            assignments[key] = val.toDouble();
        } else {
            val.remove('\'').remove('\"');
            assignments[key] = val;
        }
    }

    return m_engine->executeUpdate(tableName, assignments, whereClause);
}

Response SQLParser::execDelete(const QString &sql) {
    if (m_currentUser.isEmpty())
        return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty())
        return {ResponseStatus::ERROR, "请先选择或创建一个数据库", QVariant()};

    // 使用 indexOf 提取 WHERE
    int whereIdx = sql.indexOf(QRegularExpression("\\bWHERE\\b", QRegularExpression::CaseInsensitiveOption));
    int fromIdx = sql.indexOf(QRegularExpression("\\bFROM\\b", QRegularExpression::CaseInsensitiveOption));
    if (fromIdx == -1) return {ResponseStatus::ERROR, "Invalid DELETE syntax", QVariant()};

    // 表名：FROM 之后到下一个空格或 WHERE
    QString afterFrom = sql.mid(fromIdx + 4).trimmed();
    QStringList tokens = afterFrom.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (tokens.isEmpty()) return {ResponseStatus::ERROR, "Invalid DELETE syntax", QVariant()};
    QString tableName = tokens.first();

    QString whereClause;
    if (whereIdx != -1) {
        whereClause = sql.mid(whereIdx + 5).trimmed();
        if (whereClause.endsWith(';')) whereClause.chop(1);
    }

    return m_engine->executeDelete(tableName, whereClause);
}

// ==================== DDL 执行函数（原有代码） ====================

Response SQLParser::execCreateDatabase(const QString &dbName)
{
    if (m_currentUser.isEmpty())
        return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};

    bool ok = m_storage->createDatabase(m_currentUser, dbName);
    if (ok) {
        emit databaseChanged(dbName);
        return {ResponseStatus::OK, QString("数据库 '%1' 创建成功").arg(dbName), QVariant()};
    } else {
        return {ResponseStatus::ERROR, QString("数据库 '%1' 已存在或创建失败").arg(dbName), QVariant()};
    }
}

Response SQLParser::execCreateTable(const QString &tableName, const QString &fieldsStr)
{
    if (m_currentUser.isEmpty())
        return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty())
        return {ResponseStatus::ERROR, "请先选择或创建一个数据库", QVariant()};

    QList<Field> fields = parseFieldDefinitions(fieldsStr);
    if (fields.isEmpty())
        return {ResponseStatus::ERROR, "字段定义解析失败", QVariant()};

    bool ok = m_storage->createTable(m_currentUser, m_currentDB, tableName, fields);
    if (ok) {
        emit tableChanged(m_currentDB, tableName);
        return {ResponseStatus::OK, QString("表 '%1' 创建成功（%2个字段）").arg(tableName).arg(fields.size()), QVariant()};
    } else {
        return {ResponseStatus::ERROR, QString("表 '%1' 已存在或创建失败").arg(tableName), QVariant()};
    }
}

Response SQLParser::execDropDatabase(const QString &dbName)
{
    if (m_currentUser.isEmpty())
        return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};

    bool ok = m_storage->dropDatabase(m_currentUser, dbName);
    if (ok) {
        emit databaseChanged(dbName);
        return {ResponseStatus::OK, QString("数据库 '%1' 已删除").arg(dbName), QVariant()};
    } else {
        return {ResponseStatus::ERROR, QString("数据库 '%1' 删除失败（可能不存在）").arg(dbName), QVariant()};
    }
}

Response SQLParser::execDropTable(const QString &tableName)
{
    if (m_currentUser.isEmpty())
        return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty())
        return {ResponseStatus::ERROR, "请先选择或创建一个数据库", QVariant()};

    bool ok = m_storage->dropTable(m_currentUser, m_currentDB, tableName);
    if (ok) {
        emit tableChanged(m_currentDB, tableName);
        return {ResponseStatus::OK, QString("表 '%1' 已删除").arg(tableName), QVariant()};
    } else {
        return {ResponseStatus::ERROR, QString("表 '%1' 删除失败（可能不存在）").arg(tableName), QVariant()};
    }
}

QList<Field> SQLParser::parseFieldDefinitions(const QString &fieldsStr) const
{
    QList<Field> fields;
    QStringList parts = fieldsStr.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        QString trimmed = part.trimmed();

        Field field;

        // 解析字段名
        QStringList tokens = trimmed.split(' ', Qt::SkipEmptyParts);
        if (tokens.isEmpty()) continue;
        field.name = tokens[0];

        // 解析字段类型
        if (tokens.size() < 2) continue;
        field.type = strToFieldType(tokens[1].toUpper());

        // 设置默认长度
        field.length = 255;
        if (field.type == FieldType::INT) field.length = 10;

        // 解析约束
        for (int i = 2; i < tokens.size(); ++i) {
            QString token = tokens[i].toUpper();
            QString rawToken = tokens[i];

            if (token == "PRIMARY" && i+1 < tokens.size() && tokens[i+1].toUpper() == "KEY") {
                field.isPrimaryKey = true;
                field.isNotNull = true;
                i++;
            }
            else if (token == "UNIQUE") {
                field.isUnique = true;
            }
            else if (token == "NOT" && i+1 < tokens.size() && tokens[i+1].toUpper() == "NULL") {
                field.isNotNull = true;
                i++;
            }
            else if (token == "DEFAULT") {
                QString defaultValue;
                int j = i + 1;
                if (j < tokens.size()) {
                    QString val = tokens[j];
                    if (val.startsWith('\'') || val.startsWith('"')) {
                        defaultValue = val;
                        j++;
                        while (j < tokens.size()) {
                            defaultValue += " " + tokens[j];
                            if (tokens[j].endsWith('\'') || tokens[j].endsWith('"')) {
                                break;
                            }
                            j++;
                        }
                        if (defaultValue.length() >= 2) {
                            defaultValue = defaultValue.mid(1, defaultValue.length() - 2);
                        }
                    } else {
                        defaultValue = val;
                    }
                }
                field.defaultValue = defaultValue;
                i = j;
            }
            else if (token == "CHECK" || token.startsWith("CHECK(")) {
                field.hasCheck = true;
                QString checkExpr;
                int braceCount = 0;
                int j = i;

                if (token.startsWith("CHECK(")) {
                    int contentStart = rawToken.indexOf('(') + 1;
                    QString inner = rawToken.mid(contentStart);
                    braceCount = 1;
                    if (inner.endsWith(')')) {
                        inner.chop(1);
                        braceCount = 0;
                    }
                    checkExpr = inner;
                    j = i + 1;
                    while (j < tokens.size() && braceCount > 0) {
                        QString tk = tokens[j];
                        checkExpr += " " + tk;
                        braceCount += tk.count('(');
                        braceCount -= tk.count(')');
                        j++;
                    }
                } else {
                    j = i + 1;
                    if (j < tokens.size() && tokens[j] == "(") {
                        j++;
                    }
                    braceCount = 1;
                    while (j < tokens.size() && braceCount > 0) {
                        QString tk = tokens[j];
                        checkExpr += tk + " ";
                        braceCount += tk.count('(');
                        braceCount -= tk.count(')');
                        j++;
                    }
                }

                checkExpr = checkExpr.trimmed();
                if (checkExpr.endsWith(')')) {
                    checkExpr = checkExpr.left(checkExpr.length() - 1).trimmed();
                }
                field.checkExpr = checkExpr;
                i = j - 1;
            }
            else if (token == "FOREIGN" && i+1 < tokens.size() && tokens[i+1].toUpper() == "KEY") {
                field.isForeignKey = true;
                i += 2;
                while (i < tokens.size()) {
                    if (tokens[i].toUpper() == "REFERENCES") {
                        i++;
                        if (i < tokens.size()) {
                            QString refRaw = tokens[i];
                            int parenIdx = refRaw.indexOf('(');
                            if (parenIdx != -1) {
                                int closeParen = refRaw.lastIndexOf(')');
                                if (closeParen > parenIdx) {
                                    field.referenceField = refRaw.mid(parenIdx + 1, closeParen - parenIdx - 1);
                                }
                                field.referenceTable = refRaw.left(parenIdx);
                            } else {
                                field.referenceTable = refRaw;
                                i++;
                                if (i < tokens.size() && (tokens[i].startsWith('(') || tokens[i] == "(")) {
                                    QString refField = tokens[i];
                                    if (refField == "(" && i+1 < tokens.size()) {
                                        i++;
                                        refField = tokens[i];
                                    }
                                    if (refField.startsWith('(')) refField = refField.mid(1);
                                    if (refField.endsWith(')')) refField.chop(1);
                                    field.referenceField = refField;
                                }
                            }
                            i++;
                            if (i < tokens.size() && tokens[i].toUpper() == "CASCADE") {
                                field.cascadeRule = "CASCADE";
                                i++;
                            } else if (i < tokens.size() && tokens[i].toUpper() == "SET" && i+1 < tokens.size() && tokens[i+1].toUpper() == "NULL") {
                                field.cascadeRule = "SET NULL";
                                i += 2;
                            }
                        }
                        break;
                    }
                    i++;
                }
                i--;
            }
            else if (token == "FORMAT") {
                if (i+1 < tokens.size()) {
                    field.formatValidation = tokens[i+1].toLower();
                    i++;
                }
            }
            else if (token == "ENCRYPTED") {
                field.isEncrypted = true;
            }
        }

        fields.append(field);
    }
    return fields;
}

FieldType SQLParser::strToFieldType(const QString &typeStr) const
{
    if (typeStr == "INT")     return FieldType::INT;
    if (typeStr == "TEXT")    return FieldType::TEXT;
    if (typeStr == "DOUBLE")  return FieldType::DOUBLE;
    if (typeStr == "BOOLEAN" || typeStr == "BOOL") return FieldType::BOOLEAN;
    return FieldType::TEXT;
}

Response SQLParser::execAlterTable(const QString &tableName, const QString &alterType, const QString &fieldStr)
{
    if (m_currentUser.isEmpty())
        return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty())
        return {ResponseStatus::ERROR, "请先选择或创建一个数据库", QVariant()};

    QList<Field> currentFields = m_storage->loadTableSchema(m_currentUser, m_currentDB, tableName);
    if (currentFields.isEmpty()) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("表 '%1' 不存在").arg(tableName), QVariant()};
    }

    QList<Field> newFields = parseFieldDefinitions(fieldStr);
    if (newFields.isEmpty() && alterType != "DROP") {
        return {ResponseStatus::ERROR, "字段定义解析失败", QVariant()};
    }

    QString resultMsg;
    bool ok = false;

    if (alterType == "ADD") {
        for (const Field &newField : newFields) {
            bool exists = false;
            for (const Field &f : currentFields) {
                if (f.name == newField.name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                currentFields.append(newField);
            }
        }
        ok = m_storage->alterTable(m_currentUser, m_currentDB, tableName, currentFields);
        resultMsg = QString("表 '%1' 添加字段成功").arg(tableName);
    }
    else if (alterType == "DROP") {
        QStringList fieldNamesToRemove;
        QStringList parts = fieldStr.split(' ', Qt::SkipEmptyParts);
        for (const QString &part : parts) {
            fieldNamesToRemove.append(part.trimmed());
        }

        QList<Field> remainingFields;
        for (const Field &f : currentFields) {
            if (!fieldNamesToRemove.contains(f.name)) {
                remainingFields.append(f);
            }
        }

        if (remainingFields.isEmpty()) {
            return {ResponseStatus::ERROR, "表至少需要一个字段", QVariant()};
        }

        ok = m_storage->alterTable(m_currentUser, m_currentDB, tableName, remainingFields);
        resultMsg = QString("表 '%1' 删除字段成功").arg(tableName);
    }
    else if (alterType == "MODIFY") {
        if (newFields.isEmpty()) {
            return {ResponseStatus::ERROR, "缺少字段定义", QVariant()};
        }

        Field fieldToModify = newFields.first();
        for (int i = 0; i < currentFields.size(); i++) {
            if (currentFields[i].name == fieldToModify.name) {
                currentFields[i].type = fieldToModify.type;
                currentFields[i].length = fieldToModify.length;
                break;
            }
        }

        ok = m_storage->alterTable(m_currentUser, m_currentDB, tableName, currentFields);
        resultMsg = QString("表 '%1' 修改字段成功").arg(tableName);
    }
    else {
        return {ResponseStatus::ERROR, QString("不支持的 ALTER 操作: %1").arg(alterType), QVariant()};
    }

    if (ok) {
        emit tableChanged(m_currentDB, tableName);
        return {ResponseStatus::OK, resultMsg, QVariant()};
    } else {
        return {ResponseStatus::ERROR, QString("表 '%1' 修改失败").arg(tableName), QVariant()};
    }
}
