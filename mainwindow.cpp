#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "queryengine.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QRegularExpression>
#include <QColor>
#include <QFileDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QStringConverter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_auth(new AuthManager(this))
    , m_schema(new SchemaManager(this))
    , m_record(new RecordManager())
    , m_storage(new StorageManager())
    , m_parser(new SQLParser(this))
    , m_queryEngine(new QueryEngine(this))
    , m_migrator(new DataMigrator())
{
    ui->setupUi(this);

    // 认证按钮
    connect(ui->btnLogin,    &QPushButton::clicked, this, &MainWindow::onLogin);
    connect(ui->btnRegister, &QPushButton::clicked, this, &MainWindow::onRegister);

    // 数据库按钮
    connect(ui->btnCreateDb, &QPushButton::clicked, this, &MainWindow::onCreateDatabase);
    connect(ui->btnDropDb, &QPushButton::clicked, this, &MainWindow::onDropDatabase);

    // 表管理按钮
    connect(ui->btnCreateTable, &QPushButton::clicked, this, &MainWindow::onCreateTable);
    connect(ui->btnDropTable,   &QPushButton::clicked, this, &MainWindow::onDropTable);
    connect(ui->btnAlterTable,  &QPushButton::clicked, this, &MainWindow::onAlterTable);

    // 字段操作按钮
    connect(ui->btnAddField,  &QPushButton::clicked, this, &MainWindow::onAddField);
    connect(ui->btnDropField, &QPushButton::clicked, this, &MainWindow::onDropField);
    connect(ui->btnAlterField, &QPushButton::clicked, this, &MainWindow::onAlterField);

    // 数据操作按钮
    connect(ui->btnRefreshData,   &QPushButton::clicked, this, &MainWindow::onRefreshData);
    connect(ui->btnInsertRecord,  &QPushButton::clicked, this, &MainWindow::onInsertRecord);
    connect(ui->btnRefreshSchema, &QPushButton::clicked, this, &MainWindow::onRefreshSchema);

    // 功能2新增: 高级搜索、导入
    connect(ui->btnAdvancedSearch, &QPushButton::clicked, this, &MainWindow::onAdvancedSearch);
    connect(ui->btnImportCSV, &QPushButton::clicked, this, &MainWindow::onImportCSV);
    connect(ui->btnImportJSON, &QPushButton::clicked, this, &MainWindow::onImportJSON);
    connect(ui->tableData->horizontalHeader(), &QHeaderView::sectionClicked, this, &MainWindow::onTableHeaderClicked);

    // 蓝圈功能: 导出CSV按钮 (动态添加到dataToolbarLayout)
    QPushButton *btnExportCSV = new QPushButton("📤 导出CSV", this);
    btnExportCSV->setObjectName("btnExportCSV");
    ui->dataToolbarLayout->addWidget(btnExportCSV);
    connect(btnExportCSV, &QPushButton::clicked, this, &MainWindow::onExportCSV);

    // 蓝圈功能: 备份/恢复数据库按钮 (动态添加到sidebarLayout)
    QPushButton *btnBackupDb = new QPushButton("💾 备份数据库", this);
    btnBackupDb->setObjectName("btnBackupDb");
    ui->sidebarLayout->addWidget(btnBackupDb);
    connect(btnBackupDb, &QPushButton::clicked, this, &MainWindow::onBackupDatabase);

    QPushButton *btnRestoreDb = new QPushButton("📥 恢复数据库", this);
    btnRestoreDb->setObjectName("btnRestoreDb");
    ui->sidebarLayout->addWidget(btnRestoreDb);
    connect(btnRestoreDb, &QPushButton::clicked, this, &MainWindow::onRestoreDatabase);

    // 功能4新增: 统计图表
    connect(ui->btnShowChart, &QPushButton::clicked, this, &MainWindow::onShowChart);

    // SQL执行按钮
    connect(ui->btnExecuteSQL, &QPushButton::clicked, this, &MainWindow::onExecuteSQL);

    // 分页按钮
    connect(ui->btnPrevPage, &QPushButton::clicked, this, &MainWindow::onPrevPage);
    connect(ui->btnNextPage, &QPushButton::clicked, this, &MainWindow::onNextPage);

    // SQL语法高亮器
    m_sqlHighlighter = new SqlHighlighter(ui->sqlInput->document());

    // 树节点
    connect(ui->dbTree, &QTreeWidget::itemClicked, this, &MainWindow::onTreeItemClicked);
    connect(ui->dbTree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::onTreeItemContextMenu);

    // 搜索
    // connect(ui->btnSearch, &QPushButton::clicked, this, &MainWindow::onSearch);

    // 菜单动作
    connect(ui->actionCreateDb,    &QAction::triggered, this, &MainWindow::onCreateDatabase);
    connect(ui->actionCreateTable, &QAction::triggered, this, &MainWindow::onCreateTable);
    connect(ui->actionDropTable,   &QAction::triggered, this, &MainWindow::onDropTable);
    connect(ui->actionAlterTable,  &QAction::triggered, this, &MainWindow::onAlterTable);
    connect(ui->actionAddField,    &QAction::triggered, this, &MainWindow::onAddField);
    connect(ui->actionDropField,   &QAction::triggered, this, &MainWindow::onDropField);
    connect(ui->actionAlterField,  &QAction::triggered, this, &MainWindow::onAlterField);
    connect(ui->actionExit,        &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout,       &QAction::triggered, this, &MainWindow::onAbout);

    // 初始化表头
    ui->tableSchema->setColumnCount(7);
    ui->tableSchema->setHorizontalHeaderLabels({"字段名", "类型", "长度", "非空", "主键", "约束", "索引"});

    // 将StorageManager和QueryEngine注入SQLParser
    m_parser->setStorageManager(m_storage);
    m_parser->setQueryEngine(m_queryEngine);

    log("系统就绪，请先登录。默认账号: admin / 123456");
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_record;
    delete m_storage;
}

void MainWindow::log(const QString &msg)
{
    ui->logOutput->append(msg);
}

void MainWindow::requireLogin()
{
    QMessageBox::warning(this, "提示", "请先登录");
}

void MainWindow::refreshTree()
{
    ui->dbTree->clear();
    if (m_currentUser.isEmpty()) return;

    QString userPath = Config::DATA_PATH + m_currentUser;
    QDir userDir(userPath);
    if (!userDir.exists()) return;

    QStringList dbNames = userDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &dbName : dbNames) {
        QTreeWidgetItem *dbItem = new QTreeWidgetItem(ui->dbTree);
        dbItem->setText(0, dbName);
        dbItem->setData(0, Qt::UserRole, "db");

        QString dbPath = userPath + "/" + dbName;
        QDir dbDir(dbPath);
        QStringList tdfFiles = dbDir.entryList({"*.tdf"}, QDir::Files);
        for (const QString &tdfFile : tdfFiles) {
            QString tableName = tdfFile.left(tdfFile.size() - 4);
            QTreeWidgetItem *tableItem = new QTreeWidgetItem(dbItem);
            tableItem->setText(0, tableName);
            tableItem->setData(0, Qt::UserRole, "table");
        }
    }
}

