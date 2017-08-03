#include "analysisparameters.h"
#include <QDebug>

AnalysisParameters::AnalysisParameters()
{
}

QStringList AnalysisParameters::getFirst()
{

}

void AnalysisParameters::add(QString bln, int idx, QString scriptId, QString productId, QString projectId, QString objectId, QString areaId, QString currentScript, QTextEdit *finalOutput)
{
    QMap<QString, QString> params;
    params.insert("idx", QString::number(idx));
    params.insert("scriptId", scriptId);
    params.insert("productId", productId);
    params.insert("projectId", projectId);
    params.insert("objectId", objectId);
    params.insert("areaId", areaId);
    params.insert("currentScript", currentScript);
    params.insert("finalOutput", finalOutput->objectName());

    parametros.insert(bln, params);
}

void AnalysisParameters::printAll()
{
    QMapIterator<QString, QMap<QString, QString>> i(parametros);
    while (i.hasNext()) {
        i.next();
        qInfo() << i.key() << " ";
    }
    qInfo()<<"***"<<endl;
}

QMap<QString,QString> AnalysisParameters::getParams(QString blName)
{
    QMap<QString,QString> mr;
    QMapIterator<QString, QMap<QString, QString>> i(parametros);
    while (i.hasNext()) {
        i.next();
        if(i.key()==blName)
            mr = i.value();
    }
    return mr;
}
