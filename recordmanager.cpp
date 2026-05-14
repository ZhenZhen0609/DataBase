#include "recordmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QDataStream>

RecordManager::RecordManager() {}

RecordManager::~RecordManager() {}

QString RecordManager::getTrdFilePath(const QString &username, const QString &dbName, const QString &tableName) const
{
    return Config::DATA_PATH + username + "/" + dbName + "/" + tableName + ".trd";
}

QString RecordManager::getTdfFilePath(const QString &username, const QString &dbName, const QString &tableName) const
{
    return Config::DATA_PATH + username + "/" + dbName + "/" + tableName + ".tdf";
}

bool RecordManager::ensureDbDirectory(const QString &username, const QString &dbName) const
{
    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    QDir dir;
    return dir.exists(dbPath) || dir.mkpath(dbPath);
}

QList<Field> RecordManager::loadTableSchema(const QString &username, const QString &dbName, const QString &tableName)
{
    QList<Field> fields;
    QString tdfPath = getTdfFilePath(username, dbName, tableName);
    QFile file(tdfPath);
    
    if (!file.exists()) {
        qDebug() << "[Record] Table schema file not found:" << tdfPath;
        return fields;
    }
    
    // 获取读锁
    lockManager.acquireReadLock(username, dbName, tableName);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[Record] Failed to open schema file:" << tdfPath;
        lockManager.releaseReadLock(username, dbName, tableName);
        return fields;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    lockManager.releaseReadLock(username, dbName, tableName);
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qDebug() << "[Record] Invalid schema file format";
        return fields;
    }
    
    QJsonArray fieldsArray = doc.object()["fields"].toArray();
    for (const QJsonValue &val : fieldsArray) {
        QJsonObject obj = val.toObject();
        Field f;
        f.name = obj["name"].toString();
        f.type = static_cast<FieldType>(obj["type"].toInt());
        f.length = obj["length"].toInt();
        f.isNotNull = obj["isNotNull"].toBool();
        f.isPrimaryKey = obj["isPrimaryKey"].toBool();
        fields.append(f);
    }
    
    return fields;
}

Response RecordManager::validateRecord(const QList<Field> &fields, const QJsonObject &data)
{
    for (const Field &f : fields) {
        if (!data.contains(f.name)) {
            if (f.isNotNull) {
                return {ResponseStatus::ERROR, QString("[Record] 字段 '%1' 为必填字段").arg(f.name), QVariant()};
            }
            continue;
        }
        
        QJsonValue val = data[f.name];
        
        switch (f.type) {
            case FieldType::INT:
                if (!val.isDouble()) {
                    return {ResponseStatus::ERROR, QString("[Record] 字段 '%1' 需要整数类型").arg(f.name), QVariant()};
                }
                if (val.toDouble() != val.toInt()) {
                    return {ResponseStatus::ERROR, QString("[Record] 字段 '%1' 需要整数类型").arg(f.name), QVariant()};
                }
                break;
                
            case FieldType::DOUBLE:
                if (!val.isDouble()) {
                    return {ResponseStatus::ERROR, QString("[Record] 字段 '%1' 需要小数类型").arg(f.name), QVariant()};
                }
                break;
                
            case FieldType::BOOLEAN:
                if (!val.isBool()) {
                    return {ResponseStatus::ERROR, QString("[Record] 字段 '%1' 需要布尔类型").arg(f.name), QVariant()};
                }
                break;
                
            case FieldType::TEXT:
                if (!val.isString()) {
                    return {ResponseStatus::ERROR, QString("[Record] 字段 '%1' 需要文本类型").arg(f.name), QVariant()};
                }
                if (f.length > 0 && val.toString().length() > f.length) {
                    return {ResponseStatus::ERROR, QString("[Record] 字段 '%1' 长度超过限制(%2)").arg(f.name).arg(f.length), QVariant()};
                }
                break;
        }
    }
    
    return {ResponseStatus::OK, "[Record] 数据校验通过", QVariant()};
}

