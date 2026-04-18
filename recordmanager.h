#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include <QString>
#include <QJsonObject>
#include "common.h"

class RecordManager
{
public:
    RecordManager();

    /**
     * @brief 插入一条记录到指定表
     * @param tableName 表名（如 "test"）
     * @param data 要插入的数据（包含 id 和 name 等字段的 JSON 对象）
     * @return Response 结构体，包含操作状态和消息
     */
    Response insertRecord(const QString &dbName, const QString &tableName, const QJsonObject &data);

    /**
     * @brief 查询指定表的所有记录
     * @param tableName 表名
     * @return Response 结构体，包含查询结果
     */
    Response selectAllRecords(const QString &dbName, const QString &tableName);

private:
    // 获取数据表文件路径
    QString getTableFilePath(const QString &dbName, const QString &tableName) const;

    // 确保数据库目录存在
    bool ensureDbDirectory(const QString &dbName) const;
};

#endif // RECORDMANAGER_H