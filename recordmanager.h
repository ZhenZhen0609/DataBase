#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include <QString>
#include <QJsonObject>
#include "common.h"

class RecordManager
{
public:
    RecordManager();

    Response insertRecord(const QString &username, const QString &dbName, const QString &tableName, const QJsonObject &data);
    Response selectAllRecords(const QString &username, const QString &dbName, const QString &tableName);

private:
    QString getTableFilePath(const QString &username, const QString &dbName, const QString &tableName) const;
    bool ensureDbDirectory(const QString &username, const QString &dbName) const;
};

#endif // RECORDMANAGER_H