//序列化数据
QByteArray RecordManager::serializeRecord(const QJsonObject &record, const QList<Field> &fields)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    int fieldCount = fields.size();
    stream << fieldCount;
    
    for (const Field &f : fields) {
        QJsonValue val = record.value(f.name);
        
        switch (f.type) {
            case FieldType::INT:
                stream << val.toInt();
                break;
            case FieldType::DOUBLE:
                stream << val.toDouble();
                break;
            case FieldType::BOOLEAN:
                stream << val.toBool();
                break;
            case FieldType::TEXT: {
                QString str = val.toString();
                stream << str;
                break;
            }
        }
    }
    
    QString createdAt = record["_created_at"].toString();
    stream << createdAt;
    
    return data;
}

QJsonObject RecordManager::deserializeRecord(const QByteArray &data, const QList<Field> &fields)
{
    QJsonObject record;
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    int fieldCount;
    stream >> fieldCount;
    
    for (const Field &f : fields) {
        switch (f.type) {
            case FieldType::INT: {
                int val;
                stream >> val;
                record[f.name] = val;
                break;
            }
            case FieldType::DOUBLE: {
                double val;
                stream >> val;
                record[f.name] = val;
                break;
            }
            case FieldType::BOOLEAN: {
                bool val;
                stream >> val;
                record[f.name] = val;
                break;
            }
            case FieldType::TEXT: {
                QString val;
                stream >> val;
                record[f.name] = val;
                break;
            }
        }
    }
    
    QString createdAt;
    stream >> createdAt;
    record["_created_at"] = createdAt;
    
    return record;
}

Response RecordManager::insertRecord(const QString &username, const QString &dbName, const QString &tableName, const QJsonObject &data)
{
    if (!ensureDbDirectory(username, dbName))
        return {ResponseStatus::ERROR, QString("[Record] Failed to create database directory '%1'").arg(dbName), QVariant()};

    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' schema not found").arg(tableName), QVariant()};
    }

    Response validateResp = validateRecord(fields, data);
    if (validateResp.status != ResponseStatus::OK) {
        return validateResp;
    }

    QString trdPath = getTrdFilePath(username, dbName, tableName);
    QFile file(trdPath);

    QJsonObject newRecord = data;
    newRecord["_created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QByteArray recordData = serializeRecord(newRecord, fields);

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, tableName);

    if (!file.open(QIODevice::Append)) {
        lockManager.releaseWriteLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to open table '%1'").arg(tableName), QVariant()};
    }

    qint64 recordSize = recordData.size();
    file.write(reinterpret_cast<const char*>(&recordSize), sizeof(qint64));
    file.write(recordData);
    file.close();
    
    lockManager.releaseWriteLock(username, dbName, tableName);

    return {ResponseStatus::OK, QString("[Record] Successfully inserted 1 record into \"%1\"").arg(tableName), QVariant(1)};
}

Response RecordManager::selectAllRecords(const QString &username, const QString &dbName, const QString &tableName)
{
    QString trdPath = getTrdFilePath(username, dbName, tableName);
    QFile file(trdPath);

    if (!file.exists())
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' not found").arg(tableName), QVariant()};

    // 获取读锁
    lockManager.acquireReadLock(username, dbName, tableName);

    if (!file.open(QIODevice::ReadOnly)) {
        lockManager.releaseReadLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to open table '%1'").arg(tableName), QVariant()};
    }

    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        lockManager.releaseReadLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to load schema for '%1'").arg(tableName), QVariant()};
    }

    QJsonArray records;
    qint64 fileSize = file.size();
    qint64 pos = 0;

    while (pos < fileSize) {
        qint64 recordSize;
        if (file.read(reinterpret_cast<char*>(&recordSize), sizeof(qint64)) != sizeof(qint64)) {
            lockManager.releaseReadLock(username, dbName, tableName);
            break;
        }

        QByteArray recordData = file.read(recordSize);
        if (recordData.size() != recordSize) {
            lockManager.releaseReadLock(username, dbName, tableName);
            break;
        }

        QJsonObject record = deserializeRecord(recordData, fields);
        records.append(record);

        pos += sizeof(qint64) + recordSize;
    }

    file.close();
    lockManager.releaseReadLock(username, dbName, tableName);

    return {ResponseStatus::OK, QString("[Record] Retrieved %1 records").arg(records.size()), QVariant::fromValue(records)};
}

//获取表的主键字段名
QString RecordManager::getPrimaryKeyField(const QList<Field> &fields) const
{
    for (const Field &f : fields) {
        if (f.isPrimaryKey) {
            return f.name;
        }
    }
    return "id";
}

