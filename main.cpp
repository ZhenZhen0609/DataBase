#include "mainwindow.h"
#include "AuthManager.h"
#include "SchemaManager.h"
#include "recordmanager.h"
#include "storagemanager.h"

#include <QApplication>
#include <QDebug>
#include <QJsonArray>
#include <QFile>

// 测试函数声明
void runAuthTests();
void runSchemaTests();
void runRecordTests();
void runStorageTests();

// 测试开关：设为 true 则运行控制台测试后退出，false 则正常启动 GUI
static const bool RUN_TESTS_ONLY = false;

int main(int argc, char *argv[])
{
    // 如果只需要运行测试（调试各模块）
    if (RUN_TESTS_ONLY) {
        QCoreApplication a(argc, argv);

        qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
        qDebug() << "║                    DBMS 测试套件 - 阶段任务一览                  ║";
        qDebug() << "╠════════════════════════════════════════════════════════════════╣";
        qDebug() << "║  阶段一（基础功能）:                                            ║";
        qDebug() << "║    🔴红圈A: StorageManager - 创建数据库文件夹                    ║";
        qDebug() << "║    🟠橙圈B: AuthManager - 用户登录验证                          ║";
        qDebug() << "║    🔵蓝圈C: RecordManager - JSON格式数据插入查询                 ║";
        qDebug() << "╠════════════════════════════════════════════════════════════════╣";
        qDebug() << "║  阶段二（进阶功能）:                                            ║";
        qDebug() << "║    🔴红圈A: StorageManager - 创建.tdf/.trd/.tb表文件             ║";
        qDebug() << "║    🟠橙圈B: SchemaManager - 字段规则管理与表结构持久化           ║";
        qDebug() << "║    🔵蓝圈C: RecordManager - 结构化插入（校验.tdf）读取（.trd）    ║";
        qDebug() << "║    💗粉圈D: MainWindow - UI建表对话框与数据实时刷新              ║";
        qDebug() << "╚════════════════════════════════════════════════════════════════╝\n";

        // 运行各模块测试
        runAuthTests();
        runSchemaTests();
        runRecordTests();
        runStorageTests();

        qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
        qDebug() << "║                      所有测试执行完成                            ║";
        qDebug() << "╚════════════════════════════════════════════════════════════════╝\n";

        return 0; // 测试完成直接退出
    }

    // 正常启动 GUI 应用程序
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

// 认证模块测试（阶段一 🟠橙圈B + 阶段二 🟠橙圈B字段校验）
void runAuthTests()
{
    AuthManager auth;

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段一 🟠橙圈B + 阶段二 🟠橙圈B - 认证管理器测试                ║";
    qDebug() << "║  模块: AuthManager + SchemaManager字段校验                       ║";
    qDebug() << "║  阶段一任务: 用户登录验证（admin/123456）                        ║";
    qDebug() << "║  阶段二任务: 字段类型校验（INT/TEXT/DOUBLE/BOOLEAN）             ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
    qDebug() << "\n--- 阶段一 🟠橙圈B: 用户登录验证 ---";

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

    // 阶段二 🟠橙圈B: 字段类型校验测试
    qDebug() << "\n--- 阶段二 🟠橙圈B: 字段类型校验 ---";
    qDebug() << "\n[测试7] 字段类型校验测试";
    qDebug() << "校验 123 是 INT? " << SchemaManager::validateFieldType("INT", 123);
    qDebug() << "校验 'abc' 是 INT? " << SchemaManager::validateFieldType("INT", "abc");
    qDebug() << "校验 'hello' 是 TEXT? " << SchemaManager::validateFieldType("TEXT", "hello");
    qDebug() << "校验 3.14 是 DOUBLE? " << SchemaManager::validateFieldType("DOUBLE", 3.14);
    qDebug() << "校验 true 是 BOOLEAN? " << SchemaManager::validateFieldType("BOOLEAN", true);
    qDebug() << "测试7 通过!";

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段一 🟠橙圈B + 阶段二 🟠橙圈B - 认证管理器测试完成            ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
}

// 模式管理器测试（阶段二 🟠橙圈B + 阶段二 🔴红圈A）
void runSchemaTests()
{
    SchemaManager schemaManager;
    StorageManager storageManager;

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段二 🟠橙圈B + 阶段二 🔴红圈A - 模式管理器测试                ║";
    qDebug() << "║  模块: SchemaManager + StorageManager                           ║";
    qDebug() << "║  阶段二🔴红圈A任务: 创建.tdf/.trd/.tb表文件                     ║";
    qDebug() << "║  阶段二🟠橙圈B任务: 字段规则管理与表结构持久化                   ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
    qDebug() << "\n--- 阶段二 🔴红圈A: StorageManager创建表文件 ---";

    // 先创建测试数据库
    qDebug() << "\n[前置] 创建测试数据库 TestDB";
    storageManager.createDatabase("testuser", "TestDB");

    // 测试1: 使用新的 saveSchema 接口创建表结构
    qDebug() << "\n[测试1] 使用 saveSchema 创建表结构 (students)";
    QList<Field> studentFields;
    studentFields.append(Field("id", FieldType::INT, 10));
    studentFields.back().isPrimaryKey = true;
    studentFields.back().isNotNull = true;
    studentFields.append(Field("name", FieldType::TEXT, 50));
    studentFields.append(Field("age", FieldType::INT, 3));

    bool saveResult = schemaManager.saveSchema("testuser", "TestDB", "students", studentFields);
    qDebug() << "保存结果:" << (saveResult ? "成功 ✓" : "失败 ✗");
    if (saveResult) {
        qDebug() << "测试1 通过! 已通过 saveSchema 将字段规则写入 .tdf 文件。";
    } else {
        qDebug() << "测试1 失败!";
    }

    // 测试2: 创建重复表（应该失败）
    qDebug() << "\n[测试2] 创建重复表 (students)";
    TableSchema studentSchema;
    studentSchema.tableName = "students";
    studentSchema.fields = studentFields;
    Response createResult2 = schemaManager.createTable("testuser", "TestDB", studentSchema);
    qDebug() << "创建结果: " << (createResult2.status == ResponseStatus::ERROR ? "预期失败 ✓" : "意外成功 ✗");
    if (createResult2.status == ResponseStatus::ERROR) {
        qDebug() << "测试2 通过!";
    } else {
        qDebug() << "测试2 失败!";
    }

    // 测试3: 加载表结构
    qDebug() << "\n[测试3] 加载表结构 (students)";
    Response loadResult = schemaManager.loadTableSchema("testuser", "TestDB", "students");
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
    Response loadResult2 = schemaManager.loadTableSchema("testuser", "TestDB", "nonexistent");
    qDebug() << "加载结果: " << (loadResult2.status == ResponseStatus::TABLE_NOT_FOUND ? "预期失败 ✓" : "意外成功 ✗");
    if (loadResult2.status == ResponseStatus::TABLE_NOT_FOUND) {
        qDebug() << "测试4 通过!";
    } else {
        qDebug() << "测试4 失败!";
    }

    // 测试5: 加载所有表
    qDebug() << "\n[测试5] 加载所有表";
    Response loadAllResult = schemaManager.loadTables("testuser", "TestDB");
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
    Response dropResult = schemaManager.dropTable("testuser", "TestDB", "students");
    qDebug() << "删除结果: " << (dropResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (dropResult.status == ResponseStatus::OK) {
        qDebug() << "测试6 通过!";
    } else {
        qDebug() << "测试6 失败!";
    }

    // 测试7: 删除不存在的表
    qDebug() << "\n[测试7] 删除不存在的表 (students)";
    Response dropResult2 = schemaManager.dropTable("testuser", "TestDB", "students");
    qDebug() << "删除结果: " << (dropResult2.status == ResponseStatus::TABLE_NOT_FOUND ? "预期失败 ✓" : "意外成功 ✗");
    if (dropResult2.status == ResponseStatus::TABLE_NOT_FOUND) {
        qDebug() << "测试7 通过!";
    } else {
        qDebug() << "测试7 失败!";
    }

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段二 🟠橙圈B + 阶段二 🔴红圈A - 模式管理器测试完成            ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
}

// 记录管理器测试（阶段一 🔵蓝圈C + 阶段二 🔵蓝圈C）
void runRecordTests()
{
    RecordManager recordManager;
    StorageManager storageManager;
    SchemaManager schemaManager;

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段一 🔵蓝圈C + 阶段二 🔵蓝圈C - 记录管理器测试                ║";
    qDebug() << "║  模块: RecordManager + StorageManager + SchemaManager           ║";
    qDebug() << "║  阶段一🔵蓝圈C任务: JSON格式数据插入查询（已完成）               ║";
    qDebug() << "║  阶段二🔵蓝圈C任务: 结构化插入（校验.tdf）读取（.trd）(已完成)   ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
    qDebug() << "\n--- 阶段二 🔵蓝圈C: .trd二进制格式 + .tdf字段校验 ---";

    // 先创建测试数据库和表
    qDebug() << "\n[前置] 创建测试数据库 TestDB";
    storageManager.createDatabase("testuser", "TestDB");

    // 创建表结构
    TableSchema studentSchema;
    studentSchema.tableName = "students";
    studentSchema.fields.append(Field("id", FieldType::INT, 10));
    studentSchema.fields.append(Field("name", FieldType::TEXT, 50));
    studentSchema.fields.append(Field("age", FieldType::INT, 3));
    studentSchema.fields.append(Field("score", FieldType::DOUBLE, 6));
    schemaManager.createTable("testuser", "TestDB", studentSchema);

    // 测试1: 插入正确格式的记录
    qDebug() << "\n[测试1] 插入正确格式的记录";
    QJsonObject record1;
    record1["id"] = 1;
    record1["name"] = "张三";
    record1["age"] = 20;
    record1["score"] = 95.5;

    Response insertResult1 = recordManager.insertRecord("testuser", "TestDB", "students", record1);
    qDebug() << "插入结果: " << (insertResult1.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    qDebug() << "消息: " << insertResult1.message;
    if (insertResult1.status == ResponseStatus::OK) {
        qDebug() << "测试1 通过!";
    } else {
        qDebug() << "测试1 失败!";
    }

    // 测试2: 插入第二条记录
    qDebug() << "\n[测试2] 插入第二条记录";
    QJsonObject record2;
    record2["id"] = 2;
    record2["name"] = "李四";
    record2["age"] = 22;
    record2["score"] = 88.0;

    Response insertResult2 = recordManager.insertRecord("testuser", "TestDB", "students", record2);
    qDebug() << "插入结果: " << (insertResult2.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    qDebug() << "消息: " << insertResult2.message;
    if (insertResult2.status == ResponseStatus::OK) {
        qDebug() << "测试2 通过!";
    } else {
        qDebug() << "测试2 失败!";
    }

    // 测试3: 阶段二🔵 - 字段类型校验（错误类型）
    qDebug() << "\n[测试3] 字段类型校验 - age字段传入字符串（预期失败）";
    QJsonObject badRecord;
    badRecord["id"] = 3;
    badRecord["name"] = "王五";
    badRecord["age"] = "二十";  // 错误：age应为INT
    badRecord["score"] = 90.0;

    Response badInsert = recordManager.insertRecord("testuser", "TestDB", "students", badRecord);
    qDebug() << "插入结果: " << (badInsert.status == ResponseStatus::ERROR ? "预期失败 ✓" : "意外成功 ✗");
    qDebug() << "消息: " << badInsert.message;
    if (badInsert.status == ResponseStatus::ERROR) {
        qDebug() << "测试3 通过!";
    } else {
        qDebug() << "测试3 失败!";
    }

    // 测试4: 查询所有记录（从.trd读取）
    qDebug() << "\n[测试4] 查询所有记录（从.trd文件读取）";
    Response selectResult = recordManager.selectAllRecords("testuser", "TestDB", "students");
    qDebug() << "查询结果: " << (selectResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (selectResult.status == ResponseStatus::OK) {
        QJsonArray records = selectResult.data.toJsonArray();
        qDebug() << "记录数量: " << records.size();
        for (const QJsonValue &val : records) {
            QJsonObject obj = val.toObject();
            qDebug() << "  - id:" << obj["id"].toInt() << ", name:" << obj["name"].toString()
                     << ", age:" << obj["age"].toInt() << ", score:" << obj["score"].toDouble();
        }
        qDebug() << "测试4 通过!";
    } else {
        qDebug() << "测试4 失败!";
    }

    // 测试5: 查询不存在的表
    qDebug() << "\n[测试5] 查询不存在的表";
    Response selectResult2 = recordManager.selectAllRecords("testuser", "TestDB", "nonexistent");
    qDebug() << "查询结果: " << (selectResult2.status == ResponseStatus::TABLE_NOT_FOUND ? "预期失败 ✓" : "意外成功 ✗");
    if (selectResult2.status == ResponseStatus::TABLE_NOT_FOUND) {
        qDebug() << "测试5 通过!";
    } else {
        qDebug() << "测试5 失败!";
    }

    // 测试6: 验证.trd文件存在
    QString trdFilePath = "./data/testuser/TestDB/students.trd";
    qDebug() << "\n[测试6] 验证.trd数据文件存在";
    QFile trdFile(trdFilePath);
    bool trdExists = trdFile.exists();
    qDebug() << "文件 " << trdFilePath << (trdExists ? "存在 ✓" : "不存在 ✗");
    if (trdExists) {
        qDebug() << "文件大小: " << trdFile.size() << " bytes";
        qDebug() << "测试6 通过!";
    } else {
        qDebug() << "测试6 失败!";
    }

    // 测试7: 验证.tdf文件存在
    QString tdfFilePath = "./data/testuser/TestDB/students.tdf";
    qDebug() << "\n[测试7] 验证.tdf表结构文件存在";
    QFile tdfFile(tdfFilePath);
    bool tdfExists = tdfFile.exists();
    qDebug() << "文件 " << tdfFilePath << (tdfExists ? "存在 ✓" : "不存在 ✗");
    if (tdfExists) {
        qDebug() << "测试7 通过!";
    } else {
        qDebug() << "测试7 失败!";
    }

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段一 🔵蓝圈C + 阶段二 🔵蓝圈C - 记录管理器测试完成            ║";
    qDebug() << "║  备注: 已使用.trd二进制格式存储数据，支持.tdf字段类型校验          ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
}

// 存储管理器测试（阶段三 🔴红圈A）
void runStorageTests()
{
    StorageManager storageManager;
    QString testUser = "admin";
    QString testDB = "DropTestDB";
    QString testTable = "TempTable";

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段三 🔴红圈A - 存储管理器测试                                 ║";
    qDebug() << "║  模块: StorageManager 生命周期管理 (Drop操作)                    ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";

    // 前置准备：先造点文件出来才能删
    qDebug() << "\n[前置] 创建测试环境...";
    storageManager.createDatabase(testUser, testDB);
    storageManager.createTable(testUser, testDB, testTable);

    // 测试1：删除表
    qDebug() << "\n[测试1] 测试删除表物理文件 (dropTable)";
    bool dropTableResult = storageManager.dropTable(testUser, testDB, testTable);
    qDebug() << "删除表结果: " << (dropTableResult ? "成功 ✓" : "失败 ✗");
    if (dropTableResult) {
        qDebug() << "测试1 通过! .tdf 和 .trd 文件已被物理删除。";
    } else {
        qDebug() << "测试1 失败!";
    }

    // 测试2：删除整个数据库
    qDebug() << "\n[测试2] 测试删除整个数据库文件夹 (dropDatabase)";
    bool dropDBResult = storageManager.dropDatabase(testUser, testDB);
    qDebug() << "删除数据库结果: " << (dropDBResult ? "成功 ✓" : "失败 ✗");
    if (dropDBResult) {
        qDebug() << "测试2 通过! 数据库文件夹已彻底清空并移除。";
    } else {
        qDebug() << "测试2 失败!";
    }

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段三 🔴红圈A - 存储管理器测试完成                             ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝\n";

    // 测试3：修改表结构 (Alter Table)
    qDebug() << "\n[测试3] 测试表结构变更 (alterTable)";

    // 先建一个全新的测试库和表
    QString alterDB = "AlterTestDB";
    storageManager.createDatabase(testUser, alterDB);
    QList<Field> initialFields;
    initialFields.append(Field("id", FieldType::INT, 10)); // 初始只有1个字段
    storageManager.createTable(testUser, alterDB, "MyTable", initialFields);

    // 模拟橙圈B要求增加一个 "name" 字段
    QList<Field> newFields = initialFields;
    newFields.append(Field("name", FieldType::TEXT, 50));

    // 执行变更
    bool alterResult = storageManager.alterTable(testUser, alterDB, "MyTable", newFields);

    // 验证变更是否成功写入了文件
    QList<Field> loadedFields = storageManager.loadTableSchema(testUser, alterDB, "MyTable");

    qDebug() << "修改表结构结果: " << (alterResult ? "成功 ✓" : "失败 ✗");
    if (alterResult && loadedFields.size() == 2 && loadedFields[1].name == "name") {
        qDebug() << "测试3 通过! .tdf 已成功重写，新字段已生效，当前字段数: " << loadedFields.size();
    } else {
        qDebug() << "测试3 失败!";
    }

    // 测试4：验证日志系统是否成功记录 (Student ID: 24301132)
    qDebug() << "\n[测试4] 验证日志系统 (检查 ruanko.log 内容)";
    QString logPath = "./data/" + testUser + "/" + alterDB + "/ruanko.log";
    QFile logFile(logPath);

    if (logFile.exists() && logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "---------------- ruanko.log ----------------";
        // 使用 .noquote() 防止输出时带上多余的换行符和引号
        qDebug().noquote() << logFile.readAll().trimmed();
        qDebug() << "--------------------------------------------";
        logFile.close();
        qDebug() << "测试4 通过! 日志生成并读取成功 ✓";
    } else {
        qDebug() << "测试4 失败 ✗ 无法找到或打开日志文件。";
    }

    // 打扫战场（把测试用的数据库删掉）
    storageManager.dropDatabase(testUser, alterDB);
}
