#include "sqlparser.h"
#include "storagemanager.h"
#include "queryengine.h"
#include "indexmanager.h"
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

SQLParser::SQLParser(QObject *parent) : QObject(parent) {}

void SQLParser::setStorageManager(StorageManager *storage) { m_storage = storage; }
void SQLParser::setCurrentUser(const QString &user) { m_currentUser = user; }
void SQLParser::setCurrentDatabase(const QString &db) { m_currentDB = db; }
void SQLParser::setQueryEngine(QueryEngine *engine) { m_engine = engine; }

QString SQLParser::errorAtToken(const QString &sql, int tokenPos, const QString &expected) const
{
    int line = 1, column = 1;
    for (int i = 0; i < tokenPos && i < sql.length(); ++i) {
        if (sql[i] == '\n') { line++; column = 1; }
        else column++;
    }
    QString snippet = sql.mid(tokenPos, 30);
    return QString("[SQL错误] 行%1 列%2: 期望 %3，实际 '%4'")
        .arg(line).arg(column).arg(expected).arg(snippet);
}

bool SQLParser::checkTableExists(const QString &tableName, QString &errorMsg) const
{
    if (!m_storage || m_currentUser.isEmpty() || m_currentDB.isEmpty()) {
        errorMsg = "未登录或未选择数据库";
        return false;
    }
    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDB, tableName);
    if (fields.isEmpty()) {
        errorMsg = QString("表 '%1' 不存在").arg(tableName);
        return false;
    }
    return true;
}

bool SQLParser::checkColumnsExist(const QString &tableName, const QStringList &colNames, QString &errorMsg) const
{
    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDB, tableName);
    QStringList aggFuncs = {"COUNT", "SUM", "AVG", "MAX", "MIN"};
    
    for (const QString &col : colNames) {
        QString colUpper = col.toUpper();
        bool isAggFunc = false;
        for (const QString &agg : aggFuncs) {
            if (colUpper.startsWith(agg + "(")) {
                isAggFunc = true;
                break;
            }
        }
        if (isAggFunc) continue;
        
        bool found = false;
        for (const Field &f : fields) {
            if (f.name == col) { found = true; break; }
        }
        if (!found) {
            errorMsg = QString("字段 '%1' 不存在于表 '%2'").arg(col).arg(tableName);
            return false;
        }
    }
    return true;
}