Response RecordManager::updateRecord(const QString &username, const QString &dbName, const QString &tableName, const QString &recordId, const QJsonObject &newData)
{
    QString trdPath = getTrdFilePath(username, dbName, tableName);
    QFile file(trdPath);

    if (!file.exists())
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' not found").arg(tableName), QVariant()};

    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::ERROR, QString("[Record] Failed to load schema for '%1'").arg(tableName), QVariant()};
    }

    QString pkField = getPrimaryKeyField(fields);

    // 读取所有记录
    if (!file.open(QIODevice::ReadOnly))
        return {ResponseStatus::ERROR, QString("[Record] Failed to open table '%1'").arg(tableName), QVariant()};

    QList<QByteArray> allRecords;
    qint64 fileSize = file.size();
    qint64 pos = 0;
    bool found = false;
    int foundIndex = -1;

    while (pos < fileSize) {
        qint64 recordSize;
        if (file.read(reinterpret_cast<char*>(&recordSize), sizeof(qint64)) != sizeof(qint64))
            break;

        QByteArray recordData = file.read(recordSize);
        if (recordData.size() != recordSize)
            break;

        QJsonObject record = deserializeRecord(recordData, fields);
        QString currentId = record[pkField].toString();
        if (currentId == recordId) {
            // 更新记录
            QJsonObject updatedRecord = record;
            for (const QString &key : newData.keys()) {
                updatedRecord[key] = newData[key];
            }
            updatedRecord["_created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            allRecords.append(serializeRecord(updatedRecord, fields));
            found = true;
            foundIndex = allRecords.size() - 1;
        } else {
            allRecords.append(recordData);
        }

        pos += sizeof(qint64) + recordSize;
    }
    file.close();

    if (!found) {
        return {ResponseStatus::ERROR, QString("[Record] Record with '%1'='%2' not found").arg(pkField).arg(recordId), QVariant()};
    }

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, tableName);

    // 重写整个文件
    if (!file.open(QIODevice::WriteOnly)) {
        lockManager.releaseWriteLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to open table '%1' for writing").arg(tableName), QVariant()};
    }

    for (const QByteArray &recordData : allRecords) {
        qint64 recordSize = recordData.size();
        file.write(reinterpret_cast<const char*>(&recordSize), sizeof(qint64));
        file.write(recordData);
    }
    file.close();
    
    lockManager.releaseWriteLock(username, dbName, tableName);

    return {ResponseStatus::OK, QString("[Record] Updated 1 record in '%1'").arg(tableName), QVariant(1)};
}

Response RecordManager::deleteRecord(const QString &username, const QString &dbName, const QString &tableName, const QString &recordId)
{
    QString trdPath = getTrdFilePath(username, dbName, tableName);
    QFile file(trdPath);

    if (!file.exists())
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' not found").arg(tableName), QVariant()};

    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::ERROR, QString("[Record] Failed to load schema for '%1'").arg(tableName), QVariant()};
    }

    QString pkField = getPrimaryKeyField(fields);

    // 读取所有记录
    if (!file.open(QIODevice::ReadOnly))
        return {ResponseStatus::ERROR, QString("[Record] Failed to open table '%1'").arg(tableName), QVariant()};

    QList<QByteArray> remainingRecords;
    qint64 fileSize = file.size();
    qint64 pos = 0;
    bool found = false;

    while (pos < fileSize) {
        qint64 recordSize;
        if (file.read(reinterpret_cast<char*>(&recordSize), sizeof(qint64)) != sizeof(qint64))
            break;

        QByteArray recordData = file.read(recordSize);
        if (recordData.size() != recordSize)
            break;

        QJsonObject record = deserializeRecord(recordData, fields);
        QString currentId = record[pkField].toString();
        if (currentId != recordId) {
            remainingRecords.append(recordData);
        } else {
            found = true;
        }

        pos += sizeof(qint64) + recordSize;
    }
    file.close();

    if (!found) {
        return {ResponseStatus::ERROR, QString("[Record] Record with '%1'='%2' not found").arg(pkField).arg(recordId), QVariant()};
    }

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, tableName);

    // 重写整个文件（不包含被删除的记录）
    if (!file.open(QIODevice::WriteOnly)) {
        lockManager.releaseWriteLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to open table '%1' for writing").arg(tableName), QVariant()};
    }

    for (const QByteArray &recordData : remainingRecords) {
        qint64 recordSize = recordData.size();
        file.write(reinterpret_cast<const char*>(&recordSize), sizeof(qint64));
        file.write(recordData);
    }
    file.close();
    
    lockManager.releaseWriteLock(username, dbName, tableName);

    return {ResponseStatus::OK, QString("[Record] Deleted 1 record from '%1'").arg(tableName), QVariant(1)};
}

