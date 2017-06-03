#include "minimalfournisseur.h"
///Error Code xxx
MinimalFournisseur::MinimalFournisseur(FctFournisseur *fct, const QString login, const QString mdp, const QString lien_Travail, const QString comp):
    m_Login(login),m_MDP(mdp),m_UserName(comp),m_WorkLink(lien_Travail)
{
    DEBUG << "Init Class FRN";
    m_Fct = fct;
}

bool MinimalFournisseur::Start()
{
    //Premier démarrage
}

///Error code xxx
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
    DEBUG << "TEST CONNEXION FRN";
    return ;
}
