#include "storagemanager.h"
#include "indexmanager.h"
#include <QDir>
#include <QDebug>
#include <cstring>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>

StorageManager::StorageManager() {}

StorageManager::~StorageManager() {}

//建库
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
    
    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, "*");
    
    if (dir.mkdir(dbPath)) {
        qDebug() << QString("[Storage] Folder \"%1\" created successfully.").arg(dbPath);
        lockManager.releaseWriteLock(username, dbName, "*");
        return true;
    }
    
    lockManager.releaseWriteLock(username, dbName, "*");
    return false;
}

//快速建表
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

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, tableName);
    
    QFile tbFile(tbPath);
    if (tbFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        tbFile.write(reinterpret_cast<const char*>(&block), sizeof(TableBlock));
        tbFile.close();
        qDebug() << QString("[Storage] Table %1 created in %2. files: .tdf and .trd generated.").arg(tableName, dbName);
        // ✅ 先释放表锁
        lockManager.releaseWriteLock(username, dbName, tableName);

        // ✅ 再调用获取全局日志锁的 writeLog
        writeLog(username, dbName, QString("CREATE TABLE: %1. Fields: 0").arg(tableName));
        return true;
    } else {
        qDebug() << "[Storage] Error: Failed to update .tb file.";
        lockManager.releaseWriteLock(username, dbName, tableName);
        return false;
    }
}

//元数据写入
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
    
    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, tableName);
    
    if (!tdfFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[Storage] Failed to open .tdf file for writing:" << tdfPath;
        lockManager.releaseWriteLock(username, dbName, tableName);
        return false;
    }

    tdfFile.write(data);
    tdfFile.close();
    
    lockManager.releaseWriteLock(username, dbName, tableName);
    
    qDebug() << QString("[Storage] Written table definition to %1").arg(tdfPath);
    return true;
}

//建表
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

    // 验证外键引用的表是否存在
    for (const Field &field : fields) {
        if (field.isForeignKey && !field.referenceTable.isEmpty()) {
            QString refTableTdfPath = dir.filePath(field.referenceTable + ".tdf");
            if (!QFile(refTableTdfPath).exists()) {
                qDebug() << "[Storage] Error: Foreign key reference table '" << field.referenceTable << "' not found for field '" << field.name << "'";
                return false;
            }
        }
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
        fieldObj["isUnique"] = field.isUnique;
        fieldObj["hasCheck"] = field.hasCheck;
        fieldObj["checkExpr"] = field.checkExpr;
        fieldObj["defaultValue"] = field.defaultValue;
        fieldObj["hasIndex"] = field.hasIndex;
        fieldObj["isForeignKey"] = field.isForeignKey;
        fieldObj["referenceTable"] = field.referenceTable;
        fieldObj["referenceField"] = field.referenceField;
        fieldObj["cascadeRule"] = field.cascadeRule;
        fieldObj["formatValidation"] = field.formatValidation;
        fieldObj["isEncrypted"] = field.isEncrypted;
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

    // 获取写锁，防止并发写入
    lockManager.acquireWriteLock(username, dbName, tableName);
    
    QFile tbFile(tbPath);
    if (!tbFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "[Storage] Error: Failed to update .tb file.";
        lockManager.releaseWriteLock(username, dbName, tableName);
        return false;
    }
    tbFile.write(reinterpret_cast<const char*>(&block), sizeof(TableBlock));
    tbFile.close();
    
    lockManager.releaseWriteLock(username, dbName, tableName);

    writeLog(username, dbName, QString("CREATE TABLE: %1. Fields: %2").arg(tableName).arg(fields.size()));
    qDebug() << QString("[Storage] Table %1 created in %2 with %3 fields. files: .tdf, .trd generated.")
                    .arg(tableName, dbName).arg(fields.size());
    return true;
}