Response RecordManager::selectWhere(const QString &username, const QString &dbName, const QString &tableName, const QString &fieldName, const QVariant &value)
{
    QString trdPath = getTrdFilePath(username, dbName, tableName);
    QFile file(trdPath);

    if (!file.exists())
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' not found").arg(tableName), QVariant()};

    // 获取读锁
    lockManager.acquireReadLock(username, dbName, tableName);

    if (!file.open(QIODevice::ReadOnly)) {
        lockManager.releaseReadLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to open table '%1'").arg(tableName), QVariant()};
    }

    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        lockManager.releaseReadLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to load schema for '%1'").arg(tableName), QVariant()};
    }

    // 检查字段是否存在
    bool fieldExists = false;
    FieldType fieldType = FieldType::TEXT;
    for (const Field &f : fields) {
        if (f.name == fieldName) {
            fieldExists = true;
            fieldType = f.type;
            break;
        }
    }
    
    if (!fieldExists) {
        lockManager.releaseReadLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Field '%1' not found in table '%2'").arg(fieldName).arg(tableName), QVariant()};
    }

    QJsonArray records;
    qint64 fileSize = file.size();
    qint64 pos = 0;

    while (pos < fileSize) {
        qint64 recordSize;
        if (file.read(reinterpret_cast<char*>(&recordSize), sizeof(qint64)) != sizeof(qint64)) {
            lockManager.releaseReadLock(username, dbName, tableName);
            break;
        }

        QByteArray recordData = file.read(recordSize);
        if (recordData.size() != recordSize) {
            lockManager.releaseReadLock(username, dbName, tableName);
            break;
        }

        QJsonObject record = deserializeRecord(recordData, fields);
        QJsonValue recordValue = record[fieldName];
        
        // 根据字段类型进行比较
        bool match = false;
        QString strValue = value.toString();
        
        switch (fieldType) {
            case FieldType::INT:
                match = recordValue.toInt() == value.toInt();
                break;
            case FieldType::DOUBLE:
                match = qFuzzyCompare(recordValue.toDouble(), value.toDouble());
                break;
            case FieldType::BOOLEAN:
                match = recordValue.toBool() == value.toBool();
                break;
            case FieldType::TEXT:
                // 文本字段支持模糊匹配
                match = recordValue.toString().contains(strValue, Qt::CaseInsensitive);
                break;
        }

        if (match) {
            records.append(record);
        }

        pos += sizeof(qint64) + recordSize;
    }

    file.close();
    lockManager.releaseReadLock(username, dbName, tableName);

    return {ResponseStatus::OK, QString("[Record] Found %1 records matching '%2'").arg(records.size()).arg(fieldName), QVariant::fromValue(records)};
}

