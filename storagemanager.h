#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <QString>
#include <QFile>
#include <QDataStream>
#include <QDateTime>
#include "common.h"

    // 按照需求文档 3.12.5 定义表格信息块的物理存储结构
    struct TableBlock {
    char name[128];       // 表格名称
    int record_num;       // 记录数
    int field_num;        // 字段数
    char tdf[256];        // 表格定义文件路径
    char tic[256];        // 完整性文件路径
    char trd[256];        // 记录文件路径
    char tid[256];        // 索引文件路径
    qint64 crtime;        // 创建时间 (用时间戳替代 DATETIME)
    qint64 mtime;         // 最后修改时间
};

class StorageManager
{
public:
    StorageManager();

    // 阶段一：创建数据库文件夹
    bool createDatabase(QString dbName);

    /**
     * @brief 阶段二任务：创建表物理文件并初始化头部信息 [cite: 49]
     * @param dbName 数据库名称
     * @param tableName 表名称
     * @return 成功返回 true，失败返回 false
     */
    bool createTable(QString dbName, QString tableName);

    /**
     * @brief 阶段二任务：创建表物理文件并写入表结构 [cite: 52, 53]
     * @param dbName 数据库名称
     * @param tableName 表名称
     * @param fields 字段列表
     * @return 成功返回 true，失败返回 false
     */
    bool createTable(QString dbName, QString tableName, const QList<Field> &fields);

    /**
     * @brief 读取 .tdf 文件中的表结构
     * @param dbName 数据库名称
     * @param tableName 表名称
     * @return 字段列表，如果失败返回空列表
     */
    QList<Field> loadTableSchema(QString dbName, QString tableName);
};

#endif // STORAGEMANAGER_H
