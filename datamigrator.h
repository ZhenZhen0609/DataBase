#ifndef DATAMIGRATOR_H
#define DATAMIGRATOR_H

#include "common.h"
#include "storagemanager.h"
#include "recordmanager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

class DataMigrator {
public:
    DataMigrator();

    // 数据导入导出
    static Response exportToCSV(const QString &username, const QString &dbName, const QString &tableName, const QString &filePath);
    static Response importFromCSV(const QString &username, const QString &dbName, const QString &tableName, const QString &filePath);
    static Response exportToJSON(const QString &username, const QString &dbName, const QString &tableName, const QString &filePath);
    static Response importFromJSON(const QString &username, const QString &dbName, const QString &tableName, const QString &filePath);
    
    // 数据库备份恢复
    static Response backupDatabase(const QString &username, const QString &dbName, const QString &backupPath);
    static Response restoreDatabase(const QString &username, const QString &dbName, const QString &backupPath);
    
    // 表/数据库复制
    static Response copyTable(const QString &username, const QString &dbName, const QString &sourceTable, const QString &targetTable);
    static Response copyDatabase(const QString &username, const QString &sourceDb, const QString &targetDb);
    
    // 数据迁移工具
    static Response migrateTableSchema(const QString &username, const QString &dbName, const QString &sourceTable, 
                                       const QString &targetTable, const QList<Field> &newSchema);
    static Response batchMigrateData(const QString &username, const QString &sourceDb, const QString &targetDb,
                                     const QStringList &tables = QStringList());
    static Response checkCompatibility(const QString &username, const QString &dbName, 
                                       const QList<Field> &newSchema, QString &compatibilityReport);
    static Response validateMigration(const QString &username, const QString &sourceDb, const QString &targetDb);

private:
    static QString escapeCSV(const QString &value);
    static QStringList parseCSVLine(const QString &line);
};

#endif // DATAMIGRATOR_H
