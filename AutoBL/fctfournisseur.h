#ifndef FCTFOURNISSEUR_H
#define FCTFOURNISSEUR_H

#include <QObject>
#include <QString>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QDebug>
#include <QDesktopServices>
#include <QWebEngineScript>
#include <QWebEnginePage>
#include <QWebEngineView>


#include "error.h"
#define DEBUG qDebug()

class FctFournisseur: public QObject,public Error
{
    Q_OBJECT

public:
    FctFournisseur(QString WorkLink);
    ~FctFournisseur();
    bool WebLoad(QString lien);
    bool FindTexte(QString texte);
    void Loop(int tMax = 0);
    bool SaveText();
    bool SaveHtml();
    QVariant InsertJavaScript(QString script);
    void WebOpen();
    void FrnError(int code, QString er = 0);
    void Info(QString info);
    void Change_Load_Window(QString text);

public slots:
    void Stop_Load();

signals:
    void error(QString e);
    void LoadProgress(int p);
    void info(QString i);
    void err(QString info);
    void change_Load_Window(QString text);

private slots:


private:
    QEventLoop *loop;
    QTimer *timer;
    QWebEngineView *web;
    QString m_WorkLink;
    Error *m_Error;
};

#endif // FCTFOURNISSEUR_H
