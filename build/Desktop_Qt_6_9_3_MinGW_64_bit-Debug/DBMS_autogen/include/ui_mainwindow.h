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
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionLogin;
    QAction *actionRegister;
    QAction *actionCreateDB;
    QAction *actionExit;
    QAction *actionAbout;
    QWidget *centralwidget;
    QWidget *horizontalLayoutWidget;
    QHBoxLayout *horizontalLayout;
    QTreeView *dbTree;
    QTabWidget *dataTableView;
    QWidget *tab;
    QWidget *tab_2;
    QTextEdit *logOutput;
    QMenuBar *menubar;
    QMenu *menu;
    QMenu *menu_2;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(917, 715);
        actionLogin = new QAction(MainWindow);
        actionLogin->setObjectName("actionLogin");
        actionRegister = new QAction(MainWindow);
        actionRegister->setObjectName("actionRegister");
        actionCreateDB = new QAction(MainWindow);
        actionCreateDB->setObjectName("actionCreateDB");
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        horizontalLayoutWidget = new QWidget(centralwidget);
        horizontalLayoutWidget->setObjectName("horizontalLayoutWidget");
        horizontalLayoutWidget->setGeometry(QRect(40, 80, 831, 421));
        horizontalLayout = new QHBoxLayout(horizontalLayoutWidget);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        dbTree = new QTreeView(horizontalLayoutWidget);
        dbTree->setObjectName("dbTree");

        horizontalLayout->addWidget(dbTree);

        dataTableView = new QTabWidget(horizontalLayoutWidget);
        dataTableView->setObjectName("dataTableView");
        tab = new QWidget();
        tab->setObjectName("tab");
        dataTableView->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName("tab_2");
        dataTableView->addTab(tab_2, QString());

        horizontalLayout->addWidget(dataTableView);

        logOutput = new QTextEdit(centralwidget);
        logOutput->setObjectName("logOutput");
        logOutput->setGeometry(QRect(40, 520, 831, 111));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 917, 24));
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
        menu->addAction(actionLogin);
        menu->addAction(actionRegister);
        menu->addAction(actionCreateDB);
        menu->addSeparator();
        menu->addAction(actionExit);
        menu_2->addAction(actionAbout);

        retranslateUi(MainWindow);

        dataTableView->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        actionLogin->setText(QCoreApplication::translate("MainWindow", "\347\231\273\345\275\225", nullptr));
        actionRegister->setText(QCoreApplication::translate("MainWindow", "\346\263\250\345\206\214", nullptr));
        actionCreateDB->setText(QCoreApplication::translate("MainWindow", "\345\210\233\345\273\272\346\225\260\346\215\256\345\272\223", nullptr));
        actionExit->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272", nullptr));
        actionAbout->setText(QCoreApplication::translate("MainWindow", "\345\205\263\344\272\216", nullptr));
        dataTableView->setTabText(dataTableView->indexOf(tab), QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\346\265\217\350\247\210", nullptr));
        dataTableView->setTabText(dataTableView->indexOf(tab_2), QCoreApplication::translate("MainWindow", "\350\241\250\347\273\223\346\236\204", nullptr));
        menu->setTitle(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\345\272\223", nullptr));
        menu_2->setTitle(QCoreApplication::translate("MainWindow", "\345\270\256\345\212\251", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
