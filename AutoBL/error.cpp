#include "error.h"

Error::Error()
{
    ///Ajouter un controle d'ouverture d'esabora et fermer la session si déjà ouverte
}

QString Error::Err(int code, QString e, QString fromClass)
{
    QString err;
    switch (code) {
    case no_Error:
        err = "";
        break;
    case bad_Login:
        err = QObject::tr("Mauvais login ou mot de passe");
        break;
    case bad_Username:
        err = QObject::tr("Mauvais nom d'utilisateur");
        break;
    case load:
        err = QObject::tr("Chargement de la page %1 échoué").arg(e);
        break;
    case open_File:
        err = QObject::tr("Ouverture du fichier %1 échouée").arg(e);
        break;
    case fail_check:
        if(e.isEmpty())
            err = QObject::tr("Vérification de la page échouée");
        else
            err = QObject::tr("Vérification de la page échouée (%1)").arg(e);
        break;
    case variable:
        err = QObject::tr("Lecture de la variable %1 échouée").arg(e);
        break;
    case requete:
        err = QObject::tr("Requête vers la base de données échouée").arg(e);
        break;
    case too_many:
        err = QObject::tr("Trop de bons de livraison trouvés(%1 trouvés contre 5 maximum autorisé)").arg(e);
        break;
    case Not_BC:
        err = QObject::tr("La création du bon de commande %0 à échouée").arg(e);
        break;
    case BC:
        err = QObject::tr("L'ajout du matériels sur le bon de commande %0 à échouée(bon de commande à supprimer)").arg(e);
        break;
    case BL:
        err = QObject::tr("La validation du bon de commande %0 à échouée").arg(e);
        break;
    case Run_Esabora:
        err = QObject::tr("Ouverture d'Esabora échouée").arg(e);
        break;
    case Traitement:
        err = QObject::tr("Une erreur s'est produite durant le traitement d'un bon de commande(%0)").arg(e);
        break;
    case Window:
        err = QObject::tr("La fenêtre %0 n'a pas été trouvée").arg(e);
        break;
    case Focus:
        err = QObject::tr("La fenêtre %0 n'est plus au premier plan").arg(e);
        break;
    case Mouse:
        err = QObject::tr("La déplacement de la souris à échoué");
        break;
    default:
        err = QObject::tr("Erreur inconnue %1").arg(e);
        break;
    }

    if(fromClass.isEmpty())
        err = fromClass + " | " + err;
    return err;
}
