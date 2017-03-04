#-------------------------------------------------
#
# Project created by QtCreator 2016-02-22T18:09:58
#
#-------------------------------------------------

QT       += core gui
QT       += network
QT       += webkitwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AutoBL
TEMPLATE = app

INCLUDEPATH += "bin/"

SOURCES += main.cpp\
        principal.cpp \
    apropos.cpp \
    cookiejar.cpp \
    SendForm.cpp

HEADERS  += principal.h \
    apropos.h \
    cookiejar.h \
    SendForm.h

FORMS    += principal.ui \
    apropos.ui \
    browser.ui

RESOURCES += \
    ressources.qrc
