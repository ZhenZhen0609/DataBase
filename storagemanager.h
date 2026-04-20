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

    bool createDatabase(const QString &username, const QString &dbName);
    bool createTable(const QString &username, QString dbName, QString tableName);
    bool createTable(const QString &username, QString dbName, QString tableName, const QList<Field> &fields);
    bool writeTableDefinition(const QString &username, const QString &dbName, const QString &tableName, const QByteArray &data);
    QList<Field> loadTableSchema(const QString &username, QString dbName, QString tableName);
};

#endif // STORAGEMANAGER_H
