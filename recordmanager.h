#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include <QString>
#include <QJsonObject>
#include "common.h"
#include "lockmanager.h"

class RecordManager
{
public:
    RecordManager();
    ~RecordManager();

    Response insertRecord(const QString &username, const QString &dbName, const QString &tableName, const QJsonObject &data);
    Response selectAllRecords(const QString &username, const QString &dbName, const QString &tableName);
    Response updateRecord(const QString &username, const QString &dbName, const QString &tableName, const QString &recordId, const QJsonObject &newData);
    Response deleteRecord(const QString &username, const QString &dbName, const QString &tableName, const QString &recordId);
    Response selectWhere(const QString &username, const QString &dbName, const QString &tableName, const QString &fieldName, const QVariant &value);
    Response selectWithCondition(const QString &username, const QString &dbName, const QString &tableName, const QJsonObject &condition);
    Response selectWithLimitOffset(const QString &username, const QString &dbName, const QString &tableName, int limit, int offset);

private:
    QString getTrdFilePath(const QString &username, const QString &dbName, const QString &tableName) const;
    QString getTdfFilePath(const QString &username, const QString &dbName, const QString &tableName) const;
    bool ensureDbDirectory(const QString &username, const QString &dbName) const;
    
    QList<Field> loadTableSchema(const QString &username, const QString &dbName, const QString &tableName);
    Response validateRecord(const QList<Field> &fields, const QJsonObject &data);
    
    QByteArray serializeRecord(const QJsonObject &record, const QList<Field> &fields);
    QJsonObject deserializeRecord(const QByteArray &data, const QList<Field> &fields);
    
    QString getPrimaryKeyField(const QList<Field> &fields) const;
    
    /**
     * @brief 读写锁管理器
     */
    LockManager lockManager;
};

#endif // RECORDMANAGER_H