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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_auth(new AuthManager(this))
    , m_schema(new SchemaManager(this))
    , m_record(new RecordManager())
    , m_storage(new StorageManager())
{
    ui->setupUi(this);

    // 认证按钮
    connect(ui->btnLogin,    &QPushButton::clicked, this, &MainWindow::onLogin);
    connect(ui->btnRegister, &QPushButton::clicked, this, &MainWindow::onRegister);

    // 数据库按钮
    connect(ui->btnCreateDb, &QPushButton::clicked, this, &MainWindow::onCreateDatabase);

    // 表管理按钮
    connect(ui->btnCreateTable, &QPushButton::clicked, this, &MainWindow::onCreateTable);
    connect(ui->btnDropTable,   &QPushButton::clicked, this, &MainWindow::onDropTable);

    // 数据操作按钮
    connect(ui->btnRefreshData,   &QPushButton::clicked, this, &MainWindow::onRefreshData);
    connect(ui->btnInsertRecord,  &QPushButton::clicked, this, &MainWindow::onInsertRecord);
    connect(ui->btnRefreshSchema, &QPushButton::clicked, this, &MainWindow::onRefreshSchema);

    // 树节点
    connect(ui->dbTree, &QTreeWidget::itemClicked, this, &MainWindow::onTreeItemClicked);

    // 菜单动作
    connect(ui->actionCreateDb,    &QAction::triggered, this, &MainWindow::onCreateDatabase);
    connect(ui->actionCreateTable, &QAction::triggered, this, &MainWindow::onCreateTable);
    connect(ui->actionDropTable,   &QAction::triggered, this, &MainWindow::onDropTable);
    connect(ui->actionExit,        &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout,       &QAction::triggered, this, &MainWindow::onAbout);

    // 初始化表头
    ui->tableSchema->setColumnCount(5);
    ui->tableSchema->setHorizontalHeaderLabels({"字段名", "类型", "长度", "非空", "主键"});

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
    ui->statusbar->showMessage(msg, 3000);
}

// ─── 认证 ────────────────────────────────────────────────────────────────────

void MainWindow::onLogin()
{
    QString user = ui->inputUsername->text().trimmed();
    QString pwd  = ui->inputPassword->text();
    if (user.isEmpty() || pwd.isEmpty()) {
        log("[Auth] 用户名或密码不能为空");
        return;
    }
    if (m_auth->login(user, pwd)) {
        m_loggedIn = true;
        m_currentUser = user;
        ui->labelWelcome->setText(QString("欢迎，%1").arg(user));
        ui->labelWelcome->setStyleSheet("color: green; font-weight: bold;");
        log(QString("[Auth] 用户 \"%1\" 登录成功").arg(user));
        refreshTree();
    } else {
        log(QString("[Auth] 登录失败：用户名或密码错误"));
        QMessageBox::warning(this, "登录失败", "用户名或密码错误");
    }
}

void MainWindow::onRegister()
{
    QString user = ui->inputUsername->text().trimmed();
    QString pwd  = ui->inputPassword->text();
    if (user.isEmpty() || pwd.isEmpty()) {
        log("[Auth] 用户名或密码不能为空");
        return;
    }
    if (m_auth->registerUser(user, pwd)) {
        ui->labelWelcome->setText(QString("注册成功：%1（请登录）").arg(user));
        ui->labelWelcome->setStyleSheet("color: blue;");
        log(QString("[Auth] 用户 \"%1\" 注册成功").arg(user));
        QMessageBox::information(this, "注册成功", QString("用户 \"%1\" 注册成功").arg(user));
    } else {
        log(QString("[Auth] 注册失败：用户 \"%1\" 已存在").arg(user));
        QMessageBox::warning(this, "注册失败", "用户已存在或写入失败");
    }
}

void MainWindow::requireLogin()
{
    if (!m_loggedIn)
        QMessageBox::warning(this, "未登录", "请先登录后再操作");
}

// ─── 数据库 ──────────────────────────────────────────────────────────────────

void MainWindow::onCreateDatabase()
{
    if (!m_loggedIn) { requireLogin(); return; }

    QString dbName = ui->inputDbName->text().trimmed();
    if (dbName.isEmpty()) {
        dbName = QInputDialog::getText(this, "创建数据库", "数据库名称:");
        if (dbName.trimmed().isEmpty()) return;
        dbName = dbName.trimmed();
    }

    if (m_storage->createDatabase(m_currentUser, dbName)) {
        log(QString("[Storage] 数据库 \"%1\" 创建成功").arg(dbName));
        m_currentDb = dbName;
        refreshTree();
    } else {
        log(QString("[Storage] 数据库 \"%1\" 已存在或创建失败").arg(dbName));
    }
}

// ─── 表管理 ──────────────────────────────────────────────────────────────────

