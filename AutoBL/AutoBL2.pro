#-------------------------------------------------
#
# Project created by QtCreator 2016-07-15T13:29:56
#
#-------------------------------------------------

QT       += core gui
QT       += network
QT       += webenginewidgets
QT       += sql

LIBS     += User32.Lib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AutoBL
TEMPLATE = app
TRANSLATIONS = AutoBL_fr.ts


SOURCES += main.cpp\
        principal.cpp \
    esabora.cpp \
    tache.cpp \
    db.cpp \
    fournisseur.cpp \
    rexelfr.cpp \
    fctfournisseur.cpp \
    error.cpp \
    socolecfr.cpp \
    infowindow.cpp \
    cged.cpp

HEADERS  += principal.h \
    esabora.h \
    tache.h \
    db.h \
    fournisseur.h \
    rexelfr.h \
    fctfournisseur.h \
    error.h \
    socolecfr.h \
    infowindow.h \
    cged.h

FORMS    += principal.ui

RESOURCES += \
    ressources.qrc

DISTFILES +=