Response RecordManager::selectWithCondition(const QString &username, const QString &dbName, const QString &tableName, const QJsonObject &condition)
{
    QString trdPath = getTrdFilePath(username, dbName, tableName);
    QFile file(trdPath);

    if (!file.exists())
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' not found").arg(tableName), QVariant()};

    // 获取读锁
    lockManager.acquireReadLock(username, dbName, tableName);

    if (!file.open(QIODevice::ReadOnly)) {
        lockManager.releaseReadLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to open table '%1'").arg(tableName), QVariant()};
    }

    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        lockManager.releaseReadLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to load schema for '%1'").arg(tableName), QVariant()};
    }

    QJsonArray records;
    qint64 fileSize = file.size();
    qint64 pos = 0;

    while (pos < fileSize) {
        qint64 recordSize;
        if (file.read(reinterpret_cast<char*>(&recordSize), sizeof(qint64)) != sizeof(qint64)) {
            lockManager.releaseReadLock(username, dbName, tableName);
            break;
        }

        QByteArray recordData = file.read(recordSize);
        if (recordData.size() != recordSize) {
            lockManager.releaseReadLock(username, dbName, tableName);
            break;
        }

        QJsonObject record = deserializeRecord(recordData, fields);
        
        // 检查是否满足所有条件
        bool match = true;
        for (const QString &key : condition.keys()) {
            QJsonValue conditionValue = condition[key];
            QJsonValue recordValue = record[key];
            
            // 简单相等比较
            if (conditionValue.isString() && recordValue.isString()) {
                if (recordValue.toString() != conditionValue.toString()) {
                    match = false;
                    break;
                }
            } else if (conditionValue.isDouble() && recordValue.isDouble()) {
                if (recordValue.toDouble() != conditionValue.toDouble()) {
                    match = false;
                    break;
                }
            } else if (conditionValue.isBool() && recordValue.isBool()) {
                if (recordValue.toBool() != conditionValue.toBool()) {
                    match = false;
                    break;
                }
            }
        }

        if (match) {
            records.append(record);
        }

        pos += sizeof(qint64) + recordSize;
    }

    file.close();
    lockManager.releaseReadLock(username, dbName, tableName);

    return {ResponseStatus::OK, QString("[Record] Found %1 records matching condition").arg(records.size()), QVariant::fromValue(records)};
}

Response RecordManager::selectWithLimitOffset(const QString &username, const QString &dbName, const QString &tableName, int limit, int offset)
{
    QString trdPath = getTrdFilePath(username, dbName, tableName);
    QFile file(trdPath);

    if (!file.exists())
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' not found").arg(tableName), QVariant()};

    // 获取读锁
    lockManager.acquireReadLock(username, dbName, tableName);

    if (!file.open(QIODevice::ReadOnly)) {
        lockManager.releaseReadLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to open table '%1'").arg(tableName), QVariant()};
    }

    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        lockManager.releaseReadLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to load schema for '%1'").arg(tableName), QVariant()};
    }

    QJsonArray records;
    qint64 fileSize = file.size();
    qint64 pos = 0;
    int skipped = 0;
    int count = 0;

    while (pos < fileSize && count < limit) {
        qint64 recordSize;
        if (file.read(reinterpret_cast<char*>(&recordSize), sizeof(qint64)) != sizeof(qint64)) {
            lockManager.releaseReadLock(username, dbName, tableName);
            break;
        }

        QByteArray recordData = file.read(recordSize);
        if (recordData.size() != recordSize) {
            lockManager.releaseReadLock(username, dbName, tableName);
            break;
        }

        // 跳过 offset 个记录
        if (skipped < offset) {
            skipped++;
            pos += sizeof(qint64) + recordSize;
            continue;
        }

        QJsonObject record = deserializeRecord(recordData, fields);
        records.append(record);
        count++;

        pos += sizeof(qint64) + recordSize;
    }

    file.close();
    lockManager.releaseReadLock(username, dbName, tableName);

    return {ResponseStatus::OK, QString("[Record] Retrieved %1 records (limit=%2, offset=%3)").arg(records.size()).arg(limit).arg(offset), QVariant::fromValue(records)};
}

Response RecordManager::replaceAllRecords(const QString &username, const QString &dbName, const QString &tableName, const QJsonArray &records)
{
    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty())
        return {ResponseStatus::TABLE_NOT_FOUND, "[Record] Table schema not found", QVariant()};

    QString trdPath = getTrdFilePath(username, dbName, tableName);
    QFile file(trdPath);

    // 获取写锁
    lockManager.acquireWriteLock(username, dbName, tableName);

    if (!file.open(QIODevice::WriteOnly)) {
        lockManager.releaseWriteLock(username, dbName, tableName);
        return {ResponseStatus::ERROR, QString("[Record] Failed to open '%1' for writing").arg(tableName), QVariant()};
    }

    for (const QJsonValue &val : records) {
        QJsonObject record = val.toObject();
        QByteArray recordData = serializeRecord(record, fields);
        qint64 recordSize = recordData.size();
        file.write(reinterpret_cast<const char*>(&recordSize), sizeof(qint64));
        file.write(recordData);
    }
    file.close();
    lockManager.releaseWriteLock(username, dbName, tableName);

    return {ResponseStatus::OK, QString("[Record] Replaced %1 records in '%2'").arg(records.size()).arg(tableName), QVariant()};
}
