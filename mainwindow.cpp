#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , authManager(new AuthManager(this))
    , storageManager(new StorageManager())
{
    ui->setupUi(this);
    
    // 设置窗口标题
    setWindowTitle("数据库管理系统");
    
    // 初始化日志输出
    ui->logOutput->append("系统启动完成，欢迎使用数据库管理系统！");
    
    qDebug() << "Starting menu setup...";
    
    // 方法1：尝试使用UI文件中定义的动作
    QAction *loginAction = findChild<QAction*>("actionLogin");
    QAction *registerAction = findChild<QAction*>("actionRegister");
    QAction *createDBAction = findChild<QAction*>("actionCreateDB");
    QAction *exitAction = findChild<QAction*>("actionExit");
    QAction *aboutAction = findChild<QAction*>("actionAbout");
    
    // 如果UI动作不存在，动态创建
    if (!loginAction || !registerAction || !createDBAction || !exitAction || !aboutAction) {
        qDebug() << "UI actions not found, creating dynamically...";
        
        // 获取或创建菜单
        QMenuBar *menuBar = this->menuBar();
        QMenu *dbMenu = menuBar->findChild<QMenu*>("menu");
        QMenu *helpMenu = menuBar->findChild<QMenu*>("menu_2");
        
        if (!dbMenu) {
            dbMenu = menuBar->addMenu("数据库");
        }
        if (!helpMenu) {
            helpMenu = menuBar->addMenu("帮助");
        }
        
        // 清空菜单并添加新动作
        dbMenu->clear();
        helpMenu->clear();
        
        loginAction = dbMenu->addAction("登录");
        registerAction = dbMenu->addAction("注册");
        createDBAction = dbMenu->addAction("创建数据库");
        dbMenu->addSeparator();
        exitAction = dbMenu->addAction("退出");
        aboutAction = helpMenu->addAction("关于");
        
        qDebug() << "Dynamically created menu items";
    } else {
        qDebug() << "Using UI-defined menu items";
    }
    
    // 连接信号槽
    connect(loginAction, &QAction::triggered, this, &MainWindow::onLogin);
    connect(registerAction, &QAction::triggered, this, &MainWindow::onRegister);
    connect(createDBAction, &QAction::triggered, this, &MainWindow::onCreateDatabase);
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);
    connect(aboutAction, &QAction::triggered, []() {
        QMessageBox::about(nullptr, "关于", "数据库管理系统 v1.0\n\n功能模块：\n- 用户认证 (AuthManager)\n- 数据库存储 (StorageManager)\n- 模式校验 (SchemaManager)");
    });
    
    qDebug() << "All menu actions connected";
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onLogin()
{
    qDebug() << "onLogin() called!";
    bool ok;
    QString username = QInputDialog::getText(this, "用户登录", "用户名:", QLineEdit::Normal, "", &ok);
    if (!ok || username.isEmpty()) return;
    
    QString password = QInputDialog::getText(this, "用户登录", "密码:", QLineEdit::Password, "", &ok);
    if (!ok || password.isEmpty()) return;
    
    bool success = authManager->login(username, password);
    
    if (success) {
        ui->logOutput->append(QString("[登录成功] 用户 %1 已登录").arg(username));
        QMessageBox::information(this, "登录成功", "登录成功！");
    } else {
        ui->logOutput->append(QString("[登录失败] 用户 %1 登录失败").arg(username));
        QMessageBox::warning(this, "登录失败", "用户名或密码错误！");
    }
}

void MainWindow::onRegister()
{
    qDebug() << "onRegister() called!";
    bool ok;
    QString username = QInputDialog::getText(this, "用户注册", "用户名:", QLineEdit::Normal, "", &ok);
    if (!ok || username.isEmpty()) return;
    
    QString password = QInputDialog::getText(this, "用户注册", "密码:", QLineEdit::Password, "", &ok);
    if (!ok || password.isEmpty()) return;
    
    QString confirmPassword = QInputDialog::getText(this, "用户注册", "确认密码:", QLineEdit::Password, "", &ok);
    if (!ok || confirmPassword.isEmpty()) return;
    
    if (password != confirmPassword) {
        QMessageBox::warning(this, "注册失败", "两次输入的密码不一致！");
        ui->logOutput->append("[注册失败] 密码不一致");
        return;
    }
    
    bool success = authManager->registerUser(username, password);
    
    if (success) {
        ui->logOutput->append(QString("[注册成功] 用户 %1 已注册").arg(username));
        QMessageBox::information(this, "注册成功", "注册成功！");
    } else {
        ui->logOutput->append(QString("[注册失败] 用户 %1 注册失败").arg(username));
        QMessageBox::warning(this, "注册失败", "注册失败，用户名可能已存在！");
    }
}

void MainWindow::onCreateDatabase()
{
    qDebug() << "onCreateDatabase() called!";
    bool ok;
    QString dbName = QInputDialog::getText(this, "创建数据库", "数据库名称:", QLineEdit::Normal, "", &ok);
    if (!ok || dbName.isEmpty()) return;
    
    bool success = storageManager->createDatabase(dbName);
    
    if (success) {
        ui->logOutput->append(QString("[数据库创建成功] %1").arg(dbName));
        QMessageBox::information(this, "创建成功", QString("数据库 '%1' 创建成功！").arg(dbName));
    } else {
        ui->logOutput->append(QString("[数据库创建失败] %1").arg(dbName));
        QMessageBox::warning(this, "创建失败", "数据库创建失败！");
    }
}

void MainWindow::onExit()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "退出", "确定要退出程序吗？",
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        ui->logOutput->append("[系统] 程序退出");
        qApp->quit();
    }
}
