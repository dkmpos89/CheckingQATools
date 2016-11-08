#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QUrl>
#include <QSslError>
#include <QStringList>
#include <QTimer>


QT_BEGIN_NAMESPACE
class QSslError;
QT_END_NAMESPACE

QT_USE_NAMESPACE


class DownloadManager : public QObject
{
    Q_OBJECT
    QNetworkAccessManager manager;
    QList<QNetworkReply *> currentDownloads;

public:
    explicit DownloadManager(QObject *parent = 0);
    //DownloadManager();
    void doDownload(const QUrl &url);
    QString saveFileName(const QUrl &url);
    bool saveToDisk(const QString &filename, QIODevice *data);
    void setProxy(QString HostName, qint16 port);
    QNetworkProxy getProxy(){ return manager.proxy(); }
signals:
    void downFinished(bool b, QString msg);
public slots:
    void downloadFinished(QNetworkReply *reply);
    void sslErrors(const QList<QSslError> &errors);
};

#endif // DOWNLOADMANAGER_H