void MainWindow::onLogin()
{
    QString username = ui->inputUsername->text().trimmed();
    QString password = ui->inputPassword->text();

    if (username.isEmpty() || password.isEmpty()) {
        log("登录失败：用户名或密码不能为空");
        return;
    }

    if (m_auth->login(username, password)) {
        m_currentUser = username;
        m_loggedIn = true;
        m_parser->setCurrentUser(username);
        // 同步当前用户到查询引擎
        m_queryEngine->setCurrentUser(username);
        ui->labelWelcome->setText("欢迎, " + username);
        ui->labelWelcome->setStyleSheet("color: green;");
        log("登录成功: " + username);
        refreshTree();
    } else {
        log("登录失败：用户名或密码错误");
    }
}

void MainWindow::onRegister()
{
    QString username = ui->inputUsername->text().trimmed();
    QString password = ui->inputPassword->text();

    if (username.isEmpty() || password.isEmpty()) {
        log("注册失败：用户名或密码不能为空");
        return;
    }

    if (m_auth->registerUser(username, password)) {
        log("注册成功: " + username);
    } else {
        log("注册失败：用户名已存在");
    }
}

void MainWindow::onCreateDatabase()
{
    if (!m_loggedIn) return requireLogin();

    QString dbName = ui->inputDbName->text().trimmed();
    if (dbName.isEmpty()) {
        log("请输入数据库名");
        return;
    }

    if (m_storage->createDatabase(m_currentUser, dbName)) {
        log("数据库创建成功: " + dbName);
        refreshTree();
    } else {
        log("数据库创建失败（可能已存在）");
    }
}

void MainWindow::onCreateTable()
{
    if (!m_loggedIn) return requireLogin();
    if (m_currentDb.isEmpty()) {
        log("请先在左侧选择一个数据库");
        return;
    }

    bool ok;
    QString tableName = QInputDialog::getText(this, "新建表", "表名:", QLineEdit::Normal, "", &ok);
    if (!ok || tableName.isEmpty()) return;

    // 获取字段定义
    QString fieldsStr = QInputDialog::getText(this, "字段定义",
                                              "字段定义 (格式: 字段名 类型, ...)\n类型: INT, TEXT, DOUBLE, BOOLEAN",
                                              QLineEdit::Normal, "id INT, name TEXT", &ok);
    if (!ok) return;

    QList<Field> fields;
    QStringList parts = fieldsStr.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        QStringList pair = part.trimmed().split(' ', Qt::SkipEmptyParts);
        if (pair.size() >= 2) {
            Field f;
            f.name = pair[0];
            f.type = pair[1].toUpper() == "INT" ? FieldType::INT :
                         pair[1].toUpper() == "DOUBLE" ? FieldType::DOUBLE :
                         pair[1].toUpper() == "BOOLEAN" ? FieldType::BOOLEAN : FieldType::TEXT;
            f.length = (f.type == FieldType::INT) ? 10 : 255;
            fields.append(f);
        }
    }

    if (fields.isEmpty()) {
        log("字段定义无效");
        return;
    }

    if (m_storage->createTable(m_currentUser, m_currentDb, tableName, fields)) {
        log("表创建成功: " + tableName);
        refreshTree();
    } else {
        log("表创建失败（可能已存在）");
    }
}

void MainWindow::onDropTable()
{
    if (!m_loggedIn) return requireLogin();
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("请先选择要删除的表");
        return;
    }

    if (QMessageBox::question(this, "确认", "确定删除表 " + m_currentTable + "？") != QMessageBox::Yes)
        return;

    if (m_storage->dropTable(m_currentUser, m_currentDb, m_currentTable)) {
        log("表已删除: " + m_currentTable);
        m_currentTable.clear();
        refreshTree();
    } else {
        log("表删除失败");
    }
}

void MainWindow::onAlterTable()
{
    if (!m_loggedIn) return requireLogin();
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("请先选择要修改的表");
        return;
    }

    // 加载当前表结构
    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
    if (fields.isEmpty()) {
        log("无法加载表结构");
        return;
    }

    // 显示当前字段
    QString currentFields;
    for (const Field &f : fields) {
        currentFields += f.name + " " +
                         (f.type == FieldType::INT ? "INT" :
                              f.type == FieldType::DOUBLE ? "DOUBLE" :
                              f.type == FieldType::BOOLEAN ? "BOOLEAN" : "TEXT") + ", ";
    }
    currentFields.chop(2);

    bool ok;
    QString newFieldsStr = QInputDialog::getText(this, "修改表结构",
                                                 "当前字段:\n" + currentFields + "\n\n输入新字段定义:",
                                                 QLineEdit::Normal, currentFields, &ok);
    if (!ok) return;

    // 解析新字段
    QList<Field> newFields;
    QStringList parts = newFieldsStr.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        QStringList pair = part.trimmed().split(' ', Qt::SkipEmptyParts);
        if (pair.size() >= 2) {
            Field f;
            f.name = pair[0];
            f.type = pair[1].toUpper() == "INT" ? FieldType::INT :
                         pair[1].toUpper() == "DOUBLE" ? FieldType::DOUBLE :
                         pair[1].toUpper() == "BOOLEAN" ? FieldType::BOOLEAN : FieldType::TEXT;
            f.length = (f.type == FieldType::INT) ? 10 : 255;
            newFields.append(f);
        }
    }

    if (newFields.isEmpty()) {
        log("新字段定义无效");
        return;
    }

    if (m_storage->alterTable(m_currentUser, m_currentDb, m_currentTable, newFields)) {
        log("表结构修改成功: " + m_currentTable);
        onRefreshSchema();
    } else {
        log("表结构修改失败");
    }
}

