//条件解析器，WHERE子句解析、条件表达式树构建（Comparison/Like/Between/In/Logic/Not节点）、条件求值
#include "conditionparser.h"
#include <QRegularExpression>
#include <QStack>
#include <stdexcept>

// ---------- ComparisonNode ----------
ComparisonNode::ComparisonNode(const QString &field, Op op, const QVariant &value)
    : m_field(field), m_op(op), m_value(value) {}

bool ComparisonNode::evaluate(const QJsonObject &record, const QList<Field> &fields) const {
    if (!record.contains(m_field)) return false;
    QVariant recVal = record.value(m_field).toVariant();
    
    QVariant compareValue = m_value;
    if (m_value.typeId() == QMetaType::QString) {
        QString possibleField = m_value.toString();
        if (record.contains(possibleField)) {
            compareValue = record.value(possibleField).toVariant();
        }
    }
    
    for (const auto &f : fields) {
        if (f.name == m_field) {
            if (f.type == FieldType::INT) {
                int a = recVal.toInt();
                int b = compareValue.toInt();
                switch (m_op) {
                case EQUAL:     return a == b;
                case NOT_EQUAL: return a != b;
                case LESS:      return a < b;
                case GREATER:   return a > b;
                case LESS_EQ:   return a <= b;
                case GREATER_EQ:return a >= b;
                }
            } else if (f.type == FieldType::DOUBLE) {
                double a = recVal.toDouble();
                double b = compareValue.toDouble();
                switch (m_op) {
                case EQUAL:     return qFuzzyCompare(a, b);
                case NOT_EQUAL: return !qFuzzyCompare(a, b);
                case LESS:      return a < b;
                case GREATER:   return a > b;
                case LESS_EQ:   return a <= b;
                case GREATER_EQ:return a >= b;
                }
            } else if (f.type == FieldType::BOOLEAN) {
                bool a = recVal.toBool();
                bool b = compareValue.toBool();
                switch (m_op) {
                case EQUAL:     return a == b;
                case NOT_EQUAL: return a != b;
                default: return false;
                }
            } else {
                QString a = recVal.toString();
                QString b = compareValue.toString();
                switch (m_op) {
                case EQUAL:     return a == b;
                case NOT_EQUAL: return a != b;
                default: return false;
                }
            }
            break;
        }
    }
    return false;
}

// ---------- LikeNode ----------
LikeNode::LikeNode(const QString &field, const QString &pattern)
    : m_field(field), m_pattern(pattern) {}

bool LikeNode::evaluate(const QJsonObject &record, const QList<Field> &) const {
    if (!record.contains(m_field)) return false;
    QString value = record.value(m_field).toString();

    QString escaped = QRegularExpression::escape(m_pattern);
    escaped.replace("\\%", ".*");
    escaped.replace("\\_", ".");
    QString regex = "^" + escaped + "$";
    return QRegularExpression(regex).match(value).hasMatch();
}

// ---------- BetweenNode ----------
BetweenNode::BetweenNode(const QString &field, const QVariant &low, const QVariant &high)
    : m_field(field), m_low(low), m_high(high) {}

bool BetweenNode::evaluate(const QJsonObject &record, const QList<Field> &) const {
    if (!record.contains(m_field)) return false;
    QVariant val = record.value(m_field).toVariant();
    double v = val.toDouble();
    return v >= m_low.toDouble() && v <= m_high.toDouble();
}

// ---------- InNode ----------
InNode::InNode(const QString &field, const QList<QVariant> &values)
    : m_field(field), m_values(values) {}

bool InNode::evaluate(const QJsonObject &record, const QList<Field> &) const {
    if (!record.contains(m_field)) return false;
    QVariant val = record.value(m_field).toVariant();
    for (const auto &v : m_values) {
        if (val == v) return true;
    }
    return false;
}

// ---------- LogicNode ----------
LogicNode::LogicNode(LogicOp op, std::unique_ptr<ConditionNode> left, std::unique_ptr<ConditionNode> right)
    : m_op(op), m_left(std::move(left)), m_right(std::move(right)) {}

bool LogicNode::evaluate(const QJsonObject &record, const QList<Field> &fields) const {
    if (!m_left || !m_right) return false;
    bool l = m_left->evaluate(record, fields);
    bool r = m_right->evaluate(record, fields);
    return m_op == AND ? (l && r) : (l || r);
}

// ---------- NotNode ----------
NotNode::NotNode(std::unique_ptr<ConditionNode> child) : m_child(std::move(child)) {}
bool NotNode::evaluate(const QJsonObject &record, const QList<Field> &fields) const {
    return m_child ? !m_child->evaluate(record, fields) : false;
}

// ---------- 解析工具 ----------
bool ConditionParser::isOperator(const QString &token) {
    static QStringList ops = {"=", "<>", "<", ">", "<=", ">=", "AND", "OR", "NOT", "LIKE", "BETWEEN", "IN"};
    return ops.contains(token.toUpper());
}

int ConditionParser::precedence(const QString &op) {
    if (op.toUpper() == "OR") return 1;
    if (op.toUpper() == "AND") return 2;
    if (op == "=" || op == "<>" || op == "<" || op == ">" || op == "<=" || op == ">=") return 3;
    if (op.toUpper() == "NOT") return 4;
    return 0;
}

FieldType ConditionParser::fieldTypeFromName(const QString &fieldName, const QList<Field> &fields) {
    for (const auto &f : fields) {
        if (f.name == fieldName) return f.type;
    }
    return FieldType::TEXT;
}

