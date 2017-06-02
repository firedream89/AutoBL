#include "error.h"

Error::Error()
{
    qDebug() << "Error | Erreur Constructeur par défaut initialisé";
}

Error::Error(QString work_Link)
{
    qDebug() << "Init classe Error";
    m_Errors.setFileName(work_Link + "/Logs/errors.txt");
    m_Errors.open(QIODevice::WriteOnly | QIODevice::Append);

    //Chargement des classes de l'application et fonctions nécéssaires
    m_Errors.write("--------------------------Run AutoBL-----------------------------");
}  

QString Error::Err(int code, QString e, QString fromClass)
{
    QString err;
    switch (code) {
    case no_Error:
        err = "";
        break;
    case bad_Login:
        err = tr("Mauvais login ou mot de passe");
        break;
    case bad_Username:
        err = tr("Mauvais nom d'utilisateur");
        break;
    case load:
        err = tr("Chargement de la page %1 échoué").arg(e);
        break;
    case open_File:
        err = tr("Ouverture du fichier %1 échouée").arg(e);
        break;
    case fail_check:
        if(e.isEmpty())
            err = tr("Vérification de la page échouée");
        else
            err = tr("Vérification de la page échouée (%1)").arg(e);
        break;
    case variable:
        err = tr("Lecture de la variable %1 échouée").arg(e);
        break;
    case requete:
        err = tr("Requête vers la base de données échouée").arg(e);
        break;
    case too_many:
        err = tr("Trop de bons de livraison trouvés(%1 trouvés contre 5 maximum autorisé)").arg(e);
        break;
    case Not_BC:
        err = tr("La création du bon de commande %0 à échouée").arg(e);
        break;
    case BC:
        err = tr("L'ajout du matériels sur le bon de commande %0 à échouée(bon de commande à supprimer)").arg(e);
        break;
    case BL:
        err = tr("La validation du bon de commande %0 à échouée").arg(e);
        break;
    case Run_Esabora:
        err = tr("Ouverture d'Esabora échouée").arg(e);
        break;
    case Traitement:
        err = tr("Une erreur s'est produite durant le traitement d'un bon de commande(%0)").arg(e);
        break;
    case Window:
        err = tr("La fenêtre %0 n'a pas été trouvée").arg(e);
        break;
    case Focus:
        err = tr("La fenêtre %0 n'est plus au premier plan").arg(e);
        break;
    case Mouse:
        err = tr("La déplacement de la souris à échoué");
        break;
    case designation:
        err = tr("La référence est inconnue").arg(e);
        break;
    case save_file:
        err = tr("Echec d'enregistrement du fichier").arg(e);
        break;
    case noConnected:
        err = tr("Non connecté à internet");
        break;
    case failFrn:
        err = tr("Erreur critique interne : le nom de fournisseur n'existe pas(%1)").arg(e);
        break;
    case failData:
        err = tr("Erreur Critique interne sur les données d'un fournisseur");
        break;
    case findFrn:
        err = tr("Fournisseur non trouvé !(%0)").arg(e);
        break;
    case openDB:
        err = tr("Ouverture de la base de données échouée");
        break;
    case updateDB:
        err = tr("Echec de modification de la base de données");
        break;
    case saveDB:
        err = tr("Echec de sauvegarde de la DB");
        break;
    default:
        err = tr("Erreur inconnue %1").arg(e);
        break;
    }

    if(!fromClass.isEmpty())
        err = fromClass + " | " + err;
    emit sError(err);
    Write_Error(err);
    return err;
}

void Error::Write_Error(QString e)
{
    m_Errors.write(e.toLatin1() + "/r/n");
}
