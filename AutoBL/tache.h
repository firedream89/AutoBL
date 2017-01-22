#ifndef TACHE_H
#define TACHE_H

#include <QString>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QObject>

#include "db.h"

class Tache: public QObject
{
    Q_OBJECT

public:
    Tache(QString version);
    ~Tache();
    void Affichage_Info(QString texte);
    void Affichage_En_Cours();
    void Login(bool etat);

private slots:
    void Temps_Restant();
    void Ajout_BC();
    void Arret_Ajout();

signals:
    void temps_Restant();
    void ajout_BC();
    void Ouvrir();
    void Arret();
    void Quitter();

private:
    QSystemTrayIcon *m_Tray;
};

#endif // TACHE_H
