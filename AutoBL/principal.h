#ifndef PRINCIPAL_H
#define PRINCIPAL_H

#include <QMainWindow>
#include <QFile>
#include <QDir>
#include <QtSql>
#include <QMessageBox>
#include <QFileDialog>
#include <Windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <iostream>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "tache.h"
#include "esabora.h"
#include "fournisseur.h"
////////////
///ADD #define PKEY ""
///ADD #define MAJLINK ""
#include <../../Cle_AutoBL.cpp>
///////////
#include "error.h"

#define DEBUG qDebug()

namespace Ui {
class Principal;
}
class Principal : public QMainWindow
{
    Q_OBJECT

public:
    explicit Principal(QWidget *parent = 0);
    ~Principal();
    bool test();
    void Erreur(int code,int string,QString info);

public slots:
    void Init_Config();
    void Sauvegarde_Parametres();
    void Chargement_Parametres();
    void Affichage_Info(QString texte,bool visible = 0);
    void Affichage_Erreurs(QString texte,bool visible = 0);
    void Affichage_Dossier_Logs();
    void Affichage_Temps_Restant();
    void Emplacement_Esabora();
    void Demarrage_Auto_BC(bool Demarrage_Timer = 0);
    void Demarrage();
    void Test_Esabora();
    void About();
    void Test_Rexel();
    void Afficher_Message_Box(QString header, QString texte, bool warning = 0);
    void Afficher_tNomFichier(int l = 0, int c = 0, int tri = 10);
    void Afficher_Dossier_Travail();
    void Purge_Bl();
    void Dble_Clique_tNomFichier(int l, int c);
    void Arret();
    void Etat_Ajout_Auto();
    void Help(bool p = 0);
    void Login();
    void Login_False();
    void Login_True();
    void Test_BC();
    void Test_BL();
    void MAJ();
    void Bug_Report();
    bool Post_Report();
    void LoadWeb(int valeur);
    void InfoTraitementBL();
    void ModifInfoTraitementBL(QString label,QString etat);
    void Get_Tableau_Matos(int l);
    void Get_Tableau_Matos(QString Numero_Commande);
    void menu_TNomFichier(QPoint point);
    void Tri_TNomFichier_Nom();
    void Tri_TNomFichier_Ref();
    void Tri_TNomFichier_Date();
    void Remove_Row_DB();
    void Modif_Cell_TNomFichier(int l,int c);
    void Verif_MDP_API();
    void AddError(QString error);
    void PurgeError();
    void Quitter();
    void MAJ_Repertoire_Esabora();
    void Add_Fournisseur();
    void Del_Fournisseur();
    void Save_Param_Fournisseur();
    void Test_Fournisseur();
    void Init_Fournisseur();
    void Load_Param_Fournisseur();
    void Create_Fen_Info(QString label1,QString label2 = 0,QString label3 = 0,QString label4 = 0);
    void Update_Fen_Info(QString label,QString info);
    void Update_Fen_Info(QString info = 0);
    void Fournisseur_Actuel(QString nom);
    void Update_Load_Window(QString text);
    void Destroy_Chargement();
    QString HashMDP(QString mdp);
    void Show_List_Sav();
    void Restaurer_DB();
    void Demarrer_Frn();

signals:
    void FinAjout();
    void End_Get_Tableau_Matos();

private:
    Ui::Principal *ui;
    QFile m_Logs;
    QFile m_Errors;
    Tache *m_Tache;
    bool login;
    QLineEdit *mdp;
    DB *m_DB;
    QTimer m_Temps;
    Esabora *m_Esabora;
    int m_Erreurs;
    QString m_Lien_Work;
    bool m_Arret;
    int m_Tri;
    bool premierDemarrage;
    Fournisseur *m_Frn;
    Error *m_Error;
};

#endif // PRINCIPAL_H
