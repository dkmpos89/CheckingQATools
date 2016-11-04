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
    //ui->panel_trazabilidad->hide();
    //QTimer::singleShot(100, this, SLOT(update_Geometry()));
}

MainWindow::~MainWindow()
{
    delete ui;
    for (int i = 0; i < listadeProcesos.size(); ++i) {
        listadeProcesos[i]->kill();
        listadeProcesos[i]->close();
    }
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
        ui->product_id_requests->addItem(tmp);
        ui->product_id_baseline->addItem(tmp);
    }
    settings->endGroup();

    /* SETTINGS: comboBox de ID DE PROYECTO */
    settings->beginGroup("project");
    QStringList listaProject = settings->childKeys();

    foreach (QString key, listaProject) {
        tmp = settings->value(key).toString();
        ui->project_id_requests->addItem(tmp);
        ui->project_id_baseline->addItem(tmp);

        ui->area_id_requests->addItem(key);
        ui->area_id_baseline->addItem(key);
    }
    settings->endGroup();

    /* SETTINGS: comboBox de trazabilodad funcional */
    QString m_SFile = QDir::currentPath()+"/config/tfuncional_config.ini";
    QSettings *config = new QSettings(m_SFile, QSettings::IniFormat);
    QStringList listaGrupos = config->childGroups();

    foreach (QString key, listaGrupos) {
        ui->comboRutas_TF->addItem(key);
    }
    config->beginGroup(listaGrupos[0]);
    ui->planillaExcel->setText(config->value("planilla").toString());
    ui->funcionalidadExcel->setText(config->value("funcionalidad").toString());
    config->endGroup();
}

void MainWindow::init()
{
    setConfigProgressBar(ui->progressBar_tab1, false, 0, 0);
    setConfigProgressBar(ui->progressBar_tab2, false, 0, 0);
    ui->btnGO->setEnabled(false);
    ui->btnEditar->setVisible(false);

/* Bloque de creacion de proceso paexec */
    procPaExec = initProcess();
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
    connect(ui->tabWidget, &QTabWidget::currentChanged, [=] (const int idx) { activarBotones(idx);  });
/* End */

/* Bloque de creaciones del binario ../bin/paexec.exe */
    QString path = QDir::currentPath()+"/bin";
    if(!QDir(path).exists()){
        QDir().mkdir(path);
        QFile::copy(":/bin/paexec.exe", path+"/paexec.exe");
        QFile::setPermissions(path+"/paexec.exe" , QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
    }
/* End */

/* Bloque de creaciones tabla Historial */
    ui->historialTWidget->setColumnCount(7);
    ui->historialTWidget->setHorizontalHeaderLabels(QStringList()<<"Date"<<"Product"<<"Project"<<"File Name"<<"URL"<<"Zip-file"<<"Status");
    ui->historialTWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->historialTWidget->horizontalHeader()->setResizeContentsPrecision(QHeaderView::Stretch);
/* End */
}

QProcess* MainWindow::initProcess()
{
    QProcess *proceso = new QProcess(this);
    proceso->setProcessChannelMode( QProcess::SeparateChannels );
    proceso->start(UPDATER, ARGUMENTS, QProcess::ReadWrite);
    proceso->waitForStarted(5000);

    QString cmd("cd "+WORKING_DIR+"\n");
    proceso->write(cmd.toLatin1());
    proceso->waitForFinished(1000);
    QByteArray ba = proceso->readAll();//clean stdout
    Q_UNUSED(ba);

    connect(proceso, SIGNAL( readyReadStandardOutput() ), this, SLOT(readOutput()) );
    connect(proceso, SIGNAL( readyReadStandardError() ), this, SLOT(readError()) );

    writeText("^ [Proceso inicializado. ID: "+QString::number(proceso->processId())+"]", msg_notify);

    listadeProcesos.append(proceso);
    return proceso;
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
        w = 900;
        h = 600;
    }else{
        ui->panel_trazabilidad->hide();
        w = 900;
        h = 220;
    }
    this->setGeometry(px,py,w,h);
}
/* Fin de las funciones de Inicializacion */


