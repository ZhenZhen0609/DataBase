#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QList>
#include <QVariant>
#include <QMap>
#include <QDateTime>

// 1. 定义支持的数据类型
enum class FieldType {
    INT,
    TEXT,
    DOUBLE,
    BOOLEAN
};

// 2. 字段定义（由橙圈B和蓝圈C重点使用）
struct Field {
    QString name;       // 字段名
    FieldType type;    // 类型
    int length;        // 长度限制（可选）
    bool isNotNull;    // 是否必填
    bool isPrimaryKey; // 是否为主键

    // 构造函数，方便初始化
    Field(QString n="", FieldType t=FieldType::TEXT, int l=255)
        : name(n), type(t), length(l), isNotNull(false), isPrimaryKey(false) {}
};

// 3. 表结构定义（由红圈A和橙圈B重点使用）
struct TableSchema {
    QString tableName;
    QList<Field> fields;
};

// 4. 操作返回状态（全员使用，用于界面提示）
enum class ResponseStatus {
    OK,
    ERROR,
    DB_NOT_FOUND,
    TABLE_NOT_FOUND,
    AUTH_FAILED,
    FIELD_MISMATCH
};

struct Response {
    ResponseStatus status;
    QString message;   // 错误详情或成功信息
    QVariant data;     // 返回的数据（比如查询结果集）
};

// 5. 全局常量（由红圈A定义，全员遵守）
namespace Config {
const QString DATA_PATH = "./data/";        // 数据存储根目录
const QString USER_FILE = "users.json";     // 用户配置文件名
}

#endif // COMMON_H
