#ifndef DB_H
#define DB_H

#include <QtSql>
#include <QString>
#include <QObject>
#include <QDebug>
#include <QCryptographicHash>
#include <QDesktopServices>

#include <../../../../Cle_AutoBL.cpp>
#include <error.h>

#define DEBUG qDebug()

typedef enum {
    download,
    error,
    updateRef,
    add,
    endAdd,
    unknownFab,
    badRef
}State;

typedef enum{
    open,
    partial,
    Close
}FRNState;

class DB: public QObject
{
    Q_OBJECT
public:
    DB(Error *err);
    QSqlQuery Requete(QString req);
    void Init();
    void Close_DB();
    void Purge();
    QStringList Find_Fournisseur_From_Invoice(QString invoice);
    QString Encrypt(QString text);
    QString Decrypt(QString text);
    QString enum_State(int state);
    QString Get_Last_Invoice(QString frn);
    bool Insert_Into_En_Cours(QString date, QString Nom_Chantier, QString Numero_Commande, QString Lien_Commande, int Etat,
                                  int Ajout, QString Info_Chantier, QString Fournisseur);
    QSqlQuery Get_Download_Invoice();
    QSqlQuery Get_Added_Invoice();
    QSqlQuery Get_No_Closed_Invoice(QString frn);
    QSqlQuery Get_Delivery_Invoice(QString frn);
    bool Update_En_Cours(QString invoice_Number, QString frn, QString column, QString var);
    bool Remove_En_Cours(QString invoice_Number, QString frn, QString reason);

    //_list_Frn = FRNLIST from Fournisseur
    void Set_List_Fournisseurs(QString list) { _list_Frn = list; }

public slots:
    void Sav();

signals:
    void sError(QString err);
    void Info(QString texte);
    void CreateTable();

private:
    Error *m_Error;
    QString _list_Frn;

};

#endif // DB_H
