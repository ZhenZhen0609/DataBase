#pragma once

#include <QObject>
#include "common.h"

class StorageManager;
class QueryEngine;

class SQLParser : public QObject
{
    Q_OBJECT
public:
    explicit SQLParser(QObject *parent = nullptr);

    void setStorageManager(StorageManager *storage);
    void setCurrentUser(const QString &user);
    void setCurrentDatabase(const QString &db);
    void setQueryEngine(QueryEngine *engine);

    Response parseSQL(const QString &sql);

signals:
    void databaseChanged(const QString &dbName);
    void tableChanged(const QString &dbName, const QString &tableName);

private:
    StorageManager *m_storage = nullptr;
    QString m_currentUser;
    QString m_currentDB;
    QueryEngine *m_engine = nullptr;

    Response parseSingleStatement(const QString &sql);

    Response execCreateDatabase(const QString &dbName);
    Response execCreateTable(const QString &tableName, const QString &fieldsStr);
    Response execDropDatabase(const QString &dbName);
    Response execDropTable(const QString &tableName);
    Response execAlterTable(const QString &tableName, const QString &alterType, const QString &fieldStr);

    Response execSelect(const QString &sql);
    Response execInsert(const QString &sql);
    Response execUpdate(const QString &sql);
    Response execDelete(const QString &sql);

    Response execJoinSelect(const QString &sql);
    Response execUnion(const QString &sql);
    Response execCreateView(const QString &viewName, const QString &selectSql);
    Response execDropView(const QString &viewName);

    // 语义检查
    bool checkTableExists(const QString &tableName, QString &errorMsg) const;
    bool checkColumnsExist(const QString &tableName, const QStringList &colNames, QString &errorMsg) const;

    // 错误消息生成
    QString errorAtToken(const QString &sql, int tokenPos, const QString &expected) const;

    QList<Field> parseFieldDefinitions(const QString &fieldsStr) const;
    FieldType strToFieldType(const QString &typeStr) const;
};