Response SQLParser::parseSQL(const QString &sql)
{
    if (!m_storage) return {ResponseStatus::ERROR, "[SQLParser] StorageManager 未设置", QVariant()};
    if (!m_engine) return {ResponseStatus::ERROR, "[SQLParser] QueryEngine 未设置", QVariant()};

    QString input = sql.trimmed();
    if (input.contains(';')) {
        QStringList statements = input.split(';', Qt::SkipEmptyParts);
        QList<QString> stmtList;
        for (QString s : statements) { s = s.trimmed(); if (!s.isEmpty()) stmtList.append(s); }
        if (stmtList.size() == 1) return parseSingleStatement(stmtList[0]);

        QString resultMessage;
        int successCount = 0, errorCount = 0;
        QVariant lastData;
        for (const QString &stmt : stmtList) {
            Response resp = parseSingleStatement(stmt);
            if (resp.status == ResponseStatus::OK) {
                successCount++;
                resultMessage += QString("语句 %1: %2\n").arg(successCount + errorCount).arg(resp.message);
                if (resp.data.isValid() && !resp.data.isNull()) lastData = resp.data;
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
    return parseSingleStatement(input);
}

Response SQLParser::parseSingleStatement(const QString &sql)
{
    QString trimmed = sql.trimmed();
    QString upper = trimmed.toUpper();
    QStringList tokens = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (tokens.isEmpty()) return {ResponseStatus::ERROR, "空语句", QVariant()};

    if (upper.startsWith("SELECT")) {
        if (trimmed.contains(QRegularExpression("\\bJOIN\\b", QRegularExpression::CaseInsensitiveOption))) {
            return execJoinSelect(trimmed);
        } else if (trimmed.contains(QRegularExpression("\\bUNION\\b", QRegularExpression::CaseInsensitiveOption))) {
            return execUnion(trimmed);
        } else {
            return execSelect(trimmed);
        }
    }
    if (upper.startsWith("INSERT")) return execInsert(trimmed);
    if (upper.startsWith("UPDATE")) return execUpdate(trimmed);
    if (upper.startsWith("DELETE")) return execDelete(trimmed);
    
    if (upper.startsWith("BEGIN") || upper.contains("BEGIN TRANSACTION")) {
        return m_engine->executeBeginTransaction();
    }
    if (upper.startsWith("COMMIT")) {
        return m_engine->executeCommit();
    }
    if (upper.startsWith("ROLLBACK")) {
        return m_engine->executeRollback();
    }

    if (upper.startsWith("CREATE DATABASE")) {
        if (tokens.size() < 3) return {ResponseStatus::ERROR, "缺少数据库名", QVariant()};
        return execCreateDatabase(tokens[2]);
    }
    if (upper.startsWith("DROP DATABASE")) {
        if (tokens.size() < 3) return {ResponseStatus::ERROR, "缺少数据库名", QVariant()};
        return execDropDatabase(tokens[2]);
    }
    if (upper.startsWith("CREATE TABLE")) {
        int parenStart = trimmed.indexOf('(');
        if (parenStart == -1) return {ResponseStatus::ERROR, "缺少字段定义括号", QVariant()};
        int braceCount = 0;
        int parenEnd = -1;
        for (int i = parenStart; i < trimmed.length(); ++i) {
            if (trimmed[i] == '(') braceCount++;
            else if (trimmed[i] == ')') { braceCount--; if (braceCount == 0) { parenEnd = i; break; } }
        }
        if (parenEnd == -1) return {ResponseStatus::ERROR, "括号不匹配", QVariant()};
        QString tableName = tokens[2];
        QString fieldsStr = trimmed.mid(parenStart+1, parenEnd-parenStart-1).trimmed();
        return execCreateTable(tableName, fieldsStr);
    }
    if (upper.startsWith("DROP TABLE")) {
        if (tokens.size() < 3) return {ResponseStatus::ERROR, "缺少表名", QVariant()};
        return execDropTable(tokens[2]);
    }
    if (upper.startsWith("ALTER TABLE")) {
        if (tokens.size() < 4) return {ResponseStatus::ERROR, "ALTER TABLE 语法错误", QVariant()};
        QString tableName = tokens[2];
        QString alterType = tokens[3].toUpper();
        QString fieldStr = trimmed.mid(trimmed.indexOf(alterType, 0, Qt::CaseInsensitive) + alterType.length()).trimmed();
        return execAlterTable(tableName, alterType, fieldStr);
    }
    if (upper.startsWith("CREATE VIEW")) {
        QRegularExpression viewRe(R"(CREATE\s+VIEW\s+(\w+)\s+AS\s+(.+))", QRegularExpression::CaseInsensitiveOption);
        auto match = viewRe.match(trimmed);
        if (!match.hasMatch()) return {ResponseStatus::ERROR, "CREATE VIEW 语法错误", QVariant()};
        QString viewName = match.captured(1);
        QString selectSql = match.captured(2).trimmed();
        return execCreateView(viewName, selectSql);
    }
    if (upper.startsWith("DROP VIEW")) {
        if (tokens.size() < 3) return {ResponseStatus::ERROR, "缺少视图名", QVariant()};
        return execDropView(tokens[2]);
    }
    if (upper.startsWith("CREATE INDEX")) {
        QRegularExpression indexRe(R"(CREATE\s+INDEX\s+(\w+)\s+ON\s+(\w+)\s*\((\w+)\))", QRegularExpression::CaseInsensitiveOption);
        auto match = indexRe.match(trimmed);
        if (!match.hasMatch()) return {ResponseStatus::ERROR, "CREATE INDEX 语法错误", QVariant()};
        QString indexName = match.captured(1);
        QString tableName = match.captured(2);
        QString fieldName = match.captured(3);
        
        QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDB, tableName);
        FieldType fieldType = FieldType::TEXT;
        for (const Field &f : fields) {
            if (f.name == fieldName) {
                fieldType = f.type;
                break;
            }
        }
        
        IndexManager idxMgr;
        bool ok = idxMgr.createIndex(m_currentUser, m_currentDB, tableName, fieldName, indexName, fieldType);
        if (ok) {
            return {ResponseStatus::OK, QString("索引 '%1' 创建成功").arg(indexName), QVariant()};
        }
        return {ResponseStatus::ERROR, QString("索引 '%1' 创建失败").arg(indexName), QVariant()};
    }
    if (upper.startsWith("DROP INDEX")) {
        QRegularExpression dropIndexRe(R"(DROP\s+INDEX\s+(\w+)\s+ON\s+(\w+))", QRegularExpression::CaseInsensitiveOption);
        auto match = dropIndexRe.match(trimmed);
        if (!match.hasMatch()) return {ResponseStatus::ERROR, "DROP INDEX 语法错误", QVariant()};
        QString indexName = match.captured(1);
        QString tableName = match.captured(2);
        IndexManager idxMgr;
        bool ok = idxMgr.dropIndex(m_currentUser, m_currentDB, tableName, indexName);
        if (ok) {
            return {ResponseStatus::OK, QString("索引 '%1' 删除成功").arg(indexName), QVariant()};
        }
        return {ResponseStatus::ERROR, QString("索引 '%1' 删除失败").arg(indexName), QVariant()};
    }

    return {ResponseStatus::ERROR, "不支持的SQL指令: " + trimmed, QVariant()};
}

// ==================== DDL 实现 ====================
Response SQLParser::execCreateDatabase(const QString &dbName)
{
    if (m_currentUser.isEmpty()) return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
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
    if (m_currentUser.isEmpty()) return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty()) return {ResponseStatus::ERROR, "请先选择或创建一个数据库", QVariant()};
    QList<Field> fields = parseFieldDefinitions(fieldsStr);
    if (fields.isEmpty()) return {ResponseStatus::ERROR, "字段定义解析失败", QVariant()};
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
    if (m_currentUser.isEmpty()) return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    bool ok = m_storage->dropDatabase(m_currentUser, dbName);
    if (ok) {
        emit databaseChanged(dbName);
        return {ResponseStatus::OK, QString("数据库 '%1' 已删除").arg(dbName), QVariant()};
    } else {
        return {ResponseStatus::ERROR, QString("数据库 '%1' 删除失败").arg(dbName), QVariant()};
    }
}

Response SQLParser::execDropTable(const QString &tableName)
{
    if (m_currentUser.isEmpty()) return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty()) return {ResponseStatus::ERROR, "请先选择数据库", QVariant()};
    bool ok = m_storage->dropTable(m_currentUser, m_currentDB, tableName);
    if (ok) {
        emit tableChanged(m_currentDB, tableName);
        return {ResponseStatus::OK, QString("表 '%1' 已删除").arg(tableName), QVariant()};
    } else {
        return {ResponseStatus::ERROR, QString("表 '%1' 删除失败").arg(tableName), QVariant()};
    }
}