void MainWindow::onAddField()
{
    if (!m_loggedIn) return requireLogin();
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("请先选择表");
        return;
    }

    // 加载当前表结构
    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
    if (fields.isEmpty()) {
        log("无法加载表结构");
        return;
    }

    // 显示当前字段
    QString currentFields;
    for (const Field &f : fields) {
        currentFields += f.name + " " +
                         (f.type == FieldType::INT ? "INT" :
                              f.type == FieldType::DOUBLE ? "DOUBLE" :
                              f.type == FieldType::BOOLEAN ? "BOOLEAN" : "TEXT") + ", ";
    }
    currentFields.chop(2);

    // 输入新字段
    bool ok;
    QString newFieldStr = QInputDialog::getText(this, "添加字段",
                                                "当前字段:\n" + currentFields + "\n\n输入新字段定义 (格式: 字段名 类型):",
                                                QLineEdit::Normal, "", &ok);
    if (!ok) return;

    QStringList pair = newFieldStr.trimmed().split(' ', Qt::SkipEmptyParts);
    if (pair.size() < 2) {
        log("字段定义格式错误");
        return;
    }

    Field newField;
    newField.name = pair[0];
    newField.type = pair[1].toUpper() == "INT" ? FieldType::INT :
                        pair[1].toUpper() == "DOUBLE" ? FieldType::DOUBLE :
                        pair[1].toUpper() == "BOOLEAN" ? FieldType::BOOLEAN : FieldType::TEXT;
    newField.length = (newField.type == FieldType::INT) ? 10 : 255;

    fields.append(newField);

    if (m_storage->alterTable(m_currentUser, m_currentDb, m_currentTable, fields)) {
        log("字段添加成功: " + newField.name);
        onRefreshSchema();
    } else {
        log("字段添加失败");
    }
}

void MainWindow::onDropField()
{
    if (!m_loggedIn) return requireLogin();
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("请先选择表");
        return;
    }

    // 加载当前表结构
    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
    if (fields.isEmpty()) {
        log("无法加载表结构");
        return;
    }

    // 显示当前字段并选择要删除的字段
    QStringList fieldNames;
    for (const Field &f : fields) {
        fieldNames.append(f.name);
    }

    bool ok;
    QString fieldName = QInputDialog::getItem(this, "删除字段", "选择要删除的字段:", fieldNames, 0, false, &ok);
    if (!ok || fieldName.isEmpty()) return;

    // 检查是否是主键字段
    for (const Field &f : fields) {
        if (f.name == fieldName && f.isPrimaryKey) {
            QMessageBox::warning(this, "错误", "不能删除主键字段");
            return;
        }
    }

    // 过滤掉要删除的字段
    QList<Field> newFields;
    for (const Field &f : fields) {
        if (f.name != fieldName) {
            newFields.append(f);
        }
    }

    if (m_storage->alterTable(m_currentUser, m_currentDb, m_currentTable, newFields)) {
        log("字段删除成功: " + fieldName);
        onRefreshSchema();
    } else {
        log("字段删除失败");
    }
}

void MainWindow::onAlterField()
{
    if (!m_loggedIn) return requireLogin();
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("请先选择表");
        return;
    }

    // 加载当前表结构
    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
    if (fields.isEmpty()) {
        log("无法加载表结构");
        return;
    }

    // 显示当前字段并选择要修改的字段
    QStringList fieldNames;
    for (const Field &f : fields) {
        fieldNames.append(f.name);
    }

    bool ok;
    QString fieldName = QInputDialog::getItem(this, "修改字段", "选择要修改的字段:", fieldNames, 0, false, &ok);
    if (!ok || fieldName.isEmpty()) return;

    // 找到要修改的字段
    Field *targetField = nullptr;
    for (Field &f : fields) {
        if (f.name == fieldName) {
            targetField = &f;
            break;
        }
    }

    if (!targetField) {
        log("字段不存在");
        return;
    }

    // 显示当前字段信息
    QString currentInfo = QString("字段名: %1\n类型: %2\n长度: %3\n非空: %4\n主键: %5")
                              .arg(targetField->name)
                              .arg(targetField->type == FieldType::INT ? "INT" :
                                       targetField->type == FieldType::DOUBLE ? "DOUBLE" :
                                       targetField->type == FieldType::BOOLEAN ? "BOOLEAN" : "TEXT")
                              .arg(targetField->length)
                              .arg(targetField->isNotNull ? "是" : "否")
                              .arg(targetField->isPrimaryKey ? "是" : "否");

    // 输入新字段信息
    QString newFieldStr = QInputDialog::getText(this, "修改字段",
                                                currentInfo + "\n\n输入新字段定义 (格式: 字段名 类型):",
                                                QLineEdit::Normal, fieldName + " " +
                                                    (targetField->type == FieldType::INT ? "INT" :
                                                         targetField->type == FieldType::DOUBLE ? "DOUBLE" :
                                                         targetField->type == FieldType::BOOLEAN ? "BOOLEAN" : "TEXT"), &ok);
    if (!ok) return;

    QStringList pair = newFieldStr.trimmed().split(' ', Qt::SkipEmptyParts);
    if (pair.size() < 2) {
        log("字段定义格式错误");
        return;
    }

    // 更新字段
    targetField->name = pair[0];
    targetField->type = pair[1].toUpper() == "INT" ? FieldType::INT :
                            pair[1].toUpper() == "DOUBLE" ? FieldType::DOUBLE :
                            pair[1].toUpper() == "BOOLEAN" ? FieldType::BOOLEAN : FieldType::TEXT;
    targetField->length = (targetField->type == FieldType::INT) ? 10 : 255;

    if (m_storage->alterTable(m_currentUser, m_currentDb, m_currentTable, fields)) {
        log("字段修改成功: " + fieldName + " -> " + pair[0]);
        onRefreshSchema();
    } else {
        log("字段修改失败");
    }
}

void MainWindow::onRefreshData()
{
    if (!m_loggedIn || m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("请先选择表");
        return;
    }
    showDataTable(m_currentUser, m_currentDb, m_currentTable);
}

void MainWindow::onInsertRecord()
{
    if (!m_loggedIn) return requireLogin();
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("请先选择表");
        return;
    }

    // 加载表结构
    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
    if (fields.isEmpty()) {
        log("无法加载表结构");
        return;
    }

    // 创建输入对话框
    QDialog dialog(this);
    dialog.setWindowTitle("插入记录");
    QFormLayout *layout = new QFormLayout(&dialog);

    QMap<QString, QLineEdit*> inputs;
    for (const Field &f : fields) {
        QLineEdit *edit = new QLineEdit(&dialog);
        edit->setPlaceholderText(f.type == FieldType::INT ? "整数" :
                                     f.type == FieldType::DOUBLE ? "小数" :
                                     f.type == FieldType::BOOLEAN ? "true/false" : "文本");
        layout->addRow(f.name + ":", edit);
        inputs[f.name] = edit;
    }

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    // 构建记录
    QJsonObject record;
    for (const Field &f : fields) {
        QString val = inputs[f.name]->text();
        if (f.type == FieldType::INT) record[f.name] = val.toInt();
        else if (f.type == FieldType::DOUBLE) record[f.name] = val.toDouble();
        else if (f.type == FieldType::BOOLEAN) record[f.name] = val.toLower() == "true";
        else record[f.name] = val;
    }

    Response res = m_record->insertRecord(m_currentUser, m_currentDb, m_currentTable, record);
    if (res.status != ResponseStatus::OK) {
        log("插入失败: " + res.message);
    } else {
        log("记录插入成功");
        onRefreshData();
    }
}

