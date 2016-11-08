#include "proxysettings.h"
#include "ui_proxysettings.h"
#include <QNetworkProxy>


ProxySettings::ProxySettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProxySettings)
{
    ui->setupUi(this);
    init();
}

ProxySettings::~ProxySettings()
{
    delete ui;
}

void ProxySettings::init()
{
    /* Proxy SETTINGS: Carga el proxy al network manager */
    QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery();

    if (listOfProxies.count() !=0){
        if (listOfProxies.at(0).type() != QNetworkProxy::NoProxy)
        {
            ui->proxyHostName->setText(listOfProxies.at(0).hostName());
            ui->proxyPort->setText(QString::number(listOfProxies.at(0).port()));
            //QNetworkProxy::setApplicationProxy(listOfProxies.at(0));
        }
    }
}