Response SQLParser::execAlterTable(const QString &tableName, const QString &alterType, const QString &fieldStr)
{
    if (m_currentUser.isEmpty()) return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty()) return {ResponseStatus::ERROR, "请先选择数据库", QVariant()};
    QList<Field> currentFields = m_storage->loadTableSchema(m_currentUser, m_currentDB, tableName);
    if (currentFields.isEmpty()) return {ResponseStatus::TABLE_NOT_FOUND, QString("表 '%1' 不存在").arg(tableName), QVariant()};

    QList<Field> newFields = parseFieldDefinitions(fieldStr);
    if (newFields.isEmpty() && alterType != "DROP") return {ResponseStatus::ERROR, "字段定义解析失败", QVariant()};

    bool ok = false;
    if (alterType == "ADD") {
        for (const Field &f : newFields) {
            bool exists = false;
            for (const Field &cf : currentFields) if (cf.name == f.name) { exists = true; break; }
            if (!exists) currentFields.append(f);
        }
        ok = m_storage->alterTable(m_currentUser, m_currentDB, tableName, currentFields);
    } else if (alterType == "DROP") {
        QStringList names = fieldStr.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QList<Field> remaining;
        for (const Field &f : currentFields) if (!names.contains(f.name)) remaining.append(f);
        if (remaining.isEmpty()) return {ResponseStatus::ERROR, "表至少需要一个字段", QVariant()};
        ok = m_storage->alterTable(m_currentUser, m_currentDB, tableName, remaining);
    } else if (alterType == "MODIFY") {
        if (newFields.isEmpty()) return {ResponseStatus::ERROR, "缺少字段定义", QVariant()};
        Field toMod = newFields.first();
        for (int i=0; i<currentFields.size(); ++i) {
            if (currentFields[i].name == toMod.name) {
                currentFields[i].type = toMod.type;
                currentFields[i].length = toMod.length;
                break;
            }
        }
        ok = m_storage->alterTable(m_currentUser, m_currentDB, tableName, currentFields);
    } else {
        return {ResponseStatus::ERROR, QString("不支持的 ALTER 操作: %1").arg(alterType), QVariant()};
    }
    if (ok) {
        emit tableChanged(m_currentDB, tableName);
        return {ResponseStatus::OK, QString("表 '%1' 修改成功").arg(tableName), QVariant()};
    } else {
        return {ResponseStatus::ERROR, QString("表 '%1' 修改失败").arg(tableName), QVariant()};
    }
}

