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
    void setCurrentDatabase(const QString &db);   // 粉圈维护
    void setQueryEngine(QueryEngine *engine);

    // 解析入口，返回执行结果
    Response parseSQL(const QString &sql);

signals:
    // 通知粉圈自动刷新（创建/删除数据库后）
    void databaseChanged(const QString &dbName);
    void tableChanged(const QString &dbName, const QString &tableName);

private:
    StorageManager *m_storage = nullptr;
    QString m_currentUser;
    QString m_currentDB;
    QueryEngine *m_engine = nullptr;

    // 内部解析辅助
    Response parseSingleStatement(const QString &sql);
    Response execCreateDatabase(const QString &dbName);
    Response execCreateTable(const QString &tableName, const QString &fieldsStr);
    Response execDropDatabase(const QString &dbName);
    Response execDropTable(const QString &tableName);
    Response execAlterTable(const QString &tableName, const QString &alterType, const QString &fieldStr);

    // DML 执行函数
    Response execSelect(const QString &sql);
    Response execInsert(const QString &sql);
    Response execUpdate(const QString &sql);
    Response execDelete(const QString &sql);

    QList<Field> parseFieldDefinitions(const QString &fieldsStr) const;
    FieldType strToFieldType(const QString &typeStr) const;
};