void MainWindow::onRefreshSchema()
{
    if (!m_loggedIn || m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("请先选择表");
        return;
    }
    showSchemaTable(m_currentUser, m_currentDb, m_currentTable);
}

void MainWindow::onTreeItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    QString type = item->data(0, Qt::UserRole).toString();

    if (type == "db") {
        m_currentDb = item->text(0);
        m_currentTable.clear();
        log("当前数据库: " + m_currentDb);
        m_parser->setCurrentDatabase(m_currentDb);
        // 同步当前数据库到查询引擎
        m_queryEngine->setCurrentDatabase(m_currentDb);
    } else if (type == "table") {
        m_currentDb = item->parent()->text(0);
        m_currentTable = item->text(0);
        m_parser->setCurrentDatabase(m_currentDb);
        // 同步当前数据库到查询引擎
        m_queryEngine->setCurrentDatabase(m_currentDb);
        log("当前表: " + m_currentDb + "." + m_currentTable);
        showSchemaTable(m_currentUser, m_currentDb, m_currentTable);
        showDataTable(m_currentUser, m_currentDb, m_currentTable);
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "关于", "DBMS 数据库管理系统\n版本 1.0");
}

void MainWindow::onExecuteSQL()
{
    if (!m_loggedIn) return requireLogin();

    QString sql = ui->sqlInput->toPlainText().trimmed();
    if (sql.isEmpty()) {
        log("请输入SQL语句");
        return;
    }

    ui->dataTableView->setCurrentIndex(2);

    m_queryTimer.start();
    Response res = m_parser->parseSQL(sql);
    qint64 elapsed = m_queryTimer.nsecsElapsed();

    ui->labelQueryTime->setText("查询用时: " + formatElapsedTime(elapsed));

    log(res.message);

    if (res.status == ResponseStatus::OK) {
        refreshTree();

        QJsonArray records;
        {
            QString jsonStr = res.data.toString();
            if (!jsonStr.isEmpty()) {
                QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                records = doc.array();
            }
        }
        qDebug() << QString("[UI] onExecuteSQL: got %1 records, empty=%2").arg(records.size()).arg(records.isEmpty());
        if (!records.isEmpty()) {
            m_totalRows = records.size();

            int offset = m_currentPage * m_pageSize;
            int limit = m_pageSize;

            ui->tableSQLResult->clear();
            ui->tableSQLResult->setRowCount(0);

            QStringList cols;
            for (const QJsonValue &v : records) {
                for (const QString &k : v.toObject().keys()) {
                    if (!cols.contains(k)) cols << k;
                }
            }

            ui->tableSQLResult->setColumnCount(cols.size());
            ui->tableSQLResult->setHorizontalHeaderLabels(cols);

            int displayCount = qMin(limit, m_totalRows - offset);
            ui->tableSQLResult->setRowCount(displayCount);

            for (int row = 0; row < displayCount; ++row) {
                QJsonObject obj = records[offset + row].toObject();
                for (int col = 0; col < cols.size(); ++col) {
                    QJsonValue val = obj.value(cols[col]);
                    QString text;
                    if (val.isBool())        text = val.toBool() ? "true" : "false";
                    else if (val.isDouble()) text = QString::number(val.toDouble());
                    else                     text = val.toString();
                    ui->tableSQLResult->setItem(row, col, new QTableWidgetItem(text));
                }
            }

            updatePaginationLabel();
        }
    }
}

void MainWindow::showDataTable(const QString &username, const QString &dbName, const QString &tableName)
{
    QList<Field> fields = m_storage->loadTableSchema(username, dbName, tableName);

    ui->tableData->setColumnCount(fields.size());
    QStringList headers;
    for (const Field &f : fields) headers << f.name;
    ui->tableData->setHorizontalHeaderLabels(headers);

    Response res = m_record->selectAllRecords(username, dbName, tableName);
    if (res.status != ResponseStatus::OK) {
        ui->tableData->setRowCount(0);
        return;
    }

    QJsonArray records = res.data.value<QJsonArray>();
    ui->tableData->setRowCount(records.size());
    for (int i = 0; i < records.size(); ++i) {
        QJsonObject rec = records[i].toObject();
        for (int j = 0; j < fields.size(); ++j) {
            ui->tableData->setItem(i, j, new QTableWidgetItem(rec[fields[j].name].toVariant().toString()));
        }
    }
}

void MainWindow::showSchemaTable(const QString &username, const QString &dbName, const QString &tableName)
{
    QList<Field> fields = m_storage->loadTableSchema(username, dbName, tableName);

    ui->tableSchema->setColumnCount(7);
    ui->tableSchema->setHorizontalHeaderLabels({"字段名", "类型", "长度", "非空", "主键", "约束", "索引"});
    ui->tableSchema->setRowCount(fields.size());

    QString tidPath = Config::DATA_PATH + username + "/" + dbName + "/" + tableName + ".tid";
    QFile tidFile(tidPath);
    QStringList indexedFields;
    if (tidFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&tidFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                indexedFields.append(line);
            }
        }
        tidFile.close();
    }

    for (int i = 0; i < fields.size(); ++i) {
        const Field &f = fields[i];
        ui->tableSchema->setItem(i, 0, new QTableWidgetItem(f.name));
        ui->tableSchema->setItem(i, 1, new QTableWidgetItem(
                                           f.type == FieldType::INT ? "INT" :
                                               f.type == FieldType::DOUBLE ? "DOUBLE" :
                                               f.type == FieldType::BOOLEAN ? "BOOLEAN" : "TEXT"));
        ui->tableSchema->setItem(i, 2, new QTableWidgetItem(QString::number(f.length)));
        ui->tableSchema->setItem(i, 3, new QTableWidgetItem(f.isNotNull ? "是" : "否"));
        ui->tableSchema->setItem(i, 4, new QTableWidgetItem(f.isPrimaryKey ? "是" : "否"));

        QStringList constraints;
        if (f.isUnique) constraints << "UNIQUE";
        if (f.hasCheck) constraints << QString("CHECK(%1)").arg(f.checkExpr);
        if (!f.defaultValue.isEmpty()) constraints << QString("DEFAULT(%1)").arg(f.defaultValue);
        ui->tableSchema->setItem(i, 5, new QTableWidgetItem(constraints.isEmpty() ? "-" : constraints.join(", ")));

        bool hasIndex = indexedFields.contains(f.name);
        ui->tableSchema->setItem(i, 6, new QTableWidgetItem(hasIndex ? "✓" : "-"));
        if (hasIndex) {
            ui->tableSchema->item(i, 6)->setForeground(QColor("#a6e3a1"));
        }
    }

    ui->dataTableView->setCurrentIndex(1);
}


