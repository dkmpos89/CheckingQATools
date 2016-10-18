#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include "analizador.h"
#include <QDir>
#include <QProcess>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum {msg_alert,msg_notify,msg_info};
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void loadSettings();
    void init();
    void writeText(QString text, int color);
    void initProcess();
    void activarBotones(int idx);
public slots:
    void update_Geometry();
    void imprimirSalida(QStringList lista);
    void readOutput();
    void readError();
private slots:
    void on_actionStart_triggered();
    void on_btnSetear_clicked();
    void on_btnEditar_clicked();
    void on_toolPlanilla_clicked();
    void on_toolFuncionalidad_clicked();
    void on_actionEliminar_Duplicados_triggered();
    void on_actionCMD_triggered();

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QString m_sSettingsFile;
    analizador* calc;
    QSettings *settings;
    QProcess *procPaExec;

    const QString UPDATER = QString("cmd");
    const QStringList ARGUMENTS = ( QStringList()<<"" );
    const QString WORKING_DIR = QDir::currentPath()+"/bin";
};

#endif // MAINWINDOW_H