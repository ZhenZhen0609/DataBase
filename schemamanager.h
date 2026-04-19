#ifndef SCHEMAMANAGER_H
#define SCHEMAMANAGER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include "common.h"

class SchemaManager : public QObject
{
    Q_OBJECT
public:
    explicit SchemaManager(QObject *parent = nullptr);

    // 校验某个值是否符合指定的类型（目前支持 "INT", "TEXT", "DOUBLE", "BOOLEAN"）
    static bool validateFieldType(const QString &type, const QVariant &value);

    // 校验某个值是否符合指定的字段定义
    bool validateField(const Field &field, const QVariant &value) const;

    // 校验整条记录的数据类型是否符合表结构
    bool validateRecord(const TableSchema &schema, const QJsonObject &record) const;

    // 创建表结构并持久化（原有方法，内部调用 saveSchema）
    Response createTable(const QString &dbName, const TableSchema &schema);

    // 保存字段定义到 .tdf 文件
    bool saveSchema(const QString &dbName, const QString &tableName, const QList<Field> &fields);

    // 加载指定数据库的所有表结构
    Response loadTables(const QString &dbName);

    // 加载指定表的结构
    Response loadTableSchema(const QString &dbName, const QString &tableName);

    // 删除指定表
    Response dropTable(const QString &dbName, const QString &tableName);

private:
    // 获取表结构文件路径
    QString getSchemaFilePath(const QString &dbName) const;

    // 确保数据库目录存在
    bool ensureDbDirectory(const QString &dbName) const;

    // 将字段列表序列化为 JSON 字节数组
    QByteArray serializeSchema(const QList<Field> &fields) const;
};

#endif
