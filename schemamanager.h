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

    static bool validateFieldType(const QString &type, const QVariant &value);
    bool validateField(const Field &field, const QVariant &value) const;
    bool validateRecord(const TableSchema &schema, const QJsonObject &record) const;

    Response createTable(const QString &username, const QString &dbName, const TableSchema &schema);
    bool saveSchema(const QString &username, const QString &dbName, const QString &tableName, const QList<Field> &fields);
    Response loadTables(const QString &username, const QString &dbName);
    Response loadTableSchema(const QString &username, const QString &dbName, const QString &tableName);
    Response dropTable(const QString &username, const QString &dbName, const QString &tableName);

    /**
     * @brief 对即将插入的记录进行完整性审查：默认值填充、类型校验、主键唯一性检查
     * @param schema 当前表的字段定义
     * @param existingRecords 表中已有记录（JSON 数组）
     * @param record 待插入的记录，校验通过后会被填充默认值
     * @return 审查结果，OK 表示通过，其它为具体错误
     */
    Response validateAndFillRecord(const TableSchema &schema,const QJsonArray &existingRecords,QJsonObject &record) const;


private:
    bool ensureDbDirectory(const QString &username, const QString &dbName) const;
    QByteArray serializeSchema(const QList<Field> &fields) const;
};

#endif
