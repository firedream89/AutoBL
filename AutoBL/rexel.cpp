#include "rexel.h"

Rexel::Rexel(QString lien_Travail,QString login,QString mdp):
    m_Lien_Work(lien_Travail),m_Login(login),m_MDP(mdp)
{
    qDebug() << "Init Class Rexel";
    cookieJar = new QNetworkCookieJar;
    web = new QWebView;
    web->page()->networkAccessManager()->setCookieJar(cookieJar);
}
Rexel::~Rexel()
{
    delete cookieJar;
    web->deleteLater();
}
///Erreur 0xx
bool Rexel::Connexion(QString login, QString MDP)
{
    qDebug() << "Rexel::Connexion()";
    emit InfoFen("Info","Chargement...");
    if(!webLoad("https://www.rexel.fr/frx"))
    {
        EmitErreur(001,0);
        qDebug() << "Connexion - Chargement échoué";
        return false;
    }

    //Vérification si déjà connecté
    QSqlQuery req = m_db.Requete("SELECT Valeur FROM Options WHERE ID='15'");
    req.next();
    if(Verification(req.value("Valeur").toString(),"Connexion Rexel",true))
    {
        qDebug() << "Connexion - Déjà connecté";
        return true;
    }

    if(Verification("Site en maintenance","USER | Rexel Maintenance",true))
    {
        EmitErreur(002,0,"Maintenance Rexel");
        qDebug() << "Connexion - Rexel est en maintenance";
        return false;
    }
    QEventLoop loop;
    QTimer t;
    connect(&t,SIGNAL(timeout()),&loop,SLOT(quit()));
    connect(web,SIGNAL(loadFinished(bool)),&loop,SLOT(quit()));
    QWebFrame *frame = web->page()->mainFrame();
    frame->evaluateJavaScript("document.getElementById('j_username').value=\"" + login + "\"");
    frame->evaluateJavaScript("document.getElementById('j_password').value=\"" + MDP + "\"");
    t.start(500);
    loop.exec();
    frame->evaluateJavaScript("document.getElementById('loginForm').submit()");
    t.start(5000);
    loop.exec();
    if(Verification("Votre compte a été verrouillé, veuillez contacter ladministrateur","Rexel est en maintenance.",true))
    {
        qDebug() << "Connexion - Rexel est en maintenance";
        return false;
    }
    qDebug() << "Fin Rexel::Connexion()";
    return Verification(req.value("Valeur").toString(),"Connexion Rexel");
}
///Erreur 1xx
bool Rexel::Navigation()
{
    qDebug() << "Rexel::Navigation()";
    ///Récuperation de la liste de bon de commandes et enregistrement dans le fichier web_Temp.txt
    if(!webLoad("https://www.rexel.fr/frx/my-account/orders"))
    {
        EmitErreur(101,0);
        qDebug() << "Navigation - Chargement échoué";
        return false;
    }
    if(!Verification("Suivi de commandes","Accès suivi de commandes"))
    {
        EmitErreur(102,0);
        qDebug() << "Navigation - Echec page";
        return false;
    }

    QFile fichier(m_Lien_Work + "/web_Temp.txt");
    fichier.resize(0);
    if(!fichier.open(QIODevice::WriteOnly))
    {
        EmitErreur(103,1,fichier.fileName());
        qDebug() << "Navigation - Echec ouverture fichier " + fichier.fileName();
        return false;
    }
    ///Récupération du texte de page de commandes et placement en fichier
    QTextStream flux(&fichier);
    QTimer t,t2;
    QEventLoop l;
    QSqlQuery r = m_db.Requete("SELECT Valeur FROM Options WHERE ID='19'");
    r.next();
    connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
    connect(&t2,SIGNAL(timeout()),&t2,SLOT(stop()));
    int nbPage(0),tPage(5);
    t.start(r.value("Valeur").toInt()*2000);
    DEBUG << r.value(0) << "Début tempo navigation";
    l.exec();
    DEBUG << "Fin tempo navigation";
    t2.start(r.value(0).toInt()*2000);

    do
    {
        t.start(1000);
        l.exec();
        DEBUG << web->page()->mainFrame()->evaluateJavaScript("$('#accountHistoryPageLoader').is(':visible')").toBool();
    }while(web->page()->mainFrame()->evaluateJavaScript("$('#accountHistoryPageLoader').is(':visible')").toBool() && t2.isActive());
    while(nbPage < tPage-1)
    {
        t2.start(r.value(0).toInt()*2000);
        web->page()->mainFrame()->evaluateJavaScript("var nextPage=ACC.orderHistoryPage.currentPageNo+1;ACC.orderHistoryPage.ajaxOrderHistoryReq(nextPage,ACC.orderHistoryPage.sortCriteria,ACC.orderHistoryPage.isAscendingVal);");
        do
        {
            t.start(1000);
            l.exec();
            DEBUG << web->page()->mainFrame()->evaluateJavaScript("$('#accountHistoryPageLoader').is(':visible')").toBool();
        }while(web->page()->mainFrame()->evaluateJavaScript("$('#accountHistoryPageLoader').is(':visible')").toBool() && t2.isActive());
        nbPage = web->page()->mainFrame()->evaluateJavaScript("ACC.orderHistoryPage.currentPageNo;").toInt();
        tPage = web->page()->mainFrame()->evaluateJavaScript("ACC.orderHistoryPage.totalPageNo;").toInt();
        qDebug() << "Navigation - Chargement AJAX " << nbPage << "/" << tPage;
        emit InfoFen("Info","Chargement page " + QString::number(nbPage) + "/" + QString::number(tPage));
    }
    web->page()->triggerAction(QWebPage::SelectAll);
    emit InfoFen("Info","Traitement des informations");
    flux <<  web->page()->selectedText();
    fichier.close();

    qDebug() << "Navigation - Fin de copie dans le fichier " + fichier.fileName();
    ///Traitement des données contenu dans le fichier web_Temp.txt
    QSqlQuery req;
    bool premierDemarrage(false);
    QString nomChantier, lienChantier, numeroCommande, etat, date, infoChantier;
    if(!fichier.open(QIODevice::ReadOnly))
    {
        EmitErreur(104,1,fichier.fileName());
        qDebug() << "Navigation - Echec ouverture du fichier " + fichier.fileName();
        return false;
    }
    flux.seek(0);
    bool fin = false;

    qDebug() << "Navigation - Début du traitement des informations";
    req = m_db.Requete("SELECT MAX(ID) FROM En_Cours");
    req.next();
    if(req.value(0).toInt() == 0)
        premierDemarrage = true;

    while(!flux.atEnd() && !fin)
    {
        int id = 0;
        req.prepare("SELECT MAX(ID) FROM En_Cours");
        req.exec();
        req.next();
        id = req.value(0).toInt();
        id++;

        infoChantier = "";
        QString ligne = flux.readLine();
        if(ligne.contains("N° de commande Rexel"))
        {
            bool error(false);
            qDebug() << id;
            numeroCommande = ligne.split(" ").last();
            lienChantier = "https://www.rexel.fr/frx/my-account/orders/" + numeroCommande;
            ligne = flux.readLine();
            if(ligne.contains("Date :"))
                date = ligne.split(" ").last().replace(".","/");
            else
            {
                EmitErreur(106,6,numeroCommande + " Date");
                error = true;
            }
            ligne = flux.readLine();
            if(ligne.contains("Réf. cde :"))
            {
                nomChantier = ligne.replace("Réf. cde : ","");
                nomChantier.replace(" ","");
            }
            else
            {
                EmitErreur(107,6,numeroCommande + " Référence");
                error = true;
            }
            ligne = flux.readLine();
            if(ligne.contains("Réf. chantier :"))
                infoChantier = ligne.replace("Réf. chantier : ","");
            ligne = flux.readLine();
            if(ligne.contains("Total:"))
                ligne = flux.readLine();
            if(ligne.contains("Status:"))
                etat = ligne.replace("Status: ","");
            else
            {
                EmitErreur(108,6,ligne + " Etat");
                error = true;
            }

            req = m_db.Requete("SELECT * FROM En_Cours WHERE Numero_Commande='" + numeroCommande + "'");
            req.next();
            if(req.value("Numero_Commande").isNull() && !error)
            {
                if(date.split("/").count() != 3)///Si la date n'a pas été trouvé
                {
                    EmitErreur(108,6,"Date");
                    return false;
                }
                QStringList l = date.split("/");
                date = l.at(2) + "-" + l.at(1) + "-" + l.at(0);
                qDebug() << "Navigation - Ajout nouveau BC dans la DB : " << req.prepare("INSERT INTO En_Cours VALUES('" + QString::number(id) + "','" + date + "','" + nomChantier + "','" + numeroCommande + "','','" + lienChantier + "','" + etat + "','','" + infoChantier + "','0','')");
                if(!req.exec())
                {
                    EmitErreur(105,4,req.lastError().text());
                    qDebug() << "Navigation - Echec Ajout";
                    return false;
                }
                else if(premierDemarrage)
                {
                    m_db.Requete("UPDATE En_Cours SET Ajout='Ok' WHERE ID='1'");
                    fin = true;
                }
            }
            else
            {
                qDebug() << "Navigation - Fin d'ajout";
                fin = true;
            }
        }
    }
    emit InfoFen("Info","");
    //cp->deleteLater();
    qDebug() << "Fin Rexel::Navigation()";
    return true;
}
///Erreur 3xx
bool Rexel::Verification(QString texte, QString reponse, bool bloquer)
{
    if(web->page()->findText(texte))
    {
        emit Info(reponse + " Réussis");
        return true;
    }
    if(!bloquer)
        EmitErreur(301,3,reponse);
    return false;
}
///Erreur 5xx
void Rexel::Affichage()
{
    web->show();
}
///Erreur 6xx
bool Rexel::Recuperation_BL(QString numero_Commande)
{
    qDebug() << "Rexel::Recuperation_BL()";
    bool dCharger(false);
    emit InfoFen("Info","Récupération BL commande " + numero_Commande.split("-").at(1));
    if(!Verification(tr("Bons de livraison (BL)"),tr("Accès BL Commande N° ") + numero_Commande,true))
    {
        if(!webLoad("https://www.rexel.fr/frx/my-account/delivery"))
        {
            EmitErreur(601,0);
            qDebug() << "Recuperation_BL - Echec chargement de page";
            return false;
        }
    }
    else
    {
        qDebug() << "Recuperation_BL - Page déjà chargée";
        dCharger = true;
    }
    if(!Verification(tr("Bons de livraison (BL)"),tr("Accès BL Commande N° ") + numero_Commande))
    {
        EmitErreur(602,0);
        qDebug() << "Recuperation_BL - Page non chargée";
        return false;
    }
    QEventLoop l;
    QTimer t,t2;
    connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
    t.start(1000);
    l.exec();
    if(!dCharger)
    {
        qDebug() << "Recuperation_BL - Remplissage des champs de recherche";
        web->page()->mainFrame()->evaluateJavaScript("document.getElementById('deliveryPropId').options[1].selected = 'selected';");
        QString date = web->page()->mainFrame()->evaluateJavaScript("document.getElementById('deliveryEndDate').value;").toString();
        date = date.split(".").at(0) + "." + date.split(".").at(1) + "." + QString::number(date.split(".").at(2).toInt()-1);
        web->page()->mainFrame()->evaluateJavaScript("document.getElementById('deliveryStartDate').value = '" + date + "';");
    }
    web->page()->mainFrame()->evaluateJavaScript("document.getElementById('searchValueDelivery').value = '" + numero_Commande.split("-").at(1) + "';");
    t2.start(10000);
    connect(&t2,SIGNAL(timeout()),&t2,SLOT(stop()));
    web->page()->mainFrame()->evaluateJavaScript("$('#deliveryHistoryTblId').empty();ACC.deliveryHistoryPage.ajaxDeliveryHistoryReq();");
    qDebug() << "Recuperation_BL - Chargement AJAX BL";
    do
    {
        t.start(1000);
        l.exec();
    }while(web->page()->mainFrame()->evaluateJavaScript("$('#deliveryHistoryPageLoader').is(':visible')").toBool() && t2.isActive());

    if(!t2.isActive())
        return false;
    QFile fichier(m_Lien_Work + "/web_Temp.txt");
    fichier.resize(0);
    if(!fichier.open(QIODevice::WriteOnly))
    {
        EmitErreur(603,1,fichier.fileName());
        qDebug() << "Recuperation_BL - Echec ouverture du fichier " + fichier.fileName();
        return false;
    }
    QTextStream flux(&fichier);
    web->page()->triggerAction(QWebPage::SelectAll);
    flux << web->page()->selectedText();
    fichier.close();
    qDebug() << "Recuperation_BL - Ecriture dans le fichier " + fichier.fileName() + " terminé";
    ///Traitement des données contenu dans le fichier web_Temp.txt
    /// -------------------------------
    ///Vérifier si les bl sont dispo sur les En Cours/Fermée sur rexel--------------------------
    /// -------------------------------
    if(!fichier.open(QIODevice::ReadOnly))
    {
        EmitErreur(604,1,fichier.fileName());
        qDebug() << "Recuperation_BL - Echec ouverture du fichier " + fichier.fileName();
        return false;
    }
    QString bl;
    bool fin(false);
    qDebug() << "Recuperation_BL - Début du traitement des BL";
    while(!fichier.atEnd() && !fin)
    {
        QString tmp = fichier.readLine();
        if(tmp.contains("Bon de livraison (BL)"))
            while(!fichier.atEnd() && !fin)
            {
                tmp = fichier.readLine();
                if(!tmp.contains("Rexel France") && tmp.isSimpleText())
                {
                    for(int cpt=0;cpt<tmp.count();cpt++)
                    {
                        if(!tmp[cpt].isSpace())
                        {
                            qDebug() << "Recuperation_BL - Ajout d'un BL";
                            bl.append(tmp[cpt]);
                        }
                        else
                        {
                            qDebug() << "Recuperation_BL - Fin d'ajout d'un BL";
                            cpt = tmp.count();
                        }
                    }
                    bl.append(" ");
                }
                else
                    fin = true;
            }
    }
    qDebug() << "Recuperation_BL - Fin du traitement";
    if(bl.split(" ").count() > 5)
    {
        EmitErreur(620,7,"BL Supérieure à 5");
        qDebug() << "Recuperation_BL - Erreur plus de 5 BL trouvée : " + bl.split(" ").count();
        return false;
    }
    m_db.Requete("UPDATE En_Cours SET Numero_Livraison='" + bl + "' WHERE Numero_Commande='" + numero_Commande + "'");
    qDebug() << "Recuperation_BL - DB mise à jour";
    qDebug() << "Fin Rexel::Recuperation_BL()";
    emit InfoFen("Info","");
    return true;
}
///Erreur 7xx
bool Rexel::VerificationEtatBC(QString numeroCommande)
{
    qDebug() << "Rexel::VerificationEtatBC()";
    emit InfoFen("Info","Vérification état BC " + numeroCommande);
    if(!webLoad("https://www.rexel.fr/frx/my-account/orders/" + numeroCommande))
    {
        qDebug() << "VerificationEtatBC - Echec chargement de la page";
        return false;
    }
    QString nBC = numeroCommande;
    if(nBC.count() > 7)
        nBC.remove(0,nBC.count()-6);
    qDebug() << "VerificationEtatBC - Mise à jour Référence BC :" + nBC;
    if(!Verification(tr("N° de commande Rexel : %0").arg(nBC),tr("Vérification Etat Commande N° ") + nBC))
    {
        qDebug() << "VerificationEtatBC - Echec chargement de la page";
        return false;
    }

    if(Verification("Livrée en totalité","Etat BC",true) || Verification("Livrée et facturée","Etat BC",true))
    {
        qDebug() << "VerificationEtatBC - Mise à jour de l'état du BC " + numeroCommande + "=" + "Livrée en totalité";
        m_db.Requete("UPDATE En_Cours SET Etat='Livrée en totalité' WHERE Numero_Commande='" + numeroCommande + "'");
    }
    else if(Verification("Partiellement livrée","Etat BC",true))
    {
        qDebug() << "VerificationEtatBC - Mise à jour de l'état du BC " + numeroCommande + "=" + "Partiellement livrée";
        m_db.Requete("UPDATE En_Cours SET Etat='Partiellement livrée' WHERE Numero_Commande='" + numeroCommande + "'");
    }
    emit InfoFen("Info","");
    qDebug() << "Fin Rexel::VerificationEtatBC()";
    return true;
}
///Erreur 8xx
bool Rexel::webLoad(QString lien)
{
    qDebug() << "Rexel::webLoad()";
    QEventLoop loop;
    QTimer timer;
    connect(&timer,SIGNAL(timeout()),&loop,SLOT(quit()));
    connect(&timer,SIGNAL(timeout()),&timer,SLOT(stop()));
    connect(web,SIGNAL(loadFinished(bool)),&loop,SLOT(quit()));
    connect(web,SIGNAL(loadProgress(int)),this,SIGNAL(LoadProgress(int)));

    for(int cpt=0;cpt<2;cpt++)
    {
        qDebug() << "webLoad - Chargement de la page " << lien;        
        web->load(QUrl(lien));
        timer.start(15000);
        loop.exec();
        qDebug() << "Fin Rexel::webLoad()";
        if(timer.isActive())
            return true;
    }
    EmitErreur(801,0,lien);
    return false;
}