void MainWindow::onCreateTable()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty()) {
        log("[UI] 请先选择或创建一个数据库");
        return;
    }

    QString tableName = QInputDialog::getText(this, "新建表", "表名称:");
    if (tableName.trimmed().isEmpty()) return;
    tableName = tableName.trimmed();

    // 逐字段添加对话框
    QList<Field> fields;
    bool firstField = true;
    while (true) {
        QDialog dlg(this);
        dlg.setWindowTitle(QString("添加字段（已添加 %1 个）").arg(fields.size()));

        QFormLayout *form = new QFormLayout;
        QLineEdit *nameEdit = new QLineEdit;
        QComboBox *typeBox  = new QComboBox;
        typeBox->addItems({"INT", "TEXT", "DOUBLE", "BOOLEAN"});

        form->addRow("字段名称:", nameEdit);
        form->addRow("字段类型:", typeBox);

        QLabel *hint = new QLabel("提示：第一个字段将自动设为主键");
        hint->setStyleSheet("color: gray; font-size: 11px;");

        QDialogButtonBox *btns = new QDialogButtonBox;
        btns->addButton("添加并继续", QDialogButtonBox::AcceptRole);
        QPushButton *doneBtn = btns->addButton("完成建表", QDialogButtonBox::RejectRole);
        connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(doneBtn, &QPushButton::clicked,    &dlg, &QDialog::reject);

        QVBoxLayout *layout = new QVBoxLayout(&dlg);
        layout->addLayout(form);
        if (firstField) layout->addWidget(hint);
        layout->addWidget(btns);

        int result = dlg.exec();

        // 如果填了字段名就先保存
        QString fname = nameEdit->text().trimmed();
        if (!fname.isEmpty()) {
            static const QMap<QString, FieldType> typeMap = {
                {"INT", FieldType::INT}, {"TEXT", FieldType::TEXT},
                {"DOUBLE", FieldType::DOUBLE}, {"BOOLEAN", FieldType::BOOLEAN}
            };
            Field f(fname, typeMap.value(typeBox->currentText(), FieldType::TEXT));
            if (firstField) { f.isPrimaryKey = true; f.isNotNull = true; firstField = false; }
            fields.append(f);
        }

        if (result == QDialog::Rejected) break; // 点"完成建表"或关闭
    }

    if (fields.isEmpty()) {
        log("[UI] 未添加任何字段，建表取消");
        return;
    }

    TableSchema schema;
    schema.tableName = tableName;
    schema.fields    = fields;

    Response r = m_schema->createTable(m_currentUser, m_currentDb, schema);
    log(r.message);
    if (r.status == ResponseStatus::OK) {
        m_currentTable = tableName;
        refreshTree();
    } else {
        QMessageBox::warning(this, "建表失败", r.message);
    }
}

void MainWindow::onDropTable()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("[UI] 请先在左侧选中一张表");
        return;
    }

    int ret = QMessageBox::question(this, "确认删除",
        QString("确定要删除表 \"%1\" 吗？此操作不可恢复！").arg(m_currentTable));
    if (ret != QMessageBox::Yes) return;

    Response r = m_schema->dropTable(m_currentUser, m_currentDb, m_currentTable);
    log(r.message);
    if (r.status == ResponseStatus::OK) {
        m_currentTable.clear();
        ui->tableData->clearContents();
        ui->tableData->setRowCount(0);
        ui->tableSchema->clearContents();
        ui->tableSchema->setRowCount(0);
        refreshTree();
    } else {
        QMessageBox::warning(this, "删除失败", r.message);
    }
}

// ─── 数据操作 ────────────────────────────────────────────────────────────────

void MainWindow::onRefreshData()
{
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("[UI] 请先选中一张表");
        return;
    }
    showDataTable(m_currentUser, m_currentDb, m_currentTable);
}

void MainWindow::onInsertRecord()
{
    if (!m_loggedIn) { requireLogin(); return; }
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("[UI] 请先选中一张表");
        return;
    }

    Response sr = m_schema->loadTableSchema(m_currentUser, m_currentDb, m_currentTable);
    if (sr.status != ResponseStatus::OK) {
        log("[UI] 无法加载表结构: " + sr.message);
        return;
    }
    TableSchema schema = sr.data.value<TableSchema>();

    // 每个字段一行的表单对话框
    QDialog dlg(this);
    dlg.setWindowTitle(QString("插入记录 - %1").arg(m_currentTable));
    QFormLayout *form = new QFormLayout;
    QList<QLineEdit*> edits;
    for (const Field &f : schema.fields) {
        QLineEdit *edit = new QLineEdit;
        QString typeHint;
        if (f.type == FieldType::INT)     typeHint = "整数";
        else if (f.type == FieldType::DOUBLE)  typeHint = "小数";
        else if (f.type == FieldType::BOOLEAN) typeHint = "true/false";
        else                                   typeHint = "文本";
        edit->setPlaceholderText(typeHint);
        form->addRow(QString("%1：").arg(f.name), edit);
        edits.append(edit);
    }
    QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);
    layout->addWidget(btns);

    if (dlg.exec() != QDialog::Accepted) return;

    QJsonObject record;
    for (int i = 0; i < schema.fields.size(); ++i) {
        const Field &f = schema.fields[i];
        QString val = edits[i]->text().trimmed();
        if (f.type == FieldType::INT)          record[f.name] = val.toInt();
        else if (f.type == FieldType::DOUBLE)  record[f.name] = val.toDouble();
        else if (f.type == FieldType::BOOLEAN) record[f.name] = (val.toLower() == "true");
        else                                   record[f.name] = val;
    }

    Response r = m_record->insertRecord(m_currentUser, m_currentDb, m_currentTable, record);
    log(r.message);
    if (r.status == ResponseStatus::OK)
        showDataTable(m_currentUser, m_currentDb, m_currentTable);
    else
        QMessageBox::warning(this, "插入失败", r.message);
}

