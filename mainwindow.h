#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include "authmanager.h"
#include "schemamanager.h"
#include "recordmanager.h"
#include "storagemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 认证
    void onLogin();
    void onRegister();

    // 数据库
    void onCreateDatabase();

    // 表管理
    void onCreateTable();
    void onDropTable();

    // 数据操作
    void onRefreshData();
    void onInsertRecord();
    void onRefreshSchema();

    // 树节点选中
    void onTreeItemClicked(QTreeWidgetItem *item, int column);

    // 菜单
    void onAbout();

private:
    Ui::MainWindow *ui;

    AuthManager   *m_auth;
    SchemaManager *m_schema;
    RecordManager *m_record;
    StorageManager *m_storage;

    QString m_currentUser;
    QString m_currentDb;
    QString m_currentTable;
    bool    m_loggedIn = false;

    void log(const QString &msg);
    void refreshTree();
    void showDataTable(const QString &username, const QString &dbName, const QString &tableName);
    void showSchemaTable(const QString &username, const QString &dbName, const QString &tableName);
    void requireLogin();
};

#endif // MAINWINDOW_H
