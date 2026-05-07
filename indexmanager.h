#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include <QString>
#include <QMap>
#include <QHash>
#include <QFile>
#include <QDataStream>
#include "common.h"

// 索引项结构
struct IndexEntry {
    QVariant key;       // 索引键值（主键值）
    int recordOffset;   // 记录在文件中的偏移量

    IndexEntry() : recordOffset(0) {}
    IndexEntry(const QVariant& k, int offset) : key(k), recordOffset(offset) {}
};

// 索引信息结构
struct IndexInfo {
    QString tableName;      // 表名
    QString indexName;      // 索引名
    QString fieldName;      // 索引字段名
    QString indexFile;      // 索引文件路径（.tid）
    FieldType fieldType;    // 字段类型

    IndexInfo() : fieldType(FieldType::TEXT) {}
};

class IndexManager
{
private:
    // 缓存已加载的索引：table_name -> field_name -> IndexInfo
    QMap<QString, QMap<QString, IndexInfo>> cachedIndexes;

    /**
     * @brief 获取索引文件路径
     */
    QString getIndexFilePath(const QString& username, const QString& dbName, 
                             const QString& tableName, const QString& fieldName);

    /**
     * @brief 从索引文件加载索引到内存
     */
    bool loadIndexFromDisk(const QString& indexFile, IndexInfo& indexInfo);

    /**
     * @brief 将索引保存到磁盘
     */
    bool saveIndexToDisk(const QString& indexFile, const IndexInfo& indexInfo);

public:
    IndexManager();

    /**
     * @brief 创建索引
     * @param username 用户名
     * @param dbName 数据库名
     * @param tableName 表名
     * @param indexName 索引名
     * @param fieldName 索引字段名
     * @return true成功，false失败
     */
    bool createIndex(const QString& username, const QString& dbName,
                     const QString& tableName, const QString& indexName,
                     const QString& fieldName, FieldType fieldType);

    /**
     * @brief 删除索引
     * @param username 用户名
     * @param dbName 数据库名
     * @param tableName 表名
     * @param indexName 索引名
     * @return true成功，false失败
     */
    bool dropIndex(const QString& username, const QString& dbName,
                   const QString& tableName, const QString& indexName);

    /**
     * @brief 根据主键值查找记录偏移量
     * @param username 用户名
     * @param dbName 数据库名
     * @param tableName 表名
     * @param indexName 索引名
     * @param keyValue 主键值
     * @return 记录偏移量，-1表示未找到
     */
    int lookup(const QString& username, const QString& dbName,
               const QString& tableName, const QString& indexName,
               const QVariant& keyValue);

    /**
     * @brief 添加索引项（插入记录时调用）
     * @param username 用户名
     * @param dbName 数据库名
     * @param tableName 表名
     * @param indexName 索引名
     * @param keyValue 键值
     * @param recordOffset 记录偏移量
     * @return true成功，false失败
     */
    bool addIndexEntry(const QString& username, const QString& dbName,
                       const QString& tableName, const QString& indexName,
                       const QVariant& keyValue, int recordOffset);

    /**
     * @brief 删除索引项（删除记录时调用）
     * @param username 用户名
     * @param dbName 数据库名
     * @param tableName 表名
     * @param indexName 索引名
     * @param keyValue 键值
     * @return true成功，false失败
     */
    bool removeIndexEntry(const QString& username, const QString& dbName,
                          const QString& tableName, const QString& indexName,
                          const QVariant& keyValue);

    /**
     * @brief 清除指定表的所有索引缓存
     */
    void clearTableIndexCache(const QString& tableName);
};

#endif // INDEXMANAGER_H
