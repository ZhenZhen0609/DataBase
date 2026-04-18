#include "recordmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QDebug>
#include <QDateTime>

RecordManager::RecordManager() {}

QString RecordManager::getTableFilePath(const QString &dbName, const QString &tableName) const
{
    return Config::DATA_PATH + dbName + "/" + tableName + ".json";
}

bool RecordManager::ensureDbDirectory(const QString &dbName) const
{
    QString dbPath = Config::DATA_PATH + dbName;
    QDir dir;
    return dir.exists(dbPath) || dir.mkpath(dbPath);
}

Response RecordManager::insertRecord(const QString &dbName, const QString &tableName, const QJsonObject &data)
{
    // 1. 确保数据库目录存在
    if (!ensureDbDirectory(dbName)) {
        qDebug() << "[Data] Failed to create database directory:" << dbName;
        return {ResponseStatus::ERROR, QString("[Data] Failed to create database directory '%1'").arg(dbName), QVariant()};
    }

    // 2. 获取表文件路径
    QString filePath = getTableFilePath(dbName, tableName);

    // 3. 读取现有数据
    QJsonArray records;
    QFile file(filePath);

    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "[Data] Failed to open table file for reading:" << filePath;
            return {ResponseStatus::ERROR, QString("[Data] Failed to open table '%1'").arg(tableName), QVariant()};
        }

        QByteArray dataBytes = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(dataBytes);
        if (doc.isArray()) {
            records = doc.array();
        } else if (doc.isObject()) {
            // 如果文件内容是单个对象，转换为数组
            records.append(doc.object());
        }
    }

    // 4. 添加新记录（带时间戳）
    QJsonObject newRecord = data;
    newRecord["_created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    records.append(newRecord);

    // 5. 写入文件
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[Data] Failed to open table file for writing:" << filePath;
        return {ResponseStatus::ERROR, QString("[Data] Failed to write to table '%1'").arg(tableName), QVariant()};
    }

    QJsonDocument doc(records);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    // 6. 输出成功日志
    qDebug() << QString("[Data] Successfully inserted 1 record into \"%1\".").arg(tableName);

    return {ResponseStatus::OK, QString("[Data] Successfully inserted 1 record into \"%1\"").arg(tableName), QVariant(records.size())};
}

Response RecordManager::selectAllRecords(const QString &dbName, const QString &tableName)
{
    QString filePath = getTableFilePath(dbName, tableName);
    QFile file(filePath);

    if (!file.exists()) {
        qDebug() << "[Data] Table file not found:" << filePath;
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Data] Table '%1' not found").arg(tableName), QVariant()};
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[Data] Failed to open table file:" << filePath;
        return {ResponseStatus::ERROR, QString("[Data] Failed to open table '%1'").arg(tableName), QVariant()};
    }

    QByteArray dataBytes = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(dataBytes);
    if (!doc.isArray()) {
        qDebug() << "[Data] Invalid table file format:" << filePath;
        return {ResponseStatus::ERROR, "[Data] Invalid table file format", QVariant()};
    }

    qDebug() << QString("[Data] Retrieved %1 records from \"%2\".").arg(doc.array().size()).arg(tableName);

    return {ResponseStatus::OK, QString("[Data] Retrieved %1 records").arg(doc.array().size()), QVariant::fromValue(doc.array())};
}