/* Funciones de los Menus y Botones */
void MainWindow::on_actionStart_triggered()
{
    QString cmd = "";
    QStringList lista;
    int idx = ui->tabWidget->currentIndex();

    /* BLoque de carga de proyecto de checking */
    QString key = "";
    if(idx==0) key = ui->area_id_requests->currentText();
    else if(idx==1) key = ui->area_id_baseline->currentText();

    settings->beginGroup("projectchk");
    QString projecNameCHK = settings->value(key).toString();
    settings->endGroup();
    /* BLoque de carga de proyecto de checking */

    if(procPaExec->state()==QProcess::Running){
        if(validarCampos(idx)){
            if(idx==0){
                lista = ( QStringList()<<projecNameCHK<<ui->name_script_requests->text()<<ui->product_id_requests->currentText()<<ui->project_id_requests->currentText()<<ui->requests_id->text()<<ui->area_id_requests->currentText() );;
                cmd = "paexec.exe \\\\192.168.10.63 -u checking -p chm.321. -d D:\\modelo_operativo_checking_4.2\\"+lista[1]+" "+lista[2]+" "+lista[3]+" "+lista[4]+" "+lista[5]+"\n";
            }else if(idx==1){
                lista = ( QStringList()<<projecNameCHK<<ui->name_script_baseline->text()<<ui->product_id_baseline->currentText()<<ui->project_id_baseline->currentText()<<ui->baseline_name->text()<<ui->area_id_baseline->currentText()<<ui->baseline_name->text() );;
                cmd = "paexec.exe \\\\192.168.10.63 -u checking -p chm.321. -d D:\\modelo_operativo_checking_4.2\\"+lista[1]+" "+lista[2]+" "+lista[3]+" "+lista[4]+" "+lista[5]+" "+lista[6]+"\n";
            }
        }else
            return;
    }else{
        writeText("^ERROR: El proceso de 'paExec'' no esta corriendo!", msg_alert);
        return;
    }


    //procPaExec->write(cmd.toLatin1());
    bloquarPanel(true);
    CursorCarga(true, idx);
    checkearSalida(lista);
}

void MainWindow::on_btnEditar_clicked()
{
    ui->btnSetear->setVisible(true);
    ui->btnEditar->setVisible(false);
    ui->textAreaEntrada->setEnabled(true);
    ui->btnGO->setEnabled(false);
    ui->btnSetear->setEnabled(true);
    ui->progressBar_tf->setValue(0);
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
    QObject *sender = QObject::sender();
    QProcess *procPaExec = qobject_cast<QProcess*>(sender);

    QString buff = QString::fromLatin1(procPaExec->readAllStandardOutput());
    writeText(buff, Qt::green);
}

void MainWindow::readError()
{
    QObject *sender = QObject::sender();
    QProcess *procPaExec = qobject_cast<QProcess*>(sender);

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
        default: break;
    }
}

void MainWindow::bloquarPanel(bool val)
{
    ui->menuBar->setEnabled(!val);
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

void MainWindow::setConfigProgressBar(QProgressBar *pg, bool bp, int min, int max){
    pg->setVisible(bp);
    pg->setRange(min, max);
}

bool MainWindow::validarCampos(int idx)
{
    bool mflag = true;
    QString area = "";
    QString current_proj = "";
    QString imputData = "";
    QString project = "";

    switch(idx)
    {
        case 0: area = ui->area_id_requests->currentText(); current_proj = ui->project_id_requests->currentText(); imputData = ui->requests_id->text(); break;
        case 1: area = ui->area_id_baseline->currentText(); current_proj = ui->project_id_baseline->currentText(); imputData = ui->baseline_name->text(); break;
        default: QMessageBox::warning(this,"CHKTools : ERROR!", "Imposible realizar el paso de validación!" ); return false; break;
    }

    //settings = new QSettings(m_sSettingsFile, QSettings::IniFormat);
    settings->beginGroup("project");
    project = settings->value(area).toString();
    settings->endGroup();

    if(imputData.isEmpty()){ mflag = false; QMessageBox::warning(this,"CHKTools : ERROR!", "Faltan datos!");}

    qInfo()<<area<<" - "<<current_proj<<" - "<<project<<endl;
    if(current_proj!=project){ mflag = false; QMessageBox::warning(this,"CHKTools : ERROR!", "El area: "+area+" no coincide con el proyecto: "+current_proj);}

    return mflag;

}

void MainWindow::downloadFinished(QNetworkReply *reply)
{
    QString msg;
    QUrl url = reply->url();
    if (reply->error()) {
        writeText("Download failed: "+reply->errorString()+"\n", msg_alert);
        //emit downFinished(false, msg);
    } else {
        QString filename = saveFileName(url);
        QString path = QDir::currentPath()+"/temp";
        QDir().mkdir(path);
        filename.prepend(QDir::currentPath()+"/temp/");
        if (saveToDisk(filename, reply))
            writeText("Download succeeded (saved to "+filename+")\n", msg_alert);
        //emit downFinished(true, msg);
    }
    reply->deleteLater();
}

QString MainWindow::saveFileName(const QUrl &url)
{
    QString path = url.path();
    QString basename = QFileInfo(path).fileName();

    if (basename.isEmpty())
        basename = "download";

    if (QFile::exists(basename)) {
        // already exists, don't overwrite
        int i = 0;
        basename += '.';
        while (QFile::exists(basename + QString::number(i)))
            ++i;

        basename += QString::number(i);
    }
    return basename;
}

bool MainWindow::saveToDisk(const QString &filename, QIODevice *data)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        fprintf(stderr, "Could not open %s for writing: %s\n",
                qPrintable(filename),
                qPrintable(file.errorString()));
        return false;
    }

    file.write(data->readAll());
    file.close();

    return true;
}

