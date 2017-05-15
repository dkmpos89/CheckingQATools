#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "downloadmanager.h"
#include "proxysettings.h"
#include "xlsxdocument.h"
#include <QDebug>
#include <QDesktopWidget>
#include <QThread>
#include <QFileDialog>
#include <QMessageBox>
#include <QLoggingCategory>
#include <QWebFrame>
#include <QWebElement>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_sSettingsFile = QDir::currentPath()+"/config/checking_tools_settings.ini";

    loadSettings();

    /* Carga de configuraciones del servidor y seteo de cosas extras */
    chkproperties = new QSettings(QDir::currentPath()+"/config/checking_properties.ini", QSettings::IniFormat);
    chkproperties->beginGroup("servidor");
    /* End */
    init();

    QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");
    ui->webView_1->load(QUrl("http://checking:8080/checking/dashboard/view.run?category=projects"));

    /* Connect para el cambio de proyecto y atributo automatico */
    connect(ui->project_id_baseline, SIGNAL(currentIndexChanged(int)), ui->area_id_baseline, SLOT(setCurrentIndex(int)));
    connect(ui->project_id_requests, SIGNAL(currentIndexChanged(int)), ui->area_id_requests, SLOT(setCurrentIndex(int)));

    connect(this, SIGNAL(informeTerminado()), this, SLOT(loadWebReport()));

    connect(ui->tabWebReport, SIGNAL(tabCloseRequested(int)), this, SLOT(sceneTabRemove_slot(int)));
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
    /* QMEDIAPLAYER: crecacion del directorio 'sounds' y carga del archivo '.mp3' */
    QString soundspath = QDir::currentPath()+"/sounds";
    if(!QDir(soundspath).exists()){
        QDir().mkdir(soundspath);
        QFile::copy(":/sounds/eco_de_campanas.mp3", soundspath+"/eco_de_campanas.mp3");
    }

    /* SETTINGS: crecacion del directorio 'config' y carga de los archivos '.ini' */
    QString tmp="";
    QString path = QDir::currentPath()+"/config";
    if(!QDir(path).exists()){
        QDir().mkdir(path);
        QFile::copy(":/config/checking_tools_settings.ini", path+"/checking_tools_settings.ini");
        QFile::copy(":/config/tfuncional_config.ini", path+"/tfuncional_config.ini");
        QFile::copy(":/config/checking_properties.ini", path+"/checking_properties.ini");

        QFile::setPermissions(path+"/checking_tools_settings.ini" , QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
        QFile::setPermissions(path+"/tfuncional_config.ini" , QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
        QFile::setPermissions(path+"/checking_properties.ini" , QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
    }
    settings = new QSettings(m_sSettingsFile, QSettings::IniFormat);

    /* SETTINGS: carga del comboBox de ID DE PRODUCTO */
    settings->beginGroup("product");
    QStringList listaKeys = settings->childKeys();

    foreach (QString key, listaKeys) {
        tmp = settings->value(key).toString();
        ui->product_id_requests->addItem(tmp);
        ui->product_id_baseline->addItem(tmp);
    }
    settings->endGroup();

    /* SETTINGS: carga del comboBox de ID DE PROYECTO */

    settings->beginGroup("DIMPROJECTS");
    QStringList listaProject = settings->allKeys();

    foreach (QString key, listaProject) {
        tmp = settings->value(key).toString();
        ui->project_id_requests->addItem(key.toUpper());
        ui->project_id_baseline->addItem(key.toUpper());

        ui->area_id_requests->addItem(tmp);
        ui->area_id_baseline->addItem(tmp);
    }
    settings->endGroup();

    /* SETTINGS: carga del comboBox de trazabilodad funcional */
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

    /* Crear el sub-directorio de reportes */
    mkdirTemp(true, "reportes");

    /* Se intenta cargar el historial de analisis de Baselines desde el archivo /logBaselines.log y /logRequests.log */
    loadHistoryFile(QDir().currentPath()+"/logBaseline.log", ui->tablaResultadoBaselines);
    loadHistoryFile(QDir().currentPath()+"/logRequest.log", ui->tablaResultadoRequest);

}

void MainWindow::init()
{
    setConfigProgressBar(ui->progressBar_tab1, false, 0, 0);
    setConfigProgressBar(ui->progressBar_tab2, false, 0, 0);
    ui->btnGO->setEnabled(false);
    ui->btnEditar->setVisible(false);
    ui->label_anal_requests->setVisible(false);
    ui->label_anal_baseline->setVisible(false);
    ui->label_nombre_requests->setVisible(false);
    ui->label_nombre_baseline->setVisible(false);

/* Bloque de creacion de de reproductor de sonidos */
    player = new QMediaPlayer;
    QString pathToSound = QDir::currentPath()+"/sounds/"+chkproperties->value("soundname").toString()+"";;
    player->setMedia(QUrl::fromLocalFile(pathToSound));
    cvolume = chkproperties->value("soundvolume").toInt();
    player->setVolume(cvolume);
/* End */

/* Bloque de creacion de la variable analizador --> se mueve a otro hilo para no congelar la interfaz*/
    QThread* thread = new QThread(this);
    calc = new analizador(NULL);
    calc->moveToThread(thread);

    connect(ui->btnGO, &QPushButton::clicked, [=] () {calc->doHeavyCalculation(); } );
    connect(calc, SIGNAL(calculationCompleted(QStringList)), this, SLOT(imprimirSalida(QStringList)));
    connect(calc, &analizador::updateProgressbar, [=] (const int value) {ui->progressBar_tf->setValue(value); } );
    connect(ui->actionSalida_compuesta, &QAction::toggled, [=] (const bool val) {calc->setSalidaComp(val);} );
/* End */

/* Bloque de 'connect' para el cambio de pestañas */
    //connect(ui->tabWidget, &QTabWidget::currentChanged, [=] (const int idx) { activarBotones(idx, true);  });
/* End */

/* Bloque de 'connect' para el cambio de Script AIM */
    connect(ui->noAIM_requests, &QCheckBox::toggled, [=] (const bool cheked) { if(cheked) ui->name_script_requests->setText("x_certificacion_request_NOAIM.bat"); else ui->name_script_requests->setText("certificacion_request.bat");  });
    connect(ui->noAIM_baseline, &QCheckBox::toggled, [=] (const bool cheked) { if(cheked) ui->name_script_baseline->setText("x_certificacion_RDC_NOAIM.bat"); else ui->name_script_baseline->setText("certificacion_RDC.bat");  });
/* End */

/* Bloque de creaciones del binario ../bin/paexec.exe */
    QString path = QDir::currentPath()+"/bin";
    if(!QDir(path).exists()){
        QDir().mkdir(path);
        QFile::copy(":/bin/paexec.exe", path+"/paexec.exe");
        QFile::copy(":/bin/checkoutBaseline.groovy", path+"/checkoutBaseline.groovy");
        QFile::copy(":/bin/checkoutRequest.groovy", path+"/checkoutRequest.groovy");
        QFile::copy(":/bin/alsUtils.jar", path+"/alsUtils.jar");
        QFile::copy(":/bin/dmclient.jar", path+"/dmclient.jar");
        QFile::setPermissions(path+"/paexec.exe" , QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
        QFile::setPermissions(path+"/checkoutBaseline.groovy" , QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
        QFile::setPermissions(path+"/checkoutRequest.groovy" , QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
        QFile::setPermissions(path+"/alsUtils.jar" , QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
        QFile::setPermissions(path+"/dmclient.jar" , QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
    }
/* End */

/* Bloque de creaciones tabla Historial */
    ui->historialTWidget->setColumnCount(7);
    ui->historialTWidget->setHorizontalHeaderLabels(QStringList()<<"Date"<<"Product"<<"Project"<<"File Name"<<"URL"<<"Zip-file"<<"Status");
    ui->historialTWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->historialTWidget->horizontalHeader()->setResizeContentsPrecision(QHeaderView::Stretch);
/* End */

/* Bloque de carga de la tabla Historial de baselines */
    //
/* End */

}

QProcess* MainWindow::initProcess()
{
    //QThread* thread = new QThread(this);
    QProcess *proceso = new QProcess(this);
    //proceso->moveToThread(thread);

    proceso->setProcessChannelMode( QProcess::SeparateChannels );
    proceso->start(UPDATER, ARGUMENTS, QProcess::ReadWrite);
    proceso->waitForStarted(5000);

    QString cmd("cd "+WORKING_DIR+"\n");
    proceso->write(cmd.toLatin1());
    proceso->waitForFinished(1000);

    connect(proceso, SIGNAL( readyReadStandardOutput() ), this, SLOT(readOutput()) );
    connect(proceso, SIGNAL( readyReadStandardError() ), this, SLOT(readError()) );

    writeText("Nuevo proceso creado. ID: "+QString::number(proceso->processId()), msg_notify);

    QByteArray ba = proceso->readAll();//clean stdout
    Q_UNUSED(ba);

    listadeProcesos.append(proceso);
    return proceso;
}
/* Fin de las funciones de Inicializacion */


/* Funciones de los Menus y Botones */
void MainWindow::on_actionStart_triggered()
{
    QProcess *procPaExec, *prcGroovy;
    QString cmd = "";
    QString key = "";
    QStringList lista;
    idx = ui->tabWidget->currentIndex();
    QPlainTextEdit *output = nullptr;
    QString script = "";


    /* BLoque de carga de proyecto de checking */
    if(idx==2) {
        key = ui->area_id_requests->currentText();
        script_id = ui->name_script_requests->text();
        product_id = ui->product_id_requests->currentText();
        project_id = ui->project_id_requests->currentText();
        object_id = ui->requests_id->text().trimmed();
        area_id = ui->area_id_requests->currentText();
        output = ui->request_output;
        script = "checkoutRequest.groovy";
    }
    else if(idx==1){
        key = ui->area_id_baseline->currentText();
        script_id = ui->name_script_baseline->text();
        product_id = ui->product_id_baseline->currentText();
        project_id = ui->project_id_baseline->currentText();
        object_id = ui->baseline_name->text().trimmed();
        area_id = ui->area_id_baseline->currentText();
        output = ui->baseline_output;
        script = "checkoutBaseline.groovy";
    }

    /* [1] - Si se cumple con los requisitos se creará un proceso que se ejecuta en el servidor de chk-qa */
    if(validarCampos(idx)){

        /* [2] - Si está instalado groovy ejecuta una llamada a la api de dimensions par sacar las extensiones de los archivos */
        prcGroovy = initProcess();
        prcGroovy->setObjectName(QString::number(idx));
        if ((prcGroovy->state()==QProcess::Running))
        {
            output->clear();
            QString commd = "groovy "+script+" "+product_id+" "+project_id+" "+object_id+" "+area_id+"\n";
            disconnect(prcGroovy, SIGNAL( readyReadStandardOutput() ), this, SLOT(readOutput()) );

            prcGroovy->write(commd.toLatin1());
            prcGroovy->waitForFinished(1000);
            QByteArray ba = prcGroovy->readAll();//clean stdout
            Q_UNUSED(ba);

            //connect(prcGroovy, &QProcess::readyReadStandardOutput, [=]() { this->CheckoutDIM(idx); } );
            connect(prcGroovy, SIGNAL( readyReadStandardOutput() ), this, SLOT(CheckoutDIM()) );
        }else
            MsgBlOut("groovy is not ready!\n", output);

        procPaExec = initProcess();
        settings->beginGroup("projectchk");
        QString projecNameCHK = settings->value(key).toString();
        settings->endGroup();
        /* BLoque de carga de proyecto de checking */

        if(procPaExec->state()==QProcess::Running){
            if(idx==1){
                lista = ( QStringList()<<"1"<<projecNameCHK<<script_id<<product_id<<project_id<<object_id<<area_id<<object_id );
                cmd = "START /B /WAIT paexec.exe \\\\192.168.10.63 -u checking -p chm.321. D:\\modelo_operativo_checking_4.2\\"+lista[2]+" "+lista[3]+" "+lista[4]+" "+lista[5]+" "+lista[6]+" "+lista[7]+"\n";
            }else if(idx==2){
                lista = ( QStringList()<<"2"<<projecNameCHK<<ui->name_script_requests->text()<<ui->product_id_requests->currentText()<<ui->project_id_requests->currentText()<<ui->requests_id->text().trimmed()<<ui->area_id_requests->currentText() );
                cmd = "START /B /WAIT paexec.exe \\\\192.168.10.63 -u checking -p chm.321. D:\\modelo_operativo_checking_4.2\\"+lista[2]+" "+lista[3]+" "+lista[4]+" "+lista[5]+" "+lista[6]+"\n";
            }
        }else{
            writeText("^ERROR: El proceso de 'paExec'' no esta corriendo!", msg_alert);
            return;
        }
    }else{
        return;
    }
    /* [1] END */

    procPaExec->write(cmd.toLatin1());
    bloquarPanel(true,idx);
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
            ui->statusBar->showMessage("[ERROR] : No se han cargado los archivos Excel.", 5000);
        }
    }else{
        ui->statusBar->showMessage("[ERROR] : Faltan los nombreas de los procedimientos.", 5000);
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
    //
}

void MainWindow::on_btnFormato_clicked()
{
    ui->statusBar->showMessage("Aplicando el formato a las lineas de entrada....", 2000);
    QString m_SFile = QDir::currentPath()+"/config/checking_tools_settings.ini";
    QSettings *config = new QSettings(m_SFile, QSettings::IniFormat);
    QString txsalida = "";

    config->beginGroup("DIMPROJECTS");

    QString textIn = ui->inputPane->toPlainText();
    QStringList listIn;
    if(!textIn.isEmpty()){
        listIn<<textIn.split("\n");
        foreach (QString lbase, listIn) {
            if(!lbase.contains(";")){
                if(lbase.isEmpty()){}
                else{
                    QString tmpAtrib;
                    if(buscarLB(lbase, &tmpAtrib, config))
                        txsalida.append(lbase+";"+tmpAtrib+"\n");
                    else
                        txsalida.append(lbase+";----------\n");
                }
            }else{
                txsalida.append(lbase+"\n");
            }
        }
    }else{
        ui->statusBar->showMessage("Lista vacia!", 2000);
    }
    ui->inputPane->clear();
    ui->inputPane->appendPlainText(txsalida);
    config->endGroup();
}

void MainWindow::on_btnReloadBL_clicked()
{
    QItemSelectionModel *select = ui->tablaResultadoBaselines->selectionModel();
    QModelIndex index = select->currentIndex();

    if(select->hasSelection())
    {
        int row = index.row();
        QString dataURL = index.sibling(row, 2).data().toString();
        QString dataTipo = index.sibling(row, 1).data().toString();

        QLineEdit *URL = new QLineEdit(ui->tabWebReport);
        URL->setReadOnly(true);
        QWebView *report = new QWebView(ui->tabWebReport);

        QWidget *centralWidget = new QWidget();
        centralWidget->setLayout( new QGridLayout() );
        centralWidget->layout()->addWidget( URL );
        centralWidget->layout()->addWidget( report );
        int reid = ui->tabWebReport->addTab( centralWidget, dataTipo );

        URL->setText(dataURL);
        report->load(QUrl(dataURL));

        ui->tabWidget->setCurrentIndex(0);
        ui->tabWebReport->setCurrentIndex(reid);

    }
    else{
        QMessageBox::information(this, "Info:",
                              "Primero debes seleccionar un informe de la tabla.",
                              QMessageBox::Ok);
    }
}

void MainWindow::on_btnReloadR_clicked()
{
    QItemSelectionModel *select = ui->tablaResultadoRequest->selectionModel();
    QModelIndex index = select->currentIndex();

    if(select->hasSelection())
    {
        int row = index.row();
        QString dataURL = index.sibling(row, 2).data().toString();
        QString dataTipo = index.sibling(row, 1).data().toString();

        QLineEdit *URL = new QLineEdit(ui->tabWebReport);
        URL->setReadOnly(true);
        QWebView *report = new QWebView(ui->tabWebReport);

        QWidget *centralWidget = new QWidget();
        centralWidget->setLayout( new QGridLayout() );
        centralWidget->layout()->addWidget( URL );
        centralWidget->layout()->addWidget( report );
        int reid = ui->tabWebReport->addTab( centralWidget, dataTipo );

        URL->setText(dataURL);
        report->load(QUrl(dataURL));

        ui->tabWidget->setCurrentIndex(0);
        ui->tabWebReport->setCurrentIndex(reid);

    }
    else{
        QMessageBox::information(this, "Info:",
                              "Primero debes seleccionar un informe de la tabla.",
                              QMessageBox::Ok);
    }
}

/* Fin Menus y Botones */


/* Slots de control para los connect */
void MainWindow::update_Geometry()
{
    double px,py,w,h;
    QRect pantalla = this->geometry();
    px = pantalla.x();
    py = pantalla.y();

    int indice = ui->tabWidget->currentIndex();
    if(indice==2){
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

void MainWindow::readDotout()
{
    QObject *sender = QObject::sender();
    QProcess *procPaExec = qobject_cast<QProcess*>(sender);
    int idp = procPaExec->objectName().toInt();

    QString buff = QString::fromLatin1(procPaExec->readAllStandardOutput());
    if(buff.contains("ERR_TIMEOUT")){
        writeText("^ [Tiempo de espera agotado] - Estado del analisis: TIMEOUT", msg_alert);
        bloquarPanel(false, idp);
        CursorCarga(false, idp);
        player->play();
        return;
    }

    if(buff.contains("READYOUT")){
        writeText("^ [Archivo '.out' encontrado] - Estado del analisis: FINALIZADO", msg_notify);
        emit informeTerminado();
        if(buff.contains("FAILOUT"))
            writeText("^ [Archivo '.err' encontrado] - Estado del analisis: ERROR", msg_alert);
        bloquarPanel(false, idp);
        CursorCarga(false, idp);
        player->play();
    }
}

void MainWindow::bloquarPanel(bool val, int id)
{
    activarBotones(id, !val);
    switch(id)
    {
        case 1: ui->contenedorTab2->setEnabled(!val);
                ui->btnReloadBL->setEnabled(!val);
                break;
        case 2: ui->contenedorTab1->setEnabled(!val);
                ui->btnReloadR->setEnabled(!val);
                break;
        default: QMessageBox::critical(this, "___Error General___", "Se ha producido un error interno\ny el programa debe cerrarse"); exit(1); break;
    }

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

void MainWindow::sceneTabRemove_slot(int index)
{
    if(index>0){
        ui->tabWebReport->currentWidget()->deleteLater();
        ui->tabWebReport->removeTab(index);
    }
}

bool MainWindow::saveToHistory(QString filePath, QStringList data)
{
    QFile logFile(filePath);

    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append))
        return false;
    else{
        QTextStream inH(&logFile);
        for (int var = 0; var < data.size(); ++var) {
            if(var>0)
                inH<<";";
            inH<<data[var];
        }
        inH<<"\n";
        logFile.close();
        inH.flush();
    }
    return true;
}

bool MainWindow::loadHistoryFile(QString filePath, QTableWidget *tabla)
{
    QFile logFile(filePath);
    if (!logFile.open(QIODevice::ReadOnly))
        return false;
    else{
        QTextStream in(&logFile);
        while(!in.atEnd()){
            QString line = in.readLine();
            QStringList data = line.split(";");
            int nrows = tabla->rowCount();
            tabla->setRowCount(tabla->rowCount()+1);

            for (int var = 0; var < data.size(); ++var)
                tabla->setItem(nrows,var,new QTableWidgetItem(data[var]));
        }
        logFile.close();
    }
    return true;
}

/* Fin de Slots de control */


/* Funciones de control */
void MainWindow::activarBotones(int id, bool b)
{
    switch(id)
    {
        case 0: break;
        case 1: ui->actionStart->setEnabled(b);ui->actionStop->setEnabled(b); break;
        case 2: ui->actionStart->setEnabled(b);ui->actionStop->setEnabled(b); break;
        case 3: break;
        default: break;
    }
}

void MainWindow::setConfigProgressBar(QProgressBar *pg, bool bp, int min, int max){
    pg->setVisible(bp);
    pg->setRange(min, max);

    switch(idx)
    {
        case 1: ui->label_anal_baseline->setVisible(bp);
                ui->label_nombre_baseline->setText(ui->baseline_name->text().trimmed());
                ui->label_nombre_baseline->setVisible(bp);
                break;
        case 2: ui->label_anal_requests->setVisible(bp);
                ui->label_nombre_requests->setText(ui->requests_id->text().trimmed());
                ui->label_nombre_requests->setVisible(bp);
                break;
        default: ui->statusBar->showMessage("Error pestaña no configurada.. ", 1000); break;
    }


}

bool MainWindow::validarCampos(int id)
{
    bool mflag = true;
    QString area = "";
    QString current_proj = "";
    QString imputData = "";
    QString areaproject = "";

    switch(id)
    {
        case 1: area = ui->area_id_baseline->currentText(); current_proj = ui->project_id_baseline->currentText(); imputData = ui->baseline_name->text().trimmed(); break;
        case 2: area = ui->area_id_requests->currentText(); current_proj = ui->project_id_requests->currentText(); imputData = ui->requests_id->text().trimmed(); break;
        default: QMessageBox::warning(this,"CHKTools : ERROR!", "Imposible realizar el paso de validación!" ); return false; break;
    }

    //settings = new QSettings(m_sSettingsFile, QSettings::IniFormat);
    settings->beginGroup("DIMPROJECTS");
    areaproject = settings->value(current_proj).toString();
    settings->endGroup();

    if(imputData.isEmpty()){ mflag = false; QMessageBox::warning(this,"[ERROR]", "Faltan datos");}

    //qInfo()<<area<<" - "<<current_proj<<" - "<<project<<endl;
    if(area!=areaproject){ mflag = false; QMessageBox::warning(this,"[ERROR]", "El area: "+area+" no coincide con el proyecto: "+current_proj);}

    return mflag;

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

void MainWindow::CursorCarga(bool b, int id)
{
    switch(id)
    {
        case 1: setConfigProgressBar(ui->progressBar_tab2, b, 0, 0);
                break;
        case 2: setConfigProgressBar(ui->progressBar_tab1, b, 0, 0);
                break;
        default: break;
    }

}

void MainWindow::checkearSalida(QStringList arg)
{

    ui->statusBar->showMessage("Analizando: "+arg[5]+" por favor espere....", 5000);
    QProcess *proSalida = initProcess();
    disconnect(proSalida, SIGNAL( readyReadStandardOutput() ), this, SLOT(readOutput()) );
    connect(proSalida, SIGNAL( readyReadStandardOutput() ), this, SLOT(readDotout()) );

    QString cmd = "START /B /WAIT "+chkproperties->value("exepa").toString()+" \\\\"+chkproperties->value("ip").toString()+" -u "+chkproperties->value("user").toString()+" -p "+chkproperties->value("pass").toString()+" "+chkproperties->value("pathmo").toString()+chkproperties->value("existfile").toString()+" "+arg[3]+" "+arg[4]+" "+arg[5]+" "+arg[1]+"\n";
    writeText(cmd, msg_info);
    proSalida->setObjectName(arg[0]);
    proSalida->write(cmd.toLatin1());

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

void MainWindow::mkdirTemp(bool f, QString dir)
{
    writeText("Directorio temporal ../../"+dir+"", msg_notify);
    QString path = QDir::currentPath()+"/"+dir+"";

    if(f){
        if(!QDir(path).exists())
            QDir().mkdir(path);
        writeText("^ [OK]", msg_info);
    }else{
        if(QDir(path).exists())
            QDir().rmdir(path);
        writeText("^ [NO]", msg_info);
    }
}

bool MainWindow::buscarLB(QString lbase, QString *tmpA, QSettings *config)
{
    QStringList projects = config->childKeys();
    foreach (QString cproj, projects) {
        if(lbase.contains(cproj)){
            *tmpA = config->value(cproj).toString();
            return true;
        }
    }
    return false;
}

void MainWindow::MsgBlOut(QString msg, QPlainTextEdit *QP)
{
    QP->insertPlainText(msg);
}

void MainWindow::CheckoutDIM()
{
    int ini = 0;
    int fin = -1;
    QStringList lista_lineas;
    QObject *sender = QObject::sender();
    QProcess *procPaExec = qobject_cast<QProcess*>(sender);
    QString buff = QString::fromLatin1(procPaExec->readAllStandardOutput());
    QPlainTextEdit *currPTEdit;
    int id = procPaExec->objectName().toInt();

    if(id == 1)
        currPTEdit = ui->baseline_output;
    else
        currPTEdit = ui->request_output;

    MsgBlOut(buff, currPTEdit);
    output__dm.append(buff);

    if(buff.contains("PROCESO-TERMINADO-SUCCESS"))
    {
        output__dm.replace("\r\n", "\n");
        lista_lineas = output__dm.split("\n");

        /* Desde [INICIO] hasta [FIN] estan los nombres de los items de la linea base */
        ini = lista_lineas.lastIndexOf("[INICIO]");
        fin = lista_lineas.lastIndexOf("[FIN]");
        int tam = fin - ini;

        /* Se calcula la sub lista de los nombres de los items y se obtiene sus extensiones */
        disconnect(procPaExec, SIGNAL( readyReadStandardOutput() ), this, SLOT(CheckoutDIM()) );
        getExtensiones(lista_lineas.mid(ini, tam));
        return;

    }else if(buff.contains("PROCESO-TERMINADO-FAIL")){
        MsgBlOut("[ERROR] No se pueden descargar los componentes desde Dimensions.", currPTEdit);
        disconnect(procPaExec, SIGNAL( readyReadStandardOutput() ), this, SLOT(CheckoutDIM()) );
        return;
    }
}

void MainWindow::getExtensiones(QStringList lt)
{
    QTableWidget *tablaSalida;
    QString logPath = "";
    QString fecha = QDateTime(QDate().currentDate()).toString("dd-MM-yyyy");
    int nrows = 0;
    lt.pop_front(); // quitamos la etiqueta [INICIO] al principio del QStringList
    mapURLs.clear();

    //[projectchk]
    settings->beginGroup("projectchk");
    QString projecNameCHK = settings->value(area_id).toString();
    settings->endGroup();

    for(QString it : lt)
    {
        QString ext = it.mid(it.lastIndexOf("."));

        if(LIST_PLSQL.contains(ext)) mapURLs.insert("plsql", BASE_URL+projecNameCHK+"/"+product_id+"/"+project_id+"/"+object_id+"/reportplsql.html");
        if(LIST_JAVA.contains(ext)) mapURLs.insert("java", BASE_URL+projecNameCHK+"/"+product_id+"/"+project_id+"/"+object_id+"/reportjava.html");
        if(LIST_ACTIONSCRIPT.contains(ext)) mapURLs.insert("actionscript", BASE_URL+projecNameCHK+"/"+product_id+"/"+project_id+"/"+object_id+"/reportactionscript.html");
        if(LIST_COBOL.contains(ext)) mapURLs.insert("cobol", BASE_URL+projecNameCHK+"/"+product_id+"/"+project_id+"/"+object_id+"/reportcobol.html");
        if(LIST_ASP.contains(ext)) mapURLs.insert("asp", BASE_URL+projecNameCHK+"/"+product_id+"/"+project_id+"/"+object_id+"/reportasp.html");
        if(LIST_JSP.contains(ext)) mapURLs.insert("jsp", BASE_URL+projecNameCHK+"/"+product_id+"/"+project_id+"/"+object_id+"/reportjsp.html");
        if(LIST_JAVASCRIPT.contains(ext)) mapURLs.insert("javascript", BASE_URL+projecNameCHK+"/"+product_id+"/"+project_id+"/"+object_id+"/reportjavascript.html");

    }
    if(idx==1){
        tablaSalida = ui->tablaResultadoBaselines;
        logPath = QDir().currentPath()+"/logBaseline.log";
    }
    else {
        tablaSalida = ui->tablaResultadoRequest;
        logPath = QDir().currentPath()+"/logRequest.log";
    }

    QMapIterator<QString, QString> i(mapURLs);
    while (i.hasNext()) {
        i.next();
        nrows = tablaSalida->rowCount();
        tablaSalida->setRowCount(tablaSalida->rowCount()+1);

        tablaSalida->setItem(nrows,0,new QTableWidgetItem(fecha));
        tablaSalida->setItem(nrows,1,new QTableWidgetItem(i.key()));
        tablaSalida->setItem(nrows,2,new QTableWidgetItem(i.value()));

        saveToHistory(logPath, (QStringList()<<fecha<<i.key()<<i.value()));

    }

}

void MainWindow::loadWebReport()
{
    QMapIterator<QString, QString> i(mapURLs);
    while (i.hasNext()) {
        i.next();

        QLineEdit *URL = new QLineEdit(ui->tabWebReport);
        URL->setReadOnly(true);
        QWebView *report = new QWebView(ui->tabWebReport);

        QWidget *centralWidget = new QWidget();
        centralWidget->setLayout( new QGridLayout() );
        centralWidget->layout()->addWidget( URL );
        centralWidget->layout()->addWidget( report );
        ui->tabWebReport->addTab( centralWidget, i.key() );

        URL->setText(i.value());
        report->load(QUrl(i.value()));

    }
}
///* Fin funciones de control *///




/* Zona de pruebas */
void MainWindow::on_actionGet_triggered()
{
    DownloadManager *downManager = new DownloadManager(this);
    //QString url("https://github.com/dkmpos89/softEGM_updates/raw/master/soft-updates.zip");
    QString url("https://raw.githubusercontent.com/dkmpos89/softEGM_updates/master/soft-emg-version.xml");

    const QUrl iUrl(url);
    downManager->setProxy("192.168.10.52", 80);
    downManager->doDownload(iUrl);

    connect(downManager, SIGNAL(downFinished(bool, QString)), this, SLOT(mostrarSalidaDM(bool, QString)) );

    writeText("Descargando archivo en: "+QDir::currentPath()+"/temp", msg_notify);
}

void MainWindow::on_actionTest_triggered()
{
    player->setVolume(cvolume);
    player->play();

}

void MainWindow::mostrarSalidaDM(bool b, QString s)
{
    if(b) writeText("^ ["+s+" ]",msg_info);
    else writeText("^ ["+s+" ]",msg_alert);
}

void MainWindow::on_actionProxy_Settings_triggered()
{
    ProxySettings *proxyssetings = new ProxySettings;
    proxyssetings->exec();
}


void MainWindow::getInfoFromReport()
{
    view = new QWebView();
    QString urlString("http://checking:8080/report/qaking/Dimensions/Admision_Banco/COREBANCO/ADMISION_BANCO/COREBANCO_CR_PPROD_100/reportactionscript.html");
    // Load the page
    view->load(QUrl(urlString));
    connect(view, SIGNAL(loadFinished(bool)), this, SLOT(showDetails(bool)));

}
void MainWindow::showDetails(bool status)
{
    if(status){
        disconnect(view, SIGNAL(loadFinished(bool)), this, SLOT(showDetails(bool)));
        QWebElement e = view->page()->mainFrame()->findFirstElement("div#sumreg_layer > ul");
        QString pageString = e.isNull() ? "No se encontro el reporte" : e.toPlainText();
        ui->baseline_output->insertPlainText(pageString+"\n");
        ui->baseline_output->insertPlainText("TERMINADO\n");
    }else{
        ui->baseline_output->insertPlainText("Error #404\n");
    }

}

/* Fin de la zona de pruebas */






void MainWindow::on_btnIniciarAct_clicked()
{
    QProcess *procPaExec;
    QString cmd = "";
    cmd = "START /B /WAIT paexec.exe \\\\192.168.10.63 -u checking -p chm.321. -w D:\\dimensions\\pendiente\\ -s cmd.exe\n";
    procPaExec = initProcess();
    procPaExec->write(cmd.toLatin1());
    procPaExec->waitForStarted(3000);
    procPaExec->write("DIR /B\n");

}

void MainWindow::on_actionDoTest_triggered()
{

//    ui->baseline_name->setText("FISA_NOCOBROFUNC_20170418_OBJ");
//    ui->project_id_baseline->setCurrentIndex(13);
//    ui->area_id_baseline->setCurrentIndex(13);
//    ui->noAIM_baseline->setChecked(true);

    getInfoFromReport();

    //on_actionStart_triggered();

//    QByteArray listOFitems = qgetenv("LIST_OF_ITEMS");
//    QString StringItems(listOFitems);
//    qInfo()<< "getENV: "+StringItems<<endl;
//    qInfo()<< "Process-ENV: "+QProcessEnvironment::systemEnvironment().value("LIST_OF_ITEMS") <<endl;

}




