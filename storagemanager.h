#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <QString>
#include "common.h"

class StorageManager
{
public:
    StorageManager();

    /**
     * @brief 第一步任务：实现创建数据库文件夹 [cite: 7]
     * @param dbName 数据库名称（如 "MyTestDB"）
     * @return 成功返回 true，失败返回 false
     */
    bool createDatabase(QString dbName);
};

#endif // STORAGEMANAGER_H
