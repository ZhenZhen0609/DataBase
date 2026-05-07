#include "lockmanager.h"

LockManager::LockManager() {}

LockManager::~LockManager() {
    // 释放所有锁
    qDeleteAll(tableLocks);
    tableLocks.clear();
}

QString LockManager::getLockKey(const QString& username, const QString& dbName, const QString& tableName) const {
    return QString("%1/%2/%3").arg(username).arg(dbName).arg(tableName);
}

void LockManager::acquireReadLock(const QString& username, const QString& dbName, const QString& tableName) {
    QString key = getLockKey(username, dbName, tableName);
    
    if (!tableLocks.contains(key)) {
        tableLocks[key] = new QReadWriteLock();
    }
    
    tableLocks[key]->lockForRead();
}

void LockManager::releaseReadLock(const QString& username, const QString& dbName, const QString& tableName) {
    QString key = getLockKey(username, dbName, tableName);
    
    if (tableLocks.contains(key)) {
        tableLocks[key]->unlock();
    }
}

void LockManager::acquireWriteLock(const QString& username, const QString& dbName, const QString& tableName) {
    QString key = getLockKey(username, dbName, tableName);
    
    if (!tableLocks.contains(key)) {
        tableLocks[key] = new QReadWriteLock();
    }
    
    tableLocks[key]->lockForWrite();
}

void LockManager::releaseWriteLock(const QString& username, const QString& dbName, const QString& tableName) {
    QString key = getLockKey(username, dbName, tableName);
    
    if (tableLocks.contains(key)) {
        tableLocks[key]->unlock();
    }
}