//元数据读取
QList<Field> StorageManager::loadTableSchema(const QString &username, QString dbName, QString tableName)
{
    QString tdfPath = Config::DATA_PATH + username + "/" + dbName + "/" + tableName + ".tdf";
    QFile file(tdfPath);

    if (!file.exists()) {
        qDebug() << "[Storage] Error: .tdf file not found:" << tdfPath;
        return QList<Field>();
    }

    // 获取读锁
    lockManager.acquireReadLock(username, dbName, tableName);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[Storage] Error: Failed to open .tdf file:" << tdfPath;
        lockManager.releaseReadLock(username, dbName, tableName);
        return QList<Field>();
    }

    QByteArray data = file.readAll();
    file.close();
    
    lockManager.releaseReadLock(username, dbName, tableName);

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
        field.isUnique = fieldObj["isUnique"].toBool();
        field.hasCheck = fieldObj["hasCheck"].toBool();
        field.checkExpr = fieldObj["checkExpr"].toString();
        field.defaultValue = fieldObj["defaultValue"].toString();
        field.hasIndex = fieldObj["hasIndex"].toBool();
        field.isForeignKey = fieldObj["isForeignKey"].toBool();
        field.referenceTable = fieldObj["referenceTable"].toString();
        field.referenceField = fieldObj["referenceField"].toString();
        field.cascadeRule = fieldObj["cascadeRule"].toString();
        field.formatValidation = fieldObj["formatValidation"].toString();
        field.isEncrypted = fieldObj["isEncrypted"].toBool();
        fields.append(field);
    }

    qDebug() << QString("[Storage] Loaded schema for table %1 with %2 fields").arg(tableName).arg(fields.size());
    return fields;
}

//删除表物理文件
bool StorageManager::dropTable(const QString &username, const QString &dbName, const QString &tableName)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database folder does not exist:" << dbName;
        return false;
    }

    QString tdfPath = dir.filePath(tableName + ".tdf");
    QString trdPath = dir.filePath(tableName + ".trd");

    QFile tdfFile(tdfPath);
    QFile trdFile(trdPath);

    // 1. 获取写锁
    lockManager.acquireWriteLock(username, dbName, tableName);

    bool success = true;

    // 2. 物理删除文件
    if (tdfFile.exists()) {
        if (!tdfFile.remove()) success = false;
    }
    if (trdFile.exists()) {
        if (!trdFile.remove()) success = false;
    }

    if (success) {
        qDebug() << QString("[Storage] Table %1 dropped successfully from %2.").arg(tableName, dbName);
    }

    // 3. ✅ 释放表锁
    lockManager.releaseWriteLock(username, dbName, tableName);

    // 4. ✅ 释放锁后再执行写日志（防止日志操作尝试获取锁时产生死锁）
    if (success) {
        writeLog(username, dbName, QString("DROP TABLE: %1").arg(tableName));
    }

    // 5. ✅ 清除对应的内存缓存
    clearTableCache(dbName, tableName);

    return success;
}

//删除整个数据库文件夹
bool StorageManager::dropDatabase(const QString &username, const QString &dbName)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);

    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database does not exist:" << dbName;
        return false;
    }

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, "*");
    
    // removeRecursively() 会删除该目录下的所有文件以及目录本身
    if (dir.removeRecursively()) {
        qDebug() << QString("[Storage] Database %1 and all its files dropped successfully.").arg(dbName);

        dataCache.clear();

        lockManager.releaseWriteLock(username, dbName, "*");
        return true;
    } else {
        qDebug() << "[Storage] Error: Failed to drop database. Some files might be locked/in use.";
        lockManager.releaseWriteLock(username, dbName, "*");
        return false;
    }
}

//改表结构
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
    // ✅ 在这里才开始加锁！仅保护 .tb 文件的写入
    lockManager.acquireWriteLock(username, dbName, tableName);
    QFile tbFile(tbPath);
    // 使用 ReadWrite 模式，既能读找位置，又能写，且不会清空原文件
    if (!tbFile.open(QIODevice::ReadWrite)) {
        qDebug() << "[Storage] Error: Failed to open .tb file for altering.";
        lockManager.releaseWriteLock(username, dbName, tableName);
        return false;
    }
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
    
    lockManager.releaseWriteLock(username, dbName, tableName);

    writeLog(username, dbName, QString("ALTER TABLE: %1. Fields modified.").arg(tableName));
    qDebug() << QString("[Storage] Table %1 altered successfully. New field count: %2.").arg(tableName).arg(newFields.size());
    return true;
}

//写日志
void StorageManager::writeLog(const QString &username, const QString &dbName, const QString &logMessage)
{
    // 定位到该数据库下的 ruanko.log
    QString logPath = Config::DATA_PATH + username + "/" + dbName + "/ruanko.log";
    QFile logFile(logPath);

    // 以追加 (Append) 和文本 (Text) 模式打开。如果文件不存在会自动创建。
    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, "*");
    
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
    
    lockManager.releaseWriteLock(username, dbName, "*");
}

