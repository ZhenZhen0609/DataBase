#ifndef QUERYENGINE_H
#define QUERYENGINE_H

#include <QObject>
#include <QString>
#include "common.h"
#include "recordmanager.h"
#include "schemamanager.h"
#include "storagemanager.h"
#include "conditionparser.h"

class SQLParser;

class QueryEngine : public QObject
{
    Q_OBJECT
public:
    explicit QueryEngine(QObject *parent = nullptr);

    void setCurrentUser(const QString &user);
    void setCurrentDatabase(const QString &db);
    void setParser(SQLParser *parser);

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

    // 扩展功能：JOIN, UNION, VIEW
    Response executeJoinSelect(const QString &sql);
    Response executeUnion(const QString &leftSql, const QString &rightSql, bool distinct);
    Response executeCreateView(const QString &viewName, const QString &selectSql);
    Response executeDropView(const QString &viewName);
    
    // 事务管理
    Response executeBeginTransaction();
    Response executeCommit();
    Response executeRollback();

    // 合并两个结果集（供 UNION 使用）
    Response mergeUnion(const QJsonArray &leftRows, const QJsonArray &rightRows, bool distinct);

    // 视图展开辅助（递归展开，最大深度5）
    QString expandView(const QString &viewName, int depth = 0);

private:
    QString m_currentUser;
    QString m_currentDb;
    RecordManager m_record;
    SchemaManager m_schema;
    SQLParser *m_parser = nullptr;

    // 辅助：获取表结构和所有记录
    bool loadTableData(const QString &tableName, QList<Field> &fields, QJsonArray &records, Response &error);
    // 将记录数组重新写回表文件
    Response rewriteTable(const QString &tableName, const QJsonArray &records, const QList<Field> &fields);
    // 聚合函数计算
    QVariant computeAggregate(const QString &funcName, const QJsonArray &groupRecords, const QString &fieldName);
};

#endif // QUERYENGINE_H