void MainWindow::onDropDatabase()
{
    if (!m_loggedIn) { requireLogin(); return; }

    QString dbName = ui->inputDbName->text().trimmed();
    if (dbName.isEmpty()) {
        dbName = QInputDialog::getText(this, "删除数据库", "数据库名称:");
        if (dbName.trimmed().isEmpty()) return;
        dbName = dbName.trimmed();
    }

    int ret = QMessageBox::question(this, "确认删除",
                                    QString("确定要删除数据库 \"%1\" 吗？此操作将删除所有表和数据，不可恢复！").arg(dbName));
    if (ret != QMessageBox::Yes) return;

    if (m_storage->dropDatabase(m_currentUser, dbName)) {
        log(QString("[Storage] 数据库 \"%1\" 删除成功").arg(dbName));
        if (m_currentDb == dbName) {
            m_currentDb.clear();
            m_currentTable.clear();
            ui->tableData->clearContents();
            ui->tableData->setRowCount(0);
            ui->tableSchema->clearContents();
            ui->tableSchema->setRowCount(0);
        }
        refreshTree();
    } else {
        log(QString("[Storage] 数据库 \"%1\" 删除失败").arg(dbName));
    }
}

void MainWindow::onTreeItemContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = ui->dbTree->itemAt(pos);
    if (!item || !m_loggedIn) return;

    m_contextMenuTarget = item;
    QMenu menu(this);

    if (item->parent() == nullptr) {
        // 数据库节点
        QString dbName = item->text(0);
        m_currentDb = dbName;

        QAction *newTableAction = menu.addAction("新建表");
        connect(newTableAction, &QAction::triggered, this, &MainWindow::onContextMenuAction);
        newTableAction->setData(QVariant::fromValue(QPair<QString, QString>("create_table", dbName)));

        QAction *backupAction = menu.addAction("备份数据库");
        connect(backupAction, &QAction::triggered, this, &MainWindow::onContextMenuAction);
        backupAction->setData(QVariant::fromValue(QPair<QString, QString>("backup_database", dbName)));

        QAction *restoreAction = menu.addAction("恢复数据库");
        connect(restoreAction, &QAction::triggered, this, &MainWindow::onContextMenuAction);
        restoreAction->setData(QVariant::fromValue(QPair<QString, QString>("restore_database", dbName)));

        menu.addSeparator();

        QAction *dropDbAction = menu.addAction("删除数据库");
        connect(dropDbAction, &QAction::triggered, this, &MainWindow::onContextMenuAction);
        dropDbAction->setData(QVariant::fromValue(QPair<QString, QString>("drop_database", dbName)));
    } else {
        // 表节点
        QString dbName = item->parent()->text(0);
        QString tableName = item->text(0);
        m_currentDb = dbName;
        m_currentTable = tableName;

        QAction *exportAction = menu.addAction("导出CSV");
        connect(exportAction, &QAction::triggered, this, &MainWindow::onContextMenuAction);
        exportAction->setData(QVariant::fromValue(QPair<QString, QString>("export_csv", tableName)));

        QAction *dropTableAction = menu.addAction("删除表");
        connect(dropTableAction, &QAction::triggered, this, &MainWindow::onContextMenuAction);
        dropTableAction->setData(QVariant::fromValue(QPair<QString, QString>("drop_table", tableName)));
    }

    menu.exec(ui->dbTree->mapToGlobal(pos));
}

void MainWindow::onContextMenuAction()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action) return;

    QPair<QString, QString> data = action->data().value<QPair<QString, QString>>();
    QString actionType = data.first;
    QString name = data.second;

    if (actionType == "create_table") {
        onCreateTable();
    } else if (actionType == "drop_database") {
        int ret = QMessageBox::question(this, "确认删除",
                                        QString("确定要删除数据库 \"%1\" 吗？此操作将删除所有表和数据，不可恢复！").arg(name));
        if (ret == QMessageBox::Yes) {
            if (m_storage->dropDatabase(m_currentUser, name)) {
                log(QString("[Storage] 数据库 \"%1\" 删除成功").arg(name));
                if (m_currentDb == name) {
                    m_currentDb.clear();
                    m_currentTable.clear();
                    ui->tableData->clearContents();
                    ui->tableData->setRowCount(0);
                    ui->tableSchema->clearContents();
                    ui->tableSchema->setRowCount(0);
                }
                refreshTree();
            } else {
                log(QString("[Storage] 数据库 \"%1\" 删除失败").arg(name));
            }
        }
    } else if (actionType == "drop_table") {
        int ret = QMessageBox::question(this, "确认删除",
                                        QString("确定要删除表 \"%1\" 吗？此操作不可恢复！").arg(name));
        if (ret == QMessageBox::Yes) {
            Response r = m_schema->dropTable(m_currentUser, m_currentDb, name);
            log(r.message);
            if (r.status == ResponseStatus::OK) {
                if (m_currentTable == name) {
                    m_currentTable.clear();
                    ui->tableData->clearContents();
                    ui->tableData->setRowCount(0);
                    ui->tableSchema->clearContents();
                    ui->tableSchema->setRowCount(0);
                }
                refreshTree();
            } else {
                QMessageBox::warning(this, "删除失败", r.message);
            }
        }
    } else if (actionType == "export_csv") {
        onExportCSV();
    } else if (actionType == "backup_database") {
        onBackupDatabase();
    } else if (actionType == "restore_database") {
        onRestoreDatabase();
    }
}

void MainWindow::onSearch()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("[UI] 请先选中一张表");
        return;
    }

    QString searchText = ui->inputSearch->text().trimmed();
    if (searchText.isEmpty()) {
        showDataTable(m_currentUser, m_currentDb, m_currentTable);
        return;
    }

    // 获取表结构，找到第一个文本字段进行搜索
    Response sr = m_schema->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
    if (sr.status != ResponseStatus::OK) {
        log("[UI] 无法加载表结构: " + sr.message);
        return;
    }

    TableSchema schema = sr.data.value<TableSchema>();
    QString searchField = "name"; // 默认搜索字段

    // 优先选择第一个TEXT类型的字段
    for (const Field &f : schema.fields) {
        if (f.type == FieldType::TEXT) {
            searchField = f.name;
            break;
        }
    }

    Response r = m_record->selectWhere(m_currentUser, m_currentDb, m_currentTable, searchField, searchText);
    log(r.message);

    ui->tableData->clearContents();
    ui->tableData->setRowCount(0);

    if (r.status != ResponseStatus::OK) return;

    QJsonArray records = r.data.value<QJsonArray>();

    QStringList cols;
    for (const Field &f : schema.fields)
        cols << f.name;
    for (const QJsonValue &v : records) {
        for (const QString &k : v.toObject().keys())
            if (!cols.contains(k)) cols << k;
    }
    if (cols.isEmpty()) return;

    ui->tableData->setColumnCount(cols.size());
    ui->tableData->setHorizontalHeaderLabels(cols);
    ui->tableData->setRowCount(records.size());

    for (int row = 0; row < records.size(); ++row) {
        QJsonObject obj = records[row].toObject();
        for (int col = 0; col < cols.size(); ++col) {
            QJsonValue val = obj.value(cols[col]);
            QString text;
            if (val.isBool())        text = val.toBool() ? "true" : "false";
            else if (val.isDouble()) text = QString::number(val.toDouble());
            else                     text = val.toString();
            ui->tableData->setItem(row, col, new QTableWidgetItem(text));
        }
    }
}

