#include "tache.h"

Tache::Tache(QString version)
{
    m_Tray = new QSystemTrayIcon;
    m_Tray->setIcon(QIcon(":/icone/Doc"));
    m_Tray->blockSignals(true);

    QMenu *menu = new QMenu();
    QAction *tempsRestant = menu->addAction(tr("Temps Restant"));
    QObject::connect(tempsRestant,SIGNAL(triggered(bool)),this,SLOT(Temps_Restant()));
    QAction *Lancement = menu->addAction(tr("Lancer l'ajout de BL"));
    QObject::connect(Lancement, SIGNAL(triggered(bool)), this, SLOT(Ajout_BC()));
    QAction *frn = menu->addAction(tr("MAJ bons de commande"));
    QObject::connect(frn,SIGNAL(triggered(bool)),this,SIGNAL(MAJ_BC()));
    QAction *Afficher = menu->addAction(tr("Ouvrir"));
    QObject::connect(Afficher, SIGNAL(triggered(bool)), this, SIGNAL(Ouvrir()));
    QAction *quitter = menu->addAction(tr("Quitter"));
    QObject::connect(quitter, SIGNAL(triggered(bool)), this, SIGNAL(Quitter()));

    m_Tray->setContextMenu(menu);
    m_Tray->setToolTip("AutoBL V" + version);
    m_Tray->show();
}

Tache::~Tache()
{
    delete m_Tray;
}

void Tache::Affichage_Info(QString texte)
{
    m_Tray->showMessage("",texte);
}

void Tache::Temps_Restant()
{
    emit temps_Restant();
}

void Tache::Ajout_BC()
{
    connect(m_Tray,SIGNAL(messageClicked()),this,SLOT(Arret_Ajout()));
    emit ajout_BC();
}

void Tache::Affichage_En_Cours()
{
    Affichage_Info(tr("Ajout de BC en cours\r\nNe pas utiliser le pc"));
}

void Tache::Arret_Ajout()
{
    Affichage_Info(tr("Arret en cours veuillez patienter..."));
    qDebug() << "arret demandé";
    disconnect(m_Tray,SIGNAL(messageClicked()),this,SLOT(Arret_Ajout()));
    emit Arret();
}

void Tache::Login(bool etat)
{
    QMenu *m = m_Tray->contextMenu();
    if(!etat)
    {
        m->actions().at(1)->setEnabled(false);
    }
    else
    {
        m->actions().at(1)->setEnabled(true);
    }
}
