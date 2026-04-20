#include "recordmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QDebug>
#include <QDateTime>

RecordManager::RecordManager() {}

QString RecordManager::getTableFilePath(const QString &username, const QString &dbName, const QString &tableName) const
{
    return Config::DATA_PATH + username + "/" + dbName + "/" + tableName + ".json";
}

bool RecordManager::ensureDbDirectory(const QString &username, const QString &dbName) const
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir;
    return dir.exists(dbPath) || dir.mkpath(dbPath);
}

Response RecordManager::insertRecord(const QString &username, const QString &dbName, const QString &tableName, const QJsonObject &data)
{
    if (!ensureDbDirectory(username, dbName))
        return {ResponseStatus::ERROR, QString("[Data] Failed to create database directory '%1'").arg(dbName), QVariant()};

    QString filePath = getTableFilePath(username, dbName, tableName);
    QJsonArray records;
    QFile file(filePath);

    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return {ResponseStatus::ERROR, QString("[Data] Failed to open table '%1'").arg(tableName), QVariant()};
        QByteArray dataBytes = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(dataBytes);
        if (doc.isArray()) records = doc.array();
        else if (doc.isObject()) records.append(doc.object());
    }

    QJsonObject newRecord = data;
    newRecord["_created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    records.append(newRecord);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return {ResponseStatus::ERROR, QString("[Data] Failed to write to table '%1'").arg(tableName), QVariant()};

    file.write(QJsonDocument(records).toJson(QJsonDocument::Indented));
    file.close();

    return {ResponseStatus::OK, QString("[Data] Successfully inserted 1 record into \"%1\"").arg(tableName), QVariant(records.size())};
}

Response RecordManager::selectAllRecords(const QString &username, const QString &dbName, const QString &tableName)
{
    QString filePath = getTableFilePath(username, dbName, tableName);
    QFile file(filePath);

    if (!file.exists())
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Data] Table '%1' not found").arg(tableName), QVariant()};

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {ResponseStatus::ERROR, QString("[Data] Failed to open table '%1'").arg(tableName), QVariant()};

    QByteArray dataBytes = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(dataBytes);
    if (!doc.isArray())
        return {ResponseStatus::ERROR, "[Data] Invalid table file format", QVariant()};

    return {ResponseStatus::OK, QString("[Data] Retrieved %1 records").arg(doc.array().size()), QVariant::fromValue(doc.array())};
}