// SQL语法高亮器实现
SqlHighlighter::SqlHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    keywordFormat.setForeground(QColor("#89b4fa"));
    keywordFormat.setFontWeight(QFont::Bold);

    stringFormat.setForeground(QColor("#a6e3a1"));

    numberFormat.setForeground(QColor("#f38ba8"));
}

void SqlHighlighter::highlightBlock(const QString &text)
{
    QStringList keywords = {
        "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES", "UPDATE", "SET",
        "DELETE", "CREATE", "DROP", "TABLE", "DATABASE", "ALTER", "ADD", "COLUMN",
        "PRIMARY", "KEY", "FOREIGN", "REFERENCES", "NOT", "NULL", "UNIQUE",
        "DEFAULT", "CHECK", "CONSTRAINT", "INDEX", "ON", "ORDER", "BY", "ASC",
        "DESC", "GROUP", "HAVING", "JOIN", "LEFT", "RIGHT", "INNER", "OUTER",
        "FULL", "CROSS", "UNION", "ALL", "DISTINCT", "AS", "AND", "OR", "IN",
        "LIKE", "BETWEEN", "IS", "EXISTS", "CASE", "WHEN", "THEN", "ELSE", "END",
        "LIMIT", "OFFSET", "INT", "TEXT", "DOUBLE", "BOOLEAN", "VARCHAR", "CHAR",
        "INTEGER", "FLOAT", "DATE", "TIME", "DATETIME", "TIMESTAMP"
    };

    for (const QString &keyword : keywords) {
        QRegularExpression expr("\\b" + keyword + "\\b", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = expr.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), keywordFormat);
        }
    }

    QRegularExpression stringExpr("'[^']*'|\"[^\"]*\"");
    QRegularExpressionMatchIterator stringIt = stringExpr.globalMatch(text);
    while (stringIt.hasNext()) {
        QRegularExpressionMatch match = stringIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), stringFormat);
    }

    QRegularExpression numberExpr("\\b\\d+(\\.\\d+)?\\b");
    QRegularExpressionMatchIterator numberIt = numberExpr.globalMatch(text);
    while (numberIt.hasNext()) {
        QRegularExpressionMatch match = numberIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), numberFormat);
    }
}

void MainWindow::onPrevPage()
{
    if (m_currentPage > 0) {
        m_currentPage--;
        onExecuteSQL();
    }
}

void MainWindow::onNextPage()
{
    int totalPages = (m_totalRows + m_pageSize - 1) / m_pageSize;
    if (m_currentPage < totalPages - 1) {
        m_currentPage++;
        onExecuteSQL();
    }
}

void MainWindow::updatePaginationLabel()
{
    int totalPages = (m_totalRows + m_pageSize - 1) / m_pageSize;
    if (totalPages == 0) totalPages = 1;
    ui->labelPagination->setText(QString("第 %1/%2 页").arg(m_currentPage + 1).arg(totalPages));
    ui->btnPrevPage->setEnabled(m_currentPage > 0);
    ui->btnNextPage->setEnabled(m_currentPage < totalPages - 1);
}

QString MainWindow::formatElapsedTime(qint64 ns)
{
    if (ns < 1000) {
        return QString("%1 ns").arg(ns);
    } else if (ns < 1000000) {
        return QString("%1 μs").arg(ns / 1000.0, 0, 'f', 2);
    } else if (ns < 1000000000) {
        return QString("%1 ms").arg(ns / 1000000.0, 0, 'f', 2);
    } else {
        return QString("%1 s").arg(ns / 1000000000.0, 0, 'f', 3);
    }
}

void MainWindow::onAdvancedSearch()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("[UI] 请先选中一张表");
        return;
    }

    QString whereClause = ui->inputSearch->text().trimmed();
    if (whereClause.isEmpty()) {
        showDataTable(m_currentUser, m_currentDb, m_currentTable);
        return;
    }

    QString sql = QString("SELECT * FROM %1 WHERE %2").arg(m_currentTable).arg(whereClause);

    if (m_sortColumn >= 0) {
        QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
        if (m_sortColumn < fields.size()) {
            QString sortField = fields[m_sortColumn].name;
            sql += QString(" ORDER BY %1 %2").arg(sortField).arg(m_sortOrder == Qt::AscendingOrder ? "ASC" : "DESC");
        }
    }

    Response res = m_parser->parseSQL(sql);
    log("[高级搜索] " + res.message);

    ui->tableData->clearContents();
    ui->tableData->setRowCount(0);

    if (res.status != ResponseStatus::OK || !res.data.canConvert<QJsonArray>()) return;

    QJsonArray records = res.data.value<QJsonArray>();
    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);

    QStringList cols;
    for (const Field &f : fields) cols << f.name;

    ui->tableData->setColumnCount(cols.size());
    ui->tableData->setHorizontalHeaderLabels(cols);
    ui->tableData->setRowCount(records.size());

    for (int row = 0; row < records.size(); ++row) {
        QJsonObject obj = records[row].toObject();
        for (int col = 0; col < cols.size(); ++col) {
            QJsonValue val = obj.value(cols[col]);
            QString text;
            if (val.isBool())        text = val.toBool() ? "true" : "false";
            else if (val.isDouble()) text = QString::number(val.toDouble());
            else                     text = val.toString();
            ui->tableData->setItem(row, col, new QTableWidgetItem(text));
        }
    }
}

