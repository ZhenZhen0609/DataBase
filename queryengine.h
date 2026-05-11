#ifndef QUERYENGINE_H
#define QUERYENGINE_H

#include <QObject>
#include <QString>
#include "common.h"
#include "recordmanager.h"
#include "schemamanager.h"
#include "storagemanager.h"
#include "conditionparser.h"

class QueryEngine : public QObject
{
    Q_OBJECT
public:
    explicit QueryEngine(QObject *parent = nullptr);

    void setCurrentUser(const QString &user);
    void setCurrentDatabase(const QString &db);

    // DML 执行入口
    Response executeSelect(const QString &tableName, const QStringList &columns,
                           const QString &whereClause, const QString &orderBy,
                           const QStringList &groupBy, const QString &having,
                           int limit = -1, int offset = -1, bool distinct = false);
    Response executeInsert(const QString &tableName, const QStringList &colNames,
                           const QList<QJsonArray> &rows);
    Response executeUpdate(const QString &tableName, const QJsonObject &assignments,
                           const QString &whereClause);
    Response executeDelete(const QString &tableName, const QString &whereClause);

private:
    QString m_currentUser;
    QString m_currentDb;
    RecordManager m_record;
    SchemaManager m_schema;

    // 辅助：获取表结构和所有记录
    bool loadTableData(const QString &tableName, QList<Field> &fields, QJsonArray &records, Response &error);
    // 将记录数组重新写回表文件
    Response rewriteTable(const QString &tableName, const QJsonArray &records, const QList<Field> &fields);
    // 聚合函数计算
    QVariant computeAggregate(const QString &funcName, const QJsonArray &groupRecords, const QString &fieldName);
};

#endif // QUERYENGINE_H
