#-------------------------------------------------
#
# Project created by QtCreator 2016-10-11T17:46:17
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CheckingQATools
TEMPLATE = app

include(QtXlsxWriter-master/src/xlsx/qtxlsx.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
    codeeditor.cpp \
    analizador.cpp

HEADERS  += mainwindow.h \
    codeeditor.h \
    analizador.h

FORMS    += mainwindow.ui

RESOURCES += \
    data.qrc

QMAKE_CXXFLAGS += -std=gnu++11


