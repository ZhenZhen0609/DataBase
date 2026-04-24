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
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        return value.metaType().id() == QMetaType::Bool ||
#else
        return value.type() == QVariant::Bool ||
#endif
               value.toString().toLower() == "true" ||
               value.toString().toLower() == "false";
    } else {
        qDebug() << "[Schema] 不支持的类型:" << type;
        return false;
    }
}

bool SchemaManager::validateField(const Field &field, const QVariant &value) const
{
    if (field.isPrimaryKey && (value.isNull() || value.toString().isEmpty())) {
        qDebug() << "[Schema] Primary key field" << field.name << "cannot be null";
        return false;
    }
    if (field.isNotNull && (value.isNull() || value.toString().isEmpty())) {
        qDebug() << "[Schema] Field" << field.name << "is required but empty";
        return false;
    }
    if (value.isNull() || value.toString().isEmpty()) return true;

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

bool SchemaManager::ensureDbDirectory(const QString &username, const QString &dbName) const
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir;
    return dir.exists(dbPath) || dir.mkpath(dbPath);
}

QByteArray SchemaManager::serializeSchema(const QList<Field> &fields) const
{
    QJsonArray fieldsArray;
    for (const Field &field : fields) {
        QJsonObject fieldObj;
        fieldObj["name"] = field.name;
        fieldObj["type"] = static_cast<int>(field.type);
        fieldObj["length"] = field.length;
        fieldObj["isNotNull"] = field.isNotNull;
        fieldObj["isPrimaryKey"] = field.isPrimaryKey;
        fieldsArray.append(fieldObj);
    }
    QJsonObject root;
    root["fields"] = fieldsArray;
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool SchemaManager::saveSchema(const QString &username, const QString &dbName, const QString &tableName, const QList<Field> &fields)
{
    if (fields.isEmpty()) return false;

    QStringList fieldNames;
    for (const Field &f : fields) {
        if (fieldNames.contains(f.name)) return false;
        fieldNames.append(f.name);
    }

    QByteArray schemaData = serializeSchema(fields);
    if (schemaData.isEmpty()) return false;

    if (!ensureDbDirectory(username, dbName)) return false;

    StorageManager storage;
    return storage.writeTableDefinition(username, dbName, tableName, schemaData);
}

Response SchemaManager::createTable(const QString &username, const QString &dbName, const TableSchema &schema)
{
    StorageManager storage;
    if (!storage.createTable(username, dbName, schema.tableName, schema.fields)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to create table '%1'").arg(schema.tableName), QVariant()};
    }
    return {ResponseStatus::OK, QString("[Schema] Table '%1' created successfully").arg(schema.tableName), QVariant()};
}

Response SchemaManager::loadTableSchema(const QString &username, const QString &dbName, const QString &tableName)
{
    StorageManager storageManager;
    QList<Field> fields = storageManager.loadTableSchema(username, dbName, tableName);

    if (fields.isEmpty()) {
        QString dbPath = Config::DATA_PATH + username + "/" + dbName;
        if (!QDir(dbPath).exists())
            return {ResponseStatus::DB_NOT_FOUND, QString("[Schema] Database '%1' not found").arg(dbName), QVariant()};
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Schema] Table '%1' not found").arg(tableName), QVariant()};
    }

    TableSchema schema;
    schema.tableName = tableName;
    schema.fields = fields;
    return {ResponseStatus::OK, QString("[Schema] Loaded table '%1' with %2 fields").arg(tableName).arg(fields.size()), QVariant::fromValue(schema)};
}

