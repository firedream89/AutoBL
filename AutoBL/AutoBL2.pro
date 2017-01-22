#-------------------------------------------------
#
# Project created by QtCreator 2016-07-15T13:29:56
#
#-------------------------------------------------

QT       += core gui
QT       += network
QT       += webkitwidgets
QT       += sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AutoBL
TEMPLATE = app
TRANSLATIONS = AutoBL_fr.ts


SOURCES += main.cpp\
        principal.cpp \
    rexel.cpp \
    esabora.cpp \
    tache.cpp \
    db.cpp

HEADERS  += principal.h \
    rexel.h \
    esabora.h \
    tache.h \
    db.h

FORMS    += principal.ui

RESOURCES += \
    ressources.qrc

DISTFILES +=
