#-------------------------------------------------
#
# Project created by QtCreator 2016-10-11T17:46:17
#
#-------------------------------------------------

QT += core gui network
QT += webkitwidgets multimedia
QT += axcontainer

CONFIG += qaxcontainer

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CheckingQATools
TEMPLATE = app

include(QtXlsxWriter-master/src/xlsx/qtxlsx.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
    codeeditor.cpp \
    analizador.cpp \
    console.cpp \
    downloadmanager.cpp \
    proxysettings.cpp \
    analysisparameters.cpp \
    ctoolslogin.cpp

HEADERS  += mainwindow.h \
    codeeditor.h \
    analizador.h \
    console.h \
    downloadmanager.h \
    proxysettings.h \
    analysisparameters.h \
    webaxwidget.h \
    ctoolslogin.h

FORMS    += mainwindow.ui \
    proxysettings.ui \
    ctoolslogin.ui

RESOURCES += \
    data.qrc

RC_FILE = app_icon.rc

QMAKE_CXXFLAGS += -std=gnu++11

