#ifndef DB_H
#define DB_H

#include <QtSql>
#include <QString>
#include <QObject>

class DB: public QObject
{
    Q_OBJECT
public:
    DB();
    QSqlQuery Requete(QString req);
    void Init();
    void Purge();

public slots:
    void Sav();

signals:
    void Error(QString err);
    void Info(QString texte);
    void CreateTable();

};

#endif // DB_H
