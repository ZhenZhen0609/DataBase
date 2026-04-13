#include "common.h"
class AuthManager {
public:
    bool login(QString username, QString password);
};

class SchemaManager {
public:
    bool validateData(QString tableName, QVariantMap data); // 校验插入的数据是否符合字段要求
};
