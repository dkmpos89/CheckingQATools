#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Fusion");
    MainWindow w;
    w.show();
    w.setWindowState(Qt::WindowMaximized);

    return a.exec();
}