//创建索引文件
bool StorageManager::createIndexFile(const QString &username, const QString &dbName,
                                      const QString &tableName, const QString &fieldName, FieldType fieldType)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database folder does not exist:" << dbName;
        return false;
    }

    // 创建索引目录
    QString indexDir = dir.filePath("indexes");
    if (!dir.exists(indexDir)) {
        if (!dir.mkpath(indexDir)) {
            qDebug() << "[Storage] Error: Failed to create indexes directory";
            return false;
        }
    }

    // 创建索引文件路径
    QString indexFilePath = QString("%1/%2_%3.tid")
                            .arg(indexDir)
                            .arg(fieldName)
                            .arg(tableName);

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, tableName);

    // 创建空的索引文件
    QFile indexFile(indexFilePath);
    if (!indexFile.open(QIODevice::WriteOnly)) {
        qDebug() << "[Storage] Error: Failed to create index file:" << indexFilePath;
        lockManager.releaseWriteLock(username, dbName, tableName);
        return false;
    }
    indexFile.close();
    
    lockManager.releaseWriteLock(username, dbName, tableName);

    qDebug() << QString("[Storage] Index file created: %1").arg(indexFilePath);
    return true;
}

//删除索引文件
bool StorageManager::dropIndexFile(const QString &username, const QString &dbName,
                                    const QString &tableName, const QString &fieldName)
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << "[Storage] Error: Database folder does not exist:" << dbName;
        return false;
    }

    QString indexFilePath = QString("%1/indexes/%2_%3.tid")
                            .arg(dbPath)
                            .arg(fieldName)
                            .arg(tableName);

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, tableName);

    QFile indexFile(indexFilePath);
    if (!indexFile.exists()) {
        qDebug() << "[Storage] Warning: Index file does not exist:" << indexFilePath;
        lockManager.releaseWriteLock(username, dbName, tableName);
        return false;
    }

    bool success = indexFile.remove();
    
    lockManager.releaseWriteLock(username, dbName, tableName);
    
    if (success) {
        qDebug() << QString("[Storage] Index file deleted: %1").arg(indexFilePath);
        return true;
    } else {
        qDebug() << "[Storage] Error: Failed to delete index file:" << indexFilePath;
        return false;
    }
}

// ========================================================
// 阶段五：查询性能优化与缓存机制 (第一周补全)
// ========================================================

// 带缓存机制的读取
QByteArray StorageManager::readTableData(const QString &username, const QString &dbName, const QString &tableName)
{
    QString cacheKey = dbName + "_" + tableName;

    // 获取读锁
    lockManager.acquireReadLock(username, dbName, tableName);

    // 1. 尝试从缓存中命中
    if (dataCache.contains(cacheKey)) {
        QByteArray cachedData = dataCache.value(cacheKey);
        lockManager.releaseReadLock(username, dbName, tableName);
        // qDebug() << "[Storage] Cache HIT for table:" << tableName; // 测试时可解开注释看效果
        return cachedData;
    }

    // 2. 缓存未命中，从物理磁盘读取
    recordDiskRead(); // 埋点记录物理磁盘读取
    QString trdPath = Config::DATA_PATH + username + "/" + dbName + "/" + tableName + ".trd";
    QFile trdFile(trdPath);

    if (!trdFile.open(QIODevice::ReadOnly)) {
        qDebug() << "[Storage] Error: Failed to read .trd file:" << trdPath;
        lockManager.releaseReadLock(username, dbName, tableName);
        return QByteArray();
    }
    QByteArray data = trdFile.readAll();
    trdFile.close();

    // 3. 写入缓存，以便下次极速读取
    dataCache.insert(cacheKey, data);
    lockManager.releaseReadLock(username, dbName, tableName);

    // qDebug() << "[Storage] Disk read & Cache UPDATED for table:" << tableName;
    return data;
}

// 同步写入硬盘与缓存
bool StorageManager::writeTableData(const QString &username, const QString &dbName, const QString &tableName, const QByteArray &data)
{
    recordDiskWrite(); // 埋点记录物理磁盘写入

    QString trdPath = Config::DATA_PATH + username + "/" + dbName + "/" + tableName + ".trd";
    QFile trdFile(trdPath);

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, tableName);

    if (!trdFile.open(QIODevice::WriteOnly)) {
        qDebug() << "[Storage] Error: Failed to write .trd file:" << trdPath;
        lockManager.releaseWriteLock(username, dbName, tableName);
        return false;
    }
    trdFile.write(data);
    trdFile.close();

    // 同步更新缓存，防止脏读
    QString cacheKey = dbName + "_" + tableName;
    dataCache.insert(cacheKey, data);

    lockManager.releaseWriteLock(username, dbName, tableName);
    return true;
}

