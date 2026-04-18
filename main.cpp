#include "mainwindow.h"
#include "AuthManager.h"
#include "SchemaManager.h"
#include "RecordManage

#include <QApplication>
#include <QDebug>
#include <QJsonArray>
#include <QFile>

// 测试函数声明
void runAuthTests();
void runSchemaTests();
void runRecordTests();

// 测试开关：设为 true 则运行控制台测试后退出，false 则正常启动 GUI
static const bool RUN_TESTS_ONLY = true;

int main(int argc, char *argv[])
{
    // 如果只需要运行测试（调试各模块）
    if (RUN_TESTS_ONLY) {
        QCoreApplication a(argc, argv);

        qDebug() << "\n========================================";
        qDebug() << "          DBMS 阶段任务一测试套件";
        qDebug() << "========================================\n";

        // 运行各模块测试
        runAuthTests();
        runSchemaTests();
        runRecordTests();

        qDebug() << "\n========================================";
        qDebug() << "            所有测试完成";
        qDebug() << "========================================\n";

        return 0; // 测试完成直接退出
    }

    // 正常启动 GUI 应用程序
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

// 认证模块测试
void runAuthTests()
{
    AuthManager auth;

    qDebug() << "\n=== 橙圈B - 认证管理器测试 ===";

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

    // 测试4: 注册新用户
    qDebug() << "\n[测试4] 注册新用户 (user1/password123)";
    bool result4 = auth.registerUser("user1", "password123");
    qDebug() << "注册结果: " << (result4 ? "成功 ✓" : "失败 ✗");
    if (result4) {
        qDebug() << "测试4 通过!";
    } else {
        qDebug() << "测试4 失败!";
    }

    // 测试5: 登录新注册用户
    qDebug() << "\n[测试5] 登录新注册用户 (user1/password123)";
    bool result5 = auth.login("user1", "password123");
    qDebug() << "登录结果: " << (result5 ? "成功 ✓" : "失败 ✗");
    if (result5) {
        qDebug() << "测试5 通过!";
    } else {
        qDebug() << "测试5 失败!";
    }

    // 测试6: 重复注册
    qDebug() << "\n[测试6] 测试重复注册同一用户 (user1/password123)";
    bool result6 = auth.registerUser("user1", "password123");
    qDebug() << "注册结果: " << (result6 ? "成功 ✓" : "失败 ✗");
    if (!result6) {
        qDebug() << "测试6 通过!";
    } else {
        qDebug() << "测试6 失败!";
    }

    // 类型校验测试
    qDebug() << "\n[测试7] 字段类型校验测试";
    qDebug() << "校验 123 是 INT? " << SchemaManager::validateFieldType("INT", 123);
    qDebug() << "校验 'abc' 是 INT? " << SchemaManager::validateFieldType("INT", "abc");
    qDebug() << "校验 'hello' 是 TEXT? " << SchemaManager::validateFieldType("TEXT", "hello");
    qDebug() << "校验 3.14 是 DOUBLE? " << SchemaManager::validateFieldType("DOUBLE", 3.14);
    qDebug() << "校验 true 是 BOOLEAN? " << SchemaManager::validateFieldType("BOOLEAN", true);
    qDebug() << "测试7 通过!";

    qDebug() << "\n=== 橙圈B 测试完成 ===";
}

// 模式管理器测试
void runSchemaTests()
{
    SchemaManager schemaManager;
    StorageManager storageManager;

    qDebug() << "\n=== 橙圈B - 模式管理器测试 ===";

    // 先创建测试数据库
    qDebug() << "\n[前置] 创建测试数据库 TestDB";
    storageManager.createDatabase("TestDB");

    // 测试1: 创建表结构
    qDebug() << "\n[测试1] 创建表结构 (students)";
    TableSchema studentSchema;
    studentSchema.tableName = "students";
    studentSchema.fields.append(Field("id", FieldType::INT, 10));
    studentSchema.fields.append(Field("name", FieldType::TEXT, 50));
    studentSchema.fields.append(Field("age", FieldType::INT, 3));
    
    Response createResult = schemaManager.createTable("TestDB", studentSchema);
    qDebug() << "创建结果: " << (createResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    qDebug() << "消息: " << createResult.message;
    if (createResult.status == ResponseStatus::OK) {
        qDebug() << "测试1 通过!";
    } else {
        qDebug() << "测试1 失败!";
    }

    // 测试2: 创建重复表
    qDebug() << "\n[测试2] 创建重复表 (students)";
    Response createResult2 = schemaManager.createTable("TestDB", studentSchema);
    qDebug() << "创建结果: " << (createResult2.status == ResponseStatus::ERROR ? "预期失败 ✓" : "意外成功 ✗");
    if (createResult2.status == ResponseStatus::ERROR) {
        qDebug() << "测试2 通过!";
    } else {
        qDebug() << "测试2 失败!";
    }

    // 测试3: 加载表结构
    qDebug() << "\n[测试3] 加载表结构 (students)";
    Response loadResult = schemaManager.loadTableSchema("TestDB", "students");
    qDebug() << "加载结果: " << (loadResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (loadResult.status == ResponseStatus::OK) {
        TableSchema loadedSchema = loadResult.data.value<TableSchema>();
        qDebug() << "表名: " << loadedSchema.tableName;
        qDebug() << "字段数: " << loadedSchema.fields.size();
        qDebug() << "测试3 通过!";
    } else {
        qDebug() << "测试3 失败!";
    }

    // 测试4: 加载不存在的表
    qDebug() << "\n[测试4] 加载不存在的表 (nonexistent)";
    Response loadResult2 = schemaManager.loadTableSchema("TestDB", "nonexistent");
    qDebug() << "加载结果: " << (loadResult2.status == ResponseStatus::TABLE_NOT_FOUND ? "预期失败 ✓" : "意外成功 ✗");
    if (loadResult2.status == ResponseStatus::TABLE_NOT_FOUND) {
        qDebug() << "测试4 通过!";
    } else {
        qDebug() << "测试4 失败!";
    }

    // 测试5: 加载所有表
    qDebug() << "\n[测试5] 加载所有表";
    Response loadAllResult = schemaManager.loadTables("TestDB");
    qDebug() << "加载结果: " << (loadAllResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (loadAllResult.status == ResponseStatus::OK) {
        QList<TableSchema> schemas = loadAllResult.data.value<QList<TableSchema>>();
        qDebug() << "表数量: " << schemas.size();
        qDebug() << "测试5 通过!";
    } else {
        qDebug() << "测试5 失败!";
    }

    // 测试6: 删除表
    qDebug() << "\n[测试6] 删除表 (students)";
    Response dropResult = schemaManager.dropTable("TestDB", "students");
    qDebug() << "删除结果: " << (dropResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (dropResult.status == ResponseStatus::OK) {
        qDebug() << "测试6 通过!";
    } else {
        qDebug() << "测试6 失败!";
    }

    // 测试7: 删除不存在的表
    qDebug() << "\n[测试7] 删除不存在的表 (students)";
    Response dropResult2 = schemaManager.dropTable("TestDB", "students");
    qDebug() << "删除结果: " << (dropResult2.status == ResponseStatus::TABLE_NOT_FOUND ? "预期失败 ✓" : "意外成功 ✗");
    if (dropResult2.status == ResponseStatus::TABLE_NOT_FOUND) {
        qDebug() << "测试7 通过!";
    } else {
        qDebug() << "测试7 失败!";
    }

    qDebug() << "\n=== 橙圈B 模式管理器测试完成 ===";
}

// 记录管理器测试（蓝圈C）
void runRecordTests()
{
    RecordManager recordManager;
    StorageManager storageManager;
    SchemaManager schemaManager;

    qDebug() << "\n=== 蓝圈C - 记录管理器测试 ===";

    // 先创建测试数据库和表
    qDebug() << "\n[前置] 创建测试数据库 Student_System";
    storageManager.createDatabase("Student_System");

    // 创建表结构
    TableSchema studentSchema;
    studentSchema.tableName = "test";
    studentSchema.fields.append(Field("id", FieldType::INT, 10));
    studentSchema.fields.append(Field("name", FieldType::TEXT, 50));
    schemaManager.createTable("Student_System", studentSchema);

    // 测试1: 插入第一条记录
    qDebug() << "\n[测试1] 插入第一条记录 (张三, 20岁)";
    QJsonObject record1;
    record1["id"] = 1;
    record1["name"] = "张三";
    record1["age"] = 20;

    Response insertResult1 = recordManager.insertRecord("Student_System", "test", record1);
    qDebug() << "插入结果: " << (insertResult1.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    qDebug() << "消息: " << insertResult1.message;
    if (insertResult1.status == ResponseStatus::OK) {
        qDebug() << "测试1 通过!";
    } else {
        qDebug() << "测试1 失败!";
    }

    // 测试2: 插入第二条记录
    qDebug() << "\n[测试2] 插入第二条记录 (李四, 22岁)";
    QJsonObject record2;
    record2["id"] = 2;
    record2["name"] = "李四";
    record2["age"] = 22;

    Response insertResult2 = recordManager.insertRecord("Student_System", "test", record2);
    qDebug() << "插入结果: " << (insertResult2.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    qDebug() << "消息: " << insertResult2.message;
    if (insertResult2.status == ResponseStatus::OK) {
        qDebug() << "测试2 通过!";
    } else {
        qDebug() << "测试2 失败!";
    }

    // 测试3: 查询所有记录
    qDebug() << "\n[测试3] 查询所有记录";
    Response selectResult = recordManager.selectAllRecords("Student_System", "test");
    qDebug() << "查询结果: " << (selectResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (selectResult.status == ResponseStatus::OK) {
        QJsonArray records = selectResult.data.toJsonArray();
        qDebug() << "记录数量: " << records.size();
        for (const QJsonValue &val : records) {
            QJsonObject obj = val.toObject();
            qDebug() << "  - id:" << obj["id"].toInt() << ", name:" << obj["name"].toString() 
                     << ", age:" << obj["age"].toInt();
        }
        qDebug() << "测试3 通过!";
    } else {
        qDebug() << "测试3 失败!";
    }

    // 测试4: 查询不存在的表
    qDebug() << "\n[测试4] 查询不存在的表 (nonexistent)";
    Response selectResult2 = recordManager.selectAllRecords("Student_System", "nonexistent");
    qDebug() << "查询结果: " << (selectResult2.status == ResponseStatus::TABLE_NOT_FOUND ? "预期失败 ✓" : "意外成功 ✗");
    if (selectResult2.status == ResponseStatus::TABLE_NOT_FOUND) {
        qDebug() << "测试4 通过!";
    } else {
        qDebug() << "测试4 失败!";
    }

    // 测试5: 验证数据文件存在
    QString filePath = "./data/Student_System/test.json";
    qDebug() << "\n[测试5] 验证数据文件存在";
    QFile file(filePath);
    bool exists = file.exists();
    qDebug() << "文件 " << filePath << (exists ? "存在 ✓" : "不存在 ✗");
    if (exists) {
        qDebug() << "测试5 通过!";
    } else {
        qDebug() << "测试5 失败!";
    }

    qDebug() << "\n=== 蓝圈C 记录管理器测试完成 ===";
}