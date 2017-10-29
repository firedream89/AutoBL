#ifndef FOURNISSEUR_H
#define FOURNISSEUR_H

#include <rexelfr.h>
#include <QDebug>

#include "fctfournisseur.h"
#include "rexelfr.h"
#include "socolecfr.h"
#include "error.h"

#define DEBUG qDebug()
#define FRN1 "Rexel.fr"
#define FRN2 "Socolec.fr"
#define FRN3 ""
#define FRN4 ""
#define FRN5 ""
#define FRN6 ""
#define FRN7 ""
#define FRN "Fournisseur"
#define FRNLIST "Rexel.fr|Socolec.fr"


class Fournisseur : public QObject
{
    Q_OBJECT

public:
    Fournisseur(QString lien_Travail, DB *db, Error *err);
    ~Fournisseur();
    bool Add(const QString nom, const QString complement, const QString login, const QString mdp);
    bool Add(const QString nom);
    bool Del(const QString nom);
    bool Start();
    bool Update_Var(const QString &frn, const QString &login, const QString &mdp, const QString &complement);
    bool Test_Connexion(const QString& nom);
    void Show_Web();
    QString List_Frn() const;
    QString Get_Frn_Inf(QString frn) const;
    QStringList Control_Invoice_List(QStringList list);

public slots:
    QStringList Get_Invoice_List(const QString &frn,const QString &invoiceNumber);
    void Set_Fab(QString fab);

signals:
    void En_Cours_Fournisseur(QString nom);
    void En_Cours_Info(QString info);
    void Erreur(QString err);
    void Info(QString info);
    void LoadProgress(int l);
    void Change_Load_Window(QString text);
    void Find_Fab(QString fab);

private:
    QStringList Find_Fournisseur(QString nom);
    QStringList Find_Fournisseur_From_DB(const QString nom);
    QStringList fournisseurs;
    QString m_Lien_Travail;
    FctFournisseur *m_fct;
    DB *m_DB;
    Error *m_Error;
};

#endif // FOURNISSEUR_H
