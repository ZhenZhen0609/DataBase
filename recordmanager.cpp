#include "recordmanager.h"
#include "storagemanager.h"
#include "constraintmanager.h"
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
        f.isUnique = obj["isUnique"].toBool();
        f.hasCheck = obj["hasCheck"].toBool();
        f.checkExpr = obj["checkExpr"].toString();
        f.defaultValue = obj["defaultValue"].toString();
        f.hasIndex = obj["hasIndex"].toBool();
        f.isForeignKey = obj["isForeignKey"].toBool();
        f.referenceTable = obj["referenceTable"].toString();
        f.referenceField = obj["referenceField"].toString();
        f.cascadeRule = obj["cascadeRule"].toString();
        f.formatValidation = obj["formatValidation"].toString();
        f.isEncrypted = obj["isEncrypted"].toBool();
        fields.append(f);
    }

    return fields;
}

Response RecordManager::validateRecord(const QList<Field> &fields, const QJsonObject &data)
{
    for (const Field &f : fields) {
        QJsonValue val = data.value(f.name);

        if (f.isNotNull) {
            if (val.isNull() || val.isUndefined()) {
                return {ResponseStatus::ERROR, QString("[Record] Field '%1' is required").arg(f.name), QVariant()};
            }
            if (val.isString() && val.toString().trimmed().isEmpty()) {
                return {ResponseStatus::ERROR, QString("[Record] Field '%1' is required").arg(f.name), QVariant()};
            }
            if (val.isDouble() && val.toDouble() == 0 && !data.contains(f.name)) {
                return {ResponseStatus::ERROR, QString("[Record] Field '%1' is required").arg(f.name), QVariant()};
            }
        }

        if (!data.contains(f.name)) {
            continue;
        }

        switch (f.type) {
            case FieldType::INT:
                if (!val.isDouble()) {
                    return {ResponseStatus::ERROR, QString("[Record] Field '%1' must be integer").arg(f.name), QVariant()};
                }
                if (val.toDouble() != val.toInt()) {
                    return {ResponseStatus::ERROR, QString("[Record] Field '%1' must be integer").arg(f.name), QVariant()};
                }
                break;

            case FieldType::DOUBLE:
                if (!val.isDouble()) {
                    return {ResponseStatus::ERROR, QString("[Record] Field '%1' must be number").arg(f.name), QVariant()};
                }
                break;

            case FieldType::BOOLEAN:
                if (!val.isBool()) {
                    return {ResponseStatus::ERROR, QString("[Record] Field '%1' must be boolean").arg(f.name), QVariant()};
                }
                break;

            case FieldType::TEXT:
                if (!val.isString()) {
                    return {ResponseStatus::ERROR, QString("[Record] Field '%1' must be text").arg(f.name), QVariant()};
                }
                if (f.length > 0 && val.toString().length() > f.length) {
                    return {ResponseStatus::ERROR, QString("[Record] Field '%1' exceeds length limit (%2)").arg(f.name).arg(f.length), QVariant()};
                }
                break;
        }
    }

    return {ResponseStatus::OK, "[Record] Data validation passed", QVariant()};
}

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
    QByteArray mutableData = data;
    QDataStream stream(&mutableData, QIODevice::ReadOnly);
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

QJsonArray RecordManager::readAllRecordsFromCache(const QString &username, const QString &dbName, const QString &tableName, const QList<Field> &fields)
{
    QJsonArray records;
    StorageManager storage;

    QByteArray data = storage.readTableData(username, dbName, tableName);
    if (data.isEmpty()) {
        return records;
    }

    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    while (!stream.atEnd()) {
        qint64 recordSize;
        if (stream.readRawData(reinterpret_cast<char*>(&recordSize), sizeof(qint64)) != sizeof(qint64)) {
            break;
        }

        QByteArray recordData(recordSize, 0);
        if (stream.readRawData(recordData.data(), recordSize) != recordSize) {
            break;
        }

        QJsonObject record = deserializeRecord(recordData, fields);
        decryptRecord(record, fields);
        records.append(record);
    }

    return records;
}

bool RecordManager::writeAllRecordsToCache(const QString &username, const QString &dbName, const QString &tableName, const QList<Field> &fields, const QJsonArray &records)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    for (const QJsonValue &val : records) {
        QJsonObject record = val.toObject();
        QJsonObject encryptedRecord = record;
        encryptRecord(encryptedRecord, fields);
        QByteArray recordData = serializeRecord(encryptedRecord, fields);
        qint64 recordSize = recordData.size();
        stream.writeRawData(reinterpret_cast<const char*>(&recordSize), sizeof(qint64));
        stream.writeRawData(recordData.data(), recordSize);
    }

    StorageManager storage;
    return storage.writeTableData(username, dbName, tableName, data);
}

