#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "AuthManager.h"
#include "StorageManager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

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
    void onExit();

private:
    Ui::MainWindow *ui;
    AuthManager *authManager;
    StorageManager *storageManager;
};
#endif // MAINWINDOW_H
