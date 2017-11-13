#include "rexelfr.h"
///Error Code 9xx
RexelFr::RexelFr(FctFournisseur *fct, const QString login, const QString mdp, const QString lien_Travail, const QString comp, DB *db):
    m_Login(login),m_MDP(mdp),m_UserName(comp),m_WorkLink(lien_Travail),m_DB(db)
{
    DEBUG << "Init Class RexelFr";
    m_Fct = fct;
}

bool RexelFr::Start()
{
    if(Connexion())
    {
        QSqlQuery req;
        if(Create_List_Invoice() == false)
        {
            m_Fct->FrnError(bad_Login,REXEL);
        }

        //Vérification état BC
        req = m_DB->Requete("SELECT * FROM En_Cours WHERE Etat!='" + QString::number(Close) + "' AND Fournisseur='" + QString(REXEL) + "'");
        while(req.next())
        {
            m_Fct->Info(tr("Mise à jour état commande %1").arg(req.value("Numero_Commande").toString()));
            if(Check_State(req.value("Numero_Commande").toString()) == false)
            {
                m_Fct->FrnError(fail_check,REXEL,req.value("Numero_Commande").toString());
            }
        }
        //Récupération des BL
        req = m_DB->Requete("SELECT * FROM En_Cours WHERE Etat='" + QString::number(Close) + "' AND Fournisseur='" + QString(REXEL) + "'");
        qDebug() << "BOUCLE Récupération des BL";
        while(req.next())
        {
            if(req.value("Numero_Livraison").toString() == "" || req.value("Numero_Livraison").toString() == " ")
            {
                qDebug() << "BOUCLE Récupération d'un BL";
                m_Fct->Info(tr("Récupération des bons de livraison commande %1").arg(req.value("Numero_Commande").toString()));
                if(Check_Delivery(req.value("Numero_Commande").toString()) == false)
                {
                    m_Fct->FrnError(load,REXEL,req.value("Numero_Livraison").toString());
                }
            }
        }
        qDebug() << "BOUCLE Fin Récupération des BL";
    }
    return true;
}

bool RexelFr::Connexion()
{
    DEBUG << "RexelFr::Connexion()";
    m_Fct->Info(tr("Connexion..."));

    if(m_Fct->WebLoad("https://www.rexel.fr/frx") == false)
    {
        m_Fct->FrnError(load,REXEL,"https://www.rexel.fr/frx");
        return false;
    }

    //Vérification si déjà connecté
    if(m_Fct->FindTexte(m_UserName)) { return true; }

    m_Fct->InsertJavaScript("document.getElementById('j_username').value=\"" + m_Login + "\"");
    m_Fct->InsertJavaScript("document.getElementById('j_password').value=\"" + m_MDP + "\"");
    m_Fct->InsertJavaScript("document.getElementById('loginForm').submit()");
    m_Fct->Loop();

    if(m_Fct->FindTexte("Votre compte a été verrouillé, veuillez contacter l'administrateur"))
    {
        m_Fct->FrnError(fail_check,REXEL,tr("Compte verrouillé, réessayez plus tard"));
        return false;
    }

    DEBUG << "Fin Rexel::Connexion()";
    if(m_Fct->FindTexte(m_UserName)) { return true; }

    if(m_Fct->FindTexte("Informations invalides, veuillez réessayer") || m_Fct->Get_Load_Finished())
    {
        m_Fct->FrnError(fail_check,REXEL,tr("Connexion échouée : Informations invalides"));
    }
    else
    {
        m_Fct->FrnError(fail_check,REXEL,tr("Connexion échouée : Erreur inconnue"));
    }
    return false;
}

