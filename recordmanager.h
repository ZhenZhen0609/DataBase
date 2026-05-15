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

    Response replaceAllRecords(const QString &username, const QString &dbName, const QString &tableName, const QJsonArray &records);

    // 获取主键字段名
    QString getPrimaryKeyField(const QList<Field> &fields) const;

    // 加载表结构
    QList<Field> loadTableSchema(const QString &username, const QString &dbName, const QString &tableName);

    // 验证记录
    Response validateRecord(const QList<Field> &fields, const QJsonObject &data);

private:
    QString getTrdFilePath(const QString &username, const QString &dbName, const QString &tableName) const;
    QString getTdfFilePath(const QString &username, const QString &dbName, const QString &tableName) const;
    bool ensureDbDirectory(const QString &username, const QString &dbName) const;

    QByteArray serializeRecord(const QJsonObject &record, const QList<Field> &fields);
    QJsonObject deserializeRecord(const QByteArray &data, const QList<Field> &fields);

    // 从缓存读取所有记录
    QJsonArray readAllRecordsFromCache(const QString &username, const QString &dbName, const QString &tableName, const QList<Field> &fields);

    // 写入所有记录到缓存
    bool writeAllRecordsToCache(const QString &username, const QString &dbName, const QString &tableName, const QList<Field> &fields, const QJsonArray &records);

    // 加密/解密记录
    void encryptRecord(QJsonObject &record, const QList<Field> &fields);
    void decryptRecord(QJsonObject &record, const QList<Field> &fields);

    /**
     * @brief 读写锁管理器
     */
    LockManager lockManager;
};

#endif // RECORDMANAGER_H
