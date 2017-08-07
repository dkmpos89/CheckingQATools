#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Fusion");
    qDebug() << QStyleFactory::keys();
    MainWindow w;
    w.show();
    w.setWindowState(Qt::WindowMaximized);

    return a.exec();
}
