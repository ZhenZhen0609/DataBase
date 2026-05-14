#ifndef CONDITIONPARSER_H
#define CONDITIONPARSER_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QJsonObject>
#include <QList>
#include <memory>
#include "common.h"

// 条件求值基类
class ConditionNode {
public:
    virtual ~ConditionNode() {}
    virtual bool evaluate(const QJsonObject &record, const QList<Field> &fields) const = 0;
};

// 比较节点：=, <>, <, >, <=, >=
class ComparisonNode : public ConditionNode {
public:
    enum Op { EQUAL, NOT_EQUAL, LESS, GREATER, LESS_EQ, GREATER_EQ };

    ComparisonNode(const QString &field, Op op, const QVariant &value);
    bool evaluate(const QJsonObject &record, const QList<Field> &fields) const override;

private:
    QString m_field;
    Op m_op;
    QVariant m_value;
};

// LIKE 节点
class LikeNode : public ConditionNode {
public:
    LikeNode(const QString &field, const QString &pattern);
    bool evaluate(const QJsonObject &record, const QList<Field> &fields) const override;

private:
    QString m_field;
    QString m_pattern;
};

// BETWEEN 节点
class BetweenNode : public ConditionNode {
public:
    BetweenNode(const QString &field, const QVariant &low, const QVariant &high);
    bool evaluate(const QJsonObject &record, const QList<Field> &fields) const override;

private:
    QString m_field;
    QVariant m_low, m_high;
};

// IN 节点
class InNode : public ConditionNode {
public:
    InNode(const QString &field, const QList<QVariant> &values);
    bool evaluate(const QJsonObject &record, const QList<Field> &fields) const override;

private:
    QString m_field;
    QList<QVariant> m_values;
};

// 逻辑节点：AND, OR
class LogicNode : public ConditionNode {
public:
    enum LogicOp { AND, OR };
    LogicNode(LogicOp op, std::unique_ptr<ConditionNode> left, std::unique_ptr<ConditionNode> right);
    bool evaluate(const QJsonObject &record, const QList<Field> &fields) const override;

private:
    LogicOp m_op;
    std::unique_ptr<ConditionNode> m_left;
    std::unique_ptr<ConditionNode> m_right;
};

// NOT 节点
class NotNode : public ConditionNode {
public:
    explicit NotNode(std::unique_ptr<ConditionNode> child);
    bool evaluate(const QJsonObject &record, const QList<Field> &fields) const override;

private:
    std::unique_ptr<ConditionNode> m_child;
};

// 条件解析器
class ConditionParser {
public:
    // 解析 WHERE 子句文本（不含 "WHERE" 关键字），返回条件树根节点
    static std::unique_ptr<ConditionNode> parse(const QString &conditionStr);

    // 以下三个函数必须为 public，因为文件级解析函数（parseExpression、parseAtom 等）需要调用它们
    static bool isOperator(const QString &token);
    static int precedence(const QString &op);
    static FieldType fieldTypeFromName(const QString &fieldName, const QList<Field> &fields);
};

#endif // CONDITIONPARSER_H