void Rexel::ResetWeb()
{
    qDebug() << "Rexel::ReseWeb()";
    web->close();
    web = new QWebView;
    web->page()->networkAccessManager()->setCookieJar(cookieJar);
    //web->page()->settings()->setAttribute(QWebSettings::JavaEnabled,false);
    connect(web->page(),SIGNAL(unsupportedContent(QNetworkReply*)),this,SLOT(Telechargement(QNetworkReply*)));
    qDebug() << "Fin Rexel::webLoad()";
}

///Erreur 9xx
QStringList Rexel::AffichageTableau()
{
    qDebug() << "Rexel::AffichageTableau()";
    //Retourne une list d'un tableau de commande
    //0 = nb commande
    //boucle de 6 strings designation,reference,fabricant,prix unitaire,quantité livré,quantité restante
    QFile fichier(m_Lien_Work + "/web_Temp.txt");
    fichier.resize(0);
    qDebug() << "AffichageTableau - Copie de la page";
    fichier.open(QIODevice::WriteOnly);
    QTextStream flux(&fichier);
    flux << web->page()->mainFrame()->toHtml();
    fichier.close();
    if(!fichier.open(QIODevice::ReadOnly))
    {
        EmitErreur(901,1,fichier.fileName());
        qDebug() << "AffichageTableau - Echec ouverture du fichier " + fichier.fileName();
        return QStringList("Erreur");
    }
    fichier.seek(0);
    QTextStream flux2(&fichier);
    QStringList l;
    QString var;
    int etat(6);

    qDebug() << "AffichageTableau - Démarrage traitement des information";
    while(!flux2.atEnd())
    {
        var = flux2.readLine();
        if(var.contains("<th class=\"offscreen\" headers=\"header1\" scope=\"row\">") && !var.contains("APPAREIL DE CHAUFFAGE ELECTRIQUE") &&
                !var.contains("TAXE DE BASE") && !var.contains("EQUIPEMENT POUR LA VENTILATION") &&
                !var.contains("PRODUIT RELEVANT DU DECRET") && !var.contains("EQUIPEMENT DE CONTROLE ET DE SURVEILLANCE") &&
                !var.contains("PETIT OUTILLAGE (HORS PERCEUSE ET VISSEUSE)") && etat == 6)
        {
            etat = 0;
        }
        if(etat == 0)//Designation
        {
            qDebug() << "AffichageTableau - Désignation";
            l.append(var.split(">").at(1).split("</").at(0));
            etat = 1;
        }
        else if((var.contains("<a href=\"/frx/") || var.contains("missing-product-96x96.jpg")) && etat == 1)//Reference
        {
            if(var.contains("<a href=\"/frx/"))//Référence connue de rexel
            {
                qDebug() << "AffichageTableau - Référence";
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
                qDebug() << "AffichageTableau - Référence inconnue";
                bool t(false);
                while(!t && !flux2.atEnd())
                {
                    var = flux2.readLine();
                    if(var.contains("<span class=\"item-Label\"> Réf :"))
                    {
                        QString s = var.split(">").at(2).split("<").at(0);
                        for(int cpt = 0;cpt<s.count();cpt++)
                        {
                            if(s.at(cpt).isLetterOrNumber())
                                cpt = s.count();
                            s.remove(0,1);
                        }
                        l.append(s);
                        t = true;
                    }
                }
            }
            etat = 2;
        }
        else if(var.contains("<span class=\"item-Label\"> Fab :") && etat == 2)//fabricant
        {
            qDebug() << "AffichageTableau - Fabricant";
            l.append(var.split(">").at(4).split("<").at(0));
            l.append(var.split("\"").at(5).split("/").last());
            etat = 3;
        }
        else if(var.contains("header3") && etat == 2)//fabricant absent
        {
            qDebug() << "AffichageTableau - Fabricant inconnue";
            l.append("");
            l.append("");
            etat = 3;
        }
        else if(var.contains("headers=\"header4\"") &&  etat == 3)//Prix unitaire
        {
            qDebug() << "AffichageTableau - Prix";
            var = flux2.readLine();
            if(!var.contains("span class"))
                var = flux2.readLine();
            var.replace(" ","");
            var.replace("&nbsp;","");
            l.append(var.split(">").at(1).split("€").at(0));
            etat = 4;
        }
        else if(var.contains("header5") && etat == 4)//Quantité
        {
            qDebug() << "AffichageTableau - Quantité total";
            l.append(var.split(">").at(2).split("<").at(0));
            etat = 5;
        }
        else if(var.contains("header6") && etat == 5)//Quantité restante
        {
            qDebug() << "AffichageTableau - Quantité restante";
            var = flux2.readLine();
            var.replace("   ","");
            l.append(QString::number(var.split("<").at(0).toInt()));
            etat = 6;
        }
    }
    if(etat != 6)
    {
        qDebug() << "AffichageTableau - Erreur, Liste incomplète :" << etat;
        EmitErreur(902,5,l.at(l.count()-1) + " Etat=" + QString::number(etat));
        l.clear();
    }
    qDebug() << l;
    qDebug() << "Fin Rexel::AffichageTableau()";
    return l;
}

