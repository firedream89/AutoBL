#ifndef MINIMALFOURNISSEUR_H
#define MINIMALFOURNISSEUR_H

#include "db.h"
#include "fctfournisseur.h"
#include <QDebug>

#define DEBUG qDebug()
#define FRN ""
#define INF "Nom d'utilisateur|Email|Mot de passe"

class MinimalFournisseur: public QObject
{   
    Q_OBJECT

public:
    MinimalFournisseur(FctFournisseur *fct, const QString login, const QString mdp, const QString lien_Travail, const QString comp, DB *db);
    bool Start();
    QStringList Get_Invoice(const QString InvoiceNumber);
    void Set_Var(const QString login,const QString mdp,const QString comp);
    bool Test_Connexion();

private slots:
    bool Connexion();
    bool Create_List_Invoice();

signals:
    void Info(const QString i);
    void FindTexte(const QString texte);

private:
    QString m_Login,m_MDP,m_UserName,m_WorkLink;
    FctFournisseur *m_Fct;
    DB *m_DB;
};

#endif // MINIMALFOURNISSEUR_H
