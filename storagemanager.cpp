#include "storagemanager.h"
#include <QDir>
#include <QDebug>
#include <cstring> // 用于 strncpy
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

    StorageManager::StorageManager() {}

bool StorageManager::createDatabase(QString dbName)
{
    // ... （保持你原有的阶段一代码不变）...
    QString rootPath = Config::DATA_PATH;
    QDir dir;
    if (!dir.exists(rootPath)) {
        if (!dir.mkpath(rootPath)) return false;
    }
    QString dbPath = rootPath + dbName;
    if (dir.exists(dbPath)) return true;
    if (dir.mkdir(dbPath)) {
        qDebug() << QString("[Storage] Folder \"%1\" created successfully.").arg(dbPath);
        return true;
    }
    return false;
}

bool StorageManager::createTable(QString dbName, QString tableName)
{
    // 1. 确认数据库文件夹是否存在
    QString dbPath = Config::DATA_PATH + dbName;
    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database folder does not exist:" << dbName;
        return false;
    }

    // 2. 构建需要创建的文件路径 [cite: 52, 53]
    QString tdfPath = dir.filePath(tableName + ".tdf");
    QString trdPath = dir.filePath(tableName + ".trd");
    QString tbPath  = dir.filePath(dbName + ".tb"); // 表描述文件 [cite: 403]

    QFile tdfFile(tdfPath);
    QFile trdFile(trdPath);

    // 3. 检查表是否已经存在
    if (tdfFile.exists() || trdFile.exists()) {
        qDebug() << "[Storage] Error: Table already exists:" << tableName;
        return false;
    }

    // 4. 创建 .tdf 和 .trd 物理文件 [cite: 51]
    if (!tdfFile.open(QIODevice::WriteOnly) || !trdFile.open(QIODevice::WriteOnly)) {
        qDebug() << "[Storage] Error: Failed to create table physical files.";
        return false;
    }
    tdfFile.close();
    trdFile.close();

    // 5. 按照 3.12.5 要求，组装 TableBlock 头部信息 [cite: 54, 407]
    TableBlock block;
    memset(&block, 0, sizeof(TableBlock)); // 初始化清空内存

    // 安全地拷贝字符串，防止溢出
    strncpy(block.name, tableName.toUtf8().constData(), sizeof(block.name) - 1);
    strncpy(block.tdf, tdfPath.toUtf8().constData(), sizeof(block.tdf) - 1);
    strncpy(block.trd, trdPath.toUtf8().constData(), sizeof(block.trd) - 1);

    block.record_num = 0; // 初始记录数为 0 [cite: 221]
    block.field_num = 0;  // 初始字段数为 0 [cite: 221]
    block.crtime = QDateTime::currentSecsSinceEpoch();
    block.mtime = block.crtime;

    // 6. 将表信息追加写入 .tb 文件
    QFile tbFile(tbPath);
    // Append 模式：如果文件不存在会自动创建，如果存在则在末尾追加
    if (tbFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        tbFile.write(reinterpret_cast<const char*>(&block), sizeof(TableBlock));
        tbFile.close();

        // 按照任务书要求的成功标准反馈
        qDebug() << QString("[Storage] Table %1 created in %2. files: .tdf and .trd generated.").arg(tableName, dbName);
        return true;
    } else {
        qDebug() << "[Storage] Error: Failed to update .tb file.";
        return false;
    }
}

bool StorageManager::createTable(QString dbName, QString tableName, const QList<Field> &fields)
{
    // 1. 确认数据库文件夹是否存在
    QString dbPath = Config::DATA_PATH + dbName;
    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database folder does not exist:" << dbName;
        return false;
    }

    // 2. 构建需要创建的文件路径 [cite: 52, 53]
    QString tdfPath = dir.filePath(tableName + ".tdf");
    QString trdPath = dir.filePath(tableName + ".trd");
    QString tbPath  = dir.filePath(dbName + ".tb");

    QFile tdfFile(tdfPath);
    QFile trdFile(trdPath);

    // 3. 检查表是否已经存在
    if (tdfFile.exists() || trdFile.exists()) {
        qDebug() << "[Storage] Error: Table already exists:" << tableName;
        return false;
    }

    // 4. 创建 .trd 物理文件（数据记录文件）
    if (!trdFile.open(QIODevice::WriteOnly)) {
        qDebug() << "[Storage] Error: Failed to create .trd file.";
        return false;
    }
    trdFile.close();

    // 5. 按照 3.12.5 要求，组装 TableBlock 头部信息 [cite: 54, 407]
    TableBlock block;
    memset(&block, 0, sizeof(TableBlock)); // 初始化清空内存

    // 安全地拷贝字符串，防止溢出
    strncpy(block.name, tableName.toUtf8().constData(), sizeof(block.name) - 1);
    strncpy(block.tdf, tdfPath.toUtf8().constData(), sizeof(block.tdf) - 1);
    strncpy(block.trd, trdPath.toUtf8().constData(), sizeof(block.trd) - 1);

    block.record_num = 0; // 初始记录数为 0 [cite: 221]
    block.field_num = fields.size();  // 设置字段数 [cite: 221]
    block.crtime = QDateTime::currentSecsSinceEpoch();
    block.mtime = block.crtime;

    // 6. 将表信息追加写入 .tb 文件
    QFile tbFile(tbPath);
    if (!tbFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "[Storage] Error: Failed to update .tb file.";
        return false;
    }
    tbFile.write(reinterpret_cast<const char*>(&block), sizeof(TableBlock));
    tbFile.close();

    // 7. 将字段结构写入 .tdf 文件 [cite: 52]
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

    if (!tdfFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[Storage] Error: Failed to create .tdf file.";
        return false;
    }

    QJsonDocument doc(schemaObj);
    tdfFile.write(doc.toJson(QJsonDocument::Indented));
    tdfFile.close();

    // 按照任务书要求的成功标准反馈
    qDebug() << QString("[Storage] Table %1 created in %2 with %3 fields. files: .tdf, .trd generated.").arg(tableName, dbName).arg(fields.size());
    return true;
}

QList<Field> StorageManager::loadTableSchema(QString dbName, QString tableName)
{
    QString tdfPath = Config::DATA_PATH + dbName + "/" + tableName + ".tdf";
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