void MainWindow::addToHistorial(QStringList data)
{
    QString proyecto = "buscarProyecto(atr)"; //Se debe buscar el nombre del proyecto que tiene en checking asociado al atributo --> idealmente se deberia buscar en algun archivo de cnfiguraciones donde este la lista de los nombres.
    QString url = "http://checking:8080/report/auditrep/Dimensions/"+proyecto+"/"+data[0]+"/"+data[1]+"/"+data[2]+"/auditrep.html";

    int rowcont = ui->historialTWidget->rowCount();
    ui->historialTWidget->setRowCount(rowcont+1);

    QString fecha_actual = QDateTime::currentDateTime().toString("dd.MM.yyyy - hh:mm");

    QToolButton *icotool = new QToolButton();
    icotool->setIcon(QIcon(QPixmap(":/images/wr-icon.png")));
    icotool->setIconSize(QSize(30,30));
    icotool->setFixedHeight(30);
    icotool->setFixedWidth(30);
    icotool->setAutoRaise(true);
    icotool->setText(QString::number(rowcont));

    QLabel *urlabel = new QLabel();
    urlabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    urlabel->setOpenExternalLinks(true);
    urlabel->setTextFormat(Qt::RichText);
    urlabel->setText("<a href=\""+url+"/\">Informe de violaciones</a>");

    ui->historialTWidget->setItem(rowcont, 0, new QTableWidgetItem(fecha_actual));
    ui->historialTWidget->setItem(rowcont, 1, new QTableWidgetItem(data[0]));
    ui->historialTWidget->setItem(rowcont, 2, new QTableWidgetItem(data[1]));
    ui->historialTWidget->setItem(rowcont, 3, new QTableWidgetItem(data[2]));
    ui->historialTWidget->setCellWidget(rowcont, 4, urlabel);
    ui->historialTWidget->setCellWidget(rowcont, 5, icotool);
    ui->historialTWidget->setItem(rowcont, 6, new QTableWidgetItem("En ejecucion Checking-QA"));

}

void MainWindow::CursorCarga(bool b, int idx)
{
    switch(idx)
    {
        case 0: setConfigProgressBar(ui->progressBar_tab1, b, 0, 0); break;
        case 1: setConfigProgressBar(ui->progressBar_tab2, b, 0, 0); break;
        default: break;
    }

}

void MainWindow::checkearSalida(QStringList arg)
{
    ui->statusBar->showMessage("Analizando: "+arg[4]+" por favor espere....", 5000);
    QProcess *proSalida = initProcess();

    //QStringList lista = ( QStringList()<<"\\\\192.168.10.63"<<"-u checking"<<"-p chm.321."<<"D:\\modelo_operativo_checking_4.2\\x_existfile.bat"<<"RDC"<<"TEF"<<"BOTEF_ADM_SAVSEG_PP_TEF_2925_V3"<<"TEF" );
    QString cmd = "paexec.exe \\\\192.168.10.63 -u checking -p chm.321. D:\\modelo_operativo_checking_4.2\\x_existfile.bat "+arg[2]+" "+arg[3]+" "+arg[4]+" "+arg[5]+"\n";
    proSalida->write(cmd.toLatin1());

}

/* Fin funciones de control */




/* Zona de pruebas */
void MainWindow::on_btnTest_clicked()
{
    QStringList lista = ( QStringList()<<"\\\\192.168.10.63"<<"-u checking"<<"-p chm.321."<<"D:\\modelo_operativo_checking_4.2\\x_existfile.bat"<<"RDC"<<"TEF"<<"BOTEF_ADM_SAVSEG_PP_TEF_2925_V3"<<"TEF" );

    QString cmd = "paexec.exe \\\\192.168.10.63 -u checking -p chm.321. D:\\modelo_operativo_checking_4.2\\x_existfile.bat "+lista[4]+" "+lista[5]+" "+lista[6]+" "+lista[7]+"\n";

    procPaExec->write(cmd.toLatin1());

}

void MainWindow::on_actionGet_triggered()
{
    QUrl iUrl("https://github.com/dkmpos89/softEGM_updates/raw/master/soft-updates.zip");
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QNetworkProxyQuery npq(iUrl);
    QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);

    if (listOfProxies.count() !=0){
        if (listOfProxies.at(0).type() != QNetworkProxy::NoProxy)    {
            manager->setProxy(listOfProxies.at(0));
            writeText( "Using Proxy " + listOfProxies.at(0).hostName()+"\n", msg_info);
        }
    }

    connect(manager, &QNetworkAccessManager::finished, [=](QNetworkReply* rep) { downloadFinished(rep); } );

    manager->get(QNetworkRequest(iUrl));

    writeText("Descargando en .."+QDir::currentPath()+"/temp", msg_alert);
}
/* Fin de la zona de pruebas */


