#ifndef REXEL_H
#define REXEL_H

#include "db.h"
#include <QtWidgets>
#include <QString>
#include <QObject>
#include <QWebView>
#include <QWebFrame>
#include <QNetworkReply>
#include "cookiejar.h"
#include <QDebug>

#define DEBUG qDebug()

class Rexel: public QObject
{
    Q_OBJECT

public:
    Rexel(QString lien_Travail, QString login, QString mdp);
    ~Rexel();
    bool Connexion(QString login, QString MDP);
    bool Navigation();
    bool Verification(QString texte,QString reponse,bool bloquer = 0);
    void Affichage();
    bool Recuperation_BL(QString numero_Commande);
    bool webLoad(QString lien);
    void ResetWeb();
    QStringList AffichageTableau();
    bool VerificationEtatBC(QString numeroCommande);
    bool Start(QString &error ,QStringList &list, int option = 0,  QString numeroBC = 0);
    void Set_Var(QString login,QString mdp);

private slots:
    void EmitErreur(int codeErreur, int stringErreur, QString info = 0);

signals:
    void Info(QString texte);
    void InfoFen(QString label,QString texte);
    void Erreur(QString texte);
    void TelechargementTerminer();
    void Message(QString header,QString texte,bool warning);
    void LoadProgress(int valeur);

private:
    DB m_db;
    QWebView *web;
    QString m_NomExcel;
    QNetworkCookieJar *cookieJar;
    QString m_Lien_Work;
    QString m_Login;
    QString m_MDP;
};

#endif // REXEL_H