void MainWindow::onTableHeaderClicked(int column)
{
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) return;

    if (m_sortColumn == column) {
        m_sortOrder = (m_sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        m_sortColumn = column;
        m_sortOrder = Qt::AscendingOrder;
    }

    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
    if (column >= fields.size()) return;

    QString sortField = fields[column].name;
    QString sql = QString("SELECT * FROM %1 ORDER BY %2 %3")
                      .arg(m_currentTable)
                      .arg(sortField)
                      .arg(m_sortOrder == Qt::AscendingOrder ? "ASC" : "DESC");

    QString whereClause = ui->inputSearch->text().trimmed();
    if (!whereClause.isEmpty()) {
        sql = QString("SELECT * FROM %1 WHERE %2 ORDER BY %3 %4")
                  .arg(m_currentTable)
                  .arg(whereClause)
                  .arg(sortField)
                  .arg(m_sortOrder == Qt::AscendingOrder ? "ASC" : "DESC");
    }

    Response res = m_parser->parseSQL(sql);
    log(QString("[排序] 按 %1 %2").arg(sortField).arg(m_sortOrder == Qt::AscendingOrder ? "升序" : "降序"));

    ui->tableData->clearContents();
    ui->tableData->setRowCount(0);

    if (res.status != ResponseStatus::OK || !res.data.canConvert<QJsonArray>()) return;

    QJsonArray records = res.data.value<QJsonArray>();

    QStringList cols;
    for (const Field &f : fields) cols << f.name;

    ui->tableData->setColumnCount(cols.size());
    ui->tableData->setHorizontalHeaderLabels(cols);
    ui->tableData->setRowCount(records.size());

    for (int row = 0; row < records.size(); ++row) {
        QJsonObject obj = records[row].toObject();
        for (int col = 0; col < cols.size(); ++col) {
            QJsonValue val = obj.value(cols[col]);
            QString text;
            if (val.isBool())        text = val.toBool() ? "true" : "false";
            else if (val.isDouble()) text = QString::number(val.toDouble());
            else                     text = val.toString();
            ui->tableData->setItem(row, col, new QTableWidgetItem(text));
        }
    }
}

void MainWindow::onImportCSV()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("[UI] 请先选中一张表");
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "导入CSV", "", "CSV文件 (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        log("[导入CSV] 无法打开文件");
        return;
    }

    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
    if (fields.isEmpty()) {
        log("[导入CSV] 无法加载表结构");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    QStringList headers;
    if (!in.atEnd()) {
        QString headerLine = in.readLine();
        headers = headerLine.split(',', Qt::SkipEmptyParts);
        for (QString &h : headers) h = h.trimmed();
    }

    int count = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(',', Qt::KeepEmptyParts);

        QJsonObject record;
        for (int i = 0; i < headers.size() && i < values.size(); ++i) {
            QString val = values[i].trimmed();
            val.remove('"');

            if (i < fields.size()) {
                switch (fields[i].type) {
                    case FieldType::INT:
                        record[headers[i]] = val.toInt();
                        break;
                    case FieldType::DOUBLE:
                        record[headers[i]] = val.toDouble();
                        break;
                    case FieldType::BOOLEAN:
                        record[headers[i]] = (val.toLower() == "true" || val == "1");
                        break;
                    default:
                        record[headers[i]] = val;
                        break;
                }
            } else {
                record[headers[i]] = val;
            }
        }

        Response res = m_record->insertRecord(m_currentUser, m_currentDb, m_currentTable, record);
        if (res.status == ResponseStatus::OK) {
            count++;
        } else {
            log(QString("[导入CSV] 行 %1 失败: %2").arg(count + 1).arg(res.message));
        }
    }

    file.close();
    log(QString("[导入CSV] 成功导入 %1 条记录").arg(count));
    showDataTable(m_currentUser, m_currentDb, m_currentTable);
}

void MainWindow::onImportJSON()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("[UI] 请先选中一张表");
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "导入JSON", "", "JSON文件 (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        log("[导入JSON] 无法打开文件");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        log("[导入JSON] JSON格式错误，需要数组格式");
        return;
    }

    QJsonArray records = doc.array();
    int count = 0;

    for (const QJsonValue &val : records) {
        if (!val.isObject()) continue;

        QJsonObject record = val.toObject();
        Response res = m_record->insertRecord(m_currentUser, m_currentDb, m_currentTable, record);
        if (res.status == ResponseStatus::OK) {
            count++;
        } else {
            log(QString("[导入JSON] 记录 %1 失败: %2").arg(count + 1).arg(res.message));
        }
    }

    log(QString("[导入JSON] 成功导入 %1 条记录").arg(count));
    showDataTable(m_currentUser, m_currentDb, m_currentTable);
}

void MainWindow::onExportCSV()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("[UI] 请先选中一张表");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "导出CSV", m_currentTable + ".csv", "CSV文件 (*.csv)");
    if (fileName.isEmpty()) return;

    Response res = m_migrator->exportToCSV(m_currentUser, m_currentDb, m_currentTable, fileName);
    log(res.message);
}

void MainWindow::onBackupDatabase()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty()) {
        log("[UI] 请先选中一个数据库");
        return;
    }

    Response res = m_migrator->backupDatabase(m_currentUser, m_currentDb, "");
    log(res.message);
    if (res.status == ResponseStatus::OK) {
        refreshTree();
    }
}

void MainWindow::onRestoreDatabase()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty()) {
        log("[UI] 请先选中一个数据库");
        return;
    }

    // 列出可用的备份
    QString dataPath = Config::DATA_PATH + m_currentUser + "/";
    QDir dataDir(dataPath);
    QStringList backupDirs;
    QStringList entries = dataDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        if (entry.startsWith(m_currentDb + "_backup_")) {
            backupDirs.append(entry);
        }
    }

    if (backupDirs.isEmpty()) {
        QMessageBox::information(this, "恢复数据库",
                                 QString("未找到数据库 \"%1\" 的备份。\n请先备份数据库。").arg(m_currentDb));
        return;
    }

    // 让用户选择备份
    bool ok;
    QString chosen = QInputDialog::getItem(this, "恢复数据库",
                                           QString("选择要恢复的备份:"), backupDirs, 0, false, &ok);
    if (!ok || chosen.isEmpty()) return;

    int ret = QMessageBox::question(this, "确认恢复",
                                    QString("确定要从备份 \"%1\" 恢复数据库 \"%2\" 吗？\n当前数据将被覆盖！")
                                        .arg(chosen).arg(m_currentDb));
    if (ret != QMessageBox::Yes) return;

    Response res = m_migrator->restoreDatabase(m_currentUser, m_currentDb, chosen);
    log(res.message);
    if (res.status == ResponseStatus::OK) {
        refreshTree();
        m_currentTable.clear();
        ui->tableData->clearContents();
        ui->tableData->setRowCount(0);
        ui->tableSchema->clearContents();
        ui->tableSchema->setRowCount(0);
    }
}

