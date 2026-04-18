#include "storagemanager.h"
#include <QDir>
#include <QDebug>
#include <cstring> // 用于 strncpy

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
