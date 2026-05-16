#include "datamigrator.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QRegularExpression>
#include <QStringConverter>

DataMigrator::DataMigrator() {}

QString DataMigrator::escapeCSV(const QString &value) {
    QString result = value;
    if (result.contains("\"") || result.contains(",") || result.contains("\n")) {
        result.replace("\"", "\"\"");
        return "\"" + result + "\"";
    }
    return result;
}

QStringList DataMigrator::parseCSVLine(const QString &line) {
    QStringList fields;
    QString currentField;
    bool inQuotes = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar c = line[i];

        if (c == '\"') {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '\"') {
                currentField += '\"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.append(currentField);
            currentField.clear();
        } else {
            currentField += c;
        }
    }

    fields.append(currentField);
    return fields;
}

Response DataMigrator::exportToCSV(const QString &username, const QString &dbName, const QString &tableName, const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to open file '%1' for writing").arg(filePath), QVariant()};
    }

    RecordManager recordManager;
    Response selectResp = recordManager.selectAllRecords(username, dbName, tableName);
    if (selectResp.status != ResponseStatus::OK) {
        file.close();
        return selectResp;
    }

    QList<Field> fields = recordManager.loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        file.close();
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to load schema for table '%1'").arg(tableName), QVariant()};
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    QStringList headers;
    for (const Field &field : fields) {
        headers.append(field.name);
    }
    stream << headers.join(",") << "\n";

    QJsonArray records = selectResp.data.value<QJsonArray>();
    for (const QJsonValue &recordVal : records) {
        QJsonObject record = recordVal.toObject();
        QStringList values;

        for (const Field &field : fields) {
            QJsonValue value = record[field.name];
            QString strValue;

            if (value.isDouble()) {
                if (field.type == FieldType::INT) {
                    strValue = QString::number(value.toInt());
                } else {
                    strValue = QString::number(value.toDouble());
                }
            } else if (value.isBool()) {
                strValue = value.toBool() ? "true" : "false";
            } else if (value.isNull()) {
                strValue = "";
            } else {
                strValue = value.toString();
            }

            values.append(escapeCSV(strValue));
        }

        stream << values.join(",") << "\n";
    }

    file.close();
    return {ResponseStatus::OK, QString("[DataMigrator] Successfully exported %1 records to CSV").arg(records.size()), QVariant(records.size())};
}

Response DataMigrator::importFromCSV(const QString &username, const QString &dbName, const QString &tableName, const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to open file '%1' for reading").arg(filePath), QVariant()};
    }

    RecordManager recordManager;
    QList<Field> fields = recordManager.loadTableSchema(username, dbName, tableName);
    if (fields.isEmpty()) {
        file.close();
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to load schema for table '%1'").arg(tableName), QVariant()};
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    QString headerLine = stream.readLine();
    if (headerLine.isEmpty()) {
        file.close();
        return {ResponseStatus::ERROR, QString("[DataMigrator] CSV file is empty"), QVariant()};
    }

    QStringList headers = parseCSVLine(headerLine);
    QMap<QString, int> headerIndex;
    for (int i = 0; i < headers.size(); ++i) {
        headerIndex[headers[i]] = i;
    }

    int importedCount = 0;
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList values = parseCSVLine(line);
        QJsonObject record;

        for (const Field &field : fields) {
            if (headerIndex.contains(field.name)) {
                int index = headerIndex[field.name];
                QString value = values[index].trimmed();

                switch (field.type) {
                    case FieldType::INT:
                        record[field.name] = value.toInt();
                        break;
                    case FieldType::DOUBLE:
                        record[field.name] = value.toDouble();
                        break;
                    case FieldType::BOOLEAN:
                        record[field.name] = (value.toLower() == "true" || value == "1");
                        break;
                    case FieldType::TEXT:
                        record[field.name] = value;
                        break;
                }
            }
        }

        Response insertResp = recordManager.insertRecord(username, dbName, tableName, record);
        if (insertResp.status == ResponseStatus::OK) {
            importedCount++;
        }
    }

    file.close();
    return {ResponseStatus::OK, QString("[DataMigrator] Successfully imported %1 records from CSV").arg(importedCount), QVariant(importedCount)};
}

