#include "storagemanager.h"
#include <QDir>
#include <QDebug>
#include <cstring>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>

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
    if (dir.exists(dbPath)) {
        // 存在则返回 false 并说明原因
        qDebug() << "[Storage] Error: Database already exists:" << dbName;
        return false;
    }
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
        writeLog(username, dbName, QString("CREATE TABLE: %1. Fields: 0").arg(tableName));
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

    writeLog(username, dbName, QString("CREATE TABLE: %1. Fields: %2").arg(tableName).arg(fields.size()));
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

bool StorageManager::dropTable(const QString &username, const QString &dbName, const QString &tableName)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database folder does not exist:" << dbName;
        return false;
    }

    // 锁定你要删除的两个目标文件
    QString tdfPath = dir.filePath(tableName + ".tdf");
    QString trdPath = dir.filePath(tableName + ".trd");

    QFile tdfFile(tdfPath);
    QFile trdFile(trdPath);

    bool success = true;

    // 检查并删除 .tdf 文件
    if (tdfFile.exists()) {
        if (!tdfFile.remove()) {
            qDebug() << "[Storage] Error: Failed to remove .tdf file. It might be in use by another stream.";
            success = false;
        }
    }

    // 检查并删除 .trd 文件
    if (trdFile.exists()) {
        if (!trdFile.remove()) {
            qDebug() << "[Storage] Error: Failed to remove .trd file. It might be in use.";
            success = false;
        }
    }

    if (success) {
        writeLog(username, dbName, QString("DROP TABLE: %1").arg(tableName));
        qDebug() << QString("[Storage] Table %1 dropped successfully from %2.").arg(tableName, dbName);
    }
    return success;
}

bool StorageManager::dropDatabase(const QString &username, const QString &dbName)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);

    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database does not exist:" << dbName;
        return false;
    }

    // removeRecursively() 会删除该目录下的所有文件以及目录本身
    if (dir.removeRecursively()) {
        qDebug() << QString("[Storage] Database %1 and all its files dropped successfully.").arg(dbName);
        return true;
    } else {
        qDebug() << "[Storage] Error: Failed to drop database. Some files might be locked/in use.";
        return false;
    }
}

bool StorageManager::alterTable(const QString &username, const QString &dbName, const QString &tableName, const QList<Field> &newFields)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database does not exist.";
        return false;
    }

    QString tdfPath = dir.filePath(tableName + ".tdf");
    QString tbPath = dir.filePath(dbName + ".tb");

    if (!QFile::exists(tdfPath)) {
        qDebug() << "[Storage] Error: Table does not exist:" << tableName;
        return false;
    }

    // 1. 更新 .tdf 文件 (把最新的字段结构序列化为 JSON 覆盖进去)
    QJsonObject schemaObj;
    schemaObj["tableName"] = tableName;
    schemaObj["fieldCount"] = newFields.size();
    QJsonArray fieldsArray;
    for (const Field &field : newFields) {
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
        qDebug() << "[Storage] Error: Failed to rewrite .tdf file.";
        return false;
    }

    // 2. 更新 .tb 二进制表头文件中的 field_num 和 mtime
    QFile tbFile(tbPath);
    // 使用 ReadWrite 模式，既能读找位置，又能写，且不会清空原文件
    if (tbFile.open(QIODevice::ReadWrite)) {
        TableBlock block;
        bool found = false;
        qint64 pos = 0; // 记录当前游标位置

        // 遍历二进制文件中的所有 TableBlock
        while (tbFile.read(reinterpret_cast<char*>(&block), sizeof(TableBlock)) == sizeof(TableBlock)) {
            if (QString(block.name) == tableName) {
                found = true;
                break;
            }
            pos += sizeof(TableBlock);
        }

        if (found) {
            block.field_num = newFields.size(); // 更新字段数
            block.mtime = QDateTime::currentSecsSinceEpoch(); // 更新修改时间

            tbFile.seek(pos); // 把游标指针拨回到这个块的开头
            tbFile.write(reinterpret_cast<const char*>(&block), sizeof(TableBlock)); // 覆盖重写
        }
        tbFile.close();
    } else {
        qDebug() << "[Storage] Error: Failed to open .tb file for altering.";
        return false;
    }

    writeLog(username, dbName, QString("ALTER TABLE: %1. Fields modified.").arg(tableName));
    qDebug() << QString("[Storage] Table %1 altered successfully. New field count: %2.").arg(tableName).arg(newFields.size());
    return true;
}

void StorageManager::writeLog(const QString &username, const QString &dbName, const QString &logMessage)
{
    // 定位到该数据库下的 ruanko.log
    QString logPath = Config::DATA_PATH + username + "/" + dbName + "/ruanko.log";
    QFile logFile(logPath);

    // 以追加 (Append) 和文本 (Text) 模式打开。如果文件不存在会自动创建。
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        // 生成标准时间戳格式
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

        // 写入日志并换行
        out << "[" << timestamp << "] " << logMessage << "\n";
        logFile.close();
    } else {
        qDebug() << "[Storage] Warning: Could not write to log file:" << logPath;
    }
}
