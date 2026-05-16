#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <QString>
#include <QFile>
#include <QDataStream>
#include <QDateTime>
#include <QHash>
#include "common.h"
#include "lockmanager.h"

// 阶段五第二周：系统监控统计结构体
struct SystemMonitorStats {
    int totalDiskReads;       // 物理磁盘读次数 (未命中缓存)
    int totalDiskWrites;      // 物理磁盘写次数
    qint64 totalQueryTimeMs;  // 累计查询执行总耗时 (毫秒)
    int queryCount;           // 执行的查询总次数

    // 构造函数初始化为0
    SystemMonitorStats() : totalDiskReads(0), totalDiskWrites(0), totalQueryTimeMs(0), queryCount(0) {}
};

    //定义表格信息块的物理存储结构
    struct TableBlock {
    char name[128];       // 表格名称
    int record_num;       // 记录数
    int field_num;        // 字段数
    char tdf[256];        // 表格定义文件路径
    char tic[256];        // 完整性文件路径
    char trd[256];        // 记录文件路径
    char tid[256];        // 索引文件路径
    qint64 crtime;        // 创建时间 (用时间戳替代 DATETIME)
    qint64 mtime;         // 最后修改时间
};

class StorageManager
{
private:
    /**
     * @brief 内部辅助函数：向指定数据库的 ruanko.log 中追加日志
     */
    void writeLog(const QString &username, const QString &dbName, const QString &logMessage);
    
    /**
     * @brief 读写锁管理器
     */
    LockManager lockManager;

    // 简单查询缓存池 (Key: 数据库名_表名, Value: 表数据的二进制块) - 静态共享
    static QHash<QString, QByteArray> dataCache;

    // 系统监控数据存储
    SystemMonitorStats monitorStats;

public:
    StorageManager();
    ~StorageManager();

    bool createDatabase(const QString &username, const QString &dbName);
    //快速建表
    bool createTable(const QString &username, QString dbName, QString tableName);
    //建表
    bool createTable(const QString &username, QString dbName, QString tableName, const QList<Field> &fields);
    bool writeTableDefinition(const QString &username, const QString &dbName, const QString &tableName, const QByteArray &data);
    QList<Field> loadTableSchema(const QString &username, QString dbName, QString tableName);

    // 删除表物理文件
    bool dropTable(const QString &username, const QString &dbName, const QString &tableName);
    // 删除整个数据库文件夹
    bool dropDatabase(const QString &username, const QString &dbName);
    // 表结构变更 (修改字段)
    bool alterTable(const QString &username, const QString &dbName, const QString &tableName, const QList<Field> &newFields);

    /**
     * @brief 创建索引文件（在建表时自动调用主键索引）
     * @param username 用户名
     * @param dbName 数据库名
     * @param tableName 表名
     * @param fieldName 字段名
     * @param fieldType 字段类型
     * @return true成功，false失败
     */
    bool createIndexFile(const QString &username, const QString &dbName,
                         const QString &tableName, const QString &fieldName, FieldType fieldType);

    /**
     * @brief 删除索引文件
     * @param username 用户名
     * @param dbName 数据库名
     * @param tableName 表名
     * @param fieldName 字段名
     * @return true成功，false失败
     */
    bool dropIndexFile(const QString &username, const QString &dbName,
                       const QString &tableName, const QString &fieldName);

    /**
     * @brief 开始事务
     * @param username 用户名
     * @param dbName 数据库名
     * @return true成功，false失败
     */
    bool beginTransaction(const QString &username, const QString &dbName);

    /**
     * @brief 提交事务
     * @param username 用户名
     * @param dbName 数据库名
     * @return true成功，false失败
     */
    bool commitTransaction(const QString &username, const QString &dbName);

    /**
     * @brief 回滚事务
     * @param username 用户名
     * @param dbName 数据库名
     * @return true成功，false失败
     */
    bool rollbackTransaction(const QString &username, const QString &dbName);

    // 阶段五：查询性能优化与缓存接口 ---

    /**
     * @brief 优化数据读取：带缓存机制地读取 .trd 数据文件
     */
    QByteArray readTableData(const QString &username, const QString &dbName, const QString &tableName);

    /**
     * @brief 优化数据写入：写入 .trd 文件并同步更新缓存
     */
    bool writeTableData(const QString &username, const QString &dbName, const QString &tableName, const QByteArray &data);

    /**
     * @brief 清除特定表的内存缓存 (在删表或删库时调用)
     */
    void clearTableCache(const QString &dbName, const QString &tableName);
    
    /**
     * @brief 清除所有缓存 (在事务回滚时调用)
     */
    static void clearAllCache();

    // 阶段五第二周：系统监控接口 ---

    /**
     * @brief 记录一次查询耗时 (供蓝圈C或橙圈B在查询结束时调用)
     */
    void recordQueryTime(qint64 timeMs);

    /**
     * @brief 记录一次物理磁盘读操作
     */
    void recordDiskRead();

    /**
     * @brief 记录一次物理磁盘写操作
     */
    void recordDiskWrite();

    /**
     * @brief 获取当前系统监控统计数据
     */
    SystemMonitorStats getSystemStats() const;

    /**
     * @brief 打印系统监控报告到控制台
     */
    void printSystemStats() const;

    /**
     * @brief 重置监控统计数据
     */
    void resetSystemStats();

    // --- 阶段五第二周：数据备份与恢复 (任务6) ---

    /**
     * @brief 备份数据库：将当前数据库文件夹完整拷贝一份
     * 备份路径示例：./data/admin/StudentDB_backup_20260507_120000
     */
    bool backupDatabase(const QString &username, const QString &dbName);

    /**
     * @brief 恢复数据库：从指定的备份文件夹还原数据
     * @param backupFolderName 备份文件夹的名称（不含完整路径）
     */
    bool restoreDatabase(const QString &username, const QString &dbName, const QString &backupFolderName);

private:
    // 内部辅助函数：递归拷贝目录
    bool copyDirectory(const QString &srcPath, const QString &dstPath);
};

#endif // STORAGEMANAGER_H
