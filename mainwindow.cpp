#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDesktopWidget>
#include <QThread>
#include <QTimer>
#include <QFileDialog>
#include "xlsxdocument.h"
#include <QInputDialog>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_sSettingsFile = QDir::currentPath()+"/config/checking_tools_settings.ini";
    loadSettings();
    init();
    ui->panel_trazabilidad->hide();
    QTimer::singleShot(100, this, SLOT(update_Geometry()));
}

MainWindow::~MainWindow()
{
    delete ui;
    procPaExec->kill();
    procPaExec->close();
}

/* Funciones de Inicializacion */
void MainWindow::loadSettings()
{
    /* SETTINGS: crecacion del directorio 'config' y carga de los archivos '.ini' */
    QString tmp="";
    QString path = QDir::currentPath()+"/config";
    if(!QDir(path).exists()){
        QDir().mkdir(path);
        QFile::copy(":/config/checking_tools_settings.ini", path+"/checking_tools_settings.ini");
        QFile::copy(":/config/tfuncional_config.ini", path+"/tfuncional_config.ini");
        QFile::setPermissions(path+"/checking_tools_settings.ini" , QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
        QFile::setPermissions(path+"/tfuncional_config.ini" , QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
    }
    settings = new QSettings(m_sSettingsFile, QSettings::IniFormat);

    /* SETTINGS: comboBox de ID DE PRODUCTO */
    settings->beginGroup("product");
    QStringList listaKeys = settings->childKeys();

    foreach (QString key, listaKeys) {
        tmp = settings->value(key).toString();
        ui->product_id->addItem(tmp);
    }
    settings->endGroup();

    /* SETTINGS: comboBox de ID DE PROYECTO */
    settings->beginGroup("project");
    QStringList listaProject = settings->childKeys();

    foreach (QString key, listaProject) {
        tmp = settings->value(key).toString();
        ui->project_id->addItem(tmp);
        ui->area_id->addItem(key);
    }
    settings->endGroup();

    /* SETTINGS: comboBox de trazabilodad funcional */
    QString m_SFile = QDir::currentPath()+"/config/tfuncional_config.ini";
    QSettings *config = new QSettings(m_SFile, QSettings::IniFormat);
    QStringList listaGrupos = config->childGroups();

    foreach (QString key, listaGrupos) {
        ui->comboRutas_TF->addItem(key);
    }
    config->beginGroup("ADMISION_BANCO");
    ui->planillaExcel->setText(config->value("planilla").toString());
    ui->funcionalidadExcel->setText(config->value("funcionalidad").toString());
    config->endGroup();
}

void MainWindow::init()
{
    ui->progressBar_tab1->setVisible(false);
    ui->btnGO->setEnabled(false);
    ui->btnEditar->setVisible(false);

/* Bloque de creacion de proceso paexec */
    initProcess();
/* End */

/* Bloque de creacion de la variable analizador --> se mueve a otro hilo para no congelar la interface*/
    QThread* thread = new QThread(this);
    calc = new analizador(NULL);
    calc->moveToThread(thread);

    connect(ui->btnGO, &QPushButton::clicked, [=] () {calc->doHeavyCalculation(); } );
    connect(calc, SIGNAL(calculationCompleted(QStringList)), this, SLOT(imprimirSalida(QStringList)));
    connect(calc, &analizador::updateProgressbar, [=] (const int value) {ui->progressBar_tf->setValue(value); } );
    connect(ui->actionSalida_compuesta, &QAction::toggled, [=] (const bool val) {calc->setSalidaComp(val);} );
/* End */

/* Bloque de 'connect' para el cambio de pestañas */
    connect(ui->tabWidget, &QTabWidget::currentChanged, [=] (const int idx) {
        if(idx==2)
            ui->panel_trazabilidad->show();
        else
            ui->panel_trazabilidad->hide();
        QTimer::singleShot(100, this, SLOT(update_Geometry()));
        activarBotones(idx);
    });
/* End */

/* Bloque de creaciones del binario ../bin/paexec.exe */
    QString path = QDir::currentPath()+"/bin";
    if(!QDir(path).exists()){
        QDir().mkdir(path);
        QFile::copy(":/bin/paexec.exe", path+"/paexec.exe");
        QFile::setPermissions(path+"/paexec.exe" , QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
    }
/* End */

}

void MainWindow::initProcess()
{
    procPaExec = new QProcess(this);
    procPaExec->setProcessChannelMode( QProcess::SeparateChannels );
    procPaExec->start(UPDATER, ARGUMENTS, QProcess::ReadWrite);
    procPaExec->waitForStarted(5000);

    QString cmd("cd "+WORKING_DIR+"\n");
    procPaExec->write(cmd.toLatin1());
    procPaExec->waitForFinished(1000);
    QByteArray ba = procPaExec->readAll();//clean stdout
    Q_UNUSED(ba);

    connect(procPaExec, SIGNAL( readyReadStandardOutput() ), this, SLOT(readOutput()) );
    connect(procPaExec, SIGNAL( readyReadStandardError() ), this, SLOT(readError()) );

    writeText("^ [Iniciando log del sistema... por favor espere]", msg_notify);
}

void MainWindow::update_Geometry()
{
    double px,py,w,h;
    QRect pantalla = this->geometry();
    px = pantalla.x();
    py = pantalla.y();

    int idx = ui->tabWidget->currentIndex();
    if(idx==2){
        ui->panel_trazabilidad->show();
        w = 850;
        h = 600;
    }else{
        ui->panel_trazabilidad->hide();
        w = 850;
        h = 220;
    }
    this->setGeometry(px,py,w,h);
}
/* Fin de las funciones de Inicializacion */


/* Funciones de los Menus y Botones */
void MainWindow::on_actionStart_triggered()
{
    QString cmd = "";
    int idx = 0;

    if(procPaExec->state()==QProcess::Running){
        idx = ui->tabWidget->currentIndex();
        if(idx==0){
            QStringList lista = ( QStringList()<<ui->product_id->currentText()<<ui->project_id->currentText()<<ui->requests_id->text()<<ui->area_id->currentText() );;
            cmd = "START /B paexec.exe \\\\192.168.10.63 -u checking -p chm.321. -d D:\\modelo_operativo_checking_4.2\\certificacion_request.bat "+lista[0]+" "+lista[1]+" "+lista[2]+" "+lista[3]+"\n";
        }else if(idx==1){
            QStringList lista = ( QStringList()<<ui->product_id->currentText()<<ui->project_id->currentText()<<ui->requests_id->text()<<ui->area_id->currentText() );;
            cmd = "START /B paexec.exe \\\\192.168.10.63 -u checking -p chm.321. -d D:\\modelo_operativo_checking_4.2\\certificacion_request.bat "+lista[0]+" "+lista[1]+" "+lista[2]+" "+lista[3]+"\n";
        }
    }else{
        writeText("^ERROR: El proceso de 'paExec'' no esta corriendo!", msg_alert);
        return;
    }
    procPaExec->write(cmd.toLatin1());
    bloquarPanel(true);
    // MostrarCursorCarga(true);
}

void MainWindow::on_btnEditar_clicked()
{
    ui->btnSetear->setVisible(true);
    ui->btnEditar->setVisible(false);
    ui->textAreaEntrada->setEnabled(true);
    ui->btnGO->setEnabled(false);
    ui->btnSetear->setEnabled(true);
    ui->progressBar_tab1->setValue(0);
}

void MainWindow::on_btnSetear_clicked()
{
    if(!ui->textAreaEntrada->toPlainText().isEmpty())
    {
        if(!ui->planillaExcel->text().isEmpty() && !ui->funcionalidadExcel->text().isEmpty()){
            ui->textAreaSalida->clear();
            ui->btnSetear->setVisible(false);
            ui->btnEditar->setVisible(true);

            QString filePlanilla = ui->planillaExcel->text();
            QString fileFuncionalidad = ui->funcionalidadExcel->text();
            QStringList listaEntrada = ui->textAreaEntrada->toPlainText().split("\n");

            calc->setPath_to_planilla(filePlanilla);
            calc->setPath_to_funcionalidad(fileFuncionalidad);
            calc->setProcEntrada(listaEntrada);

            ui->textAreaEntrada->setEnabled(false);
            ui->btnSetear->setEnabled(false);
            ui->btnGO->setEnabled(true);
        }else{
            ui->statusBar->showMessage("ERROR: No se han cargado los archivos Excel.", 5000);
        }
    }else{
        ui->statusBar->showMessage("ERROR: Faltan los nombreas de los procedimientos.", 5000);
    }
}

void MainWindow::on_toolPlanilla_clicked()
{
    QString m_SFile = QDir::currentPath()+"/config/tfuncional_config.ini";
    QSettings * config = new QSettings(m_SFile, QSettings::IniFormat);
    QString path,tipo,tipoFile,comboText;
    tipoFile = "xlsx";
    tipo = ( tipoFile.toUpper() )+" Files (*."+(tipoFile.toLower())+")";
    path = QFileDialog::getOpenFileName(this, "Abrir Archivo:", QDir::homePath(),tipo);

    if(path != NULL){
        ui->planillaExcel->setText(path);
        comboText = ui->comboRutas_TF->currentText();
        config->beginGroup(comboText);
        config->setValue("planilla", path);
        config->endGroup();
    }
}

void MainWindow::on_toolFuncionalidad_clicked()
{
    QString m_SFile = QDir::currentPath()+"/config/tfuncional_config.ini";
    QSettings * config = new QSettings(m_SFile, QSettings::IniFormat);
    QString path,tipo,tipoFile,comboText;
    tipoFile = "xlsx";
    tipo = ( tipoFile.toUpper() )+" Files (*."+(tipoFile.toLower())+")";
    path = QFileDialog::getOpenFileName(this, "Abrir Archivo:", QDir::homePath(),tipo);

    if(path != NULL){
        ui->funcionalidadExcel->setText(path);
        comboText = ui->comboRutas_TF->currentText();
        config->beginGroup(comboText);
        config->setValue("funcionalidad", path);
        config->endGroup();
    }
}

void MainWindow::on_actionEliminar_Duplicados_triggered()
{
    QStringList listaSalida = ui->textAreaSalida->toPlainText().split("\n");
    ui->textAreaSalida->clear();
    ui->statusBar->showMessage("INFO: Eliminando salidas duplicadas...", 3000);

    foreach (QString var, listaSalida)
    {
        listaSalida.removeAll(var);
        listaSalida.append(var);
    }
    ui->textAreaSalida->setPlainText(listaSalida.join("\n"));
}

void MainWindow::on_actionCMD_triggered()
{
    bool ok;
    // Ask for birth date as a string.
    QString text = QInputDialog::getText(0, "Linea de comandos_",
                                         "Ingrese:", QLineEdit::Normal,
                                         "", &ok);
    if (ok && !text.isEmpty()) {
        if(procPaExec->state()==QProcess::Running)
            procPaExec->write(text.toLatin1()+"\n");
    }
}
/* Fin Menus y Botones */


/* Funciones de control */
void MainWindow::imprimirSalida(QStringList lista)
{
    ui->textAreaSalida->clear();
    ui->textAreaSalida->appendPlainText(lista.join("\n"));
}

void MainWindow::readOutput()
{
    QString buff = QString::fromLatin1(procPaExec->readAllStandardOutput());
    writeText(buff, Qt::green);
}

void MainWindow::readError()
{
    QString error = QString::fromLocal8Bit(procPaExec->readAllStandardError());
    writeText(error, Qt::red);
}

void MainWindow::writeText(QString text, int color)
{
    QString line = text;

    QTextCursor cursor = ui->terminal->textCursor();
    QString alertHtml = "<font color=\"red\">";
    QString notifyHtml = "<font color=\"blue\">";
    QString infoHtml = "<font color=\"green\">";
    QString endHtml = "</font><br>";

    switch(color)
    {
        case msg_alert: line.prepend(alertHtml.toLatin1()); break;
        case msg_notify: line.prepend(notifyHtml.toLatin1()); break;
        case msg_info: line.prepend(infoHtml.toLatin1()); break;
        default: line.prepend(infoHtml.toLatin1()); break;
    }

    line = line.append(endHtml.toLatin1());
    ui->terminal->putData(line);
    cursor.movePosition(QTextCursor::End);
    ui->terminal->setTextCursor(cursor);

}

void MainWindow::activarBotones(int idx)
{
    switch(idx)
    {
        case 0: ui->actionStart->setEnabled(true);ui->actionStop->setEnabled(true); break;
        case 1: ui->actionStart->setEnabled(true);ui->actionStop->setEnabled(true); break;
        case 2: ui->actionStart->setEnabled(false);ui->actionStop->setEnabled(false); break;
        case 3: ui->actionStart->setEnabled(false);ui->actionStop->setEnabled(false); break;
        default: QMessageBox::critical(this, "Error General__ X", "Se ha producido un error interno\ny el programa debe cerrarse"); exit(1); break;
    }
}

void MainWindow::bloquarPanel(bool val)
{
    int idx = ui->tabWidget->currentIndex();
    switch(idx)
    {
        case 0: ui->contenedorTab1->setEnabled(!val); break;
        case 1: ui->contenedorTab2->setEnabled(!val); break;
        //case 2: line.prepend(infoHtml.toLatin1()); break;
        //case 3: line.prepend(infoHtml.toLatin1()); break;
        default: QMessageBox::critical(this, "Error General__ X", "Se ha producido un error interno\ny el programa debe cerrarse"); exit(1); break;
    }

}
/* Fin funciones de control */




/* Zona de pruebas */
void MainWindow::on_pushButton_clicked()
{
    // algun codigo va aqui
}
/* Fin de la zona de pruebas */