// 清除特定缓存
void StorageManager::clearTableCache(const QString &dbName, const QString &tableName)
{
    QString cacheKey = dbName + "_" + tableName;
    dataCache.remove(cacheKey);
}

//开始事务
bool StorageManager::beginTransaction(const QString &username, const QString &dbName)
{
    QString logPath = Config::DATA_PATH + username + "/" + dbName + "/ruanko.log";
    QFile logFile(logPath);

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, "*");

    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        out << "[" << timestamp << "] BEGIN TRANSACTION" << "\n";
        logFile.close();
        qDebug() << QString("[Storage] Transaction started for database: %1").arg(dbName);
        lockManager.releaseWriteLock(username, dbName, "*");
        return true;
    } else {
        qDebug() << "[Storage] Error: Could not write to log file:" << logPath;
        lockManager.releaseWriteLock(username, dbName, "*");
        return false;
    }
}

//提交事务
bool StorageManager::commitTransaction(const QString &username, const QString &dbName)
{
    QString logPath = Config::DATA_PATH + username + "/" + dbName + "/ruanko.log";
    QFile logFile(logPath);

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, "*");

    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        out << "[" << timestamp << "] COMMIT TRANSACTION" << "\n";
        logFile.close();
        qDebug() << QString("[Storage] Transaction committed for database: %1").arg(dbName);
        lockManager.releaseWriteLock(username, dbName, "*");
        return true;
    } else {
        qDebug() << "[Storage] Error: Could not write to log file:" << logPath;
        lockManager.releaseWriteLock(username, dbName, "*");
        return false;
    }
}

//回滚事务
bool StorageManager::rollbackTransaction(const QString &username, const QString &dbName)
{
    QString logPath = Config::DATA_PATH + username + "/" + dbName + "/ruanko.log";
    QFile logFile(logPath);

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, "*");

    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        out << "[" << timestamp << "] ROLLBACK TRANSACTION" << "\n";
        logFile.close();
        qDebug() << QString("[Storage] Transaction rolled back for database: %1").arg(dbName);
        lockManager.releaseWriteLock(username, dbName, "*");
        return true;
    } else {
        qDebug() << "[Storage] Error: Could not write to log file:" << logPath;
        lockManager.releaseWriteLock(username, dbName, "*");
        return false;
    }

}

// ========================================================
// 阶段五第二周：系统监控功能 (任务5)
// ========================================================

void StorageManager::recordQueryTime(qint64 timeMs)
{
    // 获取写锁保护监控数据的并发修改 (这里为了性能可以使用简单的互斥锁，但复用 lockManager 的全局锁亦可)
    lockManager.acquireWriteLock("SYSTEM", "MONITOR", "STATS");
    monitorStats.queryCount++;
    monitorStats.totalQueryTimeMs += timeMs;
    lockManager.releaseWriteLock("SYSTEM", "MONITOR", "STATS");
}

void StorageManager::recordDiskRead()
{
    lockManager.acquireWriteLock("SYSTEM", "MONITOR", "STATS");
    monitorStats.totalDiskReads++;
    lockManager.releaseWriteLock("SYSTEM", "MONITOR", "STATS");
}

void StorageManager::recordDiskWrite()
{
    lockManager.acquireWriteLock("SYSTEM", "MONITOR", "STATS");
    monitorStats.totalDiskWrites++;
    lockManager.releaseWriteLock("SYSTEM", "MONITOR", "STATS");
}

SystemMonitorStats StorageManager::getSystemStats() const
{
    return monitorStats;
}

void StorageManager::printSystemStats() const
{
    qDebug() << "\n📊 === DBMS 系统性能监控报告 ===";
    qDebug() << "💿 物理磁盘读取次数 :" << monitorStats.totalDiskReads << "(未命中缓存次数)";
    qDebug() << "💾 物理磁盘写入次数 :" << monitorStats.totalDiskWrites;
    qDebug() << "🔍 记录的总查询次数 :" << monitorStats.queryCount;
    if (monitorStats.queryCount > 0) {
        double avgTime = static_cast<double>(monitorStats.totalQueryTimeMs) / monitorStats.queryCount;
        qDebug() << "⏱️ 平均查询执行耗时 :" << avgTime << "ms";
    } else {
        qDebug() << "⏱️ 平均查询执行耗时 : 0 ms";
    }
    qDebug() << "=================================\n";
}

