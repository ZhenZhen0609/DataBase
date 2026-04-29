#include "mainwindow.h"
#include "./ui_mainwindow.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_auth(new AuthManager(this))
    , m_schema(new SchemaManager(this))
    , m_record(new RecordManager())
    , m_storage(new StorageManager())
    , m_parser(new SQLParser(this))
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

    // 数据操作按钮
    connect(ui->btnRefreshData,   &QPushButton::clicked, this, &MainWindow::onRefreshData);
    connect(ui->btnInsertRecord,  &QPushButton::clicked, this, &MainWindow::onInsertRecord);
    connect(ui->btnRefreshSchema, &QPushButton::clicked, this, &MainWindow::onRefreshSchema);

    // SQL执行按钮
    connect(ui->btnExecuteSQL, &QPushButton::clicked, this, &MainWindow::onExecuteSQL);

    // 树节点
    connect(ui->dbTree, &QTreeWidget::itemClicked, this, &MainWindow::onTreeItemClicked);

    // 菜单动作
    connect(ui->actionCreateDb,    &QAction::triggered, this, &MainWindow::onCreateDatabase);
    connect(ui->actionCreateTable, &QAction::triggered, this, &MainWindow::onCreateTable);
    connect(ui->actionDropTable,   &QAction::triggered, this, &MainWindow::onDropTable);
    connect(ui->actionAlterTable,  &QAction::triggered, this, &MainWindow::onAlterTable);
    connect(ui->actionExit,        &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout,       &QAction::triggered, this, &MainWindow::onAbout);

    // 初始化表头
    ui->tableSchema->setColumnCount(5);
    ui->tableSchema->setHorizontalHeaderLabels({"字段名", "类型", "长度", "非空", "主键"});

    // 初始化SQL解析器
    m_parser->setStorageManager(m_storage);

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

void MainWindow::onDropDatabase()
{
    if (!m_loggedIn) return requireLogin();

    QString dbName = ui->inputDbName->text().trimmed();
    if (dbName.isEmpty()) {
        log("请输入要删除的数据库名");
        return;
    }

    if (QMessageBox::question(this, "确认", "确定删除数据库 " + dbName + "？") != QMessageBox::Yes)
        return;

    if (m_storage->dropDatabase(m_currentUser, dbName)) {
        log("数据库已删除: " + dbName);
        if (m_currentDb == dbName) {
            m_currentDb.clear();
            m_currentTable.clear();
        }
        refreshTree();
    } else {
        log("数据库删除失败");
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

    // 读取现有记录
    QString trdPath = Config::DATA_PATH + m_currentUser + "/" + m_currentDb + "/" + m_currentTable + ".trd";
    QFile file(trdPath);
    QJsonArray records;
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) records = doc.array();
        file.close();
    }

    // 验证并插入
    Response res = m_schema->validateAndFillRecord(
        TableSchema{m_currentTable, fields}, records, record);
    if (res.status != ResponseStatus::OK) {
        log("插入失败: " + res.message);
        return;
    }

    records.append(record);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(records).toJson());
        file.close();
        log("记录插入成功");
        onRefreshData();
    } else {
        log("写入失败");
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
    } else if (type == "table") {
        m_currentDb = item->parent()->text(0);
        m_currentTable = item->text(0);
        m_parser->setCurrentDatabase(m_currentDb);
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

    Response res = m_parser->parseSQL(sql);
    log(res.message);

    if (res.status == ResponseStatus::OK) {
        refreshTree();
    }
}

void MainWindow::showDataTable(const QString &username, const QString &dbName, const QString &tableName)
{
    QString trdPath = Config::DATA_PATH + username + "/" + dbName + "/" + tableName + ".trd";
    QFile file(trdPath);
    if (!file.open(QIODevice::ReadOnly)) {
        ui->tableData->setRowCount(0);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        ui->tableData->setRowCount(0);
        return;
    }

    QJsonArray records = doc.array();
    QList<Field> fields = m_storage->loadTableSchema(username, dbName, tableName);

    ui->tableData->setColumnCount(fields.size());
    QStringList headers;
    for (const Field &f : fields) headers << f.name;
    ui->tableData->setHorizontalHeaderLabels(headers);

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
    ui->tableSchema->setRowCount(fields.size());

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
    }
}