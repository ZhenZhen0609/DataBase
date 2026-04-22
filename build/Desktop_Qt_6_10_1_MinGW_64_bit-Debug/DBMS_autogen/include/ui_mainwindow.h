/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
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
    QAction *actionExit;
    QAction *actionAbout;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *toolbarLayout;
    QLabel *labelDb;
    QLineEdit *inputDbName;
    QPushButton *btnCreateDb;
    QSpacerItem *horizontalSpacer;
    QLabel *labelUser;
    QLineEdit *inputUsername;
    QLabel *labelPwd;
    QLineEdit *inputPassword;
    QPushButton *btnLogin;
    QPushButton *btnRegister;
    QLabel *labelWelcome;
    QHBoxLayout *mainLayout;
    QVBoxLayout *leftLayout;
    QLabel *labelTree;
    QTreeWidget *dbTree;
    QHBoxLayout *tableButtonLayout;
    QPushButton *btnCreateTable;
    QPushButton *btnDropTable;
    QVBoxLayout *rightLayout;
    QTabWidget *dataTableView;
    QWidget *tabData;
    QVBoxLayout *dataTabLayout;
    QHBoxLayout *recordToolLayout;
    QPushButton *btnRefreshData;
    QPushButton *btnInsertRecord;
    QSpacerItem *recordSpacer;
    QTableWidget *tableData;
    QWidget *tabSchema;
    QVBoxLayout *schemaTabLayout;
    QPushButton *btnRefreshSchema;
    QTableWidget *tableSchema;
    QTextEdit *logOutput;
    QMenuBar *menubar;
    QMenu *menu;
    QMenu *menu_2;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(960, 720);
        actionCreateDb = new QAction(MainWindow);
        actionCreateDb->setObjectName("actionCreateDb");
        actionCreateTable = new QAction(MainWindow);
        actionCreateTable->setObjectName("actionCreateTable");
        actionDropTable = new QAction(MainWindow);
        actionDropTable->setObjectName("actionDropTable");
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        toolbarLayout = new QHBoxLayout();
        toolbarLayout->setObjectName("toolbarLayout");
        labelDb = new QLabel(centralwidget);
        labelDb->setObjectName("labelDb");

        toolbarLayout->addWidget(labelDb);

        inputDbName = new QLineEdit(centralwidget);
        inputDbName->setObjectName("inputDbName");

        toolbarLayout->addWidget(inputDbName);

        btnCreateDb = new QPushButton(centralwidget);
        btnCreateDb->setObjectName("btnCreateDb");

        toolbarLayout->addWidget(btnCreateDb);

        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        toolbarLayout->addItem(horizontalSpacer);

        labelUser = new QLabel(centralwidget);
        labelUser->setObjectName("labelUser");

        toolbarLayout->addWidget(labelUser);

        inputUsername = new QLineEdit(centralwidget);
        inputUsername->setObjectName("inputUsername");

        toolbarLayout->addWidget(inputUsername);

        labelPwd = new QLabel(centralwidget);
        labelPwd->setObjectName("labelPwd");

        toolbarLayout->addWidget(labelPwd);

        inputPassword = new QLineEdit(centralwidget);
        inputPassword->setObjectName("inputPassword");
        inputPassword->setEchoMode(QLineEdit::Password);

        toolbarLayout->addWidget(inputPassword);

        btnLogin = new QPushButton(centralwidget);
        btnLogin->setObjectName("btnLogin");

        toolbarLayout->addWidget(btnLogin);

        btnRegister = new QPushButton(centralwidget);
        btnRegister->setObjectName("btnRegister");

        toolbarLayout->addWidget(btnRegister);

        labelWelcome = new QLabel(centralwidget);
        labelWelcome->setObjectName("labelWelcome");

        toolbarLayout->addWidget(labelWelcome);


        verticalLayout->addLayout(toolbarLayout);

        mainLayout = new QHBoxLayout();
        mainLayout->setObjectName("mainLayout");
        leftLayout = new QVBoxLayout();
        leftLayout->setObjectName("leftLayout");
        labelTree = new QLabel(centralwidget);
        labelTree->setObjectName("labelTree");

        leftLayout->addWidget(labelTree);

        dbTree = new QTreeWidget(centralwidget);
        dbTree->setObjectName("dbTree");
        dbTree->setMinimumWidth(200);

        leftLayout->addWidget(dbTree);

        tableButtonLayout = new QHBoxLayout();
        tableButtonLayout->setObjectName("tableButtonLayout");
        btnCreateTable = new QPushButton(centralwidget);
        btnCreateTable->setObjectName("btnCreateTable");

        tableButtonLayout->addWidget(btnCreateTable);

        btnDropTable = new QPushButton(centralwidget);
        btnDropTable->setObjectName("btnDropTable");

        tableButtonLayout->addWidget(btnDropTable);


        leftLayout->addLayout(tableButtonLayout);


        mainLayout->addLayout(leftLayout);

        rightLayout = new QVBoxLayout();
        rightLayout->setObjectName("rightLayout");
        dataTableView = new QTabWidget(centralwidget);
        dataTableView->setObjectName("dataTableView");
        tabData = new QWidget();
        tabData->setObjectName("tabData");
        dataTabLayout = new QVBoxLayout(tabData);
        dataTabLayout->setObjectName("dataTabLayout");
        recordToolLayout = new QHBoxLayout();
        recordToolLayout->setObjectName("recordToolLayout");
        btnRefreshData = new QPushButton(tabData);
        btnRefreshData->setObjectName("btnRefreshData");

        recordToolLayout->addWidget(btnRefreshData);

        btnInsertRecord = new QPushButton(tabData);
        btnInsertRecord->setObjectName("btnInsertRecord");

        recordToolLayout->addWidget(btnInsertRecord);

        recordSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        recordToolLayout->addItem(recordSpacer);


        dataTabLayout->addLayout(recordToolLayout);

        tableData = new QTableWidget(tabData);
        tableData->setObjectName("tableData");
        tableData->setAlternatingRowColors(true);

        dataTabLayout->addWidget(tableData);

        dataTableView->addTab(tabData, QString());
        tabSchema = new QWidget();
        tabSchema->setObjectName("tabSchema");
        schemaTabLayout = new QVBoxLayout(tabSchema);
        schemaTabLayout->setObjectName("schemaTabLayout");
        btnRefreshSchema = new QPushButton(tabSchema);
        btnRefreshSchema->setObjectName("btnRefreshSchema");

        schemaTabLayout->addWidget(btnRefreshSchema);

        tableSchema = new QTableWidget(tabSchema);
        tableSchema->setObjectName("tableSchema");
        tableSchema->setAlternatingRowColors(true);

        schemaTabLayout->addWidget(tableSchema);

        dataTableView->addTab(tabSchema, QString());

        rightLayout->addWidget(dataTableView);


        mainLayout->addLayout(rightLayout);


        verticalLayout->addLayout(mainLayout);

        logOutput = new QTextEdit(centralwidget);
        logOutput->setObjectName("logOutput");
        logOutput->setMaximumHeight(120);
        logOutput->setReadOnly(true);

        verticalLayout->addWidget(logOutput);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 960, 24));
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
        actionExit->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272", nullptr));
        actionAbout->setText(QCoreApplication::translate("MainWindow", "\345\205\263\344\272\216", nullptr));
        labelDb->setText(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\345\272\223:", nullptr));
        inputDbName->setPlaceholderText(QCoreApplication::translate("MainWindow", "\350\276\223\345\205\245\346\225\260\346\215\256\345\272\223\345\220\215", nullptr));
        btnCreateDb->setText(QCoreApplication::translate("MainWindow", "\345\210\233\345\273\272\346\225\260\346\215\256\345\272\223", nullptr));
        labelUser->setText(QCoreApplication::translate("MainWindow", "\347\224\250\346\210\267\345\220\215:", nullptr));
        inputUsername->setPlaceholderText(QCoreApplication::translate("MainWindow", "\347\224\250\346\210\267\345\220\215", nullptr));
        labelPwd->setText(QCoreApplication::translate("MainWindow", "\345\257\206\347\240\201:", nullptr));
        inputPassword->setPlaceholderText(QCoreApplication::translate("MainWindow", "\345\257\206\347\240\201", nullptr));
        btnLogin->setText(QCoreApplication::translate("MainWindow", "\347\231\273\345\275\225", nullptr));
        btnRegister->setText(QCoreApplication::translate("MainWindow", "\346\263\250\345\206\214", nullptr));
        labelWelcome->setText(QCoreApplication::translate("MainWindow", "\346\234\252\347\231\273\345\275\225", nullptr));
        labelWelcome->setStyleSheet(QCoreApplication::translate("MainWindow", "color: gray;", nullptr));
        labelTree->setText(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\345\272\223/\350\241\250 \345\210\227\350\241\250", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = dbTree->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("MainWindow", "\345\220\215\347\247\260", nullptr));
        btnCreateTable->setText(QCoreApplication::translate("MainWindow", "\346\226\260\345\273\272\350\241\250", nullptr));
        btnDropTable->setText(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244\350\241\250", nullptr));
        btnRefreshData->setText(QCoreApplication::translate("MainWindow", "\345\210\267\346\226\260\346\225\260\346\215\256", nullptr));
        btnInsertRecord->setText(QCoreApplication::translate("MainWindow", "\346\217\222\345\205\245\350\256\260\345\275\225", nullptr));
        dataTableView->setTabText(dataTableView->indexOf(tabData), QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\346\265\217\350\247\210", nullptr));
        btnRefreshSchema->setText(QCoreApplication::translate("MainWindow", "\345\210\267\346\226\260\347\273\223\346\236\204", nullptr));
        dataTableView->setTabText(dataTableView->indexOf(tabSchema), QCoreApplication::translate("MainWindow", "\350\241\250\347\273\223\346\236\204", nullptr));
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
