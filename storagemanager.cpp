#include "storagemanager.h"
#include <QDir>
#include <QDebug>
#include <cstring>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

StorageManager::StorageManager() {}

bool StorageManager::createDatabase(const QString &username, const QString &dbName)
{
    QString rootPath = Config::DATA_PATH;
    QDir dir;
    if (!dir.exists(rootPath)) {
        if (!dir.mkpath(rootPath)) return false;
    }
    QString userPath = rootPath + username;
    if (!dir.exists(userPath)) {
        if (!dir.mkpath(userPath)) return false;
    }
    QString dbPath = userPath + "/" + dbName;
    if (dir.exists(dbPath)) return true;
    if (dir.mkdir(dbPath)) {
        qDebug() << QString("[Storage] Folder \"%1\" created successfully.").arg(dbPath);
        return true;
    }
    return false;
}

bool StorageManager::createTable(const QString &username, QString dbName, QString tableName)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database folder does not exist:" << dbName;
        return false;
    }

    QString tdfPath = dir.filePath(tableName + ".tdf");
    QString trdPath = dir.filePath(tableName + ".trd");
    QString tbPath  = dir.filePath(dbName + ".tb");

    QFile tdfFile(tdfPath);
    QFile trdFile(trdPath);

    if (tdfFile.exists() || trdFile.exists()) {
        qDebug() << "[Storage] Error: Table already exists:" << tableName;
        return false;
    }

    if (!tdfFile.open(QIODevice::WriteOnly) || !trdFile.open(QIODevice::WriteOnly)) {
        qDebug() << "[Storage] Error: Failed to create table physical files.";
        return false;
    }
    tdfFile.close();
    trdFile.close();

    TableBlock block;
    memset(&block, 0, sizeof(TableBlock));
    strncpy(block.name, tableName.toUtf8().constData(), sizeof(block.name) - 1);
    strncpy(block.tdf, tdfPath.toUtf8().constData(), sizeof(block.tdf) - 1);
    strncpy(block.trd, trdPath.toUtf8().constData(), sizeof(block.trd) - 1);
    block.record_num = 0;
    block.field_num = 0;
    block.crtime = QDateTime::currentSecsSinceEpoch();
    block.mtime = block.crtime;

    QFile tbFile(tbPath);
    if (tbFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        tbFile.write(reinterpret_cast<const char*>(&block), sizeof(TableBlock));
        tbFile.close();
        qDebug() << QString("[Storage] Table %1 created in %2. files: .tdf and .trd generated.").arg(tableName, dbName);
        return true;
    } else {
        qDebug() << "[Storage] Error: Failed to update .tb file.";
        return false;
    }
}

bool StorageManager::writeTableDefinition(const QString &username, const QString &dbName, const QString &tableName, const QByteArray &data)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << "[Storage] Database folder does not exist:" << dbName;
        return false;
    }

    QString tdfPath = dir.filePath(tableName + ".tdf");
    QFile tdfFile(tdfPath);
    if (!tdfFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[Storage] Failed to open .tdf file for writing:" << tdfPath;
        return false;
    }

    tdfFile.write(data);
    tdfFile.close();
    qDebug() << QString("[Storage] Written table definition to %1").arg(tdfPath);
    return true;
}

bool StorageManager::createTable(const QString &username, QString dbName, QString tableName, const QList<Field> &fields)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database folder does not exist:" << dbName;
        return false;
    }

    QString tdfPath = dir.filePath(tableName + ".tdf");
    QString trdPath = dir.filePath(tableName + ".trd");
    QString tbPath  = dir.filePath(dbName + ".tb");

    QFile tdfFile(tdfPath);
    QFile trdFile(trdPath);

    if (tdfFile.exists() || trdFile.exists()) {
        qDebug() << "[Storage] Error: Table already exists:" << tableName;
        return false;
    }

    if (!trdFile.open(QIODevice::WriteOnly)) {
        qDebug() << "[Storage] Error: Failed to create .trd file.";
        return false;
    }
    trdFile.close();

    // 序列化字段定义并写入 .tdf
    QJsonObject schemaObj;
    schemaObj["tableName"] = tableName;
    schemaObj["fieldCount"] = fields.size();
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
    schemaObj["fields"] = fieldsArray;
    QJsonDocument doc(schemaObj);
    QByteArray schemaData = doc.toJson(QJsonDocument::Indented);

    if (!writeTableDefinition(username, dbName, tableName, schemaData)) {
        qDebug() << "[Storage] Error: Failed to write .tdf file.";
        return false;
    }

    // 写入 .tb 表头信息
    TableBlock block;
    memset(&block, 0, sizeof(TableBlock));
    strncpy(block.name, tableName.toUtf8().constData(), sizeof(block.name) - 1);
    strncpy(block.tdf, tdfPath.toUtf8().constData(), sizeof(block.tdf) - 1);
    strncpy(block.trd, trdPath.toUtf8().constData(), sizeof(block.trd) - 1);
    block.record_num = 0;
    block.field_num = fields.size();
    block.crtime = QDateTime::currentSecsSinceEpoch();
    block.mtime = block.crtime;

    QFile tbFile(tbPath);
    if (!tbFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "[Storage] Error: Failed to update .tb file.";
        return false;
    }
    tbFile.write(reinterpret_cast<const char*>(&block), sizeof(TableBlock));
    tbFile.close();

    qDebug() << QString("[Storage] Table %1 created in %2 with %3 fields. files: .tdf, .trd generated.")
                    .arg(tableName, dbName).arg(fields.size());
    return true;
}

QList<Field> StorageManager::loadTableSchema(const QString &username, QString dbName, QString tableName)
{
    QString tdfPath = Config::DATA_PATH + username + "/" + dbName + "/" + tableName + ".tdf";
    QFile file(tdfPath);

    if (!file.exists()) {
        qDebug() << "[Storage] Error: .tdf file not found:" << tdfPath;
        return QList<Field>();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[Storage] Error: Failed to open .tdf file:" << tdfPath;
        return QList<Field>();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qDebug() << "[Storage] Error: Invalid .tdf file format:" << tdfPath;
        return QList<Field>();
    }

    QJsonObject schemaObj = doc.object();
    QJsonArray fieldsArray = schemaObj["fields"].toArray();

    QList<Field> fields;
    for (const QJsonValue &val : fieldsArray) {
        QJsonObject fieldObj = val.toObject();
        Field field;
        field.name = fieldObj["name"].toString();
        field.type = static_cast<FieldType>(fieldObj["type"].toInt());
        field.length = fieldObj["length"].toInt();
        field.isNotNull = fieldObj["isNotNull"].toBool();
        field.isPrimaryKey = fieldObj["isPrimaryKey"].toBool();
        fields.append(field);
    }

    qDebug() << QString("[Storage] Loaded schema for table %1 with %2 fields").arg(tableName).arg(fields.size());
    return fields;
}
