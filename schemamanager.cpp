#include "SchemaManager.h"
#include "storagemanager.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>

SchemaManager::SchemaManager(QObject *parent) : QObject(parent) {}

bool SchemaManager::validateFieldType(const QString &type, const QVariant &value)
{
    QString upperType = type.toUpper();
    
    if (upperType == "INT") {
        bool ok;
        value.toInt(&ok);
        return ok;
    } else if (upperType == "TEXT") {
        return value.isValid() && !value.toString().isEmpty();
    } else if (upperType == "DOUBLE") {
        bool ok;
        value.toDouble(&ok);
        return ok;
    } else if (upperType == "BOOLEAN") {
        return value.type() == QVariant::Bool || 
               value.toString().toLower() == "true" || 
               value.toString().toLower() == "false";
    } else {
        qDebug() << "[Schema] 不支持的类型:" << type;
        return false;
    }
}

bool SchemaManager::validateField(const Field &field, const QVariant &value) const
{
    // 检查必填字段
    if (field.isNotNull && (value.isNull() || value.toString().isEmpty())) {
        qDebug() << "[Schema] Field" << field.name << "is required but empty";
        return false;
    }

    // 如果值为空且不是必填，跳过类型校验
    if (value.isNull() || value.toString().isEmpty()) {
        return true;
    }

    // 类型校验
    QString typeStr;
    switch (field.type) {
    case FieldType::INT: typeStr = "INT"; break;
    case FieldType::TEXT: typeStr = "TEXT"; break;
    case FieldType::DOUBLE: typeStr = "DOUBLE"; break;
    case FieldType::BOOLEAN: typeStr = "BOOLEAN"; break;
    }

    return validateFieldType(typeStr, value);
}

bool SchemaManager::validateRecord(const TableSchema &schema, const QJsonObject &record) const
{
    for (const Field &field : schema.fields) {
        QVariant value = record.value(field.name).toVariant();
        
        if (!validateField(field, value)) {
            qDebug() << "[Schema] Field validation failed for" << field.name;
            return false;
        }
    }
    return true;
}

QString SchemaManager::getSchemaFilePath(const QString &dbName) const
{
    return Config::DATA_PATH + dbName + "/schemas.json";
}

bool SchemaManager::ensureDbDirectory(const QString &dbName) const
{
    QString dbPath = Config::DATA_PATH + dbName;
    QDir dir;
    return dir.exists(dbPath) || dir.mkpath(dbPath);
}

Response SchemaManager::createTable(const QString &dbName, const TableSchema &schema)
{
    // 调用 StorageManager 创建表物理文件并写入字段结构 [cite: 52]
    StorageManager storageManager;
    
    // 确保数据库目录存在
    if (!ensureDbDirectory(dbName)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to create database directory '%1'").arg(dbName), QVariant()};
    }

    // 使用 StorageManager 创建表并写入字段结构到 .tdf 文件
    if (!storageManager.createTable(dbName, schema.tableName, schema.fields)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to create table '%1' in database '%2'").arg(schema.tableName, dbName), QVariant()};
    }

    qDebug() << QString("[Schema] Table '%1' created successfully with %2 fields").arg(schema.tableName).arg(schema.fields.size());
    return {ResponseStatus::OK, QString("[Schema] Table '%1' created successfully").arg(schema.tableName), QVariant()};
}

Response SchemaManager::loadTableSchema(const QString &dbName, const QString &tableName)
{
    // 调用 StorageManager 从 .tdf 文件加载表结构
    StorageManager storageManager;
    QList<Field> fields = storageManager.loadTableSchema(dbName, tableName);

    if (fields.isEmpty()) {
        // 检查是否是数据库不存在还是表不存在
        QString dbPath = Config::DATA_PATH + dbName;
        QDir dir(dbPath);
        if (!dir.exists()) {
            return {ResponseStatus::DB_NOT_FOUND, QString("[Schema] Database '%1' not found").arg(dbName), QVariant()};
        }
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Schema] Table '%1' not found").arg(tableName), QVariant()};
    }

    TableSchema schema;
    schema.tableName = tableName;
    schema.fields = fields;

    return {ResponseStatus::OK, QString("[Schema] Loaded table '%1' with %2 fields").arg(tableName).arg(fields.size()), QVariant::fromValue(schema)};
}

Response SchemaManager::loadTables(const QString &dbName)
{
    QString dbPath = Config::DATA_PATH + dbName;
    QDir dir(dbPath);

    if (!dir.exists()) {
        return {ResponseStatus::DB_NOT_FOUND, QString("[Schema] Database '%1' not found").arg(dbName), QVariant()};
    }

    // 查找所有 .tdf 文件
    QStringList filters;
    filters << "*.tdf";
    dir.setNameFilters(filters);
    QStringList tdfFiles = dir.entryList(QDir::Files);

    QList<TableSchema> schemas;
    StorageManager storageManager;

    for (const QString &tdfFile : tdfFiles) {
        QString tableName = tdfFile.left(tdfFile.size() - 4); // 移除 .tdf 后缀
        QList<Field> fields = storageManager.loadTableSchema(dbName, tableName);
        
        if (!fields.isEmpty()) {
            TableSchema schema;
            schema.tableName = tableName;
            schema.fields = fields;
            schemas.append(schema);
        }
    }

    return {ResponseStatus::OK, QString("[Schema] Loaded %1 tables").arg(schemas.size()), QVariant::fromValue(schemas)};
}

Response SchemaManager::dropTable(const QString &dbName, const QString &tableName)
{
    QString dbPath = Config::DATA_PATH + dbName;
    QDir dir(dbPath);

    if (!dir.exists()) {
        return {ResponseStatus::DB_NOT_FOUND, QString("[Schema] Database '%1' not found").arg(dbName), QVariant()};
    }

    // 检查 .tdf 文件是否存在
    QString tdfPath = dir.filePath(tableName + ".tdf");
    QString trdPath = dir.filePath(tableName + ".trd");

    if (!QFile::exists(tdfPath)) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Schema] Table '%1' not found").arg(tableName), QVariant()};
    }

    // 删除 .tdf 和 .trd 文件
    bool tdfRemoved = QFile::remove(tdfPath);
    bool trdRemoved = QFile::remove(trdPath);

    if (!tdfRemoved || !trdRemoved) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to delete table files for '%1'").arg(tableName), QVariant()};
    }

    // 同时删除 .json 文件（兼容旧格式）
    QString jsonPath = dir.filePath(tableName + ".json");
    QFile::remove(jsonPath);

    qDebug() << QString("[Schema] Table '%1' dropped successfully").arg(tableName);
    return {ResponseStatus::OK, QString("[Schema] Table '%1' dropped successfully").arg(tableName), QVariant()};
}