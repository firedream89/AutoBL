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
#define FCT "WEB"

class FctFournisseur: public QObject
{
    Q_OBJECT

public:
    FctFournisseur(QString WorkLink,Error *err);
    ~FctFournisseur();
    bool WebLoad(QString lien);
    bool FindTexte(QString texte);
    void Loop(int tMax = 0);
    bool SaveText();
    bool SaveHtml();
    QVariant InsertJavaScript(QString script);
    void WebOpen();
    void FrnError(int code, QString frn, QString er = 0);
    void Info(QString info);
    void Change_Load_Window(QString text);
    bool Get_Load_Finished();
    QStringList Control_Fab(QStringList list);

public slots:
    void Stop_Load();
    void Return_Fab(QString fab);

signals:
    void error(QString e);
    void LoadProgress(int p);
    void info(QString i);
    void err(QString info);
    void change_Load_Window(QString text);
    void Find_Fab(QString fab);
    void Set_Fab();

private slots:
    void Set_Load(bool state);

private:
    QEventLoop *loop;
    QTimer *timer;
    QWebEngineView *web;
    QString m_WorkLink;
    Error *m_Error;
    bool m_load;
    QString m_Fab;
};

#endif // FCTFOURNISSEUR_H
