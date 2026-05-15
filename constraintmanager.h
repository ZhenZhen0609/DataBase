#ifndef CONSTRAINTMANAGER_H
#define CONSTRAINTMANAGER_H

#include "common.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QRegularExpression>
#include <QCryptographicHash>

// 级联操作类型
enum class CascadeOperation {
    DELETE,
    UPDATE
};

class ConstraintManager {
public:
    ConstraintManager();

    // 验证外键约束
    static Response validateForeignKey(const QString& username, const QString& dbName,
                                       const Field& field, const QVariant& value);

    // 验证CHECK约束
    static Response validateCheckConstraint(const Field& field, const QJsonObject& record);

    // 验证格式（email/date/phone等）
    static Response validateFormat(const Field& field, const QVariant& value);

    // 验证唯一约束
    static Response validateUnique(const QList<Field>& fields, const QJsonArray& existingRecords,
                                   const QJsonObject& record, bool isUpdate = false);

    // 执行级联删除
    static Response cascadeDelete(const QString& username, const QString& dbName,
                                  const QString& tableName, const QString& recordId);

    // 执行级联更新
    static Response cascadeUpdate(const QString& username, const QString& dbName,
                                  const QString& tableName, const QString& recordId,
                                  const QJsonObject& updateData);

    // 数据加密/解密
    static QString encrypt(const QString& text);
    static QString decrypt(const QString& encryptedText);

private:
    // 查找引用表的主键字段
    static QString getReferenceTablePK(const QString& username, const QString& dbName,
                                        const QString& refTableName);

    // 检查值在引用表中是否存在
    static bool checkValueExistsInReference(const QString& username, const QString& dbName,
                                             const QString& refTableName, const QString& refFieldName,
                                             const QVariant& value);

    // 查找所有引用指定表的字段
    static QList<QPair<QString, Field>> findAllReferencingFields(const QString& username,
                                                                  const QString& dbName,
                                                                  const QString& refTableName);

    // 简单的XOR加密/解密
    static QString xorEncryptDecrypt(const QString& input, const QString& key);

    static const QString ENCRYPTION_KEY;
};

#endif // CONSTRAINTMANAGER_H