Response SchemaManager::loadTables(const QString &username, const QString &dbName)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);

    if (!dir.exists())
        return {ResponseStatus::DB_NOT_FOUND, QString("[Schema] Database '%1' not found").arg(dbName), QVariant()};

    dir.setNameFilters({"*.tdf"});
    QStringList tdfFiles = dir.entryList(QDir::Files);

    QList<TableSchema> schemas;
    StorageManager storageManager;

    for (const QString &tdfFile : tdfFiles) {
        QString tableName = tdfFile.left(tdfFile.size() - 4);
        QList<Field> fields = storageManager.loadTableSchema(username, dbName, tableName);
        if (!fields.isEmpty()) {
            TableSchema schema;
            schema.tableName = tableName;
            schema.fields = fields;
            schemas.append(schema);
        }
    }

    return {ResponseStatus::OK, QString("[Schema] Loaded %1 tables").arg(schemas.size()), QVariant::fromValue(schemas)};
}

Response SchemaManager::dropTable(const QString &username, const QString &dbName, const QString &tableName)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);

    if (!dir.exists())
        return {ResponseStatus::DB_NOT_FOUND, QString("[Schema] Database '%1' not found").arg(dbName), QVariant()};

    QString tdfPath = dir.filePath(tableName + ".tdf");
    QString trdPath = dir.filePath(tableName + ".trd");

    if (!QFile::exists(tdfPath))
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Schema] Table '%1' not found").arg(tableName), QVariant()};

    bool tdfRemoved = QFile::remove(tdfPath);
    bool trdRemoved = QFile::remove(trdPath);

    if (!tdfRemoved || !trdRemoved)
        return {ResponseStatus::ERROR, QString("[Schema] Failed to delete table files for '%1'").arg(tableName), QVariant()};

    QFile::remove(dir.filePath(tableName + ".json"));

    return {ResponseStatus::OK, QString("[Schema] Table '%1' dropped successfully").arg(tableName), QVariant()};
}

Response SchemaManager::validateAndFillRecord(const TableSchema &schema,
                                              const QJsonArray &existingRecords,
                                              QJsonObject &record) const
{
    // 1. 默认值填充与类型校验
    for (const Field &field : schema.fields) {
        QVariant value = record.value(field.name).toVariant();

        // 如果缺失或为空，则填充默认值
        if (!record.contains(field.name) || value.isNull() || (value.type() == QVariant::String && value.toString().isEmpty())) {
            switch (field.type) {
            case FieldType::INT:     record[field.name] = 0; break;
            case FieldType::DOUBLE:  record[field.name] = 0.0; break;
            case FieldType::BOOLEAN: record[field.name] = false; break;
            case FieldType::TEXT:    record[field.name] = ""; break;
            }
            continue; // 默认值直接视为合法，跳过类型校验
        }

        // 类型强校验
        if (!validateField(field, value)) {
            return {ResponseStatus::FIELD_MISMATCH,
                    QString("[Schema] 字段 '%1' 的值类型不匹配，期望 %2")
                        .arg(field.name)
                        .arg(field.type == FieldType::INT ? "INT" :
                                 field.type == FieldType::DOUBLE ? "DOUBLE" :
                                 field.type == FieldType::BOOLEAN ? "BOOLEAN" : "TEXT"),
                    QVariant()};
        }
    }

    // 2. 主键唯一性检查
    // 收集所有主键字段
    QStringList pkFields;
    for (const Field &field : schema.fields) {
        if (field.isPrimaryKey) {
            pkFields.append(field.name);
        }
    }

    if (!pkFields.isEmpty()) {
        // 构建当前记录的主键值对（QJsonObject 用于对比）
        QJsonObject newPkValues;
        for (const QString &key : pkFields) {
            newPkValues[key] = record[key];
        }

        // 遍历已有记录，检查主键是否重复
        for (const QJsonValue &val : existingRecords) {
            QJsonObject existingRecord = val.toObject();
            bool conflict = true;
            for (const QString &key : pkFields) {
                if (existingRecord[key] != newPkValues[key]) {
                    conflict = false;
                    break;
                }
            }
            if (conflict) {
                return {ResponseStatus::ERROR,
                        QString("[Schema] 主键冲突：'%1' 的值已存在")
                            .arg(pkFields.join(", ")),
                        QVariant()};
            }
        }
    }

    return {ResponseStatus::OK, "[Schema] 记录校验通过", QVariant()};
}
