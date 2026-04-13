#include "common.h"
class RecordManager {
public:
    Response insertRecord(QString tableName, QVariantMap data);
    Response selectRecords(QString tableName, QString condition = "");
};
