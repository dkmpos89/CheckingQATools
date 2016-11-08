#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include "analizador.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QProgressBar>
#include <QProcess>
#include <QUrl>
#include <QDir>
#include <QMediaPlayer>


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
    QProcess *initProcess();
    void activarBotones(int idx, bool b);
    bool saveToDisk(const QString &filename, QIODevice *data);
    QString saveFileName(const QUrl &url);
    bool validarCampos(int idx);
    void setConfigProgressBar(QProgressBar *pg, bool bp=true, int min=0, int max=99);
    void addToHistorial(QStringList data);
    void CursorCarga(bool b, int idx);
    void checkearSalida(QStringList arg);
    void mkdirTemp(bool f, QString dir);
public slots:
    void update_Geometry();
    void imprimirSalida(QStringList lista);
    void readOutput();
    void readError();
    void bloquarPanel(bool val, int id);
    void downloadFinished(QNetworkReply *reply);
    void readDotout();
    void mostrarSalidaDM(bool b, QString s);
private slots:
    void on_actionStart_triggered();
    void on_btnSetear_clicked();
    void on_btnEditar_clicked();
    void on_toolPlanilla_clicked();
    void on_toolFuncionalidad_clicked();
    void on_actionEliminar_Duplicados_triggered();
    void on_actionCMD_triggered();
    void on_actionGet_triggered();
    void on_actionTest_triggered();

    void on_actionProxy_Settings_triggered();

private:
    Ui::MainWindow *ui;
    QString m_sSettingsFile;
    analizador* calc;
    QSettings *settings;
    QSettings *chkproperties;
    QProcess *procPaExec;
    QList<QProcess*> listadeProcesos;
    QMediaPlayer *player;

    const QString UPDATER = QString("cmd");
    const QStringList ARGUMENTS = ( QStringList()<<"" );
    const QString WORKING_DIR = QDir::currentPath()+"/bin";
};

#endif // MAINWINDOW_H
