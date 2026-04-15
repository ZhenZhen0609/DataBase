#include "AuthManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
{
}

QString AuthManager::hashPassword(const QString &password) const
{
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

QJsonObject AuthManager::loadUsers() const
{
    QString filePath = findUserFile();
    if (filePath.isEmpty()) {
        qDebug() << "[Auth] User file not found, and failed to create default.";
        return QJsonObject();
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[Auth] Failed to open user file:" << filePath;
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qDebug() << "[Auth] Invalid user file format.";
        return QJsonObject();
    }

    return doc.object();
}

QString AuthManager::findUserFile() const
{
    const QString fileName = QStringLiteral("users.json");

    // 多路径搜索列表（按优先级）
    QStringList searchPaths;
    searchPaths << QDir::currentPath()                     // 当前工作目录
                << QCoreApplication::applicationDirPath()  // 可执行文件所在目录
                << QCoreApplication::applicationDirPath() + "/.." // 上级目录
                << "../"                                   // 项目根目录（从build目录启动时）
                << "../../";                               // 更上层

    for (const QString &path : searchPaths) {
        QString fullPath = QDir(path).absoluteFilePath(fileName);
        if (QFile::exists(fullPath)) {
            qDebug() << "[Auth] Found user file at:" << fullPath;
            return fullPath;
        }
    }

    // 未找到：在可执行文件目录下创建默认文件
    QString defaultPath = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(fileName);
    createDefaultUserFile(defaultPath);
    return defaultPath;
}

void AuthManager::createDefaultUserFile(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[Auth] Failed to create default user file at:" << filePath;
        return;
    }

    QJsonObject defaultUsers;
    // 默认管理员账户，密码哈希存储
    defaultUsers["admin"] = hashPassword("123456");

    QJsonDocument doc(defaultUsers);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "[Auth] Created default user file at:" << filePath;
}

bool AuthManager::login(const QString &username, const QString &password)
{
    QJsonObject users = loadUsers();
    if (users.isEmpty()) {
        qDebug() << "[Auth] No user data available, login failed.";
        return false;
    }

    if (!users.contains(username)) {
        qDebug() << "[Auth] User" << username << "does not exist.";
        return false;
    }

    QString storedHash = users.value(username).toString();
    QString inputHash = hashPassword(password);

    if (storedHash == inputHash) {
        QString timeStr = QDateTime::currentDateTime().toString("hh:mm AP");
        qDebug() << QString("[Auth] User \"%1\" logged in at %2.").arg(username, timeStr);
        return true;
    } else {
        qDebug() << "[Auth] Incorrect password for user" << username;
        return false;
    }
}

bool AuthManager::registerUser(const QString &username, const QString &password)
{
    // 加载现有用户数据
    QJsonObject users = loadUsers();

    // 如果文件不存在导致空对象，仍然可以注册（后续会创建文件）
    if (users.contains(username)) {
        qDebug() << "[Auth] Registration failed: user" << username << "already exists.";
        return false;
    }

    // 添加新用户（存储密码哈希）
    users[username] = hashPassword(password);

    // 确定写入路径：优先使用 findUserFile 返回的路径（可能已存在）
    QString filePath = findUserFile();
    if (filePath.isEmpty()) {
        qDebug() << "[Auth] Cannot determine user file path for writing.";
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[Auth] Failed to open user file for writing:" << filePath;
        return false;
    }

    QJsonDocument doc(users);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "[Auth] User" << username << "registered successfully.";
    return true;
}