bool RexelFr::Create_List_Invoice()
{
    ///Récuperation de la liste de bon de commandes et enregistrement dans le fichier web_Temp.txt
    m_Fct->Info(tr("Chargement des bons de commandes"));
    if(m_Fct->WebLoad("https://www.rexel.fr/frx/my-account/orders") == false)
    {
        m_Fct->FrnError(load,REXEL,"https://www.rexel.fr/frx/my-account/orders");
    }

    if(m_Fct->FindTexte("Suivi de commandes") == false) { m_Fct->FrnError(fail_check,REXEL); }

    ///Récupération du texte de page de commandes et placement en fichier
    QTimer t;
    QSqlQuery r = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='19'");
    r.next();
    connect(&t,SIGNAL(timeout()),&t,SLOT(stop()));
    int nbPage(0),tPage(6);
    bool fin = false;

    do
    {
        //Chargement de la page suivante
        t.start(r.value(0).toInt()*2000);
        m_Fct->InsertJavaScript("ACC.orderHistoryPage.ajaxOrderHistoryReq(" + QString::number(nbPage+1) + ",ACC.orderHistoryPage.sortCriteria,ACC.orderHistoryPage.isAscendingVal);");
        m_Fct->InsertJavaScript("ACC.orderHistoryPage.loadMoreResults(parseInt(" + QString::number(nbPage+1) + "),ACC.orderHistoryPage.sortCriteria,ACC.orderHistoryPage.isAscendingVal,'header');");

        while(m_Fct->InsertJavaScript("$('#accountHistoryPageLoader').is(':visible')").toBool())
        {
            if(t.isActive() == false)
            {
                m_Fct->FrnError(load,REXEL,"Script non chargé");
                return false;
            }
            m_Fct->Loop(1000);
        }
        //Mise à jour de la page actuelle
        nbPage = m_Fct->InsertJavaScript("$('#currentPageId').val();").toInt();
        tPage = m_Fct->InsertJavaScript("$('#totalPageId').val();").toInt();
        m_Fct->Info(tr("Chargement des bons de commandes %0/%1").arg(nbPage).arg(tPage));

        qDebug() << "Navigation - Chargement AJAX " << nbPage << "/" << tPage;

        m_Fct->SaveText();

        ///Traitement des données contenu dans le fichier web_Temp.txt
        QSqlQuery req;
        bool premierDemarrage(false);
        QString nomChantier, lienChantier, numeroCommande, etat, date, infoChantier;
        QFile fichier(m_WorkLink + "/web_Temp.txt");
        QTextStream flux(&fichier);
        if(fichier.open(QIODevice::ReadOnly) == false)
        {
            m_Fct->FrnError(open_File,REXEL,fichier.fileName());
        }

        DEBUG << "Navigation - Début du traitement des informations";
        req = m_DB->Requete("SELECT * FROM En_Cours WHERE Fournisseur='Rexel.fr'");
        if(req.next() == false) { premierDemarrage = true; }

        while(flux.atEnd() == false && fin == false)
        {
            int id = 0;
            req = m_DB->Requete("SELECT MAX(ID) FROM En_Cours");
            req.next();
            id = req.value(0).toInt();
            id++;

            infoChantier = "";
            bool skip(false);
            QString ligne = flux.readLine();

            //N° de commande : 4421-000192238  Statut : Livrée en totalité Total : 460,87 €
            if(ligne.contains("N° de commande:") && ligne.split(" ").last() != "Rexel")
            {
                etat.clear();
                nomChantier.clear();
                numeroCommande.clear();
                bool error(false);

                //Contrôle
                if(ligne.split(":").count() != 4)
                {
                    m_Fct->FrnError(variable,REXEL,"Compteur ligne 1 = " + QString::number(ligne.split(":").count()));
                    return false;
                }
                if(ligne.split(":").at(2).split(" ").count() < 3)
                {
                    m_Fct->FrnError(variable,REXEL,"Compteur ligne 2 = " + QString::number(ligne.split(":").at(2).split(" ").count()));
                    return false;
                }

                //Numéro de commande
                numeroCommande = ligne.split(":").at(1).split(" ").at(1);

                //status
                if(ligne.contains("Livrée en totalité") || ligne.contains("Livrée et facturée"))
                {
                    etat = QString::number(Close);
                }
                else if(ligne.contains("Partiellement livrée"))
                {
                    etat = QString::number(partial);
                }
                else if(ligne.contains("Annulée"))
                {
                    skip = true;
                }
                else
                {
                    etat = QString::number(open);
                }

                //Lien chantier
                lienChantier = "https://www.rexel.fr/frx/my-account/orders/" + numeroCommande;

                //Date
                ligne = flux.readLine();
                if(ligne.contains("Date :"))
                {
                    date = flux.readLine().replace(".","/");
                }
                else
                {
                    m_Fct->FrnError(variable,REXEL,"date");
                    error = true;
                }

                //Nom chantier
                ligne = flux.readLine();
                if(ligne.contains("Réf. chantier :"))
                {
                    infoChantier = flux.readLine();
                    ligne = flux.readLine();
                }
                if(ligne.contains("Type de commande"))
                {
                    flux.readLine();
                    ligne = flux.readLine();
                }
                //Numero chantier
                if(ligne.contains("Réf. cde :"))
                {
                    nomChantier = flux.readLine();
                }

                //Contrôle
                if(etat.isEmpty())
                {
                    m_Fct->FrnError(variable,REXEL,"Etat");
                    error = true;
                }
                else if(numeroCommande.isEmpty())
                {
                    m_Fct->FrnError(variable,REXEL,"Numéro de commande");
                    error = true;
                }
                else if(nomChantier.isEmpty())
                {
                    m_Fct->FrnError(variable,REXEL,"Numéro de chantier");
                    error = true;
                }
                DEBUG << etat << numeroCommande << nomChantier;

                req = m_DB->Requete("SELECT * FROM En_Cours WHERE Numero_Commande='" + numeroCommande + "' AND Fournisseur='" + REXEL + "'");
                req.next();

                if(req.value("Numero_Commande").isNull() && error == false && skip == false)
                {
                    if(date.split("/").count() != 3)///Si la date n'a pas été trouvé
                    {
                        m_Fct->FrnError(variable,REXEL,"date");
                        return false;
                    }
                    QStringList l = date.split("/");
                    date = l.at(2) + "-" + l.at(1) + "-" + l.at(0);
                    DEBUG << "Navigation - Ajout nouveau BC dans la DB : " << req.prepare("INSERT INTO En_Cours VALUES('" + QString::number(id) + "','" + date + "','" + nomChantier + "','" + numeroCommande + "','','" + lienChantier + "','" + etat + "','','" + infoChantier + "','0','','Rexel.fr')");
                    if(req.exec() == false)
                    {
                        m_Fct->FrnError(requete,REXEL,"Création BC");
                        DEBUG << "Navigation - Echec Ajout";
                        return false;
                    }
                    else if(premierDemarrage)
                    {
                        m_DB->Requete("UPDATE En_Cours SET Ajout='"+QString::number(endAdd)+"' WHERE ID='" + QString::number(id) + "'");
                        fin = true;
                    }
                }
                else
                {
                    DEBUG << "Navigation - Fin d'ajout";
                    fin = true;
                }
            }
        }
    fichier.close();
    }while(nbPage<tPage && fin == false);
    DEBUG << "Fin Rexel::Navigation()";
    return true;
}
///Error code 93x
bool RexelFr::Check_Delivery(const QString InvoiceNumber)
{
    DEBUG << "Rexel::Recuperation_BL()";
    bool dCharger(false);
    DEBUG << "Récupération BL commande " << InvoiceNumber;
    if(InvoiceNumber.split("-").count() != 2)
    {
        m_Fct->FrnError(variable,REXEL,"Check_Delivery | InvoiceNumber=" + InvoiceNumber);
        return false;
    }
    m_Fct->Info(tr("Récupération BL commande %1").arg(InvoiceNumber.split("-").at(1)));
    if(m_Fct->FindTexte("Bons De Livraison (BL)") == false)
    {
        if(m_Fct->WebLoad("https://www.rexel.fr/frx/my-account/delivery") == false)
        {
            m_Fct->FrnError(load,REXEL,"https://www.rexel.fr/frx/my-account/delivery");
        }
    }
    else
    {
        DEBUG << "Recuperation_BL - Page déjà chargée";
        dCharger = true;
    }
    if(m_Fct->FindTexte("Bons de livraison (BL)") == false)
    {
        m_Fct->FrnError(fail_check,REXEL);
    }

    QTimer t;
    m_Fct->Loop(1000);
    if(dCharger == false)
    {
        DEBUG << "Recuperation_BL - Remplissage des champs de recherche";
        m_Fct->InsertJavaScript("document.getElementById('deliveryPropId').options[1].selected = 'selected';");
        QString date = m_Fct->InsertJavaScript("document.getElementById('deliveryEndDate').value;").toString();
        date = date.split(".").at(0) + "." + date.split(".").at(1) + "." + QString::number(date.split(".").at(2).toInt()-1);
        m_Fct->InsertJavaScript("document.getElementById('deliveryStartDate').value = '" + date + "';");
    }
    m_Fct->InsertJavaScript("document.getElementById('searchValueDelivery').value = '" + InvoiceNumber.split("-").at(1) + "';");
    t.start(10000);
    connect(&t,SIGNAL(timeout()),&t,SLOT(stop()));
    m_Fct->InsertJavaScript("$('#deliveryHistoryTblId').empty();ACC.deliveryHistoryPage.ajaxDeliveryHistoryReq();");
    DEBUG << "Recuperation_BL - Chargement AJAX BL";
    do
    {
        m_Fct->Loop(1000);
    }while(m_Fct->InsertJavaScript("$('#deliveryHistoryPageLoader').is(':visible')").toBool() && t.isActive());

    if(t.isActive() == false)
    {
        m_Fct->FrnError(fail_check,REXEL,"Délai écoulé");
        return false;
    }
    if(m_Fct->SaveText() == false)
    {
        m_Fct->FrnError(save_file,REXEL);
        return false;
    }

    ///Traitement des données contenu dans le fichier web_Temp.txt
    /// -------------------------------
    QFile fichier(m_WorkLink + "/web_Temp.txt");
    if(fichier.open(QIODevice::ReadOnly) == false)
    {
        m_Fct->FrnError(open_File,REXEL,fichier.fileName());
    }

    QString bl;
    bool fin(false);
    DEBUG << "Recuperation_BL - Début du traitement des BL";
    while(fichier.atEnd() == false && fin == false)
    {
        QString tmp = fichier.readLine();
        if(tmp.contains("Bon de livraison (BL)"))
        {
            while(fichier.atEnd() == false && fin == false)
            {
                tmp = fichier.readLine();

                if(tmp.contains("Afficher :"))
                {
                    fin = true;
                }
                for(int cpt=0;cpt<tmp.count();cpt++)
                {
                    if(tmp.at(cpt).isDigit() || tmp.at(cpt) == '-')
                    {
                        DEBUG << "Recuperation_BL - Ajout d'un BL";
                        bl.append(tmp[cpt]);
                    }
                    else
                    {
                        DEBUG << "Recuperation_BL - Fin d'ajout d'un BL";
                        cpt = tmp.count();
                    }
                }
                bl.append(" ");
            }
        }
    }
    DEBUG << "Recuperation_BL - Fin du traitement";

    if(bl.split(" ").count() > 5)
    {
        m_Fct->FrnError(too_many,REXEL,QString(bl.split(" ").count()));
        return false;
    }
    m_DB->Requete("UPDATE En_Cours SET Numero_Livraison='" + bl + "' WHERE Numero_Commande='" + InvoiceNumber + "' AND Fournisseur='" + REXEL + "'");
    DEBUG << "Recuperation_BL - DB mise à jour";
    DEBUG << "Fin Rexel::Recuperation_BL()";
    return true;
}
///Error code 94x
bool RexelFr::Check_State(const QString InvoiceNumber)
{
    DEBUG << "Rexel::VerificationEtatBC()";
    m_Fct->Info(tr("Vérification état du bon de commande %1").arg(InvoiceNumber));
    if(m_Fct->WebLoad("https://www.rexel.fr/frx/my-account/orders/" + InvoiceNumber) == false)
    {
        DEBUG << "VerificationEtatBC - Echec chargement de la page";
        return false;
    }
    QString nBC = InvoiceNumber;
    if(nBC.count() > 7)
    {
        nBC.remove(0,nBC.count()-6);
    }
    DEBUG << "VerificationEtatBC - Mise à jour Référence BC :" + nBC;
    if(m_Fct->FindTexte("N° de commande Rexel : " + nBC))
    {
        DEBUG << "VerificationEtatBC - Echec chargement de la page";
        return false;
    }

    if(m_Fct->FindTexte("Livrée En Totalité") || m_Fct->FindTexte("Livrée Et Facturée"))
    {
        DEBUG << "VerificationEtatBC - Mise à jour de l'état du BC " + InvoiceNumber + "=" + "Livrée en totalité";
        m_DB->Requete("UPDATE En_Cours SET Etat='" + QString::number(Close) + "' WHERE Numero_Commande='" + InvoiceNumber + "' AND Fournisseur='" + REXEL + "'");
    }
    else if(m_Fct->FindTexte("Partiellement Livrée"))
    {
        DEBUG << "VerificationEtatBC - Mise à jour de l'état du BC " + InvoiceNumber + "=" + "Partiellement livrée";
        m_DB->Requete("UPDATE En_Cours SET Etat='" + QString::number(partial) + "' WHERE Numero_Commande='" + InvoiceNumber + "' AND Fournisseur='" + REXEL + "'");
    }
    DEBUG << "Fin Rexel::VerificationEtatBC()";
    return true;
}
///Error code 95x
QStringList RexelFr::Get_Invoice(const QString InvoiceNumber)
{
    DEBUG << "Rexel::AffichageTableau()";
    //Retourne une list d'un tableau de commande
    //0 = nb commande
    //boucle de 6 strings designation,reference,fabricant,prix unitaire,quantité livré,quantité restante
    m_Fct->Change_Load_Window(tr("Connexion à Rexel.fr"));
    if(Connexion() == false) { return QStringList("Erreur"); }
    m_Fct->Change_Load_Window(tr("Chargement de la commande"));
    if(m_Fct->WebLoad("https://www.rexel.fr/frx/my-account/orders/" + InvoiceNumber) == false)
    {
        return QStringList("Erreur");
    }
    m_Fct->SaveHtml();

    QFile fichier(m_WorkLink + "/web_Temp.txt");
    if(fichier.open(QIODevice::ReadOnly) == false)
    {
        m_Fct->FrnError(open_File,REXEL,fichier.fileName());
        DEBUG << "AffichageTableau - Echec ouverture du fichier " + fichier.fileName();
        return QStringList("Erreur");
    }
    QTextStream flux2(&fichier);
    QStringList l;
    QString var;
    int etat(6);

    DEBUG << "AffichageTableau - Démarrage traitement des information";
    while(flux2.atEnd() == false)
    {
        var = flux2.readLine();
        if(var.contains("<th class=\"offscreen\" headers=\"header1\" scope=\"row\">") && var.contains("APPAREIL DE CHAUFFAGE ELECTRIQUE") == false &&
                var.contains("TAXE DE BASE") == false && var.contains("EQUIPEMENT POUR LA VENTILATION") == false &&
                var.contains("PRODUIT RELEVANT du DECRET") == false && var.contains("EQUIPEMENT DE CONTROLE ET DE SURVEILLANCE") == false &&
                var.contains("PETIT OUTILLAGE (HORS PERCEUSE ET VISSEUSE)") == false && etat == 6)
        {
            etat = 0;
        }
        if(etat == 0)//Designation
        {
            DEBUG << "AffichageTableau - Désignation";
            l.append(var.split(">").at(1).split("</").at(0));
            etat = 1;
        }
        else if((var.contains("<a href=\"/frx/") || var.contains("missing-product-96x96.jpg")) && etat == 1)//Reference
        {
            if(var.contains("<a href=\"/frx/"))//Référence connue de rexel
            {
                DEBUG << "AffichageTableau - Référence";
                QStringList l2 = var.split("\"").at(1).split("/");
                QString s = l2.at(l2.count()-3);
                QString s2;
                s2.append(s[0]);
                s2.append(s[1]);
                s2.append(s[2]);
                s.replace(s2,s2 + " ");
                s.replace("%2C",",");
                s.replace("%3C","<");
                l.append(s);
            }
            else
            {
                DEBUG << "AffichageTableau - Référence inconnue";
                l.append("");
            }
            etat = 2;
        }
        else if(var.contains("<span class=\"item-Label\"> Fab :") && etat == 2)//fabricant
        {
            DEBUG << "AffichageTableau - Fabricant";
            l.append(var.split(">").at(4).split("<").at(0));
            l.append(var.split("\"").at(5).split("/").last());
            etat = 3;
        }
        else if(var.contains("header3") && etat == 2)//fabricant absent
        {
            DEBUG << "AffichageTableau - Fabricant inconnue";
            l.append("");
            l.append("");
            etat = 3;
        }
        else if(var.contains("headers=\"header4\"") &&  etat == 3)//Prix unitaire
        {
            DEBUG << "AffichageTableau - Prix";
            var = flux2.readLine();
            if(var.contains("span class") == false)
            {
                var = flux2.readLine();
            }
            l.append(var.split(">").at(1).split(" ").at(1));
            etat = 4;
        }
        else if(var.contains("header5") && etat == 4)//Quantité
        {
            DEBUG << "AffichageTableau - Quantité total";
            l.append(var.split(">").at(2).split("<").at(0));
            etat = 5;
        }
        else if(var.contains("header6") && etat == 5)//Quantité restante
        {
            DEBUG << "AffichageTableau - Quantité restante";
            var = flux2.readLine();
            var.replace("   ","");
            l.append(QString::number(var.split("<").at(0).toInt()));
            etat = 6;
        }
    }
    if(etat != 6)
    {
        DEBUG << "AffichageTableau - Erreur, Liste incomplète :" << etat;
        m_Fct->FrnError(variable,REXEL,l.at(l.count()-1) + " Etat=" + QString::number(etat));
        l.clear();
    }
    DEBUG << "Fin Rexel::AffichageTableau()";
    return l;
}

void RexelFr::Set_Var(const QString login, const QString mdp, const QString comp)
{
    m_Login = login;
    m_MDP = mdp;
    m_UserName = comp;
}

bool RexelFr::Test_Connexion()
{
    DEBUG << "TEST CONNEXION REXELFR";
    if(Connexion() == false)
    {
        if(m_Fct->FindTexte("Informations invalides, veuillez réessayer"))
        {
            m_Fct->FrnError(bad_Login,REXEL);
        }
        return false;
    }
    else
        return true;
}

QString RexelFr::Get_Info()
{
    return QString(INF);
}