void MainWindow::onShowChart()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("[UI] 请先选中一张表");
        return;
    }

    QList<Field> fields = m_storage->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
    if (fields.isEmpty()) {
        log("[图表] 无法加载表结构");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("统计图表生成器");
    dialog.setMinimumWidth(400);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    layout->addWidget(new QLabel("选择分组字段:"));
    QComboBox *groupCombo = new QComboBox();
    for (const Field &f : fields) {
        groupCombo->addItem(f.name);
    }
    layout->addWidget(groupCombo);

    layout->addWidget(new QLabel("选择聚合函数:"));
    QComboBox *aggCombo = new QComboBox();
    aggCombo->addItem("COUNT(*)");
    aggCombo->addItem("SUM");
    aggCombo->addItem("AVG");
    aggCombo->addItem("MIN");
    aggCombo->addItem("MAX");
    layout->addWidget(aggCombo);

    layout->addWidget(new QLabel("选择聚合字段 (SUM/AVG/MIN/MAX时需要):"));
    QComboBox *aggFieldCombo = new QComboBox();
    for (const Field &f : fields) {
        if (f.type == FieldType::INT || f.type == FieldType::DOUBLE) {
            aggFieldCombo->addItem(f.name);
        }
    }
    layout->addWidget(aggFieldCombo);

    layout->addWidget(new QLabel("可选WHERE条件:"));
    QLineEdit *whereEdit = new QLineEdit();
    whereEdit->setPlaceholderText("如: age > 20");
    layout->addWidget(whereEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    QString groupField = groupCombo->currentText();
    QString aggFunc = aggCombo->currentText();
    QString aggField = aggFieldCombo->currentText();
    QString whereClause = whereEdit->text().trimmed();

    QString sql;
    if (aggFunc == "COUNT(*)") {
        sql = QString("SELECT %1, COUNT(*) FROM %2").arg(groupField).arg(m_currentTable);
    } else {
        sql = QString("SELECT %1, %2(%3) FROM %4").arg(groupField).arg(aggFunc).arg(aggField).arg(m_currentTable);
    }

    if (!whereClause.isEmpty()) {
        sql += QString(" WHERE %1").arg(whereClause);
    }
    sql += QString(" GROUP BY %1").arg(groupField);

    Response res = m_parser->parseSQL(sql);
    log(QString("[图表] 执行: %1").arg(sql));
    log(res.message);

    if (res.status != ResponseStatus::OK || !res.data.canConvert<QJsonArray>()) {
        QMessageBox::warning(this, "错误", "查询失败或无数据");
        return;
    }

    QJsonArray results = res.data.value<QJsonArray>();

    QDialog resultDialog(this);
    resultDialog.setWindowTitle(QString("统计结果 - 按 %1 分组").arg(groupField));
    resultDialog.setMinimumSize(500, 400);
    QVBoxLayout *resultLayout = new QVBoxLayout(&resultDialog);

    QTableWidget *resultTable = new QTableWidget();
    resultTable->setColumnCount(2);
    resultTable->setHorizontalHeaderLabels({groupField, aggFunc});
    resultTable->setRowCount(results.size());

    double maxVal = 0;
    for (int i = 0; i < results.size(); ++i) {
        QJsonObject obj = results[i].toObject();
        QString key = obj[groupField].toVariant().toString();
        double val = obj.contains("COUNT(*)") ? obj["COUNT(*)"].toDouble() :
                     obj.contains(QString("SUM(%1)").arg(aggField)) ? obj[QString("SUM(%1)").arg(aggField)].toDouble() :
                     obj.contains(QString("AVG(%1)").arg(aggField)) ? obj[QString("AVG(%1)").arg(aggField)].toDouble() :
                     obj.contains(QString("MIN(%1)").arg(aggField)) ? obj[QString("MIN(%1)").arg(aggField)].toDouble() :
                     obj.contains(QString("MAX(%1)").arg(aggField)) ? obj[QString("MAX(%1)").arg(aggField)].toDouble() : 0;

        resultTable->setItem(i, 0, new QTableWidgetItem(key));
        QString valStr;
        if (obj.contains("COUNT(*)")) {
            valStr = QString::number((int)val);
        } else {
            valStr = QString::number(val, 'f', 2);
        }
        resultTable->setItem(i, 1, new QTableWidgetItem(valStr));
        if (val > maxVal) maxVal = val;
    }
    resultLayout->addWidget(resultTable);

    resultLayout->addWidget(new QLabel("简单柱状图:"));
    QWidget *chartWidget = new QWidget();
    chartWidget->setMinimumHeight(200);
    chartWidget->setStyleSheet("background-color: #1e1e2e; border-radius: 6px;");
    QVBoxLayout *chartLayout = new QVBoxLayout(chartWidget);

    QStringList barColors = {
        "#89b4fa", "#a6e3a1", "#f38ba8", "#fab387", "#cba6f7",
        "#89dceb", "#f5e0dc", "#94e2d5", "#f9e2af", "#b4befe"
    };

    for (int i = 0; i < results.size() && i < 10; ++i) {
        QJsonObject obj = results[i].toObject();
        QString key = obj[groupField].toVariant().toString();
        double val = obj.contains("COUNT(*)") ? obj["COUNT(*)"].toDouble() :
                     obj.contains(QString("SUM(%1)").arg(aggField)) ? obj[QString("SUM(%1)").arg(aggField)].toDouble() :
                     obj.contains(QString("AVG(%1)").arg(aggField)) ? obj[QString("AVG(%1)").arg(aggField)].toDouble() :
                     obj.contains(QString("MIN(%1)").arg(aggField)) ? obj[QString("MIN(%1)").arg(aggField)].toDouble() :
                     obj.contains(QString("MAX(%1)").arg(aggField)) ? obj[QString("MAX(%1)").arg(aggField)].toDouble() : 0;

        QWidget *barWidget = new QWidget();
        QHBoxLayout *barLayout = new QHBoxLayout(barWidget);
        barLayout->setContentsMargins(8, 4, 8, 4);

        QLabel *labelLabel = new QLabel(key);
        labelLabel->setMinimumWidth(80);
        labelLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        labelLabel->setStyleSheet("color: #cdd6f4; font-weight: bold;");
        barLayout->addWidget(labelLabel);

        int barWidth = maxVal > 0 ? (int)(val / maxVal * 300) : 0;
        QLabel *bar = new QLabel();
        bar->setMinimumWidth(qMax(barWidth, 5));
        bar->setMaximumWidth(qMax(barWidth, 5));
        bar->setMinimumHeight(24);
        QString barColor = barColors[i % barColors.size()];
        bar->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(barColor));
        barLayout->addWidget(bar);

        QString valStr;
        if (obj.contains("COUNT(*)")) {
            valStr = QString::number((int)val);
        } else {
            valStr = QString::number(val, 'f', 2);
        }
        QLabel *valueLabel = new QLabel(valStr);
        valueLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(barColor));
        barLayout->addWidget(valueLabel);
        barLayout->addStretch();

        chartLayout->addWidget(barWidget);
    }
    chartLayout->addStretch();
    resultLayout->addWidget(chartWidget);

    QDialogButtonBox *closeBtn = new QDialogButtonBox(QDialogButtonBox::Close);
    resultLayout->addWidget(closeBtn);
    connect(closeBtn, &QDialogButtonBox::rejected, &resultDialog, &QDialog::reject);

    resultDialog.exec();
}
