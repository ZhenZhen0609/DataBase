#include "sqlparser.h"
#include "storagemanager.h"
#include <QStringList>
#include <QRegularExpression>

SQLParser::SQLParser(QObject *parent) : QObject(parent) {}

void SQLParser::setStorageManager(StorageManager *storage) { m_storage = storage; }
void SQLParser::setCurrentUser(const QString &user) { m_currentUser = user; }
void SQLParser::setCurrentDatabase(const QString &db) { m_currentDB = db; }

Response SQLParser::parseSQL(const QString &sql)
{
    if (!m_storage) {
        return {ResponseStatus::ERROR, "[SQLParser] StorageManager 未设置", QVariant()};
    }

    // 1. 去除首尾空白及末尾分号
    QString trimmed = sql.trimmed();
    if (trimmed.endsWith(';'))
        trimmed.chop(1);
    trimmed = trimmed.trimmed();

    // 2. 统一转大写用于识别关键字（名称保留原始大小写）
    QString upper = trimmed.toUpper();
    QStringList tokens = trimmed.split(' ', Qt::SkipEmptyParts);
    QStringList upperTokens = upper.split(' ', Qt::SkipEmptyParts);

    if (tokens.size() < 2)
        return {ResponseStatus::ERROR, "语法错误：指令不完整", QVariant()};

    // 数据库操作
    if (upperTokens[0] == "CREATE" && upperTokens[1] == "DATABASE") {
        if (tokens.size() < 3)
            return {ResponseStatus::ERROR, "语法错误：缺少数据库名称", QVariant()};
        QString dbName = tokens[2];   // 原始大小写
        return execCreateDatabase(dbName);
    }

    if (upperTokens[0] == "DROP" && upperTokens[1] == "DATABASE") {
        if (tokens.size() < 3)
            return {ResponseStatus::ERROR, "语法错误：缺少数据库名称", QVariant()};
        return execDropDatabase(tokens[2]);
    }

    // 表操作
    if (upperTokens[0] == "CREATE" && upperTokens[1] == "TABLE") {
        // 使用正则获取表名和括号内容
        QRegularExpression re(R"(CREATE\s+TABLE\s+(\w+)\s*\((.*)\)\s*)",
                              QRegularExpression::CaseInsensitiveOption);
        auto match = re.match(trimmed);
        if (!match.hasMatch()) {
            return {ResponseStatus::ERROR,
                    "语法错误：CREATE TABLE 格式应为 CREATE TABLE 表名 (字段名 类型, ...)",
                    QVariant()};
        }
        QString tableName = match.captured(1);
        QString fieldsStr = match.captured(2).trimmed();
        return execCreateTable(tableName, fieldsStr);
    }

    if (upperTokens[0] == "DROP" && upperTokens[1] == "TABLE") {
        if (tokens.size() < 3)
            return {ResponseStatus::ERROR, "语法错误：缺少表名称", QVariant()};
        return execDropTable(tokens[2]);
    }

    return {ResponseStatus::ERROR, "不支持的SQL指令：" + trimmed, QVariant()};
}

// 执行函数实现

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

// 字段解析辅助

QList<Field> SQLParser::parseFieldDefinitions(const QString &fieldsStr) const
{
    QList<Field> fields;
    // 按逗号分割，再按空格分割字段名和类型
    QStringList parts = fieldsStr.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        QString trimmed = part.trimmed();
        QStringList pair = trimmed.split(' ', Qt::SkipEmptyParts);
        if (pair.size() < 2) continue;   // 格式错误，跳过
        QString fieldName = pair[0];
        FieldType type = strToFieldType(pair[1].toUpper());
        // 默认长度为：INT 10, TEXT 255, DOUBLE 0, BOOLEAN 0
        int length = 255;
        if (type == FieldType::INT) length = 10;
        fields.append(Field(fieldName, type, length));
    }
    return fields;
}

FieldType SQLParser::strToFieldType(const QString &typeStr) const
{
    if (typeStr == "INT")     return FieldType::INT;
    if (typeStr == "TEXT")    return FieldType::TEXT;
    if (typeStr == "DOUBLE")  return FieldType::DOUBLE;
    if (typeStr == "BOOLEAN" || typeStr == "BOOL") return FieldType::BOOLEAN;
    return FieldType::TEXT;   // 默认当作TEXT
}
