#include "sqlparser.h"
#include "storagemanager.h"
#include <QStringList>
#include <QRegularExpression>

SQLParser::SQLParser(QObject *parent) : QObject(parent) {}

void SQLParser::setStorageManager(StorageManager *storage) { m_storage = storage; }
void SQLParser::setCurrentUser(const QString &user) { m_currentUser = user; }
void SQLParser::setCurrentDatabase(const QString &db) { m_currentDB = db; }

/*先存一下
CREATE TABLE students (id INT, name TEXT)

ALTER TABLE students ADD age INT
ALTER TABLE students ADD gender TEXT

ALTER TABLE students MODIFY age DOUBLE

ALTER TABLE students DROP gender
 */

//解析语句
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

    // ALTER TABLE 操作
    if (upperTokens[0] == "ALTER" && upperTokens[1] == "TABLE") {
        if (tokens.size() < 4)
            return {ResponseStatus::ERROR, "语法错误：ALTER TABLE 格式应为 ALTER TABLE 表名 ADD/DROP/MODIFY 字段名 类型", QVariant()};
        
        QString tableName = tokens[2];
        QString alterType = upperTokens[3]; // ADD/DROP/MODIFY
        
        // 获取字段部分（从第4个token开始）
        QString fieldStr = trimmed;
        int pos = fieldStr.indexOf(alterType, 0, Qt::CaseInsensitive);
        if (pos != -1) {
            fieldStr = fieldStr.mid(pos + alterType.length()).trimmed();
        }
        
        return execAlterTable(tableName, alterType, fieldStr);
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

//字段解析
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

// ALTER TABLE 执行函数
Response SQLParser::execAlterTable(const QString &tableName, const QString &alterType, const QString &fieldStr)
{
    if (m_currentUser.isEmpty())
        return {ResponseStatus::AUTH_FAILED, "请先登录", QVariant()};
    if (m_currentDB.isEmpty())
        return {ResponseStatus::ERROR, "请先选择或创建一个数据库", QVariant()};

    // 获取现有表结构
    QList<Field> currentFields = m_storage->loadTableSchema(m_currentUser, m_currentDB, tableName);
    if (currentFields.isEmpty()) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("表 '%1' 不存在").arg(tableName), QVariant()};
    }

    // 解析字段定义
    QList<Field> newFields = parseFieldDefinitions(fieldStr);
    if (newFields.isEmpty() && alterType != "DROP") {
        return {ResponseStatus::ERROR, "字段定义解析失败", QVariant()};
    }

    QString resultMsg;
    bool ok = false;

    if (alterType == "ADD") {
        // 添加字段：合并现有字段和新字段
        for (const Field &newField : newFields) {
            // 检查字段是否已存在
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
        // 删除字段：从现有字段中移除
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
        // 修改字段：替换现有字段定义
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
