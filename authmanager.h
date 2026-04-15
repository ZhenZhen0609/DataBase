#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>

class AuthManager : public QObject
{
    Q_OBJECT

public:
    explicit AuthManager(QObject *parent = nullptr);

    /**
     * @brief 登录验证（密码哈希比对）
     * @param username 用户名
     * @param password 明文密码
     * @return 验证成功返回 true，失败返回 false
     */
    bool login(const QString &username, const QString &password);

    /**
     * @brief 注册新用户（自动哈希存储）
     * @param username 用户名
     * @param password 明文密码
     * @return 注册成功返回 true，若用户已存在或文件写入失败返回 false
     */
    bool registerUser(const QString &username, const QString &password);

private:
    // 密码哈希（SHA256）
    QString hashPassword(const QString &password) const;

    // 从 JSON 文件加载全部用户数据
    QJsonObject loadUsers() const;

    // 查找 users.json 文件路径，若不存在则创建默认文件
    QString findUserFile() const;

    // 创建默认用户文件（admin/123456，哈希存储）
    void createDefaultUserFile(const QString &filePath) const;
};

#endif // AUTHMANAGER_H