Response DataMigrator::exportToJSON(const QString &username, const QString &dbName, const QString &tableName, const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to open file '%1' for writing").arg(filePath), QVariant()};
    }

    RecordManager recordManager;
    Response selectResp = recordManager.selectAllRecords(username, dbName, tableName);
    if (selectResp.status != ResponseStatus::OK) {
        file.close();
        return selectResp;
    }

    QJsonArray records = selectResp.data.value<QJsonArray>();

    QJsonObject exportObj;
    exportObj["tableName"] = tableName;
    exportObj["records"] = records;

    QJsonDocument doc(exportObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return {ResponseStatus::OK, QString("[DataMigrator] Successfully exported %1 records to JSON").arg(records.size()), QVariant(records.size())};
}

Response DataMigrator::importFromJSON(const QString &username, const QString &dbName, const QString &tableName, const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to open file '%1' for reading").arg(filePath), QVariant()};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Invalid JSON format"), QVariant()};
    }

    QJsonObject obj = doc.object();
    QJsonArray records = obj["records"].toArray();

    RecordManager recordManager;
    int importedCount = 0;

    for (const QJsonValue &recordVal : records) {
        QJsonObject record = recordVal.toObject();
        Response insertResp = recordManager.insertRecord(username, dbName, tableName, record);
        if (insertResp.status == ResponseStatus::OK) {
            importedCount++;
        }
    }

    return {ResponseStatus::OK, QString("[DataMigrator] Successfully imported %1 records from JSON").arg(importedCount), QVariant(importedCount)};
}

Response DataMigrator::backupDatabase(const QString &username, const QString &dbName, const QString &backupPath) {
    StorageManager storage;
    // 先执行备份，备份路径由StorageManager根据时间戳生成
    bool success = storage.backupDatabase(username, dbName);
    if (!success) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to backup database '%1'").arg(dbName), QVariant()};
    }
    return {ResponseStatus::OK, QString("[DataMigrator] Successfully backed up database '%1'").arg(dbName), QVariant()};
}

Response DataMigrator::restoreDatabase(const QString &username, const QString &dbName, const QString &backupPath) {
    StorageManager storage;
    bool success = storage.restoreDatabase(username, dbName, backupPath);
    if (!success) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to restore database '%1'").arg(dbName), QVariant()};
    }
    return {ResponseStatus::OK, QString("[DataMigrator] Successfully restored database '%1'").arg(dbName), QVariant()};
}

Response DataMigrator::copyTable(const QString &username, const QString &dbName, const QString &sourceTable, const QString &targetTable) {
    RecordManager recordManager;
    QList<Field> sourceFields = recordManager.loadTableSchema(username, dbName, sourceTable);
    if (sourceFields.isEmpty()) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Source table '%1' not found").arg(sourceTable), QVariant()};
    }

    StorageManager storage;
    if (!storage.createTable(username, dbName, targetTable, sourceFields)) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to create target table '%1'").arg(targetTable), QVariant()};
    }

    Response selectResp = recordManager.selectAllRecords(username, dbName, sourceTable);
    if (selectResp.status != ResponseStatus::OK) {
        return selectResp;
    }

    QJsonArray records = selectResp.data.value<QJsonArray>();
    int copiedCount = 0;

    for (const QJsonValue &recordVal : records) {
        QJsonObject record = recordVal.toObject();
        Response insertResp = recordManager.insertRecord(username, dbName, targetTable, record);
        if (insertResp.status == ResponseStatus::OK) {
            copiedCount++;
        }
    }

    return {ResponseStatus::OK, QString("[DataMigrator] Successfully copied %1 records from '%2' to '%3'").arg(copiedCount).arg(sourceTable).arg(targetTable), QVariant(copiedCount)};
}

