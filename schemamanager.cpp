#include "SchemaManager.h"
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
    if (!ensureDbDirectory(dbName)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to create database directory '%1'").arg(dbName), QVariant()};
    }

    QString filePath = getSchemaFilePath(dbName);
    QFile file(filePath);
    
    // 读取现有表结构
    QJsonArray tables;
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {ResponseStatus::ERROR, QString("[Schema] Failed to open schema file '%1'").arg(filePath), QVariant()};
        }
        
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            tables = doc.array();
        }
    }

    // 检查表是否已存在
    for (const QJsonValue &tableVal : tables) {
        if (tableVal.isObject()) {
            QJsonObject tableObj = tableVal.toObject();
            if (tableObj["tableName"].toString() == schema.tableName) {
                return {ResponseStatus::ERROR, QString("[Schema] Table '%1' already exists").arg(schema.tableName), QVariant()};
            }
        }
    }

    // 将表结构转换为 JSON 对象
    QJsonObject tableObj;
    tableObj["tableName"] = schema.tableName;
    
    QJsonArray fieldsArray;
    for (const Field &field : schema.fields) {
        QJsonObject fieldObj;
        fieldObj["name"] = field.name;
        fieldObj["type"] = static_cast<int>(field.type);
        fieldObj["length"] = field.length;
        fieldObj["isNotNull"] = field.isNotNull;
        fieldObj["isPrimaryKey"] = field.isPrimaryKey;
        fieldsArray.append(fieldObj);
    }
    tableObj["fields"] = fieldsArray;

    // 添加到数组并写入文件
    tables.append(tableObj);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to write schema file '%1'").arg(filePath), QVariant()};
    }

    QJsonDocument doc(tables);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << QString("[Schema] Table '%1' created successfully").arg(schema.tableName);
    return {ResponseStatus::OK, QString("[Schema] Table '%1' created successfully").arg(schema.tableName), QVariant()};
}

Response SchemaManager::loadTableSchema(const QString &dbName, const QString &tableName)
{
    QString filePath = getSchemaFilePath(dbName);
    QFile file(filePath);

    if (!file.exists()) {
        return {ResponseStatus::DB_NOT_FOUND, QString("[Schema] Database '%1' not found").arg(dbName), QVariant()};
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to open schema file '%1'").arg(filePath), QVariant()};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return {ResponseStatus::ERROR, "[Schema] Invalid schema file format", QVariant()};
    }

    for (const QJsonValue &tableVal : doc.array()) {
        if (tableVal.isObject()) {
            QJsonObject tableObj = tableVal.toObject();
            if (tableObj["tableName"].toString() == tableName) {
                TableSchema schema;
                schema.tableName = tableName;
                
                QJsonArray fieldsArray = tableObj["fields"].toArray();
                for (const QJsonValue &fieldVal : fieldsArray) {
                    QJsonObject fieldObj = fieldVal.toObject();
                    Field field;
                    field.name = fieldObj["name"].toString();
                    field.type = static_cast<FieldType>(fieldObj["type"].toInt());
                    field.length = fieldObj["length"].toInt();
                    field.isNotNull = fieldObj["isNotNull"].toBool();
                    field.isPrimaryKey = fieldObj["isPrimaryKey"].toBool();
                    schema.fields.append(field);
                }

                return {ResponseStatus::OK, QString("[Schema] Loaded table '%1'").arg(tableName), QVariant::fromValue(schema)};
            }
        }
    }

    return {ResponseStatus::TABLE_NOT_FOUND, QString("[Schema] Table '%1' not found").arg(tableName), QVariant()};
}

Response SchemaManager::loadTables(const QString &dbName)
{
    QString filePath = getSchemaFilePath(dbName);
    QFile file(filePath);

    if (!file.exists()) {
        return {ResponseStatus::DB_NOT_FOUND, QString("[Schema] Database '%1' not found").arg(dbName), QVariant()};
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to open schema file '%1'").arg(filePath), QVariant()};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return {ResponseStatus::ERROR, "[Schema] Invalid schema file format", QVariant()};
    }

    QList<TableSchema> schemas;
    for (const QJsonValue &tableVal : doc.array()) {
        if (tableVal.isObject()) {
            QJsonObject tableObj = tableVal.toObject();
            TableSchema schema;
            schema.tableName = tableObj["tableName"].toString();
            
            QJsonArray fieldsArray = tableObj["fields"].toArray();
            for (const QJsonValue &fieldVal : fieldsArray) {
                QJsonObject fieldObj = fieldVal.toObject();
                Field field;
                field.name = fieldObj["name"].toString();
                field.type = static_cast<FieldType>(fieldObj["type"].toInt());
                field.length = fieldObj["length"].toInt();
                field.isNotNull = fieldObj["isNotNull"].toBool();
                field.isPrimaryKey = fieldObj["isPrimaryKey"].toBool();
                schema.fields.append(field);
            }
            schemas.append(schema);
        }
    }

    return {ResponseStatus::OK, QString("[Schema] Loaded %1 tables").arg(schemas.size()), QVariant::fromValue(schemas)};
}

Response SchemaManager::dropTable(const QString &dbName, const QString &tableName)
{
    QString filePath = getSchemaFilePath(dbName);
    QFile file(filePath);

    if (!file.exists()) {
        return {ResponseStatus::DB_NOT_FOUND, QString("[Schema] Database '%1' not found").arg(dbName), QVariant()};
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to open schema file '%1'").arg(filePath), QVariant()};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return {ResponseStatus::ERROR, "[Schema] Invalid schema file format", QVariant()};
    }

    QJsonArray tables = doc.array();
    QJsonArray newTables;
    bool found = false;

    for (const QJsonValue &tableVal : tables) {
        if (tableVal.isObject()) {
            QJsonObject tableObj = tableVal.toObject();
            if (tableObj["tableName"].toString() != tableName) {
                newTables.append(tableObj);
            } else {
                found = true;
            }
        }
    }

    if (!found) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Schema] Table '%1' not found").arg(tableName), QVariant()};
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to write schema file '%1'").arg(filePath), QVariant()};
    }

    QJsonDocument newDoc(newTables);
    file.write(newDoc.toJson(QJsonDocument::Indented));
    file.close();

    // 删除数据表文件
    QString tableFilePath = Config::DATA_PATH + dbName + "/" + tableName + ".json";
    QFile::remove(tableFilePath);

    qDebug() << QString("[Schema] Table '%1' dropped successfully").arg(tableName);
    return {ResponseStatus::OK, QString("[Schema] Table '%1' dropped successfully").arg(tableName), QVariant()};
}