#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QAction>
#include <QElapsedTimer>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include "authmanager.h"
#include "schemamanager.h"
#include "recordmanager.h"
#include "storagemanager.h"
#include "sqlparser.h"
#include "queryengine.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// SQL语法高亮器
class SqlHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit SqlHighlighter(QTextDocument *parent);
protected:
    void highlightBlock(const QString &text) override;
private:
    QTextCharFormat keywordFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat numberFormat;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLogin();
    void onRegister();
    void onCreateDatabase();
    void onDropDatabase();
    void onCreateTable();
    void onDropTable();
    void onAlterTable();
    void onRefreshData();
    void onInsertRecord();
    void onRefreshSchema();
    void onSearch();
    void onAddField();
    void onDropField();
    void onAlterField();
    void onExecuteSQL();

    // 功能1新增: 分页
    void onPrevPage();
    void onNextPage();

    void onTreeItemClicked(QTreeWidgetItem *item, int column);
    void onTreeItemContextMenu(const QPoint &pos);
    void onContextMenuAction();
    void onAbout();

private:
    Ui::MainWindow *ui;

    AuthManager   *m_auth;
    SchemaManager *m_schema;
    RecordManager *m_record;
    StorageManager *m_storage;
    SQLParser     *m_parser;
    QueryEngine   *m_queryEngine;
    SqlHighlighter *m_sqlHighlighter;

    QString m_currentUser;
    QString m_currentDb;
    QString m_currentTable;
    bool    m_loggedIn = false;

    QTreeWidgetItem *m_contextMenuTarget;

    // 功能1新增: 分页状态
    int m_currentPage = 0;
    int m_pageSize = 50;
    int m_totalRows = 0;

    QElapsedTimer m_queryTimer;

    void log(const QString &msg);
    void refreshTree();
    void showDataTable(const QString &username, const QString &dbName, const QString &tableName);
    void showSchemaTable(const QString &username, const QString &dbName, const QString &tableName);
    void requireLogin();
    void updatePaginationLabel();
    QString formatElapsedTime(qint64 ns);
};

#endif // MAINWINDOW_H