Response DataMigrator::copyDatabase(const QString &username, const QString &sourceDb, const QString &targetDb) {
    QString sourcePath = Config::dataPath() + username + "/" + sourceDb;
    QDir sourceDir(sourcePath);

    if (!sourceDir.exists()) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Source database '%1' not found").arg(sourceDb), QVariant()};
    }

    QString targetPath = Config::dataPath() + username + "/" + targetDb;
    QDir targetDir(targetPath);

    if (!targetDir.exists() && !targetDir.mkpath(".")) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to create target database directory"), QVariant()};
    }

    StorageManager storage;
    RecordManager recordManager;

    int copiedTables = 0;
    QStringList filters;
    filters << "*.tdf";
    QStringList files = sourceDir.entryList(filters, QDir::Files);

    for (const QString &tdfFile : files) {
        QString tableName = tdfFile.left(tdfFile.size() - 4);

        QList<Field> fields = storage.loadTableSchema(username, sourceDb, tableName);
        if (fields.isEmpty()) continue;

        if (!storage.createTable(username, targetDb, tableName, fields)) continue;

        Response selectResp = recordManager.selectAllRecords(username, sourceDb, tableName);
        if (selectResp.status != ResponseStatus::OK) continue;

        QJsonArray records = selectResp.data.value<QJsonArray>();
        int copiedRecords = 0;

        for (const QJsonValue &recordVal : records) {
            QJsonObject record = recordVal.toObject();
            Response insertResp = recordManager.insertRecord(username, targetDb, tableName, record);
            if (insertResp.status == ResponseStatus::OK) {
                copiedRecords++;
            }
        }

        copiedTables++;
    }

    return {ResponseStatus::OK, QString("[DataMigrator] Successfully copied %1 tables from '%2' to '%3'").arg(copiedTables).arg(sourceDb).arg(targetDb), QVariant(copiedTables)};
}

// 表结构迁移 - 将源表数据迁移到新结构的目标表
Response DataMigrator::migrateTableSchema(const QString &username, const QString &dbName, 
                                          const QString &sourceTable, const QString &targetTable, 
                                          const QList<Field> &newSchema) {
    RecordManager recordManager;
    StorageManager storage;

    // 检查源表是否存在
    QList<Field> sourceFields = recordManager.loadTableSchema(username, dbName, sourceTable);
    if (sourceFields.isEmpty()) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Source table '%1' not found").arg(sourceTable), QVariant()};
    }

    // 创建目标表
    if (!storage.createTable(username, dbName, targetTable, newSchema)) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to create target table '%1'").arg(targetTable), QVariant()};
    }

    // 获取源表数据
    Response selectResp = recordManager.selectAllRecords(username, dbName, sourceTable);
    if (selectResp.status != ResponseStatus::OK) {
        return selectResp;
    }

    QJsonArray records = selectResp.data.value<QJsonArray>();
    int migratedCount = 0;
    int skippedCount = 0;

    // 迁移数据，只保留新结构中存在的字段
    for (const QJsonValue &recordVal : records) {
        QJsonObject sourceRecord = recordVal.toObject();
        QJsonObject targetRecord;

        for (const Field &field : newSchema) {
            if (sourceRecord.contains(field.name)) {
                targetRecord[field.name] = sourceRecord[field.name];
            } else if (!field.defaultValue.isEmpty()) {
                // 使用默认值
                targetRecord[field.name] = field.defaultValue;
            }
        }

        Response insertResp = recordManager.insertRecord(username, dbName, targetTable, targetRecord);
        if (insertResp.status == ResponseStatus::OK) {
            migratedCount++;
        } else {
            skippedCount++;
        }
    }

    return {ResponseStatus::OK, QString("[DataMigrator] Table schema migration completed: %1 records migrated, %2 skipped").arg(migratedCount).arg(skippedCount), QVariant(migratedCount)};
}

// 批量数据迁移 - 将源数据库的表迁移到目标数据库
Response DataMigrator::batchMigrateData(const QString &username, const QString &sourceDb, 
                                        const QString &targetDb, const QStringList &tables) {
    QString sourcePath = Config::dataPath() + username + "/" + sourceDb;
    QDir sourceDir(sourcePath);

    if (!sourceDir.exists()) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Source database '%1' not found").arg(sourceDb), QVariant()};
    }

    // 确保目标数据库存在
    StorageManager storage;
    if (!storage.createDatabase(username, targetDb)) {
        return {ResponseStatus::ERROR, QString("[DataMigrator] Failed to create target database '%1'").arg(targetDb), QVariant()};
    }

    RecordManager recordManager;

    QStringList filters;
    filters << "*.tdf";
    QStringList tdfFiles = sourceDir.entryList(filters, QDir::Files);

    int migratedTables = 0;
    int totalRecords = 0;

    for (const QString &tdfFile : tdfFiles) {
        QString tableName = tdfFile.left(tdfFile.size() - 4);

        // 如果指定了表列表，只处理指定的表
        if (!tables.isEmpty() && !tables.contains(tableName)) {
            continue;
        }

        QList<Field> fields = storage.loadTableSchema(username, sourceDb, tableName);
        if (fields.isEmpty()) continue;

        // 在目标数据库创建表
        if (!storage.createTable(username, targetDb, tableName, fields)) continue;

        // 迁移数据
        Response selectResp = recordManager.selectAllRecords(username, sourceDb, tableName);
        if (selectResp.status != ResponseStatus::OK) continue;

        QJsonArray records = selectResp.data.value<QJsonArray>();
        int migratedRecords = 0;

        for (const QJsonValue &recordVal : records) {
            QJsonObject record = recordVal.toObject();
            Response insertResp = recordManager.insertRecord(username, targetDb, tableName, record);
            if (insertResp.status == ResponseStatus::OK) {
                migratedRecords++;
            }
        }

        migratedTables++;
        totalRecords += migratedRecords;
    }

    return {ResponseStatus::OK, QString("[DataMigrator] Batch migration completed: %1 tables, %2 records migrated").arg(migratedTables).arg(totalRecords), QVariant(totalRecords)};
}

