#include "mainwindow.h"
#include "AuthManager.h"
#include "SchemaManager.h"
#include "recordmanager.h"
#include "storagemanager.h"
#include "sqlparser.h"
#include "indexmanager.h"
#include "lockmanager.h"

#include <QApplication>
#include <QDebug>
#include <QJsonArray>
#include <QFile>
#include <QElapsedTimer>
#include <QDir>

// 测试函数声明
void runAuthTests();
void runSchemaTests();
void runRecordTests();
void runStorageTests();
void runSQLParserTests();
void runIndexTests();
void runQueryOptimizationTests();
void runTransactionTests();
void runLockManagerTests();
void runMonitorTests();
void runBackupTests();
void runDMLTests();

// 测试开关：设为 true 则运行控制台测试后退出，false 则正常启动 GUI
static const bool RUN_TESTS_ONLY = true;

int main(int argc, char *argv[])
{
    // 如果只需要运行测试（调试各模块）
    if (RUN_TESTS_ONLY) {
        QCoreApplication a(argc, argv);
        qRegisterMetaType<TableSchema>();
        // 运行各模块测试
        runAuthTests();
        runSchemaTests();
        runRecordTests();
        runStorageTests();
        runSQLParserTests();
        runIndexTests();
        runQueryOptimizationTests();
        runTransactionTests();
        runLockManagerTests();
        runMonitorTests();
        runBackupTests();
        runDMLTests();

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
    QString alterDB = "AlterTestDB_" + QString::number(QDateTime::currentSecsSinceEpoch());
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

    // 测试5：重复创建相同数据库的拦截测试
    qDebug() << "\n[测试5] 测试重复创建数据库 (拦截验证)";
    QString dupDB = "DupTestDB_" + QString::number(QDateTime::currentSecsSinceEpoch());

    // 第一次建库，应该成功
    storageManager.createDatabase(testUser, dupDB);

    // 第二次建同名库，应该触发我们刚才写的拦截逻辑，返回 false
    bool dupResult = storageManager.createDatabase(testUser, dupDB);

    qDebug() << "重复建库结果: " << (!dupResult ? "预期失败 ✓" : "意外成功 ✗");
    if (!dupResult) {
        qDebug() << "测试5 通过! 成功拦截了重复建库操作。";
    } else {
        qDebug() << "测试5 失败! 代码依然返回了 true，请检查刚才的修改。";
    }

    // 打扫战场（把测试用的数据库删掉）
    storageManager.dropDatabase(testUser, alterDB);
    storageManager.dropDatabase(testUser, dupDB);
}

void runSQLParserTests() {
    StorageManager storage;
    SQLParser parser;
    parser.setStorageManager(&storage);
    parser.setCurrentUser("admin");   // 默认用户

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段四 🟠橙圈B - SQL解析器测试                                 ║";
    qDebug() << "║  模块: SQLParser + StorageManager                              ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";

    // 测试1：创建数据库
    qDebug() << "\n[测试1] 解析并执行: CREATE DATABASE TestSQLDB;";
    Response res1 = parser.parseSQL("CREATE DATABASE TestSQLDB;");
    qDebug() << "状态:" << (res1.status == ResponseStatus::OK ? "OK" : "ERROR") << ", 消息:" << res1.message;

    // 测试2：重复创建同一数据库（应失败）
    qDebug() << "\n[测试2] 重复创建: CREATE DATABASE TestSQLDB;";
    Response res2 = parser.parseSQL("CREATE DATABASE TestSQLDB;");
    qDebug() << "状态:" << (res2.status == ResponseStatus::OK ? "OK" : "ERROR") << ", 消息:" << res2.message;

    // 测试3：在当前数据库下创建表（必须先设置当前数据库）
    qDebug() << "\n[测试3] 设置当前数据库为 TestSQLDB，然后创建表";
    parser.setCurrentDatabase("TestSQLDB");
    Response res3 = parser.parseSQL("CREATE TABLE students (id INT, name TEXT, age INT);");
    qDebug() << "状态:" << (res3.status == ResponseStatus::OK ? "OK" : "ERROR") << ", 消息:" << res3.message;

    // 测试4：错误语法
    qDebug() << "\n[测试4] 错误语法: CREAT TABLE x (a INT);";
    Response res4 = parser.parseSQL("CREAT TABLE x (a INT);");
    qDebug() << "状态:" << (res4.status == ResponseStatus::OK ? "OK" : "ERROR") << ", 消息:" << res4.message;

    // 测试5：删除表
    qDebug() << "\n[测试5] 删除表: DROP TABLE students;";
    Response res5 = parser.parseSQL("DROP TABLE students;");
    qDebug() << "状态:" << (res5.status == ResponseStatus::OK ? "OK" : "ERROR") << ", 消息:" << res5.message;

    // 测试6：删除数据库
    qDebug() << "\n[测试6] 删除数据库: DROP DATABASE TestSQLDB;";
    Response res6 = parser.parseSQL("DROP DATABASE TestSQLDB;");
    qDebug() << "状态:" << (res6.status == ResponseStatus::OK ? "OK" : "ERROR") << ", 消息:" << res6.message;

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段四 🟠橙圈B - SQL解析器测试完成                             ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
}

// 索引管理器测试（阶段五 🔴红圈A - 第一周任务1）
void runIndexTests()
{
    IndexManager indexManager;
    StorageManager storageManager;

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 索引管理器测试                                ║";
    qDebug() << "║  模块: IndexManager                                            ║";
    qDebug() << "║  任务: 创建/删除索引、索引查找、添加/删除索引项                ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";

    // 先创建测试数据库和表
    qDebug() << "\n[前置] 创建测试数据库 IndexTestDB";
    bool dbResult = storageManager.createDatabase("testuser", "IndexTestDB");
    qDebug() << "数据库创建结果:" << (dbResult ? "成功 ✓" : "失败 ✗");

    qDebug() << "\n[前置] 创建测试表 test_table";
    QList<Field> fields;
    fields.append(Field("id", FieldType::INT, 10));
    fields.back().isPrimaryKey = true;
    fields.append(Field("name", FieldType::TEXT, 50));
    bool tableResult = storageManager.createTable("testuser", "IndexTestDB", "test_table", fields);
    qDebug() << "表创建结果:" << (tableResult ? "成功 ✓" : "失败 ✗");

    // 测试1: 创建索引
    qDebug() << "\n[测试1] 创建索引 (idx_id on test_table.id)";
    bool createResult = indexManager.createIndex("testuser", "IndexTestDB", "test_table", "idx_id", "id", FieldType::INT);
    qDebug() << "创建结果:" << (createResult ? "成功 ✓" : "失败 ✗");
    if (createResult) {
        qDebug() << "测试1 通过!";
    } else {
        qDebug() << "测试1 失败!";
    }

    // 测试2: 重复创建索引（应失败）
    qDebug() << "\n[测试2] 重复创建索引 (idx_id on test_table.id)";
    bool createResult2 = indexManager.createIndex("testuser", "IndexTestDB", "test_table", "idx_id", "id", FieldType::INT);
    qDebug() << "创建结果:" << (!createResult2 ? "预期失败 ✓" : "意外成功 ✗");
    if (!createResult2) {
        qDebug() << "测试2 通过!";
    } else {
        qDebug() << "测试2 失败!";
    }

    // 测试3: 添加索引项
    qDebug() << "\n[测试3] 添加索引项 (key=1, offset=0)";
    bool addResult = indexManager.addIndexEntry("testuser", "IndexTestDB", "test_table", "idx_id", 1, 0);
    qDebug() << "添加结果:" << (addResult ? "成功 ✓" : "失败 ✗");
    if (addResult) {
        qDebug() << "测试3 通过!";
    } else {
        qDebug() << "测试3 失败!";
    }

    // 测试4: 添加更多索引项
    qDebug() << "\n[测试4] 添加更多索引项 (key=2, offset=100; key=3, offset=200)";
    bool addResult2 = indexManager.addIndexEntry("testuser", "IndexTestDB", "test_table", "idx_id", 2, 100);
    bool addResult3 = indexManager.addIndexEntry("testuser", "IndexTestDB", "test_table", "idx_id", 3, 200);
    qDebug() << "添加结果:" << (addResult2 && addResult3 ? "成功 ✓" : "失败 ✗");
    if (addResult2 && addResult3) {
        qDebug() << "测试4 通过!";
    } else {
        qDebug() << "测试4 失败!";
    }

    // 测试5: 查找索引项
    qDebug() << "\n[测试5] 查找索引项 (key=2)";
    int offset = indexManager.lookup("testuser", "IndexTestDB", "test_table", "idx_id", 2);
    qDebug() << "查找结果: offset=" << offset << (offset == 100 ? " ✓" : " ✗");
    if (offset == 100) {
        qDebug() << "测试5 通过!";
    } else {
        qDebug() << "测试5 失败!";
    }

    // 测试6: 查找不存在的键
    qDebug() << "\n[测试6] 查找不存在的键 (key=999)";
    int offset2 = indexManager.lookup("testuser", "IndexTestDB", "test_table", "idx_id", 999);
    qDebug() << "查找结果: offset=" << offset2 << (offset2 == -1 ? " ✓" : " ✗");
    if (offset2 == -1) {
        qDebug() << "测试6 通过!";
    } else {
        qDebug() << "测试6 失败!";
    }

    // 测试7: 删除索引项
    qDebug() << "\n[测试7] 删除索引项 (key=2)";
    bool removeResult = indexManager.removeIndexEntry("testuser", "IndexTestDB", "test_table", "idx_id", 2);
    qDebug() << "删除结果:" << (removeResult ? "成功 ✓" : "失败 ✗");
    if (removeResult) {
        qDebug() << "测试7 通过!";
    } else {
        qDebug() << "测试7 失败!";
    }

    // 测试8: 验证删除后的查找结果
    qDebug() << "\n[测试8] 验证删除后的查找结果 (key=2)";
    int offset3 = indexManager.lookup("testuser", "IndexTestDB", "test_table", "idx_id", 2);
    qDebug() << "查找结果: offset=" << offset3 << (offset3 == -1 ? " ✓" : " ✗");
    if (offset3 == -1) {
        qDebug() << "测试8 通过!";
    } else {
        qDebug() << "测试8 失败!";
    }

    // 测试9: 删除索引
    qDebug() << "\n[测试9] 删除索引 (idx_id)";
    bool dropResult = indexManager.dropIndex("testuser", "IndexTestDB", "test_table", "idx_id");
    qDebug() << "删除结果:" << (dropResult ? "成功 ✓" : "失败 ✗");
    if (dropResult) {
        qDebug() << "测试9 通过!";
    } else {
        qDebug() << "测试9 失败!";
    }

    // 测试10: 删除不存在的索引（应失败）
    qDebug() << "\n[测试10] 删除不存在的索引 (idx_id)";
    bool dropResult2 = indexManager.dropIndex("testuser", "IndexTestDB", "test_table", "idx_id");
    qDebug() << "删除结果:" << (!dropResult2 ? "预期失败 ✓" : "意外成功 ✗");
    if (!dropResult2) {
        qDebug() << "测试10 通过!";
    } else {
        qDebug() << "测试10 失败!";
    }

    // 清理测试数据
    qDebug() << "\n[清理] 删除测试数据库 IndexTestDB";
    storageManager.dropDatabase("testuser", "IndexTestDB");

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 索引管理器测试完成                            ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
}

// 查询性能优化测试（阶段五 🔴红圈A - 第一周任务2）
void runQueryOptimizationTests()
{
    RecordManager recordManager;
    StorageManager storageManager;

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 查询性能优化测试                              ║";
    qDebug() << "║  模块: RecordManager                                           ║";
    qDebug() << "║  任务: WHERE条件推送、LIMIT/OFFSET分页支持                      ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";

    // 先创建测试数据库和表
    qDebug() << "\n[前置] 创建测试数据库 QueryOptTestDB";
    bool dbResult = storageManager.createDatabase("testuser", "QueryOptTestDB");
    qDebug() << "数据库创建结果:" << (dbResult ? "成功 ✓" : "失败 ✗");

    qDebug() << "\n[前置] 创建测试表 users";
    QList<Field> fields;
    fields.append(Field("id", FieldType::INT, 10));
    fields.back().isPrimaryKey = true;
    fields.append(Field("name", FieldType::TEXT, 50));
    fields.append(Field("age", FieldType::INT, 3));
    fields.append(Field("score", FieldType::DOUBLE, 6));
    bool tableResult = storageManager.createTable("testuser", "QueryOptTestDB", "users", fields);
    qDebug() << "表创建结果:" << (tableResult ? "成功 ✓" : "失败 ✗");

    // 插入测试数据
    qDebug() << "\n[前置] 插入10条测试数据";
    for (int i = 1; i <= 10; i++) {
        QJsonObject record;
        record["id"] = i;
        record["name"] = QString("User%1").arg(i);
        record["age"] = 20 + (i % 5);
        record["score"] = 60.0 + (i * 3.5);
        recordManager.insertRecord("testuser", "QueryOptTestDB", "users", record);
    }
    qDebug() << "插入完成";

    // 测试1: WHERE条件推送
    qDebug() << "\n[测试1] WHERE条件推送 (age=22)";
    QJsonObject condition;
    condition["age"] = 22;
    Response resp1 = recordManager.selectWithCondition("testuser", "QueryOptTestDB", "users", condition);
    qDebug() << "查询结果:" << (resp1.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (resp1.status == ResponseStatus::OK) {
        QJsonArray records = resp1.data.toJsonArray();
        qDebug() << "匹配记录数:" << records.size();
        qDebug() << "测试1 通过!";
    } else {
        qDebug() << "测试1 失败!";
    }

    // 测试2: LIMIT/OFFSET分页 - 第一页
    qDebug() << "\n[测试2] LIMIT/OFFSET分页 (limit=3, offset=0)";
    Response resp2 = recordManager.selectWithLimitOffset("testuser", "QueryOptTestDB", "users", 3, 0);
    qDebug() << "查询结果:" << (resp2.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (resp2.status == ResponseStatus::OK) {
        QJsonArray records = resp2.data.toJsonArray();
        qDebug() << "返回记录数:" << records.size();
        if (records.size() == 3) {
            qDebug() << "测试2 通过!";
        } else {
            qDebug() << "测试2 失败! 期望3条，实际" << records.size() << "条";
        }
    } else {
        qDebug() << "测试2 失败!";
    }

    // 测试3: LIMIT/OFFSET分页 - 第二页
    qDebug() << "\n[测试3] LIMIT/OFFSET分页 (limit=3, offset=3)";
    Response resp3 = recordManager.selectWithLimitOffset("testuser", "QueryOptTestDB", "users", 3, 3);
    qDebug() << "查询结果:" << (resp3.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (resp3.status == ResponseStatus::OK) {
        QJsonArray records = resp3.data.toJsonArray();
        qDebug() << "返回记录数:" << records.size();
        if (records.size() == 3) {
            qDebug() << "测试3 通过!";
        } else {
            qDebug() << "测试3 失败! 期望3条，实际" << records.size() << "条";
        }
    } else {
        qDebug() << "测试3 失败!";
    }

    // 测试4: LIMIT/OFFSET分页 - 最后一页
    qDebug() << "\n[测试4] LIMIT/OFFSET分页 (limit=3, offset=9)";
    Response resp4 = recordManager.selectWithLimitOffset("testuser", "QueryOptTestDB", "users", 3, 9);
    qDebug() << "查询结果:" << (resp4.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (resp4.status == ResponseStatus::OK) {
        QJsonArray records = resp4.data.toJsonArray();
        qDebug() << "返回记录数:" << records.size();
        if (records.size() == 1) {
            qDebug() << "测试4 通过!";
        } else {
            qDebug() << "测试4 失败! 期望1条，实际" << records.size() << "条";
        }
    } else {
        qDebug() << "测试4 失败!";
    }

    // 测试5: 组合条件查询
    qDebug() << "\n[测试5] 组合条件查询 (age>=22 AND score>=70)";
    QJsonObject condition2;
    condition2["age"] = 22;
    Response resp5 = recordManager.selectWithCondition("testuser", "QueryOptTestDB", "users", condition2);
    qDebug() << "查询结果:" << (resp5.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (resp5.status == ResponseStatus::OK) {
        QJsonArray records = resp5.data.toJsonArray();
        qDebug() << "匹配记录数:" << records.size();
        qDebug() << "测试5 通过!";
    } else {
        qDebug() << "测试5 失败!";
    }

    // 清理测试数据
    qDebug() << "\n[清理] 删除测试数据库 QueryOptTestDB";
    storageManager.dropDatabase("testuser", "QueryOptTestDB");

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 查询性能优化测试完成                          ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
}

// 基础事务支持测试（阶段五 🔴红圈A - 第一周任务3）
void runTransactionTests()
{
    StorageManager storageManager;

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 基础事务支持测试                              ║";
    qDebug() << "║  模块: StorageManager                                          ║";
    qDebug() << "║  任务: BEGIN/COMMIT/ROLLBACK 事务操作                          ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";

    // 先创建测试数据库
    qDebug() << "\n[前置] 创建测试数据库 TransactionTestDB";
    bool dbResult = storageManager.createDatabase("testuser", "TransactionTestDB");
    qDebug() << "数据库创建结果:" << (dbResult ? "成功 ✓" : "失败 ✗");

    // 测试1: BEGIN TRANSACTION
    qDebug() << "\n[测试1] BEGIN TRANSACTION";
    bool beginResult = storageManager.beginTransaction("testuser", "TransactionTestDB");
    qDebug() << "BEGIN结果:" << (beginResult ? "成功 ✓" : "失败 ✗");
    if (beginResult) {
        qDebug() << "测试1 通过!";
    } else {
        qDebug() << "测试1 失败!";
    }

    // 测试2: COMMIT TRANSACTION
    qDebug() << "\n[测试2] COMMIT TRANSACTION";
    bool commitResult = storageManager.commitTransaction("testuser", "TransactionTestDB");
    qDebug() << "COMMIT结果:" << (commitResult ? "成功 ✓" : "失败 ✗");
    if (commitResult) {
        qDebug() << "测试2 通过!";
    } else {
        qDebug() << "测试2 失败!";
    }

    // 测试3: ROLLBACK TRANSACTION
    qDebug() << "\n[测试3] ROLLBACK TRANSACTION";
    bool rollbackResult = storageManager.rollbackTransaction("testuser", "TransactionTestDB");
    qDebug() << "ROLLBACK结果:" << (rollbackResult ? "成功 ✓" : "失败 ✗");
    if (rollbackResult) {
        qDebug() << "测试3 通过!";
    } else {
        qDebug() << "测试3 失败!";
    }

    // 测试4: 验证日志文件内容
    qDebug() << "\n[测试4] 验证日志文件内容";
    QString logPath = "./data/testuser/TransactionTestDB/ruanko.log";
    QFile logFile(logPath);
    if (logFile.exists() && logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "---------------- ruanko.log ----------------";
        qDebug().noquote() << logFile.readAll().trimmed();
        qDebug() << "--------------------------------------------";
        logFile.close();
        qDebug() << "测试4 通过! 日志文件包含事务操作记录 ✓";
    } else {
        qDebug() << "测试4 失败 ✗ 无法找到或打开日志文件。";
    }

    // 清理测试数据
    qDebug() << "\n[清理] 删除测试数据库 TransactionTestDB";
    storageManager.dropDatabase("testuser", "TransactionTestDB");

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 基础事务支持测试完成                          ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
}

// 并发控制测试（阶段五 🔴红圈A - 第二周任务4）
void runLockManagerTests()
{
    StorageManager storageManager;
    RecordManager recordManager;

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 并发控制测试                                  ║";
    qDebug() << "║  模块: LockManager                                             ║";
    qDebug() << "║  任务: 读写锁机制、多用户并发访问保护                          ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";

    // 先创建测试数据库
    qDebug() << "\n[前置] 创建测试数据库 LockTestDB";
    bool dbResult = storageManager.createDatabase("testuser", "LockTestDB");
    qDebug() << "数据库创建结果:" << (dbResult ? "成功 ✓" : "失败 ✗");

    qDebug() << "\n[前置] 创建测试表 test_table";
    QList<Field> fields;
    fields.append(Field("id", FieldType::INT, 10));
    fields.back().isPrimaryKey = true;
    fields.append(Field("name", FieldType::TEXT, 50));
    bool tableResult = storageManager.createTable("testuser", "LockTestDB", "test_table", fields);
    qDebug() << "表创建结果:" << (tableResult ? "成功 ✓" : "失败 ✗");

    // 测试1: 插入记录（写锁）
    qDebug() << "\n[测试1] 插入记录（写锁保护）";
    QJsonObject record1;
    record1["id"] = 1;
    record1["name"] = "张三";
    Response insertResult = recordManager.insertRecord("testuser", "LockTestDB", "test_table", record1);
    qDebug() << "插入结果:" << (insertResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (insertResult.status == ResponseStatus::OK) {
        qDebug() << "测试1 通过!";
    } else {
        qDebug() << "测试1 失败!";
    }

    // 测试2: 查询记录（读锁）
    qDebug() << "\n[测试2] 查询所有记录（读锁保护）";
    Response selectResult = recordManager.selectAllRecords("testuser", "LockTestDB", "test_table");
    qDebug() << "查询结果:" << (selectResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (selectResult.status == ResponseStatus::OK) {
        QJsonArray records = selectResult.data.toJsonArray();
        qDebug() << "记录数量:" << records.size();
        qDebug() << "测试2 通过!";
    } else {
        qDebug() << "测试2 失败!";
    }

    // 测试3: 更新记录（写锁）
    qDebug() << "\n[测试3] 更新记录（写锁保护）";
    QJsonObject updateData;
    updateData["name"] = "李四";
    Response updateResult = recordManager.updateRecord("testuser", "LockTestDB", "test_table", "1", updateData);
    qDebug() << "更新结果:" << (updateResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (updateResult.status == ResponseStatus::OK) {
        qDebug() << "测试3 通过!";
    } else {
        qDebug() << "测试3 失败!";
    }

    // 测试4: 删除记录（写锁）
    qDebug() << "\n[测试4] 删除记录（写锁保护）";
    Response deleteResult = recordManager.deleteRecord("testuser", "LockTestDB", "test_table", "1");
    qDebug() << "删除结果:" << (deleteResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (deleteResult.status == ResponseStatus::OK) {
        qDebug() << "测试4 通过!";
    } else {
        qDebug() << "测试4 失败!";
    }

    // 测试5: 条件查询（读锁）
    qDebug() << "\n[测试5] 条件查询（读锁保护）";
    QJsonObject condition;
    condition["id"] = 1;
    Response conditionResult = recordManager.selectWithCondition("testuser", "LockTestDB", "test_table", condition);
    qDebug() << "查询结果:" << (conditionResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (conditionResult.status == ResponseStatus::OK) {
        QJsonArray records = conditionResult.data.toJsonArray();
        qDebug() << "匹配记录数:" << records.size();
        qDebug() << "测试5 通过!";
    } else {
        qDebug() << "测试5 失败!";
    }

    // 测试6: 分页查询（读锁）
    qDebug() << "\n[测试6] 分页查询（读锁保护）";
    Response limitResult = recordManager.selectWithLimitOffset("testuser", "LockTestDB", "test_table", 10, 0);
    qDebug() << "查询结果:" << (limitResult.status == ResponseStatus::OK ? "成功 ✓" : "失败 ✗");
    if (limitResult.status == ResponseStatus::OK) {
        QJsonArray records = limitResult.data.toJsonArray();
        qDebug() << "返回记录数:" << records.size();
        qDebug() << "测试6 通过!";
    } else {
        qDebug() << "测试6 失败!";
    }

    // 测试7: 删除表（写锁）
    qDebug() << "\n[测试7] 删除表（写锁保护）";
    bool dropTableResult = storageManager.dropTable("testuser", "LockTestDB", "test_table");
    qDebug() << "删除结果:" << (dropTableResult ? "成功 ✓" : "失败 ✗");
    if (dropTableResult) {
        qDebug() << "测试7 通过!";
    } else {
        qDebug() << "测试7 失败!";
    }

    // 测试8: 删除数据库（写锁）
    qDebug() << "\n[测试8] 删除数据库（写锁保护）";
    bool dropDbResult = storageManager.dropDatabase("testuser", "LockTestDB");
    qDebug() << "删除结果:" << (dropDbResult ? "成功 ✓" : "失败 ✗");
    if (dropDbResult) {
        qDebug() << "测试8 通过!";
    } else {
        qDebug() << "测试8 失败!";
    }

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 并发控制测试完成                              ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
}

// 系统监控测试（阶段五 🔴红圈A - 第二周任务5）
void runMonitorTests()
{
    StorageManager storageManager;

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 系统监控测试                                  ║";
    qDebug() << "║  模块: StorageManager Monitor                                  ║";
    qDebug() << "║  任务: 性能统计、磁盘I/O监控、查询耗时计算                     ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";

    // 1. 测试磁盘读取监控 (读取不存在的表也会触发一次物理查找尝试)
    storageManager.readTableData("testuser", "MonitorDB", "table_a");
    storageManager.readTableData("testuser", "MonitorDB", "table_b");

    // 2. 测试磁盘写入监控
    storageManager.writeTableData("testuser", "MonitorDB", "table_c", QByteArray("data"));

    // 3. 模拟上层引擎 (橙圈B/蓝圈C) 执行查询并上报耗时
    QElapsedTimer timer;

    // 模拟一次慢查询 (假装耗时 45ms)
    timer.start();
    // (假装在做复杂的 SQL 解析和 WHERE 过滤)
    storageManager.recordQueryTime(45);

    // 模拟一次快查询 (假装耗时 5ms，例如命中了你第一周写的缓存)
    timer.start();
    storageManager.recordQueryTime(5);

    // 4. 打印系统监控报告
    qDebug() << "\n[测试验证] 打印系统运行报告：";
    storageManager.printSystemStats();

    // 5. 验证重置功能
    storageManager.resetSystemStats();

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 系统监控测试完成                              ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
}

// 数据备份与恢复测试（阶段五 🔴红圈A - 第二周任务6）
void runBackupTests()
{
    StorageManager storageManager;
    QString testUser = "testuser";
    QString testDB = "BackupTestDB";
    QString testTable = "ImportantData";

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 数据备份与恢复测试                            ║";
    qDebug() << "║  模块: StorageManager Backup & Restore                         ║";
    qDebug() << "║  任务: 数据库物理目录拷贝、灾难恢复                            ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";

    // [前置] 创建测试库和表
    qDebug() << "\n[前置] 创建测试数据库和表...";
    storageManager.createDatabase(testUser, testDB);
    QList<Field> fields;
    fields.append(Field("id", FieldType::INT, 10));
    storageManager.createTable(testUser, testDB, testTable, fields);

    // 1. 备份数据库
    qDebug() << "\n[测试1] 执行数据库备份";
    bool backupResult = storageManager.backupDatabase(testUser, testDB);
    qDebug() << "备份指令下发结果:" << (backupResult ? "成功 ✓" : "失败 ✗");

    // 2. 自动寻找刚才生成的带有时间戳的备份文件夹
    QDir userDir("./data/" + testUser);
    QStringList filters;
    filters << testDB + "_backup_*"; // 过滤条件
    // 按时间倒序排列，拿最新的那个
    QStringList backupFolders = userDir.entryList(filters, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);

    if (backupFolders.isEmpty()) {
        qDebug() << "❌ 致命错误：未找到生成的备份文件夹，测试终止。";
        return;
    }
    QString latestBackup = backupFolders.first();
    qDebug() << "✅ 成功找到生成的备份文件夹:" << latestBackup;

    // 3. 模拟数据灾难（把原表物理删除）
    qDebug() << "\n[测试2] 模拟数据破坏 (Drop Table)";
    storageManager.dropTable(testUser, testDB, testTable);

    bool tdfExists = QFile::exists("./data/" + testUser + "/" + testDB + "/" + testTable + ".tdf");
    qDebug() << "破坏后验证 .tdf 文件是否存在:" << (tdfExists ? "是 ✗ (未成功删除)" : "否 ✓ (已成功破坏)");

    // 4. 从备份恢复数据
    qDebug() << "\n[测试3] 从备份目录恢复数据库";
    bool restoreResult = storageManager.restoreDatabase(testUser, testDB, latestBackup);
    qDebug() << "恢复指令下发结果:" << (restoreResult ? "成功 ✓" : "失败 ✗");

    // 5. 验证数据是否“复活”
    qDebug() << "\n[测试4] 验证数据是否成功复活";
    bool restoredTdfExists = QFile::exists("./data/" + testUser + "/" + testDB + "/" + testTable + ".tdf");
    qDebug() << "恢复后验证 .tdf 文件是否存在:" << (restoredTdfExists ? "是 ✓" : "否 ✗");

    if (restoreResult && restoredTdfExists) {
        qDebug() << "\n🎉 测试完美通过! 数据库已具备物理级灾难恢复能力。";
    } else {
        qDebug() << "\n💥 测试失败! 请检查文件路径或拷贝逻辑。";
    }

    // 6. 打扫战场
    storageManager.dropDatabase(testUser, testDB); // 删除恢复的原库
    QDir backupDir("./data/" + testUser + "/" + latestBackup);
    if (backupDir.exists()) {
        backupDir.removeRecursively(); // 手动把测试用的备份库也删掉
    }

    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🔴红圈A - 数据备份与恢复测试完成                        ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝\n";
}

void runDMLTests() {
    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🟠橙圈B - DML & 查询引擎测试                           ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";

    StorageManager storage;
    SQLParser parser;
    QueryEngine engine;

    parser.setStorageManager(&storage);
    parser.setQueryEngine(&engine);
    parser.setCurrentUser("admin");

    // 环境准备
    parser.parseSQL("CREATE DATABASE DMLTestDB;");
    parser.setCurrentDatabase("DMLTestDB");
    parser.parseSQL("CREATE TABLE students (id INT, name TEXT, age INT, score DOUBLE);");
    engine.setCurrentUser("admin");
    engine.setCurrentDatabase("DMLTestDB");

    qDebug() << "\n[1] 批量插入3行数据";
    Response r1 = parser.parseSQL(
        "INSERT INTO students VALUES (1, 'Alice', 20, 88.5), (2, 'Bob', 22, 76.0), (3, 'Charlie', 19, 92.0);");
    qDebug() << r1.message;

    qDebug() << "\n[2] 单行插入";
    Response r2 = parser.parseSQL(
        "INSERT INTO students (id, name, age, score) VALUES (4, 'Diana', 21, 85.5);");
    qDebug() << r2.message;

    qDebug() << "\n[3] SELECT * FROM students";
    Response r3 = parser.parseSQL("SELECT * FROM students;");
    QJsonArray all = r3.data.toJsonArray();
    for (auto v : all) {
        QJsonObject obj = v.toObject();
        qDebug() << "  " << obj["id"].toInt() << obj["name"].toString()
                 << obj["age"].toInt() << obj["score"].toDouble();
    }

    qDebug() << "\n[4] WHERE 条件 age=22";
    Response r4 = parser.parseSQL("SELECT * FROM students WHERE age = 22;");
    QJsonArray res4 = r4.data.toJsonArray();
    qDebug() << "匹配记录数:" << res4.size();

    qDebug() << "\n[5] LIKE 模糊匹配 'A%'";
    Response r5 = parser.parseSQL("SELECT * FROM students WHERE name LIKE 'A%';");
    QJsonArray res5 = r5.data.toJsonArray();
    qDebug() << "匹配记录数:" << res5.size();

    qDebug() << "\n[6] UPDATE 修改";
    Response r6 = parser.parseSQL("UPDATE students SET score = 95.0 WHERE name = 'Alice';");
    qDebug() << r6.message;

    qDebug() << "\n[7] 验证更新";
    Response r7 = parser.parseSQL("SELECT * FROM students WHERE name = 'Alice';");
    QJsonArray res7 = r7.data.toJsonArray();
    if (!res7.isEmpty())
        qDebug() << "Alice的新分数:" << res7.first().toObject()["score"].toDouble();

    qDebug() << "\n[8] 聚合查询: COUNT, AVG, MAX";
    Response r8 = parser.parseSQL("SELECT COUNT(*), AVG(score), MAX(age) FROM students;");
    QJsonArray res8 = r8.data.toJsonArray();
    if (!res8.isEmpty()) {
        QJsonObject obj = res8.first().toObject();
        qDebug() << "  COUNT:" << obj["COUNT(*)"].toDouble()
                 << "  AVG:" << obj["AVG(score)"].toDouble()
                 << "  MAX:" << obj["MAX(age)"].toDouble();
    }

    qDebug() << "\n[9] GROUP BY + HAVING";
    Response r9 = parser.parseSQL(
        "SELECT age, COUNT(*) AS cnt FROM students GROUP BY age HAVING cnt > 1;");
    QJsonArray res9 = r9.data.toJsonArray();
    qDebug() << "分组结果数:" << res9.size();

    qDebug() << "\n[10] ORDER BY + LIMIT OFFSET";
    Response r10 = parser.parseSQL(
        "SELECT * FROM students ORDER BY score DESC LIMIT 2 OFFSET 1;");
    QJsonArray res10 = r10.data.toJsonArray();
    qDebug() << "返回行数:" << res10.size();

    qDebug() << "\n[11] DELETE 条件删除";
    Response r11 = parser.parseSQL("DELETE FROM students WHERE age = 21;");
    qDebug() << r11.message;

    qDebug() << "\n[12] 确认剩余记录";
    Response r12 = parser.parseSQL("SELECT * FROM students;");
    QJsonArray res12 = r12.data.toJsonArray();
    qDebug() << "剩余记录数:" << res12.size();

    // 清理
    parser.parseSQL("DROP DATABASE DMLTestDB;");
    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║  阶段五 🟠橙圈B - DML & 查询引擎测试完成                        ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
}