static int g_pos = 0;
static QStringList g_tokens;
static QList<Field> g_fields;

static std::unique_ptr<ConditionNode> parseExpression(int prec = 0);

static QString nextToken() {
    if (g_pos < g_tokens.size()) return g_tokens[g_pos++];
    return QString();
}
static void putBack() { if (g_pos > 0) g_pos--; }
static QString peekToken() {
    if (g_pos < g_tokens.size()) return g_tokens[g_pos];
    return QString();
}

static QString unquote(const QString &s) {
    if (s.length() >= 2) {
        QChar first = s.at(0);
        QChar last = s.at(s.length()-1);
        if ((first == '\'' && last == '\'') || (first == '"' && last == '"'))
            return s.mid(1, s.length()-2);
    }
    return s;
}

static std::unique_ptr<ConditionNode> parseAtom() {
    QString tok = nextToken();
    if (tok.isEmpty()) throw std::runtime_error("Unexpected end of condition");

    if (tok == "(") {
        auto node = parseExpression();
        if (nextToken() != ")") throw std::runtime_error("Missing ')'");
        return node;
    }
    if (tok.toUpper() == "NOT") {
        auto node = parseAtom();
        return std::make_unique<NotNode>(std::move(node));
    }

    QString field = tok;
    QString op = nextToken().toUpper();
    if (op.isEmpty()) throw std::runtime_error("Missing operator after field");

    if (op == "LIKE") {
        QString pattern = nextToken();
        pattern = unquote(pattern);
        return std::make_unique<LikeNode>(field, pattern);
    }
    else if (op == "BETWEEN") {
        QString val1 = nextToken();
        val1 = unquote(val1);
        if (nextToken().toUpper() != "AND") throw std::runtime_error("BETWEEN ... AND ... expected");
        QString val2 = nextToken();
        val2 = unquote(val2);
        return std::make_unique<BetweenNode>(field, val1, val2);
    }
    else if (op == "IN") {
        if (nextToken() != "(") throw std::runtime_error("( expected after IN");
        QList<QVariant> vals;
        while (true) {
            QString v = nextToken();
            if (v == ")") break;
            v = unquote(v);
            vals.append(v);
            if (peekToken() == ",") nextToken();
            else if (peekToken() != ")") throw std::runtime_error("Expected , or )");
        }
        return std::make_unique<InNode>(field, vals);
    }
    else {
        ComparisonNode::Op compOp;
        if (op == "=") compOp = ComparisonNode::EQUAL;
        else if (op == "<>") compOp = ComparisonNode::NOT_EQUAL;
        else if (op == "<") compOp = ComparisonNode::LESS;
        else if (op == ">") compOp = ComparisonNode::GREATER;
        else if (op == "<=") compOp = ComparisonNode::LESS_EQ;
        else if (op == ">=") compOp = ComparisonNode::GREATER_EQ;
        else throw std::runtime_error("Unknown operator: " + op.toStdString());

        QString value = nextToken();
        value = unquote(value);
        QVariant val;
        bool ok;
        int intVal = value.toInt(&ok);
        if (ok) val = intVal;
        else {
            double dblVal = value.toDouble(&ok);
            if (ok) val = dblVal;
            else val = value;
        }
        return std::make_unique<ComparisonNode>(field, compOp, val);
    }
}

static std::unique_ptr<ConditionNode> parseExpression(int prec) {
    auto left = parseAtom();
    while (g_pos < g_tokens.size()) {
        QString op = peekToken().toUpper();
        int p = ConditionParser::precedence(op);
        if (p <= prec) break;
        nextToken();
        if (op == "AND" || op == "OR") {
            LogicNode::LogicOp logicOp = (op == "AND") ? LogicNode::AND : LogicNode::OR;
            auto right = parseExpression(p);
            left = std::make_unique<LogicNode>(logicOp, std::move(left), std::move(right));
        } else {
            putBack();
            break;
        }
    }
    return left;
}

static QStringList tokenize(const QString &str) {
    QStringList tokens;
    QString current;
    bool inString = false;
    for (int i = 0; i < str.length(); ++i) {
        QChar c = str[i];
        if (c == '\'' || c == '"') {
            inString = !inString;
            current += c;
            if (!inString) {
                tokens.append(current);
                current.clear();
            }
            continue;
        }
        if (inString) {
            current += c;
            continue;
        }
        if (c.isSpace()) {
            if (!current.isEmpty()) { tokens.append(current); current.clear(); }
            continue;
        }
        if (c == '(' || c == ')' || c == ',') {
            if (!current.isEmpty()) { tokens.append(current); current.clear(); }
            tokens.append(QString(c));
            continue;
        }
        if (c == '<' || c == '>' || c == '=') {
            if (!current.isEmpty()) { tokens.append(current); current.clear(); }
            if (i+1 < str.length() && (str[i+1] == '>' || str[i+1] == '=')) {
                tokens.append(QString() + c + str[i+1]);
                i++;
            } else {
                tokens.append(QString(c));
            }
            continue;
        }
        current += c;
    }
    if (!current.isEmpty()) tokens.append(current);
    return tokens;
}

std::unique_ptr<ConditionNode> ConditionParser::parse(const QString &conditionStr) {
    g_tokens = tokenize(conditionStr);
    g_pos = 0;
    if (g_tokens.isEmpty()) return nullptr;
    try {
        return parseExpression();
    } catch (const std::runtime_error &e) {
        qDebug() << "[ConditionParser] Parse error:" << e.what();
        return nullptr;
    }
}
