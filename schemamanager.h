#ifndef SCHEMAMANAGER_H
#define SCHEMAMANAGER_H

#include <QObject>
#include <QString>
#include <QVariant>

class SchemaManager : public QObject
{
    Q_OBJECT
public:
    explicit SchemaManager(QObject *parent = nullptr);

    // 校验某个值是否符合指定的类型（目前支持 "INT", "TEXT"）
    static bool validateFieldType(const QString &type, const QVariant &value);
};

#endif
