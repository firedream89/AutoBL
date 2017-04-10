#ifndef MINIMALFOURNISSEUR_H
#define MINIMALFOURNISSEUR_H

#include "db.h"
#include "fctfournisseur.h"
#include <QDebug>

#define DEBUG qDebug()

class MinimalFournisseur
{   
public:
    MinimalFournisseur(FctFournisseur *fct, const QString login, const QString mdp, const QString lien_Travail, const QString comp);
    bool Start();
    QStringList Get_Invoice(const QString InvoiceNumber);
    void Set_Var(const QString login,const QString mdp,const QString comp);
    bool Test_Connexion();

private slots:

signals:
    void Info(const QString i);
    //void Error(const QString error);
    void FindTexte(const QString texte);

private:
    QString m_Login,m_MDP,m_UserName,m_WorkLink;
    FctFournisseur *m_Fct;
    DB m_DB;
};

#endif // MINIMALFOURNISSEUR_H
