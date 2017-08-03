#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
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
#include <QMap>
#include <QMapIterator>
#include <QTableWidget>
#include <QWebView>
#include <QMovie>
#include "analizador.h"
#include "analysisparameters.h"
#include <QLineEdit>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    bool analisis_en_curso = false;
    QMovie *loading_movie = new QMovie(":/images/743.gif");
    enum {msg_alert,msg_notify,msg_info};
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void loadSettings();
    void init();
    void initSideBar();
    void initWebBrowser();
    void initMediaPlayer();
    void writeText(QString text, int color);
    QProcess *initProcess(QString cmd, QString obj="");
    bool saveToDisk(const QString &filename, QIODevice *data);
    QString saveFileName(const QUrl &url);
    bool validarCampos(int id);
    void CursorCarga(bool b);
    void mkdirTemp(bool f, QString dir);
    bool buscarLB(QString lbase, QString *tmpA, QSettings *config);
    void getExtensiones(QStringList lt, QString objName);
    bool saveToHistory(QString filePath, QStringList data);
    bool loadHistoryFile(QString filePath, QTableWidget *tabla);
    void getInfoFromReport();
    void rstrip(QString &str);
    void putHTML(QStringList str, QTextEdit *QP);
    bool event(QEvent *event);
    void identificarTab(int tab);
    void setLoadingBaseline(bool b);
    void setLoadingRequest(bool b);
    void initConsole();
    void logMsg(QString text);
    QString address();
public slots:
    void update_Geometry();
    void imprimirSalida(QStringList lista);
    void readOutput();
    void readError();
    void bloquarPanel(bool val, int id=-1);
    void downloadFinished(QNetworkReply *reply);
    void mostrarSalidaDM(bool b, QString s);
    void MsgBlOut(QString msg, QTextEdit *tE);
    void CheckoutDIM();
    void loadWebReport(QString objName);
    void sceneTabRemove_slot(int index);
    void showDetails(bool status);
    void ImprimirDetallesDIM();
    void setProgressBar(int a, int b);

private slots:
    void on_actionStart_triggered();
    void on_btnSetear_clicked();
    void on_btnEditar_clicked();
    void on_toolPlanilla_clicked();
    void on_toolFuncionalidad_clicked();
    void on_actionEliminar_Duplicados_triggered();
    void on_actionGet_triggered();
    void on_actionTest_triggered();
    void on_actionProxy_Settings_triggered();
    void on_actionDoTest_triggered();
    void on_btnReload_clicked();
    void on_actionStop_triggered();
    void on_actionGetProcess_triggered();
    void on_btnBuscarBL_clicked();
    void on_btnBuscarR_clicked();
    void on_actionGo_triggered();
    void on_btnHome_clicked();
    void on_btnRefresh_clicked();

signals:
    informeTerminado(QString objName);
private:
    Ui::MainWindow *ui;
    //QLineEdit* addressEdit;
    QProgressBar *pg;
    QWebView *view;
    QPlainTextEdit *console;
    QString m_sSettingsFile;
    analizador* calc;
    QSettings *settings;
    QSettings *chkproperties;
    QList<QProcess*> listadeProcesos;
    QMediaPlayer *player;
    int cvolume = 50;

    const QString msgEr = "<strong><span style='color:red;margin-left:80px;'>[ E R R O R ] : </span></strong>";
    const QString msgOk = "<strong><span style='color:green;margin-left:80px;'>[ CORRECTO ] : </span></strong>";
    const QString msgInf = "<strong><span style='color:#FAEB0F;margin-left:80px;'>[ DETALLES ] : </span></strong>";
    const QString UPDATER = QString("cmd");
    const QStringList ARGUMENTS = ( QStringList()<<"" );
    const QString WORKING_DIR = QDir::currentPath()+"/bin/";

    /* Variables del analisis actual */

    /* Variables de salida proceso ChdckOutBaseline */
    QString checkingHome = "http://checking:8080/checking";
    QString BASE_URL = "http://checking:8080/report/qaking/Dimensions/";
    QMap<QString, QString> mapURLs;
    QMap<QString, QStringList> listaExtBaselines;
    AnalysisParameters *listaAnalisis;
    QString output__dm;
    //int idx; //indice de la pestaña actual a la que se le aplicara el analisis


    /* Listas de extensiones que analisa chk-qa */
    QStringList LIST_PLSQL = ( QStringList()<<".sf"<<".sps"<<".spb"<<".sp"<<".fnc"<<".spp"<<".plsql"<<".trg"<<".sql"<<".st"<<".prc"<<".pks"<<".pkb"<<".pck"<<".vw" );
    QStringList LIST_JAVA = ( QStringList()<<".java" );
    QStringList LIST_ACTIONSCRIPT = ( QStringList()<<".as" );
    QStringList LIST_COBOL = ( QStringList()<<".cob"<<".cbl"<<".cpy"<<".pco" );
    QStringList LIST_ASP = ( QStringList()<<".asp" );
    QStringList LIST_JSP = ( QStringList()<<".jsp"<<".jspx"<<".xhtml" );
    QStringList LIST_JAVASCRIPT = ( QStringList()<<".js" );

    QStringList tecn_names = { "plsql", "java", "actionscript", "cobol", "asp", "jsp", "javascript" };
    bool impFlag = true;    //bandera que indica si se debe imprimir o no

};

#endif // MAINWINDOW_H
