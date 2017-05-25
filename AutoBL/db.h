#ifndef DB_H
#define DB_H

#include <QtSql>
#include <QString>
#include <QObject>
#include <QDebug>
#include <QCryptographicHash>
#include <QDesktopServices>

#include <../../../Cle_AutoBL.cpp>
#include <error.h>

#define DEBUG qDebug()

class DB: public QObject
{
    Q_OBJECT
public:
    DB(Error *err);
    QSqlQuery Requete(QString req);
    void Init();
    void Close();
    void Purge();
    QStringList Find_Fournisseur_From_Invoice(QString invoice);
    QString Encrypt(QString text);
    QString Decrypt(QString text);

public slots:
    void Sav();

signals:
    void sError(QString err);
    void Info(QString texte);
    void CreateTable();

private:
    Error *m_Error;

};

#endif // DB_H
