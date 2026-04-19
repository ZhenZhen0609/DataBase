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
    // 主键字段必须非空
    if (field.isPrimaryKey && (value.isNull() || value.toString().isEmpty())) {
        qDebug() << "[Schema] Primary key field" << field.name << "cannot be null";
        return false;
    }

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

// ==================== 新增：橙圈B核心实现 ====================

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
    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Indented);
}

bool SchemaManager::saveSchema(const QString &dbName, const QString &tableName, const QList<Field> &fields)
{
    // 1. 字段规则校验
    if (fields.isEmpty()) {
        qDebug() << "[Schema] Cannot save schema: field list is empty.";
        return false;
    }

    // 检查字段名是否重复
    QStringList fieldNames;
    for (const Field &f : fields) {
        if (fieldNames.contains(f.name)) {
            qDebug() << "[Schema] Duplicate field name:" << f.name;
            return false;
        }
        fieldNames.append(f.name);
    }

    // 2. 序列化字段定义（转为 JSON 字节流）
    QByteArray schemaData = serializeSchema(fields);
    if (schemaData.isEmpty()) {
        qDebug() << "[Schema] Failed to serialize fields.";
        return false;
    }

    // 3. 确保数据库目录存在
    if (!ensureDbDirectory(dbName)) {
        qDebug() << "[Schema] Database directory does not exist and cannot be created:" << dbName;
        return false;
    }

    // 4. 调用接口：写入 .tdf 定义文件
    StorageManager storage;
    if (!storage.writeTableDefinition(dbName, tableName, schemaData)) {
        qDebug() << "[Schema] Failed to write table definition via StorageManager.";
        return false;
    }

    qDebug() << QString("[Schema] Schema saved for table '%1' with %2 fields.").arg(tableName).arg(fields.size());
    return true;
}

// 兼容保留，内部调用新接口

Response SchemaManager::createTable(const QString &dbName, const TableSchema &schema)
{
    // 直接复用 saveSchema
    if (!saveSchema(dbName, schema.tableName, schema.fields)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to save schema for table '%1'").arg(schema.tableName), QVariant()};
    }

    // 创建物理文件（.trd 和 .tb）
    StorageManager storage;
    if (!storage.createTable(dbName, schema.tableName, schema.fields)) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to create physical files for table '%1'").arg(schema.tableName), QVariant()};
    }

    qDebug() << QString("[Schema] Table '%1' created successfully with %2 fields").arg(schema.tableName).arg(schema.fields.size());
    return {ResponseStatus::OK, QString("[Schema] Table '%1' created successfully").arg(schema.tableName), QVariant()};
}

Response SchemaManager::loadTableSchema(const QString &dbName, const QString &tableName)
{
    StorageManager storageManager;
    QList<Field> fields = storageManager.loadTableSchema(dbName, tableName);

    if (fields.isEmpty()) {
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

    QStringList filters;
    filters << "*.tdf";
    dir.setNameFilters(filters);
    QStringList tdfFiles = dir.entryList(QDir::Files);

    QList<TableSchema> schemas;
    StorageManager storageManager;

    for (const QString &tdfFile : tdfFiles) {
        QString tableName = tdfFile.left(tdfFile.size() - 4);
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

    QString tdfPath = dir.filePath(tableName + ".tdf");
    QString trdPath = dir.filePath(tableName + ".trd");

    if (!QFile::exists(tdfPath)) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Schema] Table '%1' not found").arg(tableName), QVariant()};
    }

    bool tdfRemoved = QFile::remove(tdfPath);
    bool trdRemoved = QFile::remove(trdPath);

    if (!tdfRemoved || !trdRemoved) {
        return {ResponseStatus::ERROR, QString("[Schema] Failed to delete table files for '%1'").arg(tableName), QVariant()};
    }

    QString jsonPath = dir.filePath(tableName + ".json");
    QFile::remove(jsonPath);

    qDebug() << QString("[Schema] Table '%1' dropped successfully").arg(tableName);
    return {ResponseStatus::OK, QString("[Schema] Table '%1' dropped successfully").arg(tableName), QVariant()};
}