void MainWindow::onRefreshSchema()
{
    if (m_currentDb.isEmpty() || m_currentTable.isEmpty()) {
        log("[UI] 请先选中一张表");
        return;
    }
    showSchemaTable(m_currentUser, m_currentDb, m_currentTable);
}

// ─── 树节点 ──────────────────────────────────────────────────────────────────

void MainWindow::onTreeItemClicked(QTreeWidgetItem *item, int /*column*/)
{
    if (!item) return;

    if (item->parent() == nullptr) {
        // 点击的是数据库节点
        m_currentDb    = item->text(0);
        m_currentTable.clear();
        log(QString("[UI] 已选中数据库: %1").arg(m_currentDb));
    } else {
        // 点击的是表节点
        m_currentDb    = item->parent()->text(0);
        m_currentTable = item->text(0);
        log(QString("[UI] 已选中表: %1.%2").arg(m_currentDb, m_currentTable));
        showDataTable(m_currentUser, m_currentDb, m_currentTable);
        showSchemaTable(m_currentUser, m_currentDb, m_currentTable);
    }
}

// ─── 菜单 ────────────────────────────────────────────────────────────────────

void MainWindow::onAbout()
{
    QMessageBox::about(this, "关于",
        "DBMS 数据库管理系统\n\n"
        "模块：StorageManager / AuthManager / SchemaManager / RecordManager\n"
        "UI：MainWindow (粉圈D)");
}

// ─── 私有辅助 ────────────────────────────────────────────────────────────────

void MainWindow::refreshTree()
{
    ui->dbTree->clear();
    if (m_currentUser.isEmpty()) return;

    QString userPath = Config::DATA_PATH + m_currentUser;
    QDir dir(userPath);
    if (!dir.exists()) return;

    for (const QString &dbName : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QTreeWidgetItem *dbItem = new QTreeWidgetItem(ui->dbTree, {dbName});

        Response r = m_schema->loadTables(m_currentUser, dbName);
        if (r.status == ResponseStatus::OK) {
            QList<TableSchema> tables = r.data.value<QList<TableSchema>>();
            for (const TableSchema &t : tables)
                new QTreeWidgetItem(dbItem, {t.tableName});
        }
        dbItem->setExpanded(true);
    }
}

void MainWindow::showDataTable(const QString &username, const QString &dbName, const QString &tableName)
{
    Response r = m_record->selectAllRecords(username, dbName, tableName);
    log(r.message);

    ui->tableData->clearContents();
    ui->tableData->setRowCount(0);

    if (r.status != ResponseStatus::OK) return;

    QJsonArray records = r.data.value<QJsonArray>();

    // 从表结构获取列名，保证列头始终显示
    Response sr = m_schema->loadTableSchema(username, dbName, tableName);
    QStringList cols;
    if (sr.status == ResponseStatus::OK) {
        for (const Field &f : sr.data.value<TableSchema>().fields)
            cols << f.name;
    }
    // 补充数据中有但结构里没有的列（含 _created_at）
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

    ui->dataTableView->setCurrentIndex(0);
}

void MainWindow::showSchemaTable(const QString &username, const QString &dbName, const QString &tableName)
{
    Response r = m_schema->loadTableSchema(username, dbName, tableName);
    log(r.message);

    ui->tableSchema->clearContents();
    ui->tableSchema->setRowCount(0);

    if (r.status != ResponseStatus::OK) return;

    TableSchema schema = r.data.value<TableSchema>();
    ui->tableSchema->setRowCount(schema.fields.size());

    static const QStringList typeNames = {"INT", "TEXT", "DOUBLE", "BOOLEAN"};

    for (int i = 0; i < schema.fields.size(); ++i) {
        const Field &f = schema.fields[i];
        ui->tableSchema->setItem(i, 0, new QTableWidgetItem(f.name));
        ui->tableSchema->setItem(i, 1, new QTableWidgetItem(typeNames.value(static_cast<int>(f.type))));
        ui->tableSchema->setItem(i, 2, new QTableWidgetItem(QString::number(f.length)));
        ui->tableSchema->setItem(i, 3, new QTableWidgetItem(f.isNotNull   ? "是" : "否"));
        ui->tableSchema->setItem(i, 4, new QTableWidgetItem(f.isPrimaryKey ? "是" : "否"));
    }

    ui->dataTableView->setCurrentIndex(1);
}
