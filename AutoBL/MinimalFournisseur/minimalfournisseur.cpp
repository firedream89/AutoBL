#include "minimalfournisseur.h"

MinimalFournisseur::MinimalFournisseur(FctFournisseur *fct, const QString login, const QString mdp, const QString lien_Travail, const QString comp, DB *db):
    m_Login(login),m_MDP(mdp),m_UserName(comp),m_WorkLink(lien_Travail),m_Fct(fct),m_DB(db)
{
    DEBUG << "Init Class " << FRN;
    m_Fct = fct;
}

bool MinimalFournisseur::Start()
{
    DEBUG << "Start " << FRN;
    //Premier démarrage
}

bool MinimalFournisseur::Connexion()
{

}

QStringList MinimalFournisseur::Get_Invoice(const QString InvoiceNumber)
{

}

void MinimalFournisseur::Set_Var(const QString login, const QString mdp, const QString comp)
{
    m_Login = login;
    m_MDP = mdp;
    m_UserName = comp;
}

bool MinimalFournisseur::Test_Connexion()
{
    DEBUG << "TEST CONNEXION " << FRN;
    Connexion();
    return ;
}

QString MinimalFournisseur::Get_Inf()
{
    return QString(INF);
}
