#ifndef LOCKMANAGER_H
#define LOCKMANAGER_H

#include <QReadWriteLock>
#include <QMap>
#include <QString>

// 读写锁管理器 - 用于并发控制
class LockManager
{
private:
    // 表级读写锁：username/dbName/tableName -> QReadWriteLock*
    QMap<QString, QReadWriteLock*> tableLocks;
    
    // 获取锁的键名
    QString getLockKey(const QString& username, const QString& dbName, const QString& tableName) const;

public:
    LockManager();
    ~LockManager();

    // 获取表的读锁（多个读操作可并发）
    void acquireReadLock(const QString& username, const QString& dbName, const QString& tableName);
    
    // 释放表的读锁
    void releaseReadLock(const QString& username, const QString& dbName, const QString& tableName);
    
    // 获取表的写锁（写操作独占）
    void acquireWriteLock(const QString& username, const QString& dbName, const QString& tableName);
    
    // 释放表的写锁
    void releaseWriteLock(const QString& username, const QString& dbName, const QString& tableName);
};

#endif // LOCKMANAGER_H
