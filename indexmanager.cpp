#include "indexmanager.h"
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QStandardPaths>
#include <QDebug>

// 索引文件扩展名
const QString INDEX_EXT = ".tid";

IndexManager::IndexManager()
{
}

QString IndexManager::getIndexFilePath(const QString& username, const QString& dbName,
                                        const QString& tableName, const QString& fieldName)
{
    QString dataPath = Config::dataPath();
    QString indexDir = QString("%1%2/%3/indexes/")
                       .arg(dataPath)
                       .arg(dbName)
                       .arg(tableName);
    QDir dir(indexDir);
    if (!dir.exists()) {
        dir.mkpath(indexDir);
    }
    return QString("%1%2_%3%4")
            .arg(indexDir)
            .arg(fieldName)
            .arg(INDEX_EXT);
}

bool IndexManager::loadIndexFromDisk(const QString& indexFile, IndexInfo& indexInfo)
{
    QFile file(indexFile);
    if (!file.exists()) {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream in(&file);
    in >> indexInfo.tableName
       >> indexInfo.indexName
       >> indexInfo.fieldName
       >> indexInfo.indexFile
       >> indexInfo.fieldType;

    file.close();
    return true;
}

bool IndexManager::saveIndexToDisk(const QString& indexFile, const IndexInfo& indexInfo)
{
    QFile file(indexFile);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream out(&file);
    out << indexInfo.tableName
        << indexInfo.indexName
        << indexInfo.fieldName
        << indexInfo.indexFile
        << indexInfo.fieldType;

    file.close();
    return true;
}

bool IndexManager::createIndex(const QString& username, const QString& dbName,
                                const QString& tableName, const QString& indexName,
                                const QString& fieldName, FieldType fieldType)
{
    // 检查索引是否已存在
    if (cachedIndexes.contains(tableName) && cachedIndexes[tableName].contains(indexName)) {
        return false; // 索引已存在
    }

    QString indexFile = getIndexFilePath(username, dbName, tableName, fieldName);

    IndexInfo indexInfo;
    indexInfo.tableName = tableName;
    indexInfo.indexName = indexName;
    indexInfo.fieldName = fieldName;
    indexInfo.indexFile = indexFile;
    indexInfo.fieldType = fieldType;

    // 保存索引元数据到磁盘
    if (!saveIndexToDisk(indexFile, indexInfo)) {
        return false;
    }

    // 添加到缓存
    cachedIndexes[tableName][indexName] = indexInfo;

    return true;
}

bool IndexManager::dropIndex(const QString& username, const QString& dbName,
                              const QString& tableName, const QString& indexName)
{
    if (!cachedIndexes.contains(tableName) || !cachedIndexes[tableName].contains(indexName)) {
        return false; // 索引不存在
    }

    IndexInfo indexInfo = cachedIndexes[tableName][indexName];
    QString indexFile = indexInfo.indexFile;

    // 删除索引文件
    QFile::remove(indexFile);

    // 从缓存中移除
    cachedIndexes[tableName].remove(indexName);
    if (cachedIndexes[tableName].isEmpty()) {
        cachedIndexes.remove(tableName);
    }

    return true;
}

int IndexManager::lookup(const QString& username, const QString& dbName,
                          const QString& tableName, const QString& indexName,
                          const QVariant& keyValue)
{
    if (!cachedIndexes.contains(tableName) || !cachedIndexes[tableName].contains(indexName)) {
        return -1; // 索引不存在
    }

    IndexInfo indexInfo = cachedIndexes[tableName][indexName];
    QString indexFile = indexInfo.indexFile;

    if (!QFile::exists(indexFile)) {
        return -1;
    }

    // 从索引文件中查找
    QFile file(indexFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return -1;
    }

    QDataStream in(&file);
    while (!in.atEnd()) {
        IndexEntry entry;
        in >> entry.key >> entry.recordOffset;
        if (entry.key == keyValue) {
            file.close();
            return entry.recordOffset;
        }
    }

    file.close();
    return -1; // 未找到
}

bool IndexManager::addIndexEntry(const QString& username, const QString& dbName,
                                  const QString& tableName, const QString& indexName,
                                  const QVariant& keyValue, int recordOffset)
{
    if (!cachedIndexes.contains(tableName) || !cachedIndexes[tableName].contains(indexName)) {
        return false; // 索引不存在
    }

    IndexInfo indexInfo = cachedIndexes[tableName][indexName];
    QString indexFile = indexInfo.indexFile;

    // 以追加方式打开索引文件
    QFile file(indexFile);
    if (!file.open(QIODevice::Append)) {
        return false;
    }

    QDataStream out(&file);
    out << keyValue << recordOffset;

    file.close();
    return true;
}

bool IndexManager::removeIndexEntry(const QString& username, const QString& dbName,
                                     const QString& tableName, const QString& indexName,
                                     const QVariant& keyValue)
{
    if (!cachedIndexes.contains(tableName) || !cachedIndexes[tableName].contains(indexName)) {
        return false; // 索引不存在
    }

    IndexInfo indexInfo = cachedIndexes[tableName][indexName];
    QString indexFile = indexInfo.indexFile;

    if (!QFile::exists(indexFile)) {
        return false;
    }

    // 读取所有索引项，过滤掉要删除的项，然后写回
    QFile file(indexFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream in(&file);
    QList<IndexEntry> entries;
    while (!in.atEnd()) {
        IndexEntry entry;
        in >> entry.key >> entry.recordOffset;
        if (entry.key != keyValue) {
            entries.append(entry);
        }
    }
    file.close();

    // 写回剩余的索引项
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream out(&file);
    foreach (const IndexEntry& entry, entries) {
        out << entry.key << entry.recordOffset;
    }

    file.close();
    return true;
}

void IndexManager::clearTableIndexCache(const QString& tableName)
{
    cachedIndexes.remove(tableName);
}
