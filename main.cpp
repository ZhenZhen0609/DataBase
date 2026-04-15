#include "mainwindow.h"
#include "AuthManager.h"
#include "SchemaManager.h"

#include <QApplication>
#include <QDebug>

// 测试函数声明
void runAuthTests();

// 测试开关：设为 true 则运行控制台测试后退出，false 则正常启动 GUI
static const bool RUN_TESTS_ONLY = true;

int main(int argc, char *argv[])
{
    // 如果只需要运行测试（调试 AuthManager）
    if (RUN_TESTS_ONLY) {
        QCoreApplication a(argc, argv);

        // 控制台测试内容
        AuthManager auth;

        // 注册测试用户
        auth.registerUser("admin", "123456");

        // 尝试登录
        bool success = auth.login("admin", "123456");
        qDebug() << "登录结果:" << success;

        bool fail = auth.login("admin", "wrong");
        qDebug() << "错误密码登录结果:" << fail;

        qDebug() << "校验 123 是 INT?" << SchemaManager::validateFieldType("INT", 123);
        qDebug() << "校验 'abc' 是 INT?" << SchemaManager::validateFieldType("INT", "abc");

        // 运行详细测试函数
        runAuthTests();

        return 0; // 测试完成直接退出
    }

    // 正常启动 GUI 应用程序
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

// 详细测试函数的实现
void runAuthTests()
{
    AuthManager auth;

    qDebug() << "\n=== 测试登录功能 ===";

    // 测试1: 正确的账号密码
    qDebug() << "\n[测试1] 测试正确的账号密码 (admin/123456)";
    bool result1 = auth.login("admin", "123456");
    qDebug() << "登录结果: " << (result1 ? "成功 ✓" : "失败 ✗");
    if (result1) {
        qDebug() << "测试1 通过!";
    } else {
        qDebug() << "测试1 失败!";
    }

    // 测试2: 错误的密码
    qDebug() << "\n[测试2] 测试错误的密码 (admin/654321)";
    bool result2 = auth.login("admin", "654321");
    qDebug() << "登录结果: " << (result2 ? "成功 ✓" : "失败 ✗");
    if (!result2) {
        qDebug() << "测试2 通过!";
    } else {
        qDebug() << "测试2 失败!";
    }

    // 测试3: 不存在的用户
    qDebug() << "\n[测试3] 测试不存在的用户 (test/123456)";
    bool result3 = auth.login("test", "123456");
    qDebug() << "登录结果: " << (result3 ? "成功 ✓" : "失败 ✗");
    if (!result3) {
        qDebug() << "测试3 通过!";
    } else {
        qDebug() << "测试3 失败!";
    }

    // 测试4: 空用户名
    qDebug() << "\n[测试4] 测试空用户名";
    bool result4 = auth.login("", "123456");
    qDebug() << "登录结果: " << (result4 ? "成功 ✓" : "失败 ✗");
    if (!result4) {
        qDebug() << "测试4 通过!";
    } else {
        qDebug() << "测试4 失败!";
    }

    // 测试5: 空密码
    qDebug() << "\n[测试5] 测试空密码";
    bool result5 = auth.login("admin", "");
    qDebug() << "登录结果: " << (result5 ? "成功 ✓" : "失败 ✗");
    if (!result5) {
        qDebug() << "测试5 通过!";
    } else {
        qDebug() << "测试5 失败!";
    }

    qDebug() << "\n=== 测试完成 ===";
}
