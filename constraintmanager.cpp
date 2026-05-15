#include "constraintmanager.h"
#include "storagemanager.h"
#include "recordmanager.h"
#include <QDebug>
#include <QDir>

// 简单的加密密钥
const QString ConstraintManager::ENCRYPTION_KEY = "DBMS_PROJECT_2024";

ConstraintManager::ConstraintManager() {}

Response ConstraintManager::validateForeignKey(const QString& username, const QString& dbName,
                                                const Field& field, const QVariant& value)
{
    if (!field.isForeignKey) {
        return {ResponseStatus::OK, "", QVariant()};
    }

    // 如果允许NULL且值为NULL，则跳过验证
    if (!field.isNotNull && (value.isNull() || value.toString().isEmpty())) {
        return {ResponseStatus::OK, "", QVariant()};
    }

    if (field.referenceTable.isEmpty() || field.referenceField.isEmpty()) {
        return {ResponseStatus::ERROR, QString("[Constraint] Invalid foreign key definition for field '%1'")
                    .arg(field.name), QVariant()};
    }

    // 检查引用表中是否存在该值
    bool exists = checkValueExistsInReference(username, dbName, field.referenceTable,
                                               field.referenceField, value);
    if (!exists) {
        return {ResponseStatus::ERROR, QString("[Constraint] Foreign key constraint failed: value '%1' not found in %2.%3")
                    .arg(value.toString()).arg(field.referenceTable).arg(field.referenceField), QVariant()};
    }

    return {ResponseStatus::OK, "", QVariant()};
}

Response ConstraintManager::validateCheckConstraint(const Field& field, const QJsonObject& record)
{
    if (!field.hasCheck || field.checkExpr.isEmpty()) {
        return {ResponseStatus::OK, "", QVariant()};
    }

    // 简单的CHECK约束解析器，支持比较运算符
    QString expr = field.checkExpr.trimmed();
    
    // 检查字段是否存在于记录中
    if (!record.contains(field.name)) {
        return {ResponseStatus::OK, "", QVariant()};
    }
    
    QJsonValue fieldValue = record[field.name];

    // 解析类似 "age > 18" 或 "salary <= 10000" 的表达式
    QRegularExpression compareRegex("^([a-zA-Z0-9_]+)\\s*([<>=]+)\\s*(.+)$");
    QRegularExpressionMatch match = compareRegex.match(expr);

    if (match.hasMatch()) {
        QString fieldName = match.captured(1);
        QString op = match.captured(2);
        QString valueStr = match.captured(3).trimmed();

        if (fieldName != field.name) {
            // 不是当前字段的检查，跳过
            return {ResponseStatus::OK, "", QVariant()};
        }

        QVariant fieldVal = fieldValue.toVariant();
        bool ok;
        double numVal = valueStr.toDouble(&ok);

        if (ok) {
            // 尝试将字段值转换为数字
            bool isNumeric = false;
            double fieldNum = 0;
            
            // 处理不同类型的字段值
            if (fieldValue.isDouble()) {
                fieldNum = fieldValue.toDouble();
                isNumeric = true;
            } else if (fieldValue.isString()) {
                fieldNum = fieldValue.toString().toDouble(&isNumeric);
            } else if (fieldValue.isBool()) {
                fieldNum = fieldValue.toBool() ? 1 : 0;
                isNumeric = true;
            }
            
            if (isNumeric) {
                bool checkPassed = false;

                if (op == ">" && fieldNum > numVal) checkPassed = true;
                else if (op == ">=" && fieldNum >= numVal) checkPassed = true;
                else if (op == "<" && fieldNum < numVal) checkPassed = true;
                else if (op == "<=" && fieldNum <= numVal) checkPassed = true;
                else if (op == "=" && qFuzzyCompare(fieldNum, numVal)) checkPassed = true;
                else if (op == "==" && qFuzzyCompare(fieldNum, numVal)) checkPassed = true;
                else if (op == "!=" && !qFuzzyCompare(fieldNum, numVal)) checkPassed = true;
                else if (op == "<>" && !qFuzzyCompare(fieldNum, numVal)) checkPassed = true;

                if (!checkPassed) {
                    return {ResponseStatus::ERROR, QString("[Constraint] CHECK constraint failed: %1 (value: %2)")
                                .arg(expr).arg(fieldNum), QVariant()};
                }
            }
        } else {
            // 非数字比较，尝试字符串比较
            QString fieldStr = fieldVal.toString();
            if (!op.isEmpty()) {
                bool checkPassed = false;
                if (op == "=" && fieldStr == valueStr) checkPassed = true;
                else if (op == "==" && fieldStr == valueStr) checkPassed = true;
                else if (op == "!=" && fieldStr != valueStr) checkPassed = true;
                else if (op == "<>" && fieldStr != valueStr) checkPassed = true;
                else if (op == "<" && fieldStr < valueStr) checkPassed = true;
                else if (op == ">" && fieldStr > valueStr) checkPassed = true;
                else if (op == "<=" && fieldStr <= valueStr) checkPassed = true;
                else if (op == ">=" && fieldStr >= valueStr) checkPassed = true;
                
                if (!checkPassed) {
                    return {ResponseStatus::ERROR, QString("[Constraint] CHECK constraint failed: %1 (value: '%2')")
                                .arg(expr).arg(fieldStr), QVariant()};
                }
            }
        }
    }

    return {ResponseStatus::OK, "", QVariant()};
}

