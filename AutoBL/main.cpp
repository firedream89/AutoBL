#include "principal.h"
#include <QApplication>
#include <QtPlugin>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QFile file(QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0)+"/Logs/debug.log");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    file.write(QString("[").toLatin1()+QDateTime::currentDateTime().toString().toLatin1()+QString("] ").toLatin1());
    switch (type) {
    case QtDebugMsg:
        file.write(QString("Debug: ").toLatin1()+msg.toLatin1()+QString("\r\n").toLatin1());
        break;
    case QtWarningMsg:
        file.write(QString("Warning: ").toLatin1()+msg.toLatin1()+QString("\r\n").toLatin1());
        break;
    case QtFatalMsg:
        file.write(QString("Fatal: ").toLatin1()+msg.toLatin1()+QString("\r\n").toLatin1());
        break;
    case QtCriticalMsg:
        file.write(QString("Critical: ").toLatin1()+msg.toLatin1()+QString("\r\n").toLatin1());
        break;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator qtTranslator;
    qtTranslator.load("qt_fr");
    a.installTranslator(&qtTranslator);
    QTranslator translate;
    translate.load("AutoBL_fr");
    a.installTranslator(&translate);

    qDebug();
    if(!QApplication::arguments().contains("dvp"))
        qInstallMessageHandler(myMessageOutput);

    Principal w;

    return a.exec();
}
