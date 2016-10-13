#include "analizador.h"
#include "xlsxdocument.h"
#include <QDebug>

analizador::analizador(QObject *parent) : QObject(parent)
{

}

void analizador::buscarEnExcel(QString prc, QString fileExcelPlanilla)
{
    QXlsx::Document *docINP = new QXlsx::Document(fileExcelPlanilla);

    prc = prc.toLower();
    prc = prc.mid(0, prc.lastIndexOf("."));

    int nFilas = docINP->dimension().lastRow();
    for (int i = 2; i <= nFilas; ++i) {
        QString componente = docINP->read("A"+QString::number(i)).toString().toLower();
        QString relacion = docINP->read("B"+QString::number(i)).toString().toLower();

        if(componente==prc && relacion.contains(".wsdl")) {
            relaciones.append(relacion);
            procedimientos.append(prc);
        }
    }
}

void analizador::buscarFuncionalidad(QString fileExcelFuncionalidad)
{
    QStringList resultados;
    QXlsx::Document *docINF = new QXlsx::Document(fileExcelFuncionalidad);
    docINF->selectSheet("Hoja2");

    int numFilas = docINF->dimension().lastRow();

    for (int j = 0; j < relaciones.size(); ++j) {
        QString var = relaciones[j];
        for (int i = 2; i <= numFilas; ++i) {
            QString endpoint = docINF->read("G"+QString::number(i)).toString().toLower();
            QString funcionalidad = docINF->read("E"+QString::number(i)).toString();
            if(endpoint.contains(var)){
                if(SalidaComp)
                    resultados.append(procedimientos[j]+","+funcionalidad);
                else
                    resultados.append(funcionalidad);
            }
        }
    }
    emit calculationCompleted(resultados);
}

void analizador::doHeavyCalculation()
{
    int tam = procEntrada.size();
    int porcActual = 0;
    int cont = 0;
    relaciones.clear();
    procedimientos.clear();
    foreach (QString v, procEntrada){
        buscarEnExcel(v.trimmed(), path_to_planilla);
        cont++;
        porcActual = (cont*100)/tam;
        emit updateProgressbar(porcActual);
    }

    buscarFuncionalidad(path_to_funcionalidad);
}

bool analizador::getSalidaComp() const
{
    return SalidaComp;
}

void analizador::setSalidaComp(bool value)
{
    SalidaComp = value;
}

QStringList analizador::getProcEntrada() const
{
    return procEntrada;
}

void analizador::setProcEntrada(const QStringList &value)
{
    procEntrada = value;
}

QString analizador::getPath_to_funcionalidad() const
{
    return path_to_funcionalidad;
}

void analizador::setPath_to_funcionalidad(const QString &value)
{
    path_to_funcionalidad = value;
}

QString analizador::getPath_to_planilla() const
{
    return path_to_planilla;
}

void analizador::setPath_to_planilla(const QString &value)
{
    path_to_planilla = value;
}

QStringList analizador::getRelaciones() const
{
    return relaciones;
}

void analizador::setRelaciones(const QStringList &value)
{
    relaciones = value;
}