void StorageManager::resetSystemStats()
{
    lockManager.acquireWriteLock("SYSTEM", "MONITOR", "STATS");
    monitorStats = SystemMonitorStats();
    lockManager.releaseWriteLock("SYSTEM", "MONITOR", "STATS");
    qDebug() << "[Monitor] System statistics have been reset.";
}

// ========================================================
// 阶段五第二周：数据备份与恢复 (任务6)
// ========================================================
bool StorageManager::backupDatabase(const QString &username, const QString &dbName)
{
    QString sourcePath = Config::DATA_PATH + username + "/" + dbName;
    QDir srcDir(sourcePath);
    QString absPath = srcDir.absolutePath();
    qDebug() << "[Storage] backupDatabase: sourcePath=" << sourcePath << "absPath=" << absPath;
    if (!srcDir.exists()) {
        qDebug() << "[Storage] Backup Error: Source database not found:" << absPath;
        qDebug() << "[Storage] currentPath=" << QDir::currentPath();
        return false;
    }

    // 生成带时间戳的备份文件夹名
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString backupPath = sourcePath + "_backup_" + timestamp;

    // 获取该数据库的全局写锁
    lockManager.acquireWriteLock(username, dbName, "*");

    bool success = copyDirectory(sourcePath, backupPath);

    // ✅ 第一步：先释放锁！
    lockManager.releaseWriteLock(username, dbName, "*");

    // ✅ 第二步：再执行写日志和打印
    if (success) {
        writeLog(username, dbName, "DATABASE BACKUP CREATED: " + backupPath);
        qDebug() << "[Storage] Backup created successfully at:" << backupPath;
    } else {
        qDebug() << "[Storage] Backup failed.";
    }

    return success;
}

bool StorageManager::restoreDatabase(const QString &username, const QString &dbName, const QString &backupFolderName)
{
    QString sourcePath = Config::DATA_PATH + username + "/" + dbName;
    QString backupPath = Config::DATA_PATH + username + "/" + backupFolderName;

    if (!QDir(backupPath).exists()) {
        qDebug() << "[Storage] Restore Error: Backup folder not found.";
        return false;
    }

    // 获取最高级别写锁
    lockManager.acquireWriteLock(username, dbName, "*");

    // 1. 物理删除现有数据库文件夹
    QDir srcDir(sourcePath);
    if (srcDir.exists()) {
        srcDir.removeRecursively();
    }

    // 2. 将备份内容拷贝回原位置
    bool success = copyDirectory(backupPath, sourcePath);

    if (success) {
        // 恢复后清空一下内存缓存
        dataCache.clear();
    }

    // ✅ 第一步：先释放锁！
    lockManager.releaseWriteLock(username, dbName, "*");

    // ✅ 第二步：再执行写日志和打印
    if (success) {
        writeLog(username, dbName, "DATABASE RESTORED FROM: " + backupFolderName);
        qDebug() << "[Storage] Database restored successfully from:" << backupFolderName;
    }

    return success;
}

// 递归拷贝辅助函数
bool StorageManager::copyDirectory(const QString &srcPath, const QString &dstPath)
{
    QString absSrc = QDir(srcPath).absolutePath();
    QString absDst = QDir(dstPath).absolutePath();
    qDebug() << "[Storage] copyDirectory:" << absSrc << "->" << absDst;

    QDir sourceDir(absSrc);
    if (!sourceDir.exists()) {
        qDebug() << "[Storage] copyDirectory: source not found:" << absSrc;
        return false;
    }

    QDir destDir(absDst);
    if (!destDir.exists()) {
        bool ok = destDir.mkpath(absDst);
        qDebug() << "[Storage] copyDirectory: mkpath" << absDst << "=" << ok;
        if (!ok) return false;
    }

    foreach (QString fileName, sourceDir.entryList(QDir::Files)) {
        QString srcFilePath = absSrc + "/" + fileName;
        QString dstFilePath = absDst + "/" + fileName;
        bool ok = QFile::copy(srcFilePath, dstFilePath);
        qDebug() << "[Storage] copyDirectory: copy" << fileName << "=" << ok;
        if (!ok) return false;
    }

    foreach (QString subDirName, sourceDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString srcSubPath = absSrc + "/" + subDirName;
        QString dstSubPath = absDst + "/" + subDirName;
        if (!copyDirectory(srcSubPath, dstSubPath)) return false;
    }

    return true;
}