Response ConstraintManager::validateFormat(const Field& field, const QVariant& value)
{
    if (field.formatValidation.isEmpty()) {
        return {ResponseStatus::OK, "", QVariant()};
    }

    QString strValue = value.toString();
    if (strValue.isEmpty() && !field.isNotNull) {
        return {ResponseStatus::OK, "", QVariant()};
    }

    QString format = field.formatValidation.toLower();
    bool valid = true;
    QString errorMsg;

    if (format == "email") {
        QRegularExpression emailRegex("^[A-Za-z0-9+_.-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
        valid = emailRegex.match(strValue).hasMatch();
        errorMsg = QString("'%1' is not a valid email address").arg(strValue);
    }
    else if (format == "date") {
        QRegularExpression dateRegex("^\\d{4}-\\d{2}-\\d{2}$");
        valid = dateRegex.match(strValue).hasMatch();
        errorMsg = QString("'%1' is not a valid date (YYYY-MM-DD)").arg(strValue);
    }
    else if (format == "phone") {
        QRegularExpression phoneRegex("^[0-9+-]+$");
        valid = phoneRegex.match(strValue).hasMatch();
        errorMsg = QString("'%1' is not a valid phone number").arg(strValue);
    }
    else if (format == "url") {
        QRegularExpression urlRegex("^(https?://)?([\\da-z\\.-]+)\\.([a-z\\.]{2,6})([\\/\\w \\.-]*)*\\/?$");
        valid = urlRegex.match(strValue).hasMatch();
        errorMsg = QString("'%1' is not a valid URL").arg(strValue);
    }
    else if (format == "number") {
        bool ok;
        strValue.toDouble(&ok);
        valid = ok;
        errorMsg = QString("'%1' is not a valid number").arg(strValue);
    }

    if (!valid) {
        return {ResponseStatus::ERROR, QString("[Constraint] Format validation failed for field '%1': %2")
                    .arg(field.name).arg(errorMsg), QVariant()};
    }

    return {ResponseStatus::OK, "", QVariant()};
}

Response ConstraintManager::validateUnique(const QList<Field>& fields, const QJsonArray& existingRecords,
                                           const QJsonObject& record, bool isUpdate)
{
    for (const Field& field : fields) {
        if (!field.isUnique && !field.isPrimaryKey) {
            continue;
        }

        QJsonValue newValue = record[field.name];
        int matchCount = 0;

        for (const QJsonValue& existingVal : existingRecords) {
            QJsonObject existingRecord = existingVal.toObject();
            QJsonValue existingValue = existingRecord[field.name];

            if (existingValue == newValue) {
                matchCount++;

                // 如果是更新操作，可能匹配到自己
                if (isUpdate) {
                    // 比较主键来判断是否是同一条记录
                    QString pkFieldName;
                    for (const Field& f : fields) {
                        if (f.isPrimaryKey) {
                            pkFieldName = f.name;
                            break;
                        }
                    }
                    if (!pkFieldName.isEmpty()) {
                        if (existingRecord[pkFieldName] == record[pkFieldName]) {
                            matchCount--; // 是自己，不计入
                        }
                    }
                }

                if (matchCount > 0) {
                    return {ResponseStatus::ERROR, QString("[Constraint] Unique constraint failed: value '%1' already exists in field '%2'")
                                .arg(newValue.toString()).arg(field.name), QVariant()};
                }
            }
        }
    }

    return {ResponseStatus::OK, "", QVariant()};
}

Response ConstraintManager::cascadeDelete(const QString& username, const QString& dbName,
                                           const QString& tableName, const QString& recordId)
{
    qDebug() << "[CASCADE] START: table=" << tableName << "recordId=" << recordId;

    QList<QPair<QString, Field>> refs = findAllReferencingFields(username, dbName, tableName);
    qDebug() << "[CASCADE] found" << refs.size() << "referencing tables for" << tableName;

    for (const auto &pair : refs) {
        QString childTable = pair.first;
        Field fkField = pair.second;

        qDebug() << "[CASCADE] child=" << childTable << " fk=" << fkField.name
                 << " refTable=" << fkField.referenceTable << " rule=" << fkField.cascadeRule;

        StorageManager storage;
        QList<Field> childFields = storage.loadTableSchema(username, dbName, childTable);
        if (childFields.isEmpty()) {
            qDebug() << "[CASCADE]   skip: child schema not found";
            continue;
        }

        QString childPk = "id";
        for (const Field &f : childFields) {
            if (f.isPrimaryKey) { childPk = f.name; break; }
        }

        QByteArray raw = storage.readTableData(username, dbName, childTable);
        QJsonArray childRecords;
        {
            QDataStream ds(&raw, QIODevice::ReadOnly);
            ds.setByteOrder(QDataStream::LittleEndian);
            while (!ds.atEnd()) {
                qint64 sz = 0;
                if (ds.readRawData(reinterpret_cast<char*>(&sz), sizeof(qint64)) != sizeof(qint64)) break;
                QByteArray chunk(sz, 0);
                if (ds.readRawData(chunk.data(), sz) != sz) break;
                QDataStream rs(&chunk, QIODevice::ReadOnly);
                rs.setByteOrder(QDataStream::LittleEndian);
                int fc; rs >> fc;
                QJsonObject obj;
                for (const Field &f : childFields) {
                    switch (f.type) {
                    case FieldType::INT:    { int v;    rs >> v; obj[f.name] = v; break; }
                    case FieldType::DOUBLE: { double v; rs >> v; obj[f.name] = v; break; }
                    case FieldType::BOOLEAN:   { bool v;   rs >> v; obj[f.name] = v; break; }
                    case FieldType::TEXT:   { QString v;rs >> v; obj[f.name] = v; break; }
                    }
                }
                QString ca; rs >> ca; obj["_created_at"] = ca;

                for (const Field &f : childFields) {
                    if (f.isEncrypted && obj.contains(f.name)) {
                        obj[f.name] = ConstraintManager::decrypt(obj[f.name].toString());
                    }
                }

                childRecords.append(obj);
            }
        }

        qDebug() << "[CASCADE]   child" << childTable << "has" << childRecords.size() << "records";

        QList<QJsonObject> remainingChild;
        QStringList deletedChildPks;

        for (const QJsonValue &v : childRecords) {
            QJsonObject rec = v.toObject();
            QString fkVal = rec[fkField.name].toVariant().toString();
            QString cpk = rec[childPk].toVariant().toString();
            bool match = (fkVal == recordId);

            qDebug() << "[CASCADE]     child pk=" << cpk << " fk=" << fkVal
                     << " vs recordId=" << recordId << " match=" << match;

            if (match && fkField.cascadeRule.toUpper() == "CASCADE") {
                deletedChildPks.append(cpk);
            } else if (match && fkField.cascadeRule.toUpper() == "SET NULL") {
                rec[fkField.name] = QJsonValue::Null;
                remainingChild.append(rec);
            } else {
                remainingChild.append(rec);
            }
        }

        qDebug() << "[CASCADE]   deleted=" << deletedChildPks.size() << " remaining=" << remainingChild.size();

        if (!deletedChildPks.isEmpty()) {
            QByteArray newData;
            {
                QDataStream ds(&newData, QIODevice::WriteOnly);
                ds.setByteOrder(QDataStream::LittleEndian);
                for (const QJsonObject &obj : remainingChild) {
                    QJsonObject encObj = obj;
                    for (const Field &f : childFields) {
                        if (f.isEncrypted && encObj.contains(f.name)) {
                            encObj[f.name] = ConstraintManager::encrypt(encObj[f.name].toString());
                        }
                    }

                    QByteArray chunk;
                    QDataStream rs(&chunk, QIODevice::WriteOnly);
                    rs.setByteOrder(QDataStream::LittleEndian);
                    int fcount = childFields.size();
                    rs << fcount;
                    for (const Field &f : childFields) {
                        QJsonValue val = encObj.value(f.name);
                        switch (f.type) {
                        case FieldType::INT:    rs << val.toInt(); break;
                        case FieldType::DOUBLE: rs << val.toDouble(); break;
                        case FieldType::BOOLEAN:   rs << val.toBool(); break;
                        case FieldType::TEXT:   rs << val.toString(); break;
                        }
                    }
                    rs << obj["_created_at"].toString();
                    qint64 s = chunk.size();
                    ds.writeRawData(reinterpret_cast<const char*>(&s), sizeof(qint64));
                    ds.writeRawData(chunk.data(), s);
                }
            }
            storage.writeTableData(username, dbName, childTable, newData);
            qDebug() << "[CASCADE]   wrote" << remainingChild.size() << "records back to" << childTable;

            for (const QString &cpk : deletedChildPks) {
                cascadeDelete(username, dbName, childTable, cpk);
            }
        }
    }

    return {ResponseStatus::OK, "Cascade done", QVariant()};
}

Response ConstraintManager::cascadeUpdate(const QString& username, const QString& dbName,
                                           const QString& tableName, const QString& recordId,
                                           const QJsonObject& updateData)
{
    QList<QPair<QString, Field>> referencingFields = findAllReferencingFields(username, dbName, tableName);

    RecordManager recordManager;

    for (const auto& refPair : referencingFields) {
        QString refTableName = refPair.first;
        Field refField = refPair.second;

        if (!updateData.contains(refField.referenceField)) continue;

        Response selectResp = recordManager.selectAllRecords(username, dbName, refTableName);
        if (selectResp.status != ResponseStatus::OK) continue;

        QJsonArray records = selectResp.data.value<QJsonArray>();
        QJsonValue newValue = updateData[refField.referenceField];

        for (const QJsonValue& recordVal : records) {
            QJsonObject record = recordVal.toObject();
            if (record[refField.name].toVariant().toString() == recordId) {
                QString pkField = recordManager.getPrimaryKeyField(
                    recordManager.loadTableSchema(username, dbName, refTableName)
                );
                QString recordPk = record[pkField].toString();

                if (refField.cascadeRule.toUpper() == "CASCADE") {
                    QJsonObject updateObj;
                    updateObj[refField.name] = newValue;
                    recordManager.updateRecord(username, dbName, refTableName, recordPk, updateObj);
                }
            }
        }
    }

    return {ResponseStatus::OK, "Cascade update completed", QVariant()};
}

QString ConstraintManager::encrypt(const QString& text)
{
    if (text.isEmpty()) return text;
    return xorEncryptDecrypt(text, ENCRYPTION_KEY);
}

QString ConstraintManager::decrypt(const QString& encryptedText)
{
    if (encryptedText.isEmpty()) return encryptedText;
    return xorEncryptDecrypt(encryptedText, ENCRYPTION_KEY);
}

QString ConstraintManager::getReferenceTablePK(const QString& username, const QString& dbName,
                                                 const QString& refTableName)
{
    StorageManager storage;
    QList<Field> fields = storage.loadTableSchema(username, dbName, refTableName);

    for (const Field& field : fields) {
        if (field.isPrimaryKey) {
            return field.name;
        }
    }

    // 没有显式主键，返回默认的"id"
    return "id";
}

bool ConstraintManager::checkValueExistsInReference(const QString& username, const QString& dbName,
                                                      const QString& refTableName, const QString& refFieldName,
                                                      const QVariant& value)
{
    RecordManager recordManager;
    Response resp = recordManager.selectAllRecords(username, dbName, refTableName);

    if (resp.status != ResponseStatus::OK) {
        qDebug() << QString("[Constraint] Reference table %1 not found or error reading").arg(refTableName);
        return false;
    }

    QJsonArray records = resp.data.value<QJsonArray>();

    for (const QJsonValue& recordVal : records) {
        QJsonObject record = recordVal.toObject();
        QJsonValue fieldVal = record[refFieldName];

        // 使用更健壮的比较方式，处理数字和字符串类型
        QString fieldStr = fieldVal.toVariant().toString();
        QString valueStr = value.toString();
        
        // 如果两个都是数字，比较数值
        bool fieldIsNumber, valueIsNumber;
        double fieldNum = fieldStr.toDouble(&fieldIsNumber);
        double valueNum = valueStr.toDouble(&valueIsNumber);
        
        if (fieldIsNumber && valueIsNumber) {
            if (qFuzzyCompare(fieldNum, valueNum)) {
                return true;
            }
        } else {
            // 字符串比较
            if (fieldStr == valueStr) {
                return true;
            }
        }
    }

    return false;
}

QList<QPair<QString, Field>> ConstraintManager::findAllReferencingFields(const QString& username,
                                                                           const QString& dbName,
                                                                           const QString& refTableName)
{
    QList<QPair<QString, Field>> result;

    QString dbPath = Config::DATA_PATH + username + "/" + dbName;
    qDebug() << QString("[Constraint] findAllReferencingFields: Searching in path %1 for refs to %2").arg(dbPath).arg(refTableName);

    QDir dir(dbPath);
    if (!dir.exists()) {
        qDebug() << QString("[Constraint] Database path does not exist: %1").arg(dbPath);
        return result;
    }

    dir.setNameFilters({"*.tdf"});
    QStringList tdfFiles = dir.entryList(QDir::Files);
    qDebug() << QString("[Constraint] Found tdf files: %1").arg(tdfFiles.join(", "));

    StorageManager storage;

    for (const QString& tdfFile : tdfFiles) {
        QString tableName = tdfFile.left(tdfFile.size() - 4);
        QList<Field> fields = storage.loadTableSchema(username, dbName, tableName);

        for (const Field& field : fields) {
            if (field.isForeignKey && field.referenceTable == refTableName) {
                qDebug() << QString("[Constraint] Found foreign key: %1.%2 references %3.%4 (cascade: %5)")
                            .arg(tableName).arg(field.name).arg(field.referenceTable).arg(field.referenceField).arg(field.cascadeRule);
                result.append(qMakePair(tableName, field));
            }
        }
    }

    return result;
}

QString ConstraintManager::xorEncryptDecrypt(const QString& input, const QString& key)
{
    QByteArray inputBytes = input.toUtf8();
    QByteArray keyBytes = key.toUtf8();
    QByteArray result;

    for (int i = 0; i < inputBytes.size(); ++i) {
        result.append(inputBytes[i] ^ keyBytes[i % keyBytes.size()]);
    }

    return QString::fromUtf8(result.toBase64());
}
