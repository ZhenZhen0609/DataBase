#include "common.h"
class StorageManager {
public:
    bool createDatabase(QString dbName);
    bool createTable(QString dbName, TableSchema schema);
};
