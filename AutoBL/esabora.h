#ifndef ESABORA_H
#define ESABORA_H

#include <QString>
#include <QMessageBox>
#include <QTimer>
#include <QEventLoop>
#include <QDesktopServices>
#include <QtWidgets>
#include <QObject>
#include <QDebug>
#include <QMessageBox>

#include "db.h"
#include "error.h"

#define ESAB QString("Esabora")

class Esabora : public QObject
{
    Q_OBJECT
private:
    QWidget *m_fen;
public:
    Esabora(QWidget *fen, QString Login, QString MDP, QString Lien_Esabora, QString Lien_Travail, DB *db, Error *e);
    int GetEtat();
    bool Start(bool automatic, int &nbBC, int &nbBL);
    ~Esabora();

public slots:
    bool Lancement_API();
    bool Ajout_BC(QString Numero_Commande);
    bool Fermeture_API();
    bool Ajout_BL(QString Numero_Commande_Esab, QString Numero_BL);
    void Set_Liste_Matos(QStringList liste);
    void Semi_Auto(QString NumeroCommande);
    void Reset_Liste_Matos();
    void Apprentissage(QString &entreprise, QString &BDD);
    void Set_Var_Esabora(QString lien, QString login, QString MDP);
    bool Ouverture_Liste_BC();
    void Abort();
    bool Ajout_Stock(QString numero_Commande);
    void Stop();
    QString Test_Find_Fabricant(QString fab);

private slots:
    bool Clavier(QString commande);
    bool Souris(QString commande);
    bool Traitement_Fichier_Config(const QString file, const QString bL = 0);
    bool Verification_Fenetre(QString fenetre);
    bool Verification_Focus(QString fen,bool focus);
    bool Verification_Message_Box(QString &message);
    bool Get_List_Matos(QString invoice);
    QString Find_Fabricant(QString Fab);

signals:
    void DemandeListeMatos(QString NumeroCommande);
    void ReceptionListeMatos();
    void Info(QString texte);
    void Erreur(QString texte);
    void Message(QString header,QString texte,bool warning);

private:
    DB *m_DB;
    int etat;
    QString m_Login;
    QString m_MDP;
    QString m_Lien_Esabora;
    QString m_Tmp;
    QString m_Lien_Work;
    QStringList liste_Matos;
    QStringList m_List_Cmd;
    bool m_Arret;
    Error *err;
    QProcess p;
};

#endif // ESABORA_H
