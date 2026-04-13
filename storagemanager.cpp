#include "storagemanager.h"
#include <QDir>
#include <QDebug>

StorageManager::StorageManager() {}

bool StorageManager::createDatabase(QString dbName)
{
    // 1. 获取数据存储根目录（来自 common.h 的 "./data/"）
    QString rootPath = Config::DATA_PATH;

    // 2. 检查并创建根目录文件夹 (data)
    QDir dir;
    if (!dir.exists(rootPath)) {
        // mkpath 可以递归创建不存在的父目录
        if (!dir.mkpath(rootPath)) {
            qDebug() << "[Storage] Critical: Could not create root data directory.";
            return false;
        }
    }

    // 3. 构建完整的数据库路径（例如 "./data/Student_System"） [cite: 10]
    QString dbPath = rootPath + dbName;

    // 4. 使用 QDir().mkdir 创建新文件夹 [cite: 10]
    if (dir.exists(dbPath)) {
        qDebug() << "[Storage] Database folder already exists:" << dbPath;
        return true; // 已经存在则视为成功
    }

    if (dir.mkdir(dbPath)) {
        // 5. 按照任务书要求的格式输出成功日志
        // 使用 .arg() 动态填充路径到字符串中
        qDebug() << QString("[Storage] Folder \"%1\" created successfully.").arg(dbPath);
        return true;
    } else {
        qDebug() << "[Storage] Failed to create database folder:" << dbName;
        return false;
    }
}
