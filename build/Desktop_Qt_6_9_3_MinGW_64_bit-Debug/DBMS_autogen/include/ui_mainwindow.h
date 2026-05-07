/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionCreateDb;
    QAction *actionCreateTable;
    QAction *actionDropTable;
    QAction *actionAlterTable;
    QAction *actionAddField;
    QAction *actionDropField;
    QAction *actionAlterField;
    QAction *actionExit;
    QAction *actionAbout;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QWidget *topToolbar;
    QHBoxLayout *topToolbarLayout;
    QLabel *appIcon;
    QLabel *appTitle;
    QLabel *labelDb;
    QLineEdit *inputDbName;
    QPushButton *btnCreateDb;
    QPushButton *btnDropDb;
    QLabel *labelUser;
    QLineEdit *inputUsername;
    QLabel *labelPwd;
    QLineEdit *inputPassword;
    QPushButton *btnLogin;
    QPushButton *btnRegister;
    QSpacerItem *topToolbarSpacer;
    QLabel *labelWelcome;
    QHBoxLayout *mainLayout;
    QWidget *sidebar;
    QVBoxLayout *sidebarLayout;
    QLabel *sidebarTitle;
    QTreeWidget *dbTree;
    QWidget *dbActions;
    QHBoxLayout *dbActionsLayout;
    QPushButton *btnCreateTable;
    QPushButton *btnDropTable;
    QLabel *tableTitle;
    QWidget *tableActions;
    QGridLayout *tableActionsLayout;
    QPushButton *btnAddField;
    QPushButton *btnDropField;
    QPushButton *btnAlterTable;
    QPushButton *btnAlterField;
    QSpacerItem *sidebarSpacer;
    QWidget *mainContent;
    QVBoxLayout *mainContentLayout;
    QWidget *dataToolbar;
    QHBoxLayout *dataToolbarLayout;
    QPushButton *btnRefreshData;
    QPushButton *btnRefreshSchema;
    QPushButton *btnInsertRecord;
    QSpacerItem *dataToolbarSpacer;
    QLabel *labelSearch;
    QLineEdit *inputSearch;
    QPushButton *btnSearch;
    QTabWidget *dataTableView;
    QWidget *tabData;
    QVBoxLayout *dataTabLayout;
    QTableWidget *tableData;
    QWidget *tabSchema;
    QVBoxLayout *schemaTabLayout;
    QTableWidget *tableSchema;
    QWidget *tabSQL;
    QVBoxLayout *sqlTabLayout;
    QLabel *labelSQL;
    QTextEdit *sqlInput;
    QPushButton *btnExecuteSQL;
    QTextEdit *logOutput;
    QMenuBar *menubar;
    QMenu *menu;
    QMenu *menu_2;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1200, 800);
        MainWindow->setStyleSheet(QString::fromUtf8("QMainWindow {\n"
"    background-color: #1e1e2e;\n"
"}\n"
"\n"
"QLabel {\n"
"    color: #cdd6f4;\n"
"    font-size: 14px;\n"
"}\n"
"\n"
"QLineEdit, QTextEdit {\n"
"    background-color: #313244;\n"
"    border: 1px solid #45475a;\n"
"    border-radius: 6px;\n"
"    padding: 8px;\n"
"    color: #cdd6f4;\n"
"    font-family: Consolas, monospace;\n"
"}\n"
"\n"
"QLineEdit:focus, QTextEdit:focus {\n"
"    border: 1px solid #89b4fa;\n"
"}\n"
"\n"
"QPushButton {\n"
"    background-color: #313244;\n"
"    border: 1px solid #45475a;\n"
"    border-radius: 6px;\n"
"    padding: 8px 16px;\n"
"    color: #cdd6f4;\n"
"    min-width: 80px;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: #45475a;\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: #585b70;\n"
"}\n"
"\n"
"QPushButton#btnLogin {\n"
"    background-color: #89b4fa;\n"
"    color: #1e1e2e;\n"
"    font-weight: bold;\n"
"}\n"
"\n"
"QPushButton#btnLogin:hover {\n"
"    background-color: #b4befe;\n"
"}\n"
"\n"
"QPushButton#btnCreateDb, QPushB"
                        "utton#btnCreateTable, QPushButton#btnAddField, QPushButton#btnInsertRecord {\n"
"    background-color: #a6e3a1;\n"
"    color: #1e1e2e;\n"
"}\n"
"\n"
"QPushButton#btnCreateDb:hover, QPushButton#btnCreateTable:hover, QPushButton#btnAddField:hover, QPushButton#btnInsertRecord:hover {\n"
"    background-color: #baf0b8;\n"
"}\n"
"\n"
"QPushButton#btnDropDb, QPushButton#btnDropTable, QPushButton#btnDropField {\n"
"    background-color: #f38ba8;\n"
"    color: #1e1e2e;\n"
"}\n"
"\n"
"QPushButton#btnDropDb:hover, QPushButton#btnDropTable:hover, QPushButton#btnDropField:hover {\n"
"    background-color: #eba0ac;\n"
"}\n"
"\n"
"QPushButton#btnExecuteSQL {\n"
"    background-color: #89b4fa;\n"
"    color: #1e1e2e;\n"
"    font-weight: bold;\n"
"}\n"
"\n"
"QPushButton#btnExecuteSQL:hover {\n"
"    background-color: #b4befe;\n"
"}\n"
"\n"
"QTreeWidget {\n"
"    background-color: #313244;\n"
"    border: 1px solid #45475a;\n"
"    border-radius: 6px;\n"
"    padding: 8px;\n"
"    color: #cdd6f4;\n"
"    show-decoration-sel"
                        "ected: 1;\n"
"}\n"
"\n"
"QTreeWidget::item {\n"
"    padding: 6px;\n"
"}\n"
"\n"
"QTreeWidget::item:hover {\n"
"    background-color: #45475a;\n"
"}\n"
"\n"
"QTreeWidget::item:selected {\n"
"    background-color: #89b4fa;\n"
"    color: #1e1e2e;\n"
"}\n"
"\n"
"QTableWidget {\n"
"    background-color: #313244;\n"
"    border: 1px solid #45475a;\n"
"    border-radius: 6px;\n"
"    color: #cdd6f4;\n"
"    gridline-color: #45475a;\n"
"}\n"
"\n"
"QTableWidget::item {\n"
"    padding: 6px;\n"
"}\n"
"\n"
"QTableWidget::item:selected {\n"
"    background-color: #89b4fa;\n"
"    color: #1e1e2e;\n"
"}\n"
"\n"
"QHeaderView::section {\n"
"    background-color: #45475a;\n"
"    color: #cdd6f4;\n"
"    padding: 8px;\n"
"    border: none;\n"
"    font-weight: bold;\n"
"}\n"
"\n"
"QTabWidget::pane {\n"
"    background-color: #313244;\n"
"    border: 1px solid #45475a;\n"
"    border-radius: 6px;\n"
"}\n"
"\n"
"QTabBar::tab {\n"
"    background-color: #313244;\n"
"    color: #cdd6f4;\n"
"    padding: 10px 20px;\n"
"    border:"
                        " none;\n"
"    border-top-left-radius: 6px;\n"
"    border-top-right-radius: 6px;\n"
"    margin-right: 2px;\n"
"}\n"
"\n"
"QTabBar::tab:hover {\n"
"    background-color: #45475a;\n"
"}\n"
"\n"
"QTabBar::tab:selected {\n"
"    background-color: #313244;\n"
"    color: #89b4fa;\n"
"}\n"
"\n"
"QTabBar::tab:!selected {\n"
"    margin-bottom: 0px;\n"
"}\n"
"\n"
"QMenuBar {\n"
"    background-color: #181825;\n"
"    color: #cdd6f4;\n"
"}\n"
"\n"
"QMenuBar::item {\n"
"    background-color: #181825;\n"
"    padding: 6px 12px;\n"
"}\n"
"\n"
"QMenuBar::item:selected {\n"
"    background-color: #45475a;\n"
"}\n"
"\n"
"QMenu {\n"
"    background-color: #313244;\n"
"    color: #cdd6f4;\n"
"}\n"
"\n"
"QMenu::item {\n"
"    padding: 8px 16px;\n"
"}\n"
"\n"
"QMenu::item:selected {\n"
"    background-color: #45475a;\n"
"}\n"
"\n"
"QStatusBar {\n"
"    background-color: #181825;\n"
"    color: #a6adc8;\n"
"}\n"
"\n"
"QTextEdit#logOutput {\n"
"    background-color: #11111b;\n"
"    border: 1px solid #45475a;\n"
"    border-radi"
                        "us: 6px;\n"
"    color: #a6e3a1;\n"
"    font-family: Consolas, monospace;\n"
"}\n"
"\n"
"QScrollBar:vertical {\n"
"    background: #313244;\n"
"    width: 10px;\n"
"    border-radius: 5px;\n"
"}\n"
"\n"
"QScrollBar::handle:vertical {\n"
"    background: #45475a;\n"
"    border-radius: 5px;\n"
"}\n"
"\n"
"QScrollBar::handle:vertical:hover {\n"
"    background: #585b70;\n"
"}\n"
"\n"
"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {\n"
"    height: 0px;\n"
"}"));
        actionCreateDb = new QAction(MainWindow);
        actionCreateDb->setObjectName("actionCreateDb");
        actionCreateTable = new QAction(MainWindow);
        actionCreateTable->setObjectName("actionCreateTable");
        actionDropTable = new QAction(MainWindow);
        actionDropTable->setObjectName("actionDropTable");
        actionAlterTable = new QAction(MainWindow);
        actionAlterTable->setObjectName("actionAlterTable");
        actionAddField = new QAction(MainWindow);
        actionAddField->setObjectName("actionAddField");
        actionDropField = new QAction(MainWindow);
        actionDropField->setObjectName("actionDropField");
        actionAlterField = new QAction(MainWindow);
        actionAlterField->setObjectName("actionAlterField");
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        topToolbar = new QWidget(centralwidget);
        topToolbar->setObjectName("topToolbar");
        topToolbar->setMinimumHeight(60);
        topToolbarLayout = new QHBoxLayout(topToolbar);
        topToolbarLayout->setObjectName("topToolbarLayout");
        topToolbarLayout->setContentsMargins(16, 8, 16, 8);
        appIcon = new QLabel(topToolbar);
        appIcon->setObjectName("appIcon");
        QFont font;
        font.setPointSize(20);
        appIcon->setFont(font);

        topToolbarLayout->addWidget(appIcon);

        appTitle = new QLabel(topToolbar);
        appTitle->setObjectName("appTitle");
        QFont font1;
        font1.setPointSize(16);
        font1.setBold(true);
        appTitle->setFont(font1);

        topToolbarLayout->addWidget(appTitle);

        labelDb = new QLabel(topToolbar);
        labelDb->setObjectName("labelDb");

        topToolbarLayout->addWidget(labelDb);

        inputDbName = new QLineEdit(topToolbar);
        inputDbName->setObjectName("inputDbName");
        inputDbName->setMinimumWidth(120);

        topToolbarLayout->addWidget(inputDbName);

        btnCreateDb = new QPushButton(topToolbar);
        btnCreateDb->setObjectName("btnCreateDb");

        topToolbarLayout->addWidget(btnCreateDb);

        btnDropDb = new QPushButton(topToolbar);
        btnDropDb->setObjectName("btnDropDb");

        topToolbarLayout->addWidget(btnDropDb);

        labelUser = new QLabel(topToolbar);
        labelUser->setObjectName("labelUser");

        topToolbarLayout->addWidget(labelUser);

        inputUsername = new QLineEdit(topToolbar);
        inputUsername->setObjectName("inputUsername");
        inputUsername->setMinimumWidth(100);

        topToolbarLayout->addWidget(inputUsername);

        labelPwd = new QLabel(topToolbar);
        labelPwd->setObjectName("labelPwd");

        topToolbarLayout->addWidget(labelPwd);

        inputPassword = new QLineEdit(topToolbar);
        inputPassword->setObjectName("inputPassword");
        inputPassword->setEchoMode(QLineEdit::Password);
        inputPassword->setMinimumWidth(100);

        topToolbarLayout->addWidget(inputPassword);

        btnLogin = new QPushButton(topToolbar);
        btnLogin->setObjectName("btnLogin");

        topToolbarLayout->addWidget(btnLogin);

        btnRegister = new QPushButton(topToolbar);
        btnRegister->setObjectName("btnRegister");

        topToolbarLayout->addWidget(btnRegister);

        topToolbarSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        topToolbarLayout->addItem(topToolbarSpacer);

        labelWelcome = new QLabel(topToolbar);
        labelWelcome->setObjectName("labelWelcome");
        QFont font2;
        font2.setBold(true);
        labelWelcome->setFont(font2);
        labelWelcome->setStyleSheet(QString::fromUtf8("color: #a6e3a1;"));

        topToolbarLayout->addWidget(labelWelcome);


        verticalLayout->addWidget(topToolbar);

        mainLayout = new QHBoxLayout();
        mainLayout->setObjectName("mainLayout");
        sidebar = new QWidget(centralwidget);
        sidebar->setObjectName("sidebar");
        sidebar->setMinimumWidth(220);
        sidebar->setMaximumWidth(280);
        sidebarLayout = new QVBoxLayout(sidebar);
        sidebarLayout->setSpacing(12);
        sidebarLayout->setObjectName("sidebarLayout");
        sidebarTitle = new QLabel(sidebar);
        sidebarTitle->setObjectName("sidebarTitle");
        sidebarTitle->setFont(font2);

        sidebarLayout->addWidget(sidebarTitle);

        dbTree = new QTreeWidget(sidebar);
        dbTree->setObjectName("dbTree");
        dbTree->setMinimumWidth(200);
        dbTree->setIndentation(15);
        dbTree->setRootIsDecorated(true);
        dbTree->setItemsExpandable(true);
        dbTree->setSortingEnabled(false);

        sidebarLayout->addWidget(dbTree);

        dbActions = new QWidget(sidebar);
        dbActions->setObjectName("dbActions");
        dbActionsLayout = new QHBoxLayout(dbActions);
        dbActionsLayout->setSpacing(6);
        dbActionsLayout->setObjectName("dbActionsLayout");
        btnCreateTable = new QPushButton(dbActions);
        btnCreateTable->setObjectName("btnCreateTable");

        dbActionsLayout->addWidget(btnCreateTable);

        btnDropTable = new QPushButton(dbActions);
        btnDropTable->setObjectName("btnDropTable");

        dbActionsLayout->addWidget(btnDropTable);


        sidebarLayout->addWidget(dbActions);

        tableTitle = new QLabel(sidebar);
        tableTitle->setObjectName("tableTitle");
        tableTitle->setFont(font2);

        sidebarLayout->addWidget(tableTitle);

        tableActions = new QWidget(sidebar);
        tableActions->setObjectName("tableActions");
        tableActionsLayout = new QGridLayout(tableActions);
        tableActionsLayout->setSpacing(6);
        tableActionsLayout->setObjectName("tableActionsLayout");
        btnAddField = new QPushButton(tableActions);
        btnAddField->setObjectName("btnAddField");

        tableActionsLayout->addWidget(btnAddField, 0, 0, 1, 1);

        btnDropField = new QPushButton(tableActions);
        btnDropField->setObjectName("btnDropField");

        tableActionsLayout->addWidget(btnDropField, 0, 1, 1, 1);

        btnAlterTable = new QPushButton(tableActions);
        btnAlterTable->setObjectName("btnAlterTable");

        tableActionsLayout->addWidget(btnAlterTable, 1, 0, 1, 1);

        btnAlterField = new QPushButton(tableActions);
        btnAlterField->setObjectName("btnAlterField");

        tableActionsLayout->addWidget(btnAlterField, 1, 1, 1, 1);


        sidebarLayout->addWidget(tableActions);

        sidebarSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        sidebarLayout->addItem(sidebarSpacer);


        mainLayout->addWidget(sidebar);

        mainContent = new QWidget(centralwidget);
        mainContent->setObjectName("mainContent");
        mainContentLayout = new QVBoxLayout(mainContent);
        mainContentLayout->setSpacing(12);
        mainContentLayout->setObjectName("mainContentLayout");
        mainContentLayout->setContentsMargins(12, 12, 12, 12);
        dataToolbar = new QWidget(mainContent);
        dataToolbar->setObjectName("dataToolbar");
        dataToolbarLayout = new QHBoxLayout(dataToolbar);
        dataToolbarLayout->setSpacing(8);
        dataToolbarLayout->setObjectName("dataToolbarLayout");
        btnRefreshData = new QPushButton(dataToolbar);
        btnRefreshData->setObjectName("btnRefreshData");

        dataToolbarLayout->addWidget(btnRefreshData);

        btnRefreshSchema = new QPushButton(dataToolbar);
        btnRefreshSchema->setObjectName("btnRefreshSchema");

        dataToolbarLayout->addWidget(btnRefreshSchema);

        btnInsertRecord = new QPushButton(dataToolbar);
        btnInsertRecord->setObjectName("btnInsertRecord");

        dataToolbarLayout->addWidget(btnInsertRecord);

        dataToolbarSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        dataToolbarLayout->addItem(dataToolbarSpacer);

        labelSearch = new QLabel(dataToolbar);
        labelSearch->setObjectName("labelSearch");

        dataToolbarLayout->addWidget(labelSearch);

        inputSearch = new QLineEdit(dataToolbar);
        inputSearch->setObjectName("inputSearch");

        dataToolbarLayout->addWidget(inputSearch);

        btnSearch = new QPushButton(dataToolbar);
        btnSearch->setObjectName("btnSearch");

        dataToolbarLayout->addWidget(btnSearch);


        mainContentLayout->addWidget(dataToolbar);

        dataTableView = new QTabWidget(mainContent);
        dataTableView->setObjectName("dataTableView");
        tabData = new QWidget();
        tabData->setObjectName("tabData");
        dataTabLayout = new QVBoxLayout(tabData);
        dataTabLayout->setSpacing(0);
        dataTabLayout->setObjectName("dataTabLayout");
        dataTabLayout->setContentsMargins(0, 0, 0, 0);
        tableData = new QTableWidget(tabData);
        tableData->setObjectName("tableData");
        tableData->setAlternatingRowColors(true);
        tableData->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableData->setGridStyle(Qt::DotLine);

        dataTabLayout->addWidget(tableData);

        dataTableView->addTab(tabData, QString());
        tabSchema = new QWidget();
        tabSchema->setObjectName("tabSchema");
        schemaTabLayout = new QVBoxLayout(tabSchema);
        schemaTabLayout->setSpacing(0);
        schemaTabLayout->setObjectName("schemaTabLayout");
        schemaTabLayout->setContentsMargins(0, 0, 0, 0);
        tableSchema = new QTableWidget(tabSchema);
        tableSchema->setObjectName("tableSchema");
        tableSchema->setAlternatingRowColors(true);
        tableSchema->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableSchema->setGridStyle(Qt::DotLine);

        schemaTabLayout->addWidget(tableSchema);

        dataTableView->addTab(tabSchema, QString());
        tabSQL = new QWidget();
        tabSQL->setObjectName("tabSQL");
        sqlTabLayout = new QVBoxLayout(tabSQL);
        sqlTabLayout->setSpacing(12);
        sqlTabLayout->setObjectName("sqlTabLayout");
        sqlTabLayout->setContentsMargins(12, 12, 12, 12);
        labelSQL = new QLabel(tabSQL);
        labelSQL->setObjectName("labelSQL");

        sqlTabLayout->addWidget(labelSQL);

        sqlInput = new QTextEdit(tabSQL);
        sqlInput->setObjectName("sqlInput");
        sqlInput->setTabStopDistance(40.000000000000000);

        sqlTabLayout->addWidget(sqlInput);

        btnExecuteSQL = new QPushButton(tabSQL);
        btnExecuteSQL->setObjectName("btnExecuteSQL");

        sqlTabLayout->addWidget(btnExecuteSQL);

        dataTableView->addTab(tabSQL, QString());

        mainContentLayout->addWidget(dataTableView);


        mainLayout->addWidget(mainContent);


        verticalLayout->addLayout(mainLayout);

        logOutput = new QTextEdit(centralwidget);
        logOutput->setObjectName("logOutput");
        logOutput->setMaximumHeight(120);
        logOutput->setReadOnly(true);

        verticalLayout->addWidget(logOutput);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1200, 24));
        menu = new QMenu(menubar);
        menu->setObjectName("menu");
        menu_2 = new QMenu(menubar);
        menu_2->setObjectName("menu_2");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menu->menuAction());
        menubar->addAction(menu_2->menuAction());
        menu->addAction(actionCreateDb);
        menu->addAction(actionCreateTable);
        menu->addAction(actionDropTable);
        menu->addAction(actionAlterTable);
        menu->addSeparator();
        menu->addAction(actionAddField);
        menu->addAction(actionDropField);
        menu->addAction(actionAlterField);
        menu->addSeparator();
        menu->addAction(actionExit);
        menu_2->addAction(actionAbout);

        retranslateUi(MainWindow);

        dataTableView->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "DBMS \346\225\260\346\215\256\345\272\223\347\256\241\347\220\206\347\263\273\347\273\237", nullptr));
        actionCreateDb->setText(QCoreApplication::translate("MainWindow", "\345\210\233\345\273\272\346\225\260\346\215\256\345\272\223", nullptr));
        actionCreateTable->setText(QCoreApplication::translate("MainWindow", "\346\226\260\345\273\272\350\241\250", nullptr));
        actionDropTable->setText(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244\350\241\250", nullptr));
        actionAlterTable->setText(QCoreApplication::translate("MainWindow", "\344\277\256\346\224\271\350\241\250", nullptr));
        actionAddField->setText(QCoreApplication::translate("MainWindow", "\346\267\273\345\212\240\345\255\227\346\256\265", nullptr));
        actionDropField->setText(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244\345\255\227\346\256\265", nullptr));
        actionAlterField->setText(QCoreApplication::translate("MainWindow", "\344\277\256\346\224\271\345\255\227\346\256\265", nullptr));
        actionExit->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272", nullptr));
        actionAbout->setText(QCoreApplication::translate("MainWindow", "\345\205\263\344\272\216", nullptr));
        appIcon->setText(QCoreApplication::translate("MainWindow", "\360\237\223\246", nullptr));
        appTitle->setText(QCoreApplication::translate("MainWindow", "DBMS \346\225\260\346\215\256\345\272\223\347\256\241\347\220\206\347\263\273\347\273\237", nullptr));
        labelDb->setText(QCoreApplication::translate("MainWindow", "\360\237\223\246", nullptr));
        inputDbName->setPlaceholderText(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\345\272\223\345\220\215", nullptr));
        btnCreateDb->setText(QCoreApplication::translate("MainWindow", "\345\210\233\345\273\272", nullptr));
        btnDropDb->setText(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244", nullptr));
        labelUser->setText(QCoreApplication::translate("MainWindow", "\360\237\221\244", nullptr));
        inputUsername->setPlaceholderText(QCoreApplication::translate("MainWindow", "\347\224\250\346\210\267\345\220\215", nullptr));
        labelPwd->setText(QCoreApplication::translate("MainWindow", "\360\237\224\221", nullptr));
        inputPassword->setPlaceholderText(QCoreApplication::translate("MainWindow", "\345\257\206\347\240\201", nullptr));
        btnLogin->setText(QCoreApplication::translate("MainWindow", "\347\231\273\345\275\225", nullptr));
        btnRegister->setText(QCoreApplication::translate("MainWindow", "\346\263\250\345\206\214", nullptr));
        labelWelcome->setText(QCoreApplication::translate("MainWindow", "\346\234\252\347\231\273\345\275\225", nullptr));
        sidebarTitle->setText(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\345\272\223/\350\241\250", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = dbTree->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("MainWindow", "\345\220\215\347\247\260", nullptr));
        btnCreateTable->setText(QCoreApplication::translate("MainWindow", "\360\237\223\213 \346\226\260\345\273\272\350\241\250", nullptr));
        btnDropTable->setText(QCoreApplication::translate("MainWindow", "\360\237\227\221\357\270\217 \345\210\240\351\231\244\350\241\250", nullptr));
        tableTitle->setText(QCoreApplication::translate("MainWindow", "\345\255\227\346\256\265\346\223\215\344\275\234", nullptr));
        btnAddField->setText(QCoreApplication::translate("MainWindow", "\342\236\225 \346\267\273\345\212\240\345\255\227\346\256\265", nullptr));
        btnDropField->setText(QCoreApplication::translate("MainWindow", "\342\236\226 \345\210\240\351\231\244\345\255\227\346\256\265", nullptr));
        btnAlterTable->setText(QCoreApplication::translate("MainWindow", "\342\234\217\357\270\217 \344\277\256\346\224\271\350\241\250", nullptr));
        btnAlterField->setText(QCoreApplication::translate("MainWindow", "\342\234\217\357\270\217 \344\277\256\346\224\271\345\255\227\346\256\265", nullptr));
        btnRefreshData->setText(QCoreApplication::translate("MainWindow", "\360\237\224\204 \345\210\267\346\226\260\346\225\260\346\215\256", nullptr));
        btnRefreshSchema->setText(QCoreApplication::translate("MainWindow", "\360\237\223\213 \345\210\267\346\226\260\347\273\223\346\236\204", nullptr));
        btnInsertRecord->setText(QCoreApplication::translate("MainWindow", "\342\236\225 \346\217\222\345\205\245", nullptr));
        labelSearch->setText(QCoreApplication::translate("MainWindow", "\360\237\224\215 \346\220\234\347\264\242:", nullptr));
        inputSearch->setPlaceholderText(QCoreApplication::translate("MainWindow", "\350\276\223\345\205\245\346\220\234\347\264\242\345\205\263\351\224\256\350\257\215", nullptr));
        btnSearch->setText(QCoreApplication::translate("MainWindow", "\346\220\234\347\264\242", nullptr));
        dataTableView->setTabText(dataTableView->indexOf(tabData), QCoreApplication::translate("MainWindow", "\360\237\223\212 \346\225\260\346\215\256\346\265\217\350\247\210", nullptr));
        dataTableView->setTabText(dataTableView->indexOf(tabSchema), QCoreApplication::translate("MainWindow", "\360\237\223\213 \350\241\250\347\273\223\346\236\204", nullptr));
        labelSQL->setText(QCoreApplication::translate("MainWindow", "\350\276\223\345\205\245SQL\350\257\255\345\217\245 (\346\224\257\346\214\201: CREATE/DROP DATABASE/TABLE)", nullptr));
        sqlInput->setPlaceholderText(QCoreApplication::translate("MainWindow", "\344\276\213\345\246\202: CREATE DATABASE mydb; \346\210\226 CREATE TABLE users (id INT, name TEXT);", nullptr));
        btnExecuteSQL->setText(QCoreApplication::translate("MainWindow", "\360\237\232\200 \346\211\247\350\241\214SQL", nullptr));
        dataTableView->setTabText(dataTableView->indexOf(tabSQL), QCoreApplication::translate("MainWindow", "\360\237\222\273 SQL\346\211\247\350\241\214", nullptr));
        logOutput->setPlaceholderText(QCoreApplication::translate("MainWindow", "\346\223\215\344\275\234\346\227\245\345\277\227...", nullptr));
        menu->setTitle(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\345\272\223", nullptr));
        menu_2->setTitle(QCoreApplication::translate("MainWindow", "\345\270\256\345\212\251", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