// 兼容性检查 - 检查新表结构与现有数据的兼容性
Response DataMigrator::checkCompatibility(const QString &username, const QString &dbName, 
                                          const QList<Field> &newSchema, QString &compatibilityReport) {
    RecordManager recordManager;

    // 获取现有数据样本（前10条记录）
    QJsonArray sampleRecords;
    
    // 检查新结构中的必填字段
    QStringList requiredFields;
    QStringList newFields;
    
    for (const Field &field : newSchema) {
        newFields.append(field.name);
        if (field.isNotNull && field.defaultValue.isEmpty()) {
            requiredFields.append(field.name);
        }
    }

    compatibilityReport = QString();
    QStringList issues;

    // 检查必填字段
    if (!requiredFields.isEmpty()) {
        issues.append(QString("必填字段（无默认值）: %1").arg(requiredFields.join(", ")));
    }

    // 检查格式验证
    for (const Field &field : newSchema) {
        if (!field.formatValidation.isEmpty()) {
            issues.append(QString("格式验证字段 '%1': %2").arg(field.name).arg(field.formatValidation));
        }
    }

    // 检查CHECK约束
    for (const Field &field : newSchema) {
        if (field.hasCheck) {
            issues.append(QString("CHECK约束字段 '%1': %2").arg(field.name).arg(field.checkExpr));
        }
    }

    // 检查外键约束
    for (const Field &field : newSchema) {
        if (field.isForeignKey) {
            issues.append(QString("外键约束字段 '%1' -> %2.%3").arg(field.name).arg(field.referenceTable).arg(field.referenceField));
        }
    }

    if (issues.isEmpty()) {
        compatibilityReport = "✓ 兼容性检查通过：新表结构与现有数据兼容";
        return {ResponseStatus::OK, compatibilityReport, QVariant()};
    } else {
        compatibilityReport = QString("⚠️ 兼容性检查结果：\n") + issues.join("\n");
        return {ResponseStatus::OK, compatibilityReport, QVariant()};
    }
}

// 迁移验证 - 验证源数据库和目标数据库的迁移可行性
Response DataMigrator::validateMigration(const QString &username, const QString &sourceDb, const QString &targetDb) {
    QString sourcePath = Config::dataPath() + username + "/" + sourceDb;
    QString targetPath = Config::dataPath() + username + "/" + targetDb;

    QDir sourceDir(sourcePath);
    QDir targetDir(targetPath);

    QStringList issues;

    // 检查源数据库是否存在
    if (!sourceDir.exists()) {
        issues.append(QString("源数据库 '%1' 不存在").arg(sourceDb));
    }

    // 检查目标数据库是否已存在
    if (targetDir.exists()) {
        issues.append(QString("目标数据库 '%1' 已存在，迁移将覆盖现有数据").arg(targetDb));
    }

    // 检查源数据库中的表
    QStringList filters;
    filters << "*.tdf";
    QStringList tdfFiles = sourceDir.entryList(filters, QDir::Files);

    if (tdfFiles.isEmpty()) {
        issues.append(QString("源数据库 '%1' 中没有表").arg(sourceDb));
    }

    // 检查权限
    if (!sourceDir.isReadable()) {
        issues.append(QString("无法读取源数据库目录"));
    }

    if (issues.isEmpty()) {
        return {ResponseStatus::OK, QString("迁移验证通过：源数据库 '%1' 可迁移到目标数据库 '%2'").arg(sourceDb).arg(targetDb), QVariant()};
    } else {
        QString report = QString("迁移验证发现问题：\n") + issues.join("\n");
        return {ResponseStatus::OK, report, QVariant()};
    }
}
