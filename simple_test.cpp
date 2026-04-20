#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QMessageBox>

class TestWindow : public QMainWindow
{
    Q_OBJECT
public:
    TestWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("Test Window");
        
        // 创建菜单
        QMenu *fileMenu = menuBar()->addMenu("文件");
        QAction *testAction = fileMenu->addAction("测试");
        
        // 连接信号槽
        connect(testAction, &QAction::triggered, this, &TestWindow::onTest);
        
        qDebug() << "Test window created";
    }
    
private slots:
    void onTest()
    {
        qDebug() << "Test button clicked!";
        QMessageBox::information(this, "测试", "按钮被点击了！");
    }
};

#include "simple_test.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestWindow window;
    window.show();
    return app.exec();
}
