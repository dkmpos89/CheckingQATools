#include "paexec.h"
#include <QDir>
#include <QDebug>

const QString WORKING_DIR = QString(QDir::currentPath()+"/bin");
//const QString WORKING_DIR = QString("'"+QDir::currentPath()+"/bin'\n");
const QString CMD_BIN = QString("cmd");
//const QStringList ARGUMENTS = ( QStringList()<<"192.168.10.63"<<"-u checking"<<"-p chm.321."<<"-d D:/modelo_operativo_checking_4.2/certificacion_request.bat COREBANCO SERVICIOS_PLANES_AFINIDADES COREBANCO_CR_TSK_534 SPA" );
const QStringList ARGUMENTS = ( QStringList()<<"" );

//static member
PaExec* PaExec::procPExec = nullptr;


PaExec::PaExec(QObject* parent) : QProcess(parent)
{
    // Aqui va algo de codigo!
}

PaExec::~PaExec()
{
    procPExec->close();
    delete procPExec;
    qDebug()<<"procPexec destruido...";
}

bool PaExec::init()
{

    this->setProcessChannelMode( QProcess::SeparateChannels );
    this->start(CMD_BIN, ARGUMENTS, QProcess::ReadWrite);
    this->waitForStarted(5000);
    //this->setWorkingDirectory( WORKING_DIR );

    if( this->state() == QProcess::Running ){
        qInfo()<< "[Info] : CMD daemon running with PID " << this->pid() <<endl;
        this->write("cd "+WORKING_DIR.toUtf8());
        return true;
    }
    else{
        qWarning()<< "[Warn] : CMD daemon is not running " << endl;
        return false;
    }
}

PaExec* PaExec::getInstance() {

    if( procPExec == nullptr )
        procPExec = new PaExec();

    return procPExec;
}

void PaExec::writeCmd(const QString cmd)
{
    QByteArray ba = this->readAll();//clean stdout
    Q_UNUSED(ba);
    connect( this, SIGNAL( readyReadStandardOutput() ), this, SLOT( readOctaveOutput() ) );
    this->write(cmd.toUtf8());
}

void PaExec::readOctaveOutput()
{
    QString buff = QString( this->readAll() );
    if( buff.isEmpty() || buff.isNull() ){
        return;
    }
    disconnect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(readOctaveOutput()));

    //emit finalDataReady();

}

void PaExec::send(const QString s)
{
    QByteArray ba = this->readAll();//clean stdout
    Q_UNUSED(ba);

    this->write(s.toUtf8());

    this->waitForReadyRead(30000);

    QString buff = QString( this->readAll() );

    qDebug()<<buff<<endl;
}