void RecordManager::encryptRecord(QJsonObject &record, const QList<Field> &fields)
{
    for (const Field &field : fields) {
        if (field.isEncrypted && record.contains(field.name)) {
            QJsonValue val = record[field.name];
            if (val.isString()) {
                QString encrypted = ConstraintManager::encrypt(val.toString());
                record[field.name] = encrypted;
            }
        }
    }
}

void RecordManager::decryptRecord(QJsonObject &record, const QList<Field> &fields)
{
    for (const Field &field : fields) {
        if (field.isEncrypted && record.contains(field.name)) {
            QJsonValue val = record[field.name];
            if (val.isString()) {
                QString decrypted = ConstraintManager::decrypt(val.toString());
                record[field.name] = decrypted;
            }
        }
    }
}

Response RecordManager::insertRecord(const QString &username, const QString &dbName, const QString &tableName, const QJsonObject &data)
{
    if (!ensureDbDirectory(username, dbName))
        return {ResponseStatus::ERROR, QString("[Record] Failed to create database directory '%1'").arg(dbName), QVariant()};

    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' schema not found").arg(tableName), QVariant()};
    }

    QJsonObject newRecord = data;
    for (const Field &field : fields) {
        if (!newRecord.contains(field.name) && !field.defaultValue.isEmpty()) {
            switch (field.type) {
                case FieldType::INT:
                    newRecord[field.name] = field.defaultValue.toInt();
                    break;
                case FieldType::DOUBLE:
                    newRecord[field.name] = field.defaultValue.toDouble();
                    break;
                case FieldType::TEXT:
                    newRecord[field.name] = field.defaultValue;
                    break;
                case FieldType::BOOLEAN:
                    newRecord[field.name] = (field.defaultValue.toLower() == "true");
                    break;
            }
        }
    }
    newRecord["_created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    Response validateResp = validateRecord(fields, newRecord);
    if (validateResp.status != ResponseStatus::OK) {
        return validateResp;
    }

    QJsonArray existingRecords = readAllRecordsFromCache(username, dbName, tableName, fields);

    for (const Field &field : fields) {
        if (field.isForeignKey) {
            Response fkResp = ConstraintManager::validateForeignKey(username, dbName, field, newRecord[field.name].toVariant());
            if (fkResp.status != ResponseStatus::OK) {
                return fkResp;
            }
        }
    }

    for (const Field &field : fields) {
        if (!field.formatValidation.isEmpty()) {
            Response formatResp = ConstraintManager::validateFormat(field, newRecord[field.name].toVariant());
            if (formatResp.status != ResponseStatus::OK) {
                return formatResp;
            }
        }
    }

    for (const Field &field : fields) {
        if (field.hasCheck) {
            Response checkResp = ConstraintManager::validateCheckConstraint(field, newRecord);
            if (checkResp.status != ResponseStatus::OK) {
                return checkResp;
            }
        }
    }

    Response uniqueResp = ConstraintManager::validateUnique(fields, existingRecords, newRecord, false);
    if (uniqueResp.status != ResponseStatus::OK) {
        return uniqueResp;
    }

    lockManager.acquireWriteLock(username, dbName, tableName);

    existingRecords.append(newRecord);
    bool success = writeAllRecordsToCache(username, dbName, tableName, fields, existingRecords);

    lockManager.releaseWriteLock(username, dbName, tableName);

    if (!success) {
        return {ResponseStatus::ERROR, QString("[Record] Failed to write data to table '%1'").arg(tableName), QVariant()};
    }

    return {ResponseStatus::OK, QString("[Record] Successfully inserted 1 record into '%1'").arg(tableName), QVariant(1)};
}