void Rexel::EmitErreur(int codeErreur,int stringErreur,QString info)
{
    QString ret;
    switch(stringErreur)
    {
    case 0:
        ret = "Chargement de la page échouée";
        break;
    case 1:
        ret = "Ouverture fichier échoué";
        break;
    case 2:
        ret = "Récupération état du bon de commande échouée";
        break;
    case 3:
        ret = "Vérification page du site échouée";
        break;
    case 4:
        ret = "Requête DB échoué";
        break;
    case 5:
        ret = "Création du tableau échoué";
    case 6:
        ret = "Echec de récupération d'une variable";
    default:
        ret = "Erreur inconnue";
        break;
    }
    emit Erreur("Rexel | E" + QString::number(codeErreur) + " | " + ret + " : " + info);
}

bool Rexel::Start(QString &error, QStringList &list, int option, QString numeroBC)
{
    //0 = navigation + verifBC + RecupBL
    //1 = Récupération du tableau BC
    QSqlQuery req;
    if(option == 1 && numeroBC == "")
        return false;
    if(!Connexion(m_Login,m_MDP))
    {
        error.append("Echec de connexion au site Rexel.fr");
        return false;
    }

    switch (option) {
    case 0:
        if(!Navigation())
            error.append(tr("Echec de récupération des données"));
        //Véfification des numéro de chantier
        req = m_db.Requete("SELECT * FROM En_Cours WHERE Ajout=''");
        while(req.next())
        {
            if(req.value("Nom_Chantier").toString().at(0).isDigit() && req.value("Nom_Chantier").toString().at(req.value("Nom_Chantier").toString().count()-1).isDigit())
                m_db.Requete("UPDATE En_Cours SET Ajout='Telecharger' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
            else
                m_db.Requete("UPDATE En_Cours SET Ajout='Erreur' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
        }
        //Vérification état BC
        req = m_db.Requete("SELECT * FROM En_Cours WHERE Etat='En préparation' OR Etat='Partiellement livrée'");
        while(req.next())
            if(VerificationEtatBC(req.value("Numero_Commande").toString()))
                error.append(tr("Vérification état du bon de commande %0 échoué").arg(req.value("Numero_Commande").toString()));
        //Récupération des BL
        req = m_db.Requete("SELECT * FROM En_Cours WHERE Etat='Livrée en totalité' OR Etat='Livrée et facturée'");
        while(req.next())
            if(req.value("Numero_Livraison").toString().isEmpty())
                if(!Recuperation_BL(req.value("Numero_Commande").toString()))
                    error.append(tr("Récupération du bon de livraison %0 échoué").arg(req.value("Numero_Livraison").toString()));
        break;
    case 1:
        if(!webLoad("https://www.rexel.fr/frx/my-account/orders/" + numeroBC))
        {
            error.append(tr("Chargement du bon de commande %0 échoué").arg(numeroBC));
            return false;
        }
        list = AffichageTableau();
        break;
    default:
        error.append(tr("option %0 inconnue").arg(option));
        return false;
        break;
    }
    return true;
}

void Rexel::Set_Var(QString login, QString mdp)
{
    m_Login = login;
    m_MDP = mdp;
}
