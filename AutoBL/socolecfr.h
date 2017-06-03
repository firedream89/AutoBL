#ifndef SOCOLECFR_H
#define SOCOLECFR_H

#include "db.h"
#include "fctfournisseur.h"
#include <QDebug>
#include <QObject>

#define DEBUG qDebug()
#define FRN "Socolec.fr"

class SocolecFr: public QObject
{
    Q_OBJECT

public:
    SocolecFr(FctFournisseur *fct, const QString login, const QString mdp, const QString lien_Travail, const QString comp, DB *db);
    bool Start();
    QStringList Get_Invoice(const QString InvoiceNumber);
    void Set_Var(const QString login,const QString mdp,const QString comp);
    bool Test_Connexion();

private slots:
    bool Connexion();
    bool Create_List_Invoice(bool firstInit = 0);

signals:
    void Info(const QString i);
    void FindTexte(const QString texte);

private:
    QString m_Login,m_MDP,m_UserName,m_WorkLink;
    FctFournisseur *m_Fct;
    DB *m_DB;
};

#endif // SOCOLECFR_H