// ==================== DML 实现（支持表别名） ====================
Response SQLParser::execSelect(const QString &sql)
{
    QRegularExpression re(R"(SELECT\s+(.*?)\s+FROM\s+(\w+)(?:\s+(\w+))?(.*))", QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(sql);
    if (!match.hasMatch()) return {ResponseStatus::ERROR, "SELECT 语法错误", QVariant()};

    QString colsPart = match.captured(1).trimmed();
    QString tableName = match.captured(2).trimmed();
    QString alias = match.captured(3).trimmed();
    QString rest = match.captured(4).trimmed();

    QStringList sqlKeywords = {"WHERE", "ORDER", "GROUP", "HAVING", "LIMIT", "JOIN", "LEFT", "RIGHT", "INNER", "UNION", "SELECT", "FROM", "ON", "AND", "OR", "NOT", "IN", "IS", "NULL", "AS", "DISTINCT"};
    QString aliasUpper = alias.toUpper();
    if (sqlKeywords.contains(aliasUpper)) {
        rest = alias + " " + rest;
        alias.clear();
    }

    QString errMsg;
    if (!checkTableExists(tableName, errMsg)) {
        QString viewsPath = Config::dataPath() + m_currentUser + "/" + m_currentDB + "/views.json";
        QFile viewsFile(viewsPath);
        if (!viewsFile.exists() || !viewsFile.open(QIODevice::ReadOnly) || 
            !QJsonDocument::fromJson(viewsFile.readAll()).object().contains(tableName)) {
            return {ResponseStatus::ERROR, errMsg, QVariant()};
        }
        viewsFile.close();
    }

    bool distinct = false;
    if (colsPart.toUpper().startsWith("DISTINCT ")) {
        distinct = true;
        colsPart = colsPart.mid(9).trimmed();
    }

    QStringList columns;
    QMap<QString, QString> aliasMap;
    if (colsPart != "*") {
        QStringList raw = colsPart.split(',', Qt::SkipEmptyParts);
        for (QString c : raw) {
            c = c.trimmed();
            QString original = c;
            if (c.contains('.')) c = c.split('.').last();
            QRegularExpression asRe(R"(\s+AS\s+(\w+)\s*$)", QRegularExpression::CaseInsensitiveOption);
            auto asMatch = asRe.match(c);
            if (asMatch.hasMatch()) {
                QString alias = asMatch.captured(1);
                c = c.left(asMatch.capturedStart()).trimmed();
                aliasMap[alias] = c;
            }
            columns.append(c);
        }
        if (!checkColumnsExist(tableName, columns, errMsg)) return {ResponseStatus::ERROR, errMsg, QVariant()};
    } else {
        columns.append("*");
    }

    // 将 WHERE 条件中的别名替换为真实字段名（如果有别名）
    QString whereClause = rest;
    if (!alias.isEmpty()) {
        whereClause.replace(QRegularExpression("\\b" + alias + "\\."), "");
    }

    // 提取各个子句（注意：ORDER BY, GROUP BY 等中的别名也需要处理，简化起见，只处理 WHERE）
    QString orderBy, having;
    QStringList groupBy;
    int limit = -1, offset = 0;

    QRegularExpression whereRe(R"(\bWHERE\b\s+(.+?)(?=\b(ORDER\s+BY|GROUP\s+BY|HAVING|LIMIT)\b|$))", QRegularExpression::CaseInsensitiveOption);
    auto whereMatch = whereRe.match(rest);
    if (whereMatch.hasMatch()) whereClause = whereMatch.captured(1).trimmed();
    else whereClause.clear();

    QRegularExpression orderRe(R"(\bORDER\s+BY\b\s+(.+?)(?=\b(WHERE|GROUP\s+BY|HAVING|LIMIT)\b|$))", QRegularExpression::CaseInsensitiveOption);
    auto orderMatch = orderRe.match(rest);
    if (orderMatch.hasMatch()) {
        orderBy = orderMatch.captured(1).trimmed();
        qDebug() << "[SQLParser] ORDER BY parsed:" << orderBy;
    } else {
        qDebug() << "[SQLParser] No ORDER BY found in rest:" << rest;
    }

    QRegularExpression groupRe(R"(\bGROUP\s+BY\b\s+(.+?)(?=\b(WHERE|ORDER\s+BY|HAVING|LIMIT)\b|$))", QRegularExpression::CaseInsensitiveOption);
    auto groupMatch = groupRe.match(rest);
    if (groupMatch.hasMatch()) {
        QString gb = groupMatch.captured(1).trimmed();
        QStringList raw = gb.split(',', Qt::SkipEmptyParts);
        for (QString g : raw) groupBy.append(g.trimmed());
    }

    QRegularExpression havingRe(R"(\bHAVING\b\s+(.+?)(?=\b(WHERE|ORDER\s+BY|GROUP\s+BY|LIMIT)\b|$))", QRegularExpression::CaseInsensitiveOption);
    auto havingMatch = havingRe.match(rest);
    if (havingMatch.hasMatch()) having = havingMatch.captured(1).trimmed();
    
    if (!aliasMap.isEmpty() && !having.isEmpty()) {
        for (auto it = aliasMap.begin(); it != aliasMap.end(); ++it) {
            having.replace(QRegularExpression("\\b" + it.key() + "\\b"), it.value());
        }
        qDebug() << "[SQLParser] HAVING after alias replace:" << having;
    }

    QRegularExpression limitRe(R"(\bLIMIT\b\s+(\d+)(?:\s+OFFSET\s+(\d+))?)", QRegularExpression::CaseInsensitiveOption);
    auto limitMatch = limitRe.match(rest);
    if (limitMatch.hasMatch()) {
        limit = limitMatch.captured(1).toInt();
        if (!limitMatch.captured(2).isEmpty()) offset = limitMatch.captured(2).toInt();
    }

    return m_engine->executeSelect(tableName, columns, whereClause, orderBy, groupBy, having, limit, offset, distinct);
}

Response SQLParser::execInsert(const QString &sql)
{
    QRegularExpression re(R"(INSERT\s+INTO\s+(\w+)\s*(?:\(([^)]*)\))?\s*VALUES\s*(.+))", QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(sql);
    if (!match.hasMatch()) return {ResponseStatus::ERROR, "INSERT 语法错误", QVariant()};

    QString tableName = match.captured(1);
    QString colsPart = match.captured(2).trimmed();
    QString valsPart = match.captured(3).trimmed();

    QStringList colNames;
    if (!colsPart.isEmpty()) {
        QStringList raw = colsPart.split(',', Qt::SkipEmptyParts);
        for (QString c : raw) colNames.append(c.trimmed());
        QString errMsg;
        if (!checkColumnsExist(tableName, colNames, errMsg)) return {ResponseStatus::ERROR, errMsg, QVariant()};
    }

    QList<QJsonArray> rows;
    QRegularExpression valueRe(R"(\(([^)]+)\))");
    auto it = valueRe.globalMatch(valsPart);
    while (it.hasNext()) {
        auto vm = it.next();
        QStringList vals = vm.captured(1).split(',', Qt::SkipEmptyParts);
        QJsonArray row;
        for (const QString &v : vals) {
            QString trimmed = v.trimmed();
            bool isNum;
            trimmed.toDouble(&isNum);
            if (isNum) row.append(trimmed.toDouble());
            else {
                trimmed.remove('\'').remove('\"');
                row.append(trimmed);
            }
        }
        rows.append(row);
    }
    if (rows.isEmpty()) return {ResponseStatus::ERROR, "没有指定值", QVariant()};

    return m_engine->executeInsert(tableName, colNames, rows);
}

Response SQLParser::execUpdate(const QString &sql)
{
    if (m_currentUser.isEmpty()) return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty()) return {ResponseStatus::ERROR, "请先选择数据库", QVariant()};

    int setIdx = sql.indexOf(QRegularExpression("\\bSET\\b", QRegularExpression::CaseInsensitiveOption));
    if (setIdx == -1) return {ResponseStatus::ERROR, "UPDATE 语法错误", QVariant()};
    int whereIdx = sql.indexOf(QRegularExpression("\\bWHERE\\b", QRegularExpression::CaseInsensitiveOption), setIdx+3);

    QString prefix = sql.left(setIdx).trimmed();
    QStringList parts = prefix.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() < 2 || parts[0].toUpper() != "UPDATE") return {ResponseStatus::ERROR, "UPDATE 语法错误", QVariant()};
    QString tableName = parts[1];  // 可能包含别名，需要去除
    if (tableName.contains(',')) tableName = tableName.split(',').first();
    tableName = tableName.split(' ').first();

    QString errMsg;
    if (!checkTableExists(tableName, errMsg)) return {ResponseStatus::ERROR, errMsg, QVariant()};

    QString setPart, whereClause;
    if (whereIdx != -1) {
        setPart = sql.mid(setIdx+3, whereIdx-setIdx-3).trimmed();
        whereClause = sql.mid(whereIdx+5).trimmed();
        if (whereClause.endsWith(';')) whereClause.chop(1);
    } else {
        setPart = sql.mid(setIdx+3).trimmed();
        if (setPart.endsWith(';')) setPart.chop(1);
    }

    QJsonObject assignments;
    QStringList setPairs = setPart.split(',', Qt::SkipEmptyParts);
    for (const QString &pair : setPairs) {
        int eqIdx = pair.indexOf('=');
        if (eqIdx == -1) continue;
        QString key = pair.left(eqIdx).trimmed();
        QString val = pair.mid(eqIdx+1).trimmed();
        bool isNum;
        val.toDouble(&isNum);
        if (isNum) assignments[key] = val.toDouble();
        else {
            val.remove('\'').remove('\"');
            assignments[key] = val;
        }
    }

    return m_engine->executeUpdate(tableName, assignments, whereClause);
}

