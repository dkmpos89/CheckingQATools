#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QDebug>
#include <QDesktopWidget>
#include <QThread>
#include <QTimer>
#include <QFileDialog>
#include "xlsxdocument.h"


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
}

void MainWindow::loadSettings()
{
    QString tmp="";
    QString path = QDir::currentPath()+"/config";
    if(!QDir(path).exists()){
        QDir().mkdir(path);
        QFile::copy(":/config/checking_tools_settings.ini", path+"/checking_tools_settings.ini");
        QFile::setPermissions(path+"/checking_tools_settings.ini" , QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther);
    }
    settings = new QSettings(m_sSettingsFile, QSettings::IniFormat);

    settings->beginGroup("product");
    QStringList listaKeys = settings->childKeys();

    foreach (QString key, listaKeys) {
        tmp = settings->value(key).toString();
        ui->product_id->addItem(tmp);
    }
    settings->endGroup();

    settings->beginGroup("project");
    QStringList listaProject = settings->childKeys();

    foreach (QString key, listaProject) {
        tmp = settings->value(key).toString();
        ui->project_id->addItem(tmp);
        ui->area_id->addItem(key);
    }
    settings->endGroup();

    settings->beginGroup("rutas_admision");
    ui->planillaExcel->setText(settings->value("planilla").toString());
    ui->funcionalidadExcel->setText(settings->value("funcionalidad").toString());
    settings->endGroup();
}

void MainWindow::init()
{
    ui->progressBar_tab1->setVisible(false);
    ui->btnGO->setEnabled(false);
    ui->btnEditar->setVisible(false);

/* Bloque de creacion de la variable analizador --> se mueve a otro hilo para no congelar la interface*/
    QThread* thread = new QThread(this);
    calc = new analizador(NULL);
    calc->moveToThread(thread);

    connect(ui->btnGO, &QPushButton::clicked, [=] () {calc->doHeavyCalculation(); } );
    connect(calc, SIGNAL(calculationCompleted(QStringList)), this, SLOT(imprimirSalida(QStringList)));
    connect(calc, &analizador::updateProgressbar, [=] (const int value) {ui->progressBar_tf->setValue(value); } );
    connect(ui->actionSalida_compuesta, &QAction::toggled, [=] (const bool val) {calc->setSalidaComp(val);} );
/* End */

/* Bloque de Conncet para el cambio de pestañas */
    connect(ui->tabWidget, &QTabWidget::currentChanged, [=] (const int idx) {
        if(idx==2)
            ui->panel_trazabilidad->show();
        else
            ui->panel_trazabilidad->hide();
        QTimer::singleShot(100, this, SLOT(update_Geometry()));
    });
/* End */

}

void MainWindow::update_Geometry()
{
    QDesktopWidget *desktop = QApplication::desktop();
    QRect pantalla = desktop->screenGeometry();
    double px,w,h,py;
    int idx = ui->tabWidget->currentIndex();
    if(idx==2){
        ui->panel_trazabilidad->show();
        w = 815;
        h = 600;
    }else{
        ui->panel_trazabilidad->hide();
        w = 815;
        h = 220;
    }
    px  = (pantalla.width()/2) - w/2;
    py = (pantalla.height()/2) - h/2;

    this->setGeometry(px,py,w,h);
}

void MainWindow::on_actionStart_triggered()
{
   //ui->panel_trazabilidad->setVisible(false);
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
    QString path,tipo,tipoFile,comboText;
    tipoFile = "xlsx";
    tipo = ( tipoFile.toUpper() )+" Files (*."+(tipoFile.toLower())+")";
    path = QFileDialog::getOpenFileName(this, "Abrir Archivo:", QDir::homePath(),tipo);

    if(path != NULL){
        ui->planillaExcel->setText(path);
        comboText = ui->comboRutas->currentText();
        settings->beginGroup(comboText);
        settings->setValue("planilla", path);
        settings->endGroup();
    }
}

void MainWindow::on_toolFuncionalidad_clicked()
{
    QString path,tipo,tipoFile,comboText;
    tipoFile = "xlsx";
    tipo = ( tipoFile.toUpper() )+" Files (*."+(tipoFile.toLower())+")";
    path = QFileDialog::getOpenFileName(this, "Abrir Archivo:", QDir::homePath(),tipo);

    if(path != NULL){
        ui->funcionalidadExcel->setText(path);
        comboText = ui->comboRutas->currentText();
        settings->beginGroup(comboText);
        settings->setValue("funcionalidad", path);
        settings->endGroup();
    }
}

void MainWindow::imprimirSalida(QStringList lista)
{
    ui->textAreaSalida->clear();
    ui->textAreaSalida->appendPlainText(lista.join("\n"));
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
