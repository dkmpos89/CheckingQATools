#include "downloadmanager.h"
#include <QLoggingCategory>
#include <QNetworkProxy>
#include <QDir>

DownloadManager::DownloadManager(QObject *parent) : QObject(parent)
{
    QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");
    connect(&manager, SIGNAL(finished(QNetworkReply*)), SLOT(downloadFinished(QNetworkReply*)));
}

void DownloadManager::setProxy(QString HostName, qint16 port)
{
//    QNetworkProxy myProxy;
//    myProxy.setHostName(HostName);
//    myProxy.setPort(port);
//    manager.setProxy(myProxy);

    (void)HostName;
    (void)port;


    /* Proxy SETTINGS: Carga el proxy al network manager */
    QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery();

    if (listOfProxies.count() !=0){
        if (listOfProxies.at(0).type() != QNetworkProxy::NoProxy)
        {
            //QNetworkProxy::setApplicationProxy(listOfProxies.at(0));
            manager.setProxy(listOfProxies.at(0));
        }
    }
}

void DownloadManager::doDownload(const QUrl &url)
{
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

#ifndef QT_NO_SSL
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(sslErrors(QList<QSslError>)));
#endif

    currentDownloads.append(reply);
}

QString DownloadManager::saveFileName(const QUrl &url)
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

bool DownloadManager::saveToDisk(const QString &filename, QIODevice *data)
{
    //filename.prepend((QDir::currentPath()+"/reportes/").toLatin1());
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

void DownloadManager::sslErrors(const QList<QSslError> &sslErrors)
{
#ifndef QT_NO_SSL
    foreach (const QSslError &error, sslErrors)
        fprintf(stderr, "SSL error: %s\n", qPrintable(error.errorString()));
#else
    Q_UNUSED(sslErrors);
#endif
}

void DownloadManager::downloadFinished(QNetworkReply *reply)
{
    QString msg;
    QUrl url = reply->url();
    if (reply->error()) {
        msg = "Download failed: "+reply->errorString();
        emit downFinished(false, msg);
    } else {
        QString filename = saveFileName(url);
        filename.prepend(QDir::currentPath()+"/reportes/");
        if (saveToDisk(filename, reply))
            msg = "Download succeeded (saved to "+filename+")";
        emit downFinished(true, msg);
    }

    currentDownloads.removeAll(reply);
    reply->deleteLater();
}