Response SQLParser::execDelete(const QString &sql)
{
    if (m_currentUser.isEmpty()) return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty()) return {ResponseStatus::ERROR, "请先选择数据库", QVariant()};

    int fromIdx = sql.indexOf(QRegularExpression("\\bFROM\\b", QRegularExpression::CaseInsensitiveOption));
    if (fromIdx == -1) return {ResponseStatus::ERROR, "DELETE 语法错误", QVariant()};
    int whereIdx = sql.indexOf(QRegularExpression("\\bWHERE\\b", QRegularExpression::CaseInsensitiveOption), fromIdx+4);

    QString afterFrom = sql.mid(fromIdx+4).trimmed();
    QStringList tokens = afterFrom.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (tokens.isEmpty()) return {ResponseStatus::ERROR, "DELETE 语法错误", QVariant()};
    QString tableName = tokens.first();
    if (tableName.contains(',')) tableName = tableName.split(',').first();
    tableName = tableName.split(' ').first();

    QString errMsg;
    if (!checkTableExists(tableName, errMsg)) return {ResponseStatus::ERROR, errMsg, QVariant()};

    QString whereClause;
    if (whereIdx != -1) {
        whereClause = sql.mid(whereIdx+5).trimmed();
        if (whereClause.endsWith(';')) whereClause.chop(1);
    }

    return m_engine->executeDelete(tableName, whereClause);
}