Response RecordManager::selectAllRecords(const QString &username, const QString &dbName, const QString &tableName)
{
    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' not found").arg(tableName), QVariant()};
    }

    lockManager.acquireReadLock(username, dbName, tableName);
    QJsonArray records = readAllRecordsFromCache(username, dbName, tableName, fields);
    lockManager.releaseReadLock(username, dbName, tableName);

    return {ResponseStatus::OK, QString("[Record] Retrieved %1 records").arg(records.size()), QVariant::fromValue(records)};
}

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
    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::ERROR, QString("[Record] Failed to load schema for '%1'").arg(tableName), QVariant()};
    }

    QString pkField = getPrimaryKeyField(fields);

    lockManager.acquireReadLock(username, dbName, tableName);
    QJsonArray records = readAllRecordsFromCache(username, dbName, tableName, fields);
    lockManager.releaseReadLock(username, dbName, tableName);

    bool found = false;
    int foundIndex = -1;
    QJsonObject targetRecord;

    for (int i = 0; i < records.size(); ++i) {
        QJsonObject record = records[i].toObject();
        QString currentId = record[pkField].toString();
        if (currentId == recordId) {
            targetRecord = record;
            foundIndex = i;
            found = true;
            break;
        }
    }

    if (!found) {
        return {ResponseStatus::ERROR, QString("[Record] Record with '%1'='%2' not found").arg(pkField).arg(recordId), QVariant()};
    }

    QJsonObject updatedRecord = targetRecord;
    for (const QString &key : newData.keys()) {
        updatedRecord[key] = newData[key];
    }
    updatedRecord["_created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    Response validateResp = validateRecord(fields, updatedRecord);
    if (validateResp.status != ResponseStatus::OK) {
        return validateResp;
    }

    for (const Field &field : fields) {
        if (field.isForeignKey && newData.contains(field.name)) {
            Response fkResp = ConstraintManager::validateForeignKey(username, dbName, field, updatedRecord[field.name].toVariant());
            if (fkResp.status != ResponseStatus::OK) {
                return fkResp;
            }
        }
    }

    for (const Field &field : fields) {
        if (!field.formatValidation.isEmpty() && newData.contains(field.name)) {
            Response formatResp = ConstraintManager::validateFormat(field, updatedRecord[field.name].toVariant());
            if (formatResp.status != ResponseStatus::OK) {
                return formatResp;
            }
        }
    }

    for (const Field &field : fields) {
        if (field.hasCheck) {
            Response checkResp = ConstraintManager::validateCheckConstraint(field, updatedRecord);
            if (checkResp.status != ResponseStatus::OK) {
                return checkResp;
            }
        }
    }

    Response uniqueResp = ConstraintManager::validateUnique(fields, records, updatedRecord, true);
    if (uniqueResp.status != ResponseStatus::OK) {
        return uniqueResp;
    }

    ConstraintManager::cascadeUpdate(username, dbName, tableName, recordId, newData);

    lockManager.acquireWriteLock(username, dbName, tableName);
    records[foundIndex] = updatedRecord;
    bool success = writeAllRecordsToCache(username, dbName, tableName, fields, records);
    lockManager.releaseWriteLock(username, dbName, tableName);

    if (!success) {
        return {ResponseStatus::ERROR, QString("[Record] Failed to write updated data to table '%1'").arg(tableName), QVariant()};
    }

    return {ResponseStatus::OK, QString("[Record] Updated 1 record in '%1'").arg(tableName), QVariant(1)};
}

Response RecordManager::deleteRecord(const QString &username, const QString &dbName, const QString &tableName, const QString &recordId)
{
    qDebug() << QString("[Record] deleteRecord: table=%1 recordId=%2").arg(tableName).arg(recordId);

    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::ERROR, QString("[Record] Failed to load schema for '%1'").arg(tableName), QVariant()};
    }

    QString pkField = getPrimaryKeyField(fields);
    qDebug() << QString("[Record] deleteRecord: pkField=%1").arg(pkField);

    lockManager.acquireReadLock(username, dbName, tableName);
    QJsonArray records = readAllRecordsFromCache(username, dbName, tableName, fields);
    lockManager.releaseReadLock(username, dbName, tableName);

    qDebug() << QString("[Record] deleteRecord: read %1 records from %2").arg(records.size()).arg(tableName);

    bool found = false;
    QJsonArray remainingRecords;

    for (const QJsonValue &val : records) {
        QJsonObject record = val.toObject();
        QString currentId = record[pkField].toString();
        if (currentId != recordId) {
            remainingRecords.append(record);
        } else {
            found = true;
            qDebug() << QString("[Record] deleteRecord: found record with %1=%2, removing it").arg(pkField).arg(currentId);
        }
    }

    if (!found) {
        qDebug() << QString("[Record] deleteRecord: record with %1=%2 NOT FOUND!").arg(pkField).arg(recordId);
        return {ResponseStatus::ERROR, QString("[Record] Record with '%1'='%2' not found").arg(pkField).arg(recordId), QVariant()};
    }

    qDebug() << QString("[Record] deleteRecord: calling cascadeDelete for %1.%2").arg(tableName).arg(recordId);
    ConstraintManager::cascadeDelete(username, dbName, tableName, recordId);

    qDebug() << QString("[Record] deleteRecord: writing back %1 remaining records (was %2)")
                .arg(remainingRecords.size()).arg(records.size());

    lockManager.acquireWriteLock(username, dbName, tableName);
    bool success = writeAllRecordsToCache(username, dbName, tableName, fields, remainingRecords);
    lockManager.releaseWriteLock(username, dbName, tableName);

    if (!success) {
        return {ResponseStatus::ERROR, QString("[Record] Failed to delete record from table '%1'").arg(tableName), QVariant()};
    }

    return {ResponseStatus::OK, QString("[Record] Deleted 1 record from '%1'").arg(tableName), QVariant(1)};
}

