#ifndef ANALYSISPARAMETERS_H
#define ANALYSISPARAMETERS_H

#include <QObject>
#include <QMap>
#include <QTextEdit>

class AnalysisParameters
{
public:
    AnalysisParameters();
    QStringList getFirst();
    void add(QString bln, int idx, QString scriptId, QString productId, QString projectId, QString objectId, QString areaId, QString currentScript, QTextEdit *finalOutput);
    void printAll();
    QMap<QString,QString> getParams(QString blName);

    QMap<QString, QMap<QString,QString>> parametros;
};

#endif // ANALYSISPARAMETERS_H
