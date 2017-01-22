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

class Rexel: public QObject
{
    Q_OBJECT

public:
    Rexel(QString lien_Travail);
    ~Rexel();
    bool Connexion(QString login, QString MDP);
    bool Navigation();
    bool Exportation(QString lien);
    bool Verification(QString texte,QString reponse,bool bloquer = 0);
    void Affichage();
    bool Recuperation_BL(QString numero_Commande);
    bool webLoad(QString lien);
    void ResetWeb();
    void LectureSeule(bool lecture);
    QStringList AffichageTableau();
    bool VerificationEtatBC(QString numeroCommande);

private slots:
    void Telechargement(QNetworkReply *reply);
    void EmitErreur(int codeErreur, int stringErreur, QString info = 0);
    void Load(int Valeur);

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
};

#endif // REXEL_H
