#ifndef SCHEMAMANAGER_H
#define SCHEMAMANAGER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include "common.h"

class SchemaManager : public QObject
{
    Q_OBJECT
public:
    explicit SchemaManager(QObject *parent = nullptr);

    static bool validateFieldType(const QString &type, const QVariant &value);
    bool validateField(const Field &field, const QVariant &value) const;
    bool validateRecord(const TableSchema &schema, const QJsonObject &record) const;

    Response createTable(const QString &username, const QString &dbName, const TableSchema &schema);
    bool saveSchema(const QString &username, const QString &dbName, const QString &tableName, const QList<Field> &fields);
    Response loadTables(const QString &username, const QString &dbName);
    Response loadTableSchema(const QString &username, const QString &dbName, const QString &tableName);
    Response dropTable(const QString &username, const QString &dbName, const QString &tableName);

private:
    bool ensureDbDirectory(const QString &username, const QString &dbName) const;
    QByteArray serializeSchema(const QList<Field> &fields) const;
};

#endif