// ==================== 扩展功能实现 ====================
Response SQLParser::execJoinSelect(const QString &sql)
{
    return m_engine->executeJoinSelect(sql);
}

Response SQLParser::execUnion(const QString &sql)
{
    QRegularExpression unionRe(R"((.+?)\s+UNION\s+(ALL\s+)?(.+))", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    auto match = unionRe.match(sql);
    if (!match.hasMatch()) return {ResponseStatus::ERROR, "UNION 语法错误", QVariant()};
    QString leftSql = match.captured(1).trimmed();
    QString allFlag = match.captured(2).trimmed();
    QString rightSql = match.captured(3).trimmed();
    bool distinct = (allFlag.toUpper() != "ALL");

    Response leftRes = parseSingleStatement(leftSql);
    if (leftRes.status != ResponseStatus::OK) return leftRes;
    Response rightRes = parseSingleStatement(rightSql);
    if (rightRes.status != ResponseStatus::OK) return rightRes;

    QJsonArray leftRows = QJsonDocument::fromJson(leftRes.data.toString().toUtf8()).array();
    QJsonArray rightRows = QJsonDocument::fromJson(rightRes.data.toString().toUtf8()).array();

    return m_engine->mergeUnion(leftRows, rightRows, distinct);
}

Response SQLParser::execCreateView(const QString &viewName, const QString &selectSql)
{
    Response test = parseSingleStatement(selectSql);
    if (test.status != ResponseStatus::OK) {
        return {ResponseStatus::ERROR, "视图定义中的 SELECT 语句无效: " + test.message, QVariant()};
    }
    return m_engine->executeCreateView(viewName, selectSql);
}

Response SQLParser::execDropView(const QString &viewName)
{
    return m_engine->executeDropView(viewName);
}

// ==================== 解析辅助 ====================
QList<Field> SQLParser::parseFieldDefinitions(const QString &fieldsStr) const
{
    QList<Field> fields;
    QStringList parts = fieldsStr.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        QString trimmed = part.trimmed();
        Field field;
        QStringList tokens = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (tokens.isEmpty()) continue;
        field.name = tokens[0];
        if (tokens.size() < 2) continue;
        field.type = strToFieldType(tokens[1].toUpper());
        field.length = (field.type == FieldType::INT) ? 10 : 255;

        for (int i = 2; i < tokens.size(); ++i) {
            QString token = tokens[i].toUpper();
            if (token == "PRIMARY" && i+1 < tokens.size() && tokens[i+1].toUpper() == "KEY") {
                field.isPrimaryKey = true;
                field.isNotNull = true;
                i++;
            } else if (token == "UNIQUE") {
                field.isUnique = true;
            } else if (token == "NOT" && i+1 < tokens.size() && tokens[i+1].toUpper() == "NULL") {
                field.isNotNull = true;
                i++;
            } else if (token == "DEFAULT") {
                if (i+1 < tokens.size()) {
                    QString defaultValue = tokens[i+1];
                    if ((defaultValue.startsWith('\'') && defaultValue.endsWith('\'')) ||
                        (defaultValue.startsWith('"') && defaultValue.endsWith('"'))) {
                        defaultValue = defaultValue.mid(1, defaultValue.length()-2);
                    }
                    field.defaultValue = defaultValue;
                    i++;
                }
            } else if (token == "CHECK") {
                field.hasCheck = true;
                if (i+1 < tokens.size()) {
                    QString checkExpr = tokens[i+1];
                    if (checkExpr.startsWith('(') && checkExpr.endsWith(')')) {
                        checkExpr = checkExpr.mid(1, checkExpr.length()-2);
                    }
                    field.checkExpr = checkExpr;
                    i++;
                }
            } else if (token == "FOREIGN" && i+1 < tokens.size() && tokens[i+1].toUpper() == "KEY") {
                field.isForeignKey = true;
                i += 2;
                while (i < tokens.size()) {
                    if (tokens[i].toUpper() == "REFERENCES") {
                        i++;
                        if (i < tokens.size()) {
                            QString ref = tokens[i];
                            int paren = ref.indexOf('(');
                            if (paren != -1) {
                                field.referenceTable = ref.left(paren);
                                int close = ref.indexOf(')', paren);
                                if (close != -1) field.referenceField = ref.mid(paren+1, close-paren-1);
                            } else {
                                field.referenceTable = ref;
                                i++;
                                if (i < tokens.size() && tokens[i].startsWith('(')) {
                                    field.referenceField = tokens[i].remove('(').remove(')');
                                }
                            }
                        }
                        break;
                    }
                    i++;
                }
            } else if (token == "FORMAT") {
                if (i+1 < tokens.size()) {
                    field.formatValidation = tokens[i+1].toLower();
                    i++;
                }
            } else if (token == "ENCRYPTED") {
                field.isEncrypted = true;
            }
        }
        fields.append(field);
    }
    return fields;
}

FieldType SQLParser::strToFieldType(const QString &typeStr) const
{
    if (typeStr == "INT") return FieldType::INT;
    if (typeStr == "TEXT") return FieldType::TEXT;
    if (typeStr == "DOUBLE") return FieldType::DOUBLE;
    if (typeStr == "BOOLEAN" || typeStr == "BOOL") return FieldType::BOOLEAN;
    return FieldType::TEXT;
}
