#include "SchemaManager.h"
#include <QDebug>

SchemaManager::SchemaManager(QObject *parent) : QObject(parent) {}

bool SchemaManager::validateFieldType(const QString &type, const QVariant &value)
{
    if (type == "INT") {
        bool ok;
        value.toInt(&ok);
        return ok;
    } else if (type == "TEXT") {
        // 文本类型任何非空字符串都接受，可再细化
        return value.isValid() && !value.toString().isEmpty();
    } else {
        qDebug() << "[Schema] 不支持的类型:" << type;
        return false;
    }
}