Response RecordManager::selectWhere(const QString &username, const QString &dbName, const QString &tableName, const QString &fieldName, const QVariant &value)
{
    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' not found").arg(tableName), QVariant()};
    }

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
        return {ResponseStatus::ERROR, QString("[Record] Field '%1' not found in table '%2'").arg(fieldName).arg(tableName), QVariant()};
    }

    lockManager.acquireReadLock(username, dbName, tableName);
    QJsonArray allRecords = readAllRecordsFromCache(username, dbName, tableName, fields);
    lockManager.releaseReadLock(username, dbName, tableName);

    QJsonArray matchingRecords;
    for (const QJsonValue &val : allRecords) {
        QJsonObject record = val.toObject();
        QJsonValue recordValue = record[fieldName];

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
                match = recordValue.toString().contains(strValue, Qt::CaseInsensitive);
                break;
        }

        if (match) {
            matchingRecords.append(record);
        }
    }

    return {ResponseStatus::OK, QString("[Record] Found %1 records matching '%2'").arg(matchingRecords.size()).arg(fieldName), QVariant::fromValue(matchingRecords)};
}

Response RecordManager::selectWithCondition(const QString &username, const QString &dbName, const QString &tableName, const QJsonObject &condition)
{
    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' not found").arg(tableName), QVariant()};
    }

    lockManager.acquireReadLock(username, dbName, tableName);
    QJsonArray allRecords = readAllRecordsFromCache(username, dbName, tableName, fields);
    lockManager.releaseReadLock(username, dbName, tableName);

    QJsonArray matchingRecords;
    for (const QJsonValue &val : allRecords) {
        QJsonObject record = val.toObject();
        bool match = true;

        for (const QString &key : condition.keys()) {
            QJsonValue conditionValue = condition[key];
            QJsonValue recordValue = record[key];

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
            matchingRecords.append(record);
        }
    }

    return {ResponseStatus::OK, QString("[Record] Found %1 records matching condition").arg(matchingRecords.size()), QVariant::fromValue(matchingRecords)};
}

Response RecordManager::selectWithLimitOffset(const QString &username, const QString &dbName, const QString &tableName, int limit, int offset)
{
    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        return {ResponseStatus::TABLE_NOT_FOUND, QString("[Record] Table '%1' not found").arg(tableName), QVariant()};
    }

    lockManager.acquireReadLock(username, dbName, tableName);
    QJsonArray allRecords = readAllRecordsFromCache(username, dbName, tableName, fields);
    lockManager.releaseReadLock(username, dbName, tableName);

    QJsonArray pagedRecords;
    int skipped = 0;
    int count = 0;

    for (const QJsonValue &val : allRecords) {
        if (skipped < offset) {
            skipped++;
            continue;
        }

        if (count >= limit) {
            break;
        }

        pagedRecords.append(val);
        count++;
    }

    return {ResponseStatus::OK, QString("[Record] Retrieved %1 records (limit=%2, offset=%3)").arg(pagedRecords.size()).arg(limit).arg(offset), QVariant::fromValue(pagedRecords)};
}

Response RecordManager::replaceAllRecords(const QString &username, const QString &dbName, const QString &tableName, const QJsonArray &records)
{
    QList<Field> fields = loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty())
        return {ResponseStatus::TABLE_NOT_FOUND, "[Record] Table schema not found", QVariant()};

    lockManager.acquireWriteLock(username, dbName, tableName);
    bool success = writeAllRecordsToCache(username, dbName, tableName, fields, records);
    lockManager.releaseWriteLock(username, dbName, tableName);

    if (!success) {
        return {ResponseStatus::ERROR, QString("[Record] Failed to replace records in '%1'").arg(tableName), QVariant()};
    }

    return {ResponseStatus::OK, QString("[Record] Replaced %1 records in '%2'").arg(records.size()).arg(tableName), QVariant()};
}
