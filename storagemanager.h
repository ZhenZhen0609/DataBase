#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <QString>
#include <QFile>
#include <QDataStream>
#include <QDateTime>
#include "common.h"

    //定义表格信息块的物理存储结构
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
private:
    /**
     * @brief 内部辅助函数：向指定数据库的 ruanko.log 中追加日志
     */
    void writeLog(const QString &username, const QString &dbName, const QString &logMessage);

public:
    StorageManager();

    bool createDatabase(const QString &username, const QString &dbName);
    //快速建表
    bool createTable(const QString &username, QString dbName, QString tableName);
    //建表
    bool createTable(const QString &username, QString dbName, QString tableName, const QList<Field> &fields);
    bool writeTableDefinition(const QString &username, const QString &dbName, const QString &tableName, const QByteArray &data);
    QList<Field> loadTableSchema(const QString &username, QString dbName, QString tableName);

    // 删除表物理文件
    bool dropTable(const QString &username, const QString &dbName, const QString &tableName);
    // 删除整个数据库文件夹
    bool dropDatabase(const QString &username, const QString &dbName);
    // 表结构变更 (修改字段)
    bool alterTable(const QString &username, const QString &dbName, const QString &tableName, const QList<Field> &newFields);
};

#endif // STORAGEMANAGER_H
