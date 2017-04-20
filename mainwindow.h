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
#include <QPlainTextEdit>
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
    bool buscarLB(QString lbase, QString *tmpA, QSettings *config);
    void getExtensiones(QStringList lt);
public slots:
    void update_Geometry();
    void imprimirSalida(QStringList lista);
    void readOutput();
    void readError();
    void bloquarPanel(bool val, int id);
    void downloadFinished(QNetworkReply *reply);
    void readDotout();
    void mostrarSalidaDM(bool b, QString s);
    void MsgBlOut(QString msg, QPlainTextEdit *QP);
    void CheckoutDIM();
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
    void on_btnFormato_clicked();
    void on_btnIniciarAct_clicked();

    void on_actionDoTest_triggered();

private:
    Ui::MainWindow *ui;
    QString m_sSettingsFile;
    analizador* calc;
    QSettings *settings;
    QSettings *chkproperties;
    QList<QProcess*> listadeProcesos;
    QMediaPlayer *player;
    int cvolume = 50;

    QString output_baseline_dm;

    const QString UPDATER = QString("cmd");
    const QStringList ARGUMENTS = ( QStringList()<<"" );
    const QString WORKING_DIR = QDir::currentPath()+"/bin";

    /* Variables del analisis actual */
    QString script_id ;
    QString product_id ;
    QString project_id ;
    QString object_id ;
    QString area_id ;

    QString BASE_URL = "http://checking:8080/report/qaking/Dimensions/";

    /* Listas de extensiones que analisa chk-qa */
    QStringList LIST_PLSQL = ( QStringList()<<".sf"<<".sps"<<".spb"<<".sp"<<".fnc"<<".spp"<<".plsql"<<".trg"<<".sql"<<".st"<<".prc"<<".pks"<<".pkb"<<".pck"<<".vw" );
    QStringList LIST_ASP = ( QStringList()<<".asp" );
    QStringList LIST_JSP = ( QStringList()<<".jsp"<<".jspx"<<".xhtml" );
    QStringList LIST_COBOL = ( QStringList()<<".cob"<<".cbl"<<".cpy"<<".pco" );
    QStringList LIST_ACTIONSCRIPT = ( QStringList()<<".as" );
    QStringList LIST_JAVA = ( QStringList()<<".java" );
    QStringList LIST_JAVASCRIPT = ( QStringList()<<".js" );

    QStringList informes;
};

#endif // MAINWINDOW_H
