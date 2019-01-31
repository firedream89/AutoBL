#include "esaboraelec.h"
#include <Windows.h>

EsaboraElec::EsaboraElec(QWidget *fen, QString Login, QString MDP, QString Lien_Esabora, QString Lien_Travail, DB *db,Error *e):
    m_fen(fen),m_Login(Login),m_MDP(MDP),m_Lien_Esabora(Lien_Esabora),m_Lien_Work(Lien_Travail),m_DB(db),err(e),etat(0)
{
    qDebug() << "Init Class Esabora";
    m_Arret = false;
    m_Open = false;
}

EsaboraElec::~EsaboraElec()
{
    Clavier("Purge");
}
//Reste 1
bool EsaboraElec::Start(bool automatic,int &nbBC,int &nbBL)
{
    m_Arret = false;
    m_List_Cmd.clear();

    if(Lancement_API() == false) { return false; }

    ///Ajout des BC sur esabora
    QSqlQuery req = m_DB->Get_Download_Invoice();
    Ouverture_Liste_BC();
    DEBUG << "Liste BC ouverte";
    while(req.next() && m_Arret == false)
    {
        qDebug() << "Ajout BC";
        emit Info(tr("Ajout du bon de commande %0").arg(req.value("Numero_Commande").toString()));
        if(Get_List_Matos(req.value("Numero_Commande").toString()))
        {
            //req.value("Nom_Chantier").toString() == "0"
            if(Ajout_BC(req.value("Numero_Commande").toString()) == false)
            {
                Abort();
                Ouverture_Liste_BC();
                if(GetEtat() == 0)
                {
                    err->Err(Not_BC,req.value("Numero_Commande").toString(),ESAB);
                }
                else if(GetEtat() == 1)
                {
                    err->Err(BC,req.value("Numero_Commande").toString(),ESAB);
                    m_DB->Requete("UPDATE En_Cours SET Ajout='"+QString::number(error)+"' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
                }
            }
            else
            {
                m_DB->Requete("UPDATE En_Cours SET Ajout='" + QString::number(add) + "' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
                nbBC++;
            }
        }
        liste_Matos.clear();
    }
    Abort();


    ///Ajout des BL sur esabora
    if(Open_Delivery_Window() == false)
    {
        err->Err(UnknownError,"Impossible d'ouvrir 'Reception de commandes magasinier'",ESAB);
        return false;
    }
    req = m_DB->Get_Added_Invoice();
    while(req.next())
    {
        m_List_Cmd.append(req.value("Numero_Commande").toString());
    }

    for(int cpt=0;cpt<m_List_Cmd.count();cpt++)
    {
        if(m_Arret)
        {
            cpt = m_List_Cmd.count();
        }
        QSqlQuery req = m_DB->Requete("SELECT * FROM En_Cours WHERE Numero_Commande='" + m_List_Cmd.at(cpt) + "'");
        if(req.next())
        {
            if(Ajout_BL(req.value("Numero_BC_Esabora").toString(),req.value("Numero_Livraison").toString()) == false)
            {
                Abort();
                if(Open_Delivery_Window() == false)
                {
                    err->Err(UnknownError,"Impossible d'ouvrir 'Reception de commandes magasinier'",ESAB);
                    return false;
                }
                err->Err(BL,req.value("Numero_Livraison").toString(),ESAB);
            }
            else
            {
                nbBL++;
                emit Info(tr("Ajout BL N°%0 Réussi").arg(req.value("Numero_Livraison").toString()));
                m_DB->Requete("UPDATE En_Cours SET Ajout='"+QString::number(endAdd)+"' WHERE Numero_Commande='" + m_List_Cmd.at(cpt) + "'");
            }
        }
        else
        {
            err->Err(requete,m_List_Cmd.at(cpt),ESAB);
        }
    }

    return Fermeture_API();
}

bool EsaboraElec::SetFocus(QString windowName)
{
    HWND hWnds = FindWindow(NULL,windowName.toStdWString().c_str());
    if(hWnds != NULL)
    {
        SetFocus(hWnds);
        if(GetForegroundWindow() == hWnds)
        {
            return true;
        }
    }
    return false;
}

bool EsaboraElec::Lancement_API()
{
    qDebug() << "Esabora::Lancement_API()";

    if(m_Open)
    {
        Abort();
        return true;
    }

    QSqlQuery r = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='12'");//Nom BDD
    r.next();
    QString db = r.value("Valeur").toString();
    r = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='23'");//Nom Entreprise
    r.next();
    QString ent = r.value("Valeur").toString();

    p.setProgram(m_Lien_Esabora);
    if(m_Lien_Esabora.contains("Simul_Esabora"))
    {
        p.start();
    }
    else if(Verification_Fenetre(ent + " - SESSION : 1 - REPERTOIRE : " + db))
    {
        if(Verification_Focus(ent + " - SESSION : 1 - REPERTOIRE : " + db,true))
        {
            DEBUG << "Esabora Ouvert et au premier plan";
            Abort();
            Fermeture_API();
        }
        else
        {
            DEBUG << "Esabora déjà ouvert";
            if(Set_Focus(ent + " - SESSION : 1 - REPERTOIRE : " + db)) { Fermeture_API(); }
            else { return false; }
        }
    }

    if(QDesktopServices::openUrl(m_Lien_Esabora) == false)
    {
        err->Err(Run_Esabora,"",ESAB);
        qDebug() << "Lancement_API - Echec ouverture d'Esabora.elec";
        qDebug() << "Fin Esabora::Lancement_API()";
        return false;
    }
    if(Traitement_Fichier_Config("Open") == false)
    {
        err->Err(open_File,"Config.esab",ESAB);
        qDebug() << "Lancement_API - Echec du traitement du login sur Esabora.elec";
        qDebug() << "Fin Esabora::Lancement_API()";
        return false;
    }
    qDebug() << "Fin Esabora::Lancement_API()";
    m_Open = true;
    return true;

}

bool EsaboraElec::Ouverture_Liste_BC()
{
    qDebug() << "EsaboraElec::Ouverture_Liste_BC()";
    if(Traitement_Fichier_Config("Liste_BC")) { return true; }
    return false;
}

bool EsaboraElec::Open_Delivery_Window()
{
    qDebug() << "Esabora::Open_Delivery_Window()";
    if(Traitement_Fichier_Config("Open_BL")) { return true; }
    return false;
}

QString EsaboraElec::Find_Fabricant(QString Fab)
{
    QClipboard *pp = QApplication::clipboard();
    pp->clear();
    DEBUG << "Contrôle/Ouverture Catalogue";
    if(Lancement_API() == false) { return QString(); }
    if(Verification_Fenetre("Recherche Produits") == false)
    {
        Abort();
        if(Traitement_Fichier_Config("Open_Cat") == false)
        {
            err->Err(Traitement,ESAB,"Find_Fabricant");
            return QString();
        }
    }
    DEBUG << "Recherche du Fabricant " << Fab;
    if(Traitement_Fichier_Config("Cat",Fab) == false)
    {
        err->Err(Traitement,ESAB,"Find_Fabricant");
        return QString();
    }
    DEBUG << "Test si Fabricant trouvé";
    QString var;
    if(pp->text().isEmpty() || pp->text().contains("(") == false)
    {
        DEBUG << "Constructeur " + Fab + " non trouvé sur Esabora";
    }
    else if(pp->text().contains("Vous n'avez pas les droits pour accéder à cette option !"))
    {
        DEBUG << "Le catalogue produits n'est pas accessible !";
    }

    else
    {
        var = pp->text().split("(").at(1);
        var = var.split(")").at(0);
    }
    return var;

}

bool EsaboraElec::Ajout_BC(QString Numero_Commande)
{
    qDebug() << "EsaboraElec::Ajout_BC()";

    if(Traitement_Fichier_Config("New_BC",Numero_Commande))
    {
        qDebug() << "Ajout_BC - Traitement d'ajout de BC terminé";
        qDebug() << "Fin EsaboraElec::Ajout_BC()";
        return true;
    }
    else
    {
        qDebug() << "Ajout_BC - Echec Traitement d'ajout de BC";
        qDebug() << "Fin EsaboraElec::Ajout_BC()";
        return false;
    }
}

bool EsaboraElec::Ajout_BL(QString Numero_Commande_Esab, QString Numero_BL)
{
    qDebug() << "EsaboraElec::Ajout_BL()";
    if(Traitement_Fichier_Config("Valid_BL",Numero_Commande_Esab,Numero_BL) == false)
    {
        err->Err(BL,Numero_Commande_Esab,ESAB);
        qDebug() << "Ajout_BL - Echec Traitement Valid_BL";
        qDebug() << "Fin EsaboraElec::Ajout_BL()";
        return false;
    }
    qDebug() << "Ajout_BL - Traitement Valid_BL terminé";
    qDebug() << "Fin EsaboraElec::Ajout_BL()";
    return true;
}

bool EsaboraElec::Traitement_Fichier_Config(const QString file, const QString NumeroCommande, const QString NumeroBL, const QString var)
{
    DEBUG << "EsaboraElec::Traitement_Fichier_Config()";
    DEBUG << "-----Variables :-----";
    DEBUG << file;
    DEBUG << NumeroCommande;
    DEBUG << NumeroBL;
    DEBUG << "---------------------";

    //Ouverture fichier Config.esab
    QFile fichier(m_Lien_Work + "/Config/Config.esab");
    emit Info("Ouverture fichier Config.esab");
    if(fichier.exists() == false)
    {
        err->Err(open_File,fichier.fileName(),ESAB);
        DEBUG << "Traitement_Fichier_Config - Echec le fichier " << fichier.fileName() << "n'existe pas";
        return false;
    }
    if(fichier.open(QIODevice::ReadOnly) == false)
    {
        err->Err(open_File,fichier.fileName(),ESAB);
        DEBUG << "Traitement_Fichier_Config - Echec d'ouverture du fichier " << fichier.fileName();
        return false;
    }


    //Définition des variables
    etat = 0;
    QTextStream flux(&fichier);
    QString temp;
    QEventLoop loop;
    QTimer tmp;
    connect(&tmp,SIGNAL(timeout()),&loop,SLOT(quit()));

    //Récupération des informations de la commande correspondante
    QSqlQuery req = m_DB->Requete("SELECT * FROM En_Cours WHERE Numero_Commande='" + NumeroCommande + "'");
    req.next();
    //Valeur temps entre chaque action
    QSqlQuery r = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='14'");
    r.next();
    int varTmp = r.value("Valeur").toDouble() * 1000;


    //Boucle recherche de la fonction
    DEBUG << "Esabora::Traitement_Fichier_Config - Recherche de " << file << "dans le fichier Config";
    while(flux.atEnd() == false && flux.readLine().contains("[" + file + "]") == false) {}
    if(flux.atEnd()) { return false; }
    DEBUG << "Esabora::Traitement_Fichier_Config - " << file << "trouvé dans le fichier";


    while(flux.atEnd() == false)
    {
        //Pause
        tmp.start(varTmp);
        loop.exec();
        tmp.stop();


        temp = flux.readLine();
        DEBUG << "Traitement_Fichier_Config - Commande " << temp;
        emit Info("Préparation de la commande " + temp);


        //Commentaire /* */
        if(temp.contains("/*")) { while(flux.readLine().contains("*/") == false){} }
        else if(temp == "{LOGIN}") { Clavier("-" + m_Login); }
        else if(temp == "{MDP}") { Clavier("-" + m_MDP); }
        else if(temp == "{CHANTIER}")
        {//Si le chantier est trouvé, sinon si la variable contient 0 ajout au stock
            if(req.value("Nom_Chantier").toString() != "" && req.value("Nom_Chantier").toString() != "0") { Clavier("-" + req.value("Nom_Chantier").toString()); }
            else if(req.value("Nom_Chantier").toString() == "0") { Clavier("-Stock"); }
        }
        else if(temp == "{INTERLOCUTEUR}") { Clavier("-Autobl"); }
        else if(temp == "{BC}") { Clavier("-" + NumeroCommande); }
        else if(temp == "{NUMERO_COMMANDE_ESABORA}") { Clavier("-" + var); }
        else if(temp == "{BL}") { Clavier("-" + NumeroBL); }
        else if(temp == "{COMMENTAIRE}")
        {
            QSqlQuery req = m_DB->Requete("SELECT * FROM En_Cours WHERE Numero_BC_Esabora='" + var + "'");
            if(req.next())
                Clavier("-AutoBL : " + req.value("Fournisseur").toString() + " : BC " + req.value("Numero_Commande").toString());
            else
                Clavier("-AutoBL : Informations BC non trouvé");
        }
        else if(temp == "{FOURNISSEUR}")
        {
            QSqlQuery t = m_DB->Requete("SELECT * FROM Options WHERE Nom='" + req.value("Fournisseur").toString() + "Rcc'");
            if(t.next() == false)
            {
                DEBUG << "Fournisseur non trouvé";
                err->Err(variable,"Fournisseur non trouvé",ESAB);
                return false;
            }
            Clavier("-" + t.value("Valeur").toString());
        }
        else if(temp == "{BOUCLE}")
        {
            //boucle de 6 strings designation,reference,fabricant,prix unitaire,quantité livré,quantité restante

            //Si la liste matériels est vide stopper l'ajout
            if(liste_Matos.isEmpty())
            {
                return false;
            }

            //pointeur vers presse papier windows
            QClipboard *pp = QApplication::clipboard();

            for(int cpt=0;cpt<liste_Matos.count();cpt++)
            {
                //Si une ligne est incomplète
                if(liste_Matos.count()-cpt < 7)
                {
                    err->Err(variable,"Ligne incomplète : " + liste_Matos.at(cpt),ESAB);
                }

                pp->clear();
                tmp.start(1000);
                loop.exec();

                Clavier("Ctrl+C");
                tmp.start(1000);
                loop.exec();
                emit Info(pp->text());

                if(pp->text() != "")//Si D3E, passé à la ligne suivante
                {
                    Clavier("Tab");
                    Clavier("Tab");
                    Clavier("Tab");
                    Clavier("Tab");
                    Clavier("Tab");
                    Clavier("Tab");
                }

                if(liste_Matos.at(cpt+1) == "")//Si Référence est vide
                {
                    err->Err(designation,"Ref=" + liste_Matos.at(cpt) + " Chantier=" + req.value("Numero_Commande").toString(),ESAB);
                }
                Clavier("-" + liste_Matos.at(cpt+1));//Ref

                Clavier("Tab");
                pp->clear();
                tmp.start(1000);
                loop.exec();
                Clavier("Ctrl+C");
                tmp.start(1000);
                loop.exec();
                if(pp->text() == "" || pp->text() == liste_Matos.at(cpt+1) || liste_Matos.at(cpt+1).isEmpty() || pp->text() == " ")//Si La désignation n'a pas été trouvé
                {
                    err->Err(designation,"Ref=" + liste_Matos.at(cpt+1) + " Chantier=" + req.value("Numero_Commande").toString(),ESAB);
                    pp->clear();
                    tmp.start(1000);
                    loop.exec();
                    pp->setText(liste_Matos.at(cpt));
                    tmp.start(1000);
                    loop.exec();
                    Clavier("Ctrl+V");
                }
                Clavier("Tab");
                Clavier("-" + liste_Matos.at(cpt+5));//Quantité
                Clavier("Tab");
                QString var = liste_Matos.at(cpt+4);
                var.replace(",",".");
                if(var.toDouble() > 0)
                {
                    Clavier("-" + liste_Matos.at(cpt+4));//Prix
                }
                Clavier("Tab");
                Clavier("Tab");
                Clavier("Tab");
                cpt += 6;
            }
        }
        else if(temp == "{BOUCLE_CONSTRUCTEUR}")
        {
            Clavier("-" + var);
            tmp.start(500);
            loop.exec();
            Clavier("Ctrl+C");
            tmp.start(1000);
            loop.exec();
            Clavier("Ctrl+A");
        }
        else if(temp[0] == '=')
        {
            //remplacement des valeurs
            if(temp.contains("%0"))
            {
                r = m_DB->Requete("SELECT * FROM En_Cours WHERE Numero_Commande='" + NumeroCommande + "'");
                if(r.next() == false)
                {
                    err->Err(requete,r.lastError().text(),ESAB);
                }
                QString t = r.value("Numero_BC_Esabora").toString();
                temp.replace("%0",t);
            }
            if(temp.contains("%1"))
            {
                r = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='12'");
                if(r.next() == false)
                {
                    err->Err(requete,r.lastError().text(),ESAB);
                }
                temp.replace("%1",r.value("Valeur").toString());
            }
            if(temp.contains("%2"))
            {
                r = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='23'");
                if(r.next() == false)
                {
                    err->Err(requete,r.lastError().text(),ESAB);
                }
                temp.replace("%2",r.value("Valeur").toString());
            }
            DEBUG << "Variable '=' = " << temp;
            
            
            QString fen = temp.split("=").at(1);
            fen.replace("Focus","");
            if(fen.at(fen.count()-1) == ' ')
            {
                fen.remove(fen.count()-1,fen.count()-1);
            }
            bool focus(false);
            if(temp.split(" ").last() == "Focus")
            {
                focus = true;
            }
            
            bool find(false),isFocus(false);
            Control_Window(fen,find,isFocus,focus);
            //a suivre
            if(find == false)
            {
                err->Err(Window,fen,ESAB);
                return false;
            }
            else
            {
                bool t(false);
                if(temp.split(" ").last() == "Focus")
                {
                    t = true;
                }
                if(isFocus != t)
                {
                    err->Err(Focus,fen,ESAB);
                    Abort();
                    if(file == "New_BC")
                    {
                        Ouverture_Liste_BC();
                    }
                    return false;
                }
                emit Info("Esabora | Verification de fenêtre Réussis");
            }
        }
        else if(temp.split(" ").at(0) == "Pause" && temp.split(" ").count() == 2)
        {
            if(temp.split(" ").count() < 2)
                return false;
            tmp.start(temp.split(" ").at(1).toInt() * 1000);
            loop.exec();
            tmp.stop();
        }
        else if(temp.split(" ").at(0) == "Souris")
        {
            if(Souris(temp) == false)
            {
               err->Err(Mouse,"",ESAB);
            }
        }
        else if(temp.split(" ").at(0) == "Copier")
        {
            QClipboard *pP = QApplication::clipboard();
            Copy();
            emit Info("Copie de " + pP->text());
            if(pP->text().count() != 7)
            {
                err->Err(Traitement,"Copie PP échouée ! " + pP->text(),ESAB);
                return false;
            }
            if(temp.split(" ").at(1) == "Numero_BC_Esabora")
            {
                m_DB->Requete("UPDATE En_Cours SET Numero_BC_Esabora='" + pP->text() + "' WHERE Numero_Commande='" + NumeroCommande + "'");
            }
        }
        else if(temp.at(0) == '|')
        {
            if(temp.split(" ").count() < 2)
                return false;
            etat = temp.split(" ").at(1).toInt();
        }
        else if(temp.contains(file)) { return true; }
        else if(temp != "")
        {
            if(temp.split(" ").last() == "oui" || temp.split(" ").last() == "non")
            {
                temp = temp.remove(temp.count() - (temp.split(" ").last().count() + 1),temp.count());
            }

            qDebug() << temp;
            Clavier(temp);
        }
        else err->Err(Traitement,temp,ESAB);
    }
    liste_Matos.clear();
    qDebug() << "Fin Esabora::Traitement_Fichier_Config()";
    return true;
}

void EsaboraElec::Abort()
{
    qDebug() << "Esabora::Abort()";
    Traitement_Fichier_Config("Cancel");
    qDebug() << "Fin Esabora::Abort()";
}

bool EsaboraElec::Fermeture_API()
{
    qDebug() << "Esabora::Fermeture_API()";
    Clavier("Echap");
    Clavier("Echap");
    Clavier("Echap");
    Clavier("Echap");
    Clavier("Echap");
    Clavier("Alt+F4");
    Clavier("Purge");
    p.close();
    DEBUG << "Fin Esabora::Fermeture_API()";
    m_Open = false;
    return true;
}

bool EsaboraElec::Clavier(QString commande)
{
    qDebug() << "Esabora::Clavier()";

    //Reset Touches fonction
    if(GetKeyState(VK_LMENU) < 0) { keybd_event(VK_LMENU,0,KEYEVENTF_KEYUP,0); }
    if(GetKeyState(VK_SHIFT) < 0) { keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0); }
    if(GetKeyState(VK_LCONTROL) < 0) { keybd_event(VK_LCONTROL,0,KEYEVENTF_KEYUP,0); }

    //Création boucle évènements
    QEventLoop l;
    QTimer t;
    bool addition(true);
    connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));

    //condition de découpage commande
    QStringList ligne;
    if(commande == "Purge")
    {
        return true;
    }
    else if(commande.at(0) == '-')
    {
        ligne.clear();
        for(int cpt=1;cpt<=commande.count();cpt++)
        {
            ligne.append(commande.at(cpt));
        }
        addition = false;
    }
    else if(commande.split("+").count() > 1)
    {
        ligne = commande.split("+");
    }
    else
    {
        err->Err(variable,"commande = " + commande,ESAB);
        return false;
    }

    //Boucle execution commande
    for(int i = 0;i<ligne.count();i++)
    {
        QString var = ligne.at(i);
        //Verif majuscule
        if(GetKeyState(VK_SHIFT) < 0)
        {
            keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
        }

        if((var.at(0).isUpper() || var.at(0) == '/' || var.at(0) == '.' || var.at(0).isDigit()) && GetKeyState(VK_SHIFT) >= 0 && GetKeyState(VK_LMENU) >= 0 &&
                addition == false)
        {
            keybd_event(VK_SHIFT,0,0,0);
        }
        else if((var.at(0).isLower() || var.at(0) == ':' || var.at(0) == '-' || var.at(0) == 'é' || var.at(0) == ',') && GetKeyState(VK_SHIFT) < 0)
        {
            keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
        }

        if(var == "Ctrl")
        {
            keybd_event(VK_LCONTROL,0,0,0);
        }
        else if(var == "Alt")
        {
            if(GetKeyState(VK_LMENU) >= 0)
            {
                keybd_event(VK_LMENU,0,0,0);
            }
        }
        else if(var == "Tab")
        {
            keybd_event(VK_TAB,0,0,0);
            keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Entrée")
        {
            keybd_event(VK_RETURN,0,0,0);
            keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Echap")
        {
            keybd_event(VK_ESCAPE,0,0,0);
            keybd_event(VK_ESCAPE,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Gauche")
        {
            keybd_event(VK_LEFT,0,0,0);
            keybd_event(VK_LEFT,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Droite")
        {
            keybd_event(VK_RIGHT,0,0,0);
            keybd_event(VK_RIGHT,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Haut")
        {
            keybd_event(VK_UP,0,0,0);
            keybd_event(VK_UP,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Bas")
        {
            keybd_event(VK_DOWN,0,0,0);
            keybd_event(VK_DOWN,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "F4")
        {
            keybd_event(VK_F4,0,0,0);
            keybd_event(VK_F4,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == " ")
        {
            keybd_event(' ',0,0,0);
        }
        else if(var == "-")
        {
            keybd_event(54,0,0,0);
            keybd_event(54,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "/" || var == ":")
        {
            keybd_event(191,0,0,0);
            keybd_event(191,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == ".")
        {
            keybd_event(190,0,0,0);
            keybd_event(190,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == ",")
        {
            keybd_event(188,0,0,0);
            keybd_event(188,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "+")
        {
            keybd_event(187,0,0,0);
            keybd_event(187,0,KEYEVENTF_KEYUP,0);
        }
        else if(var.at(0).isLetterOrNumber())
        {
            keybd_event(var.at(0).toLatin1(),0,0,0);
        }
        else
        {
            emit Info("Erreur de lecture de " + var);
            DEBUG << "Clavier - Erreur de lecture de '" << var << "', annulation";
            return false;
        }

        //boucle d'évènements avant prochaine saisie
        t.start(100);
        l.exec();
    }

    //Reset touches fonctions
    if(GetKeyState(VK_SHIFT) < 0) { keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0); }
    if(GetKeyState(VK_LMENU) < 0) { keybd_event(VK_LMENU,0,KEYEVENTF_KEYUP,0); }
    if(GetKeyState(VK_LCONTROL) < 0) { keybd_event(VK_LCONTROL,0,KEYEVENTF_KEYUP,0); }

    qDebug() << "Fin Esabora::Clavier()";
    return true;
}

bool EsaboraElec::Souris(QString commande)
{
    DEBUG << "Esabora::Souris()";
    int x = 0,y = 0;
    if(commande.split(" ").count() != 4) { return false; }
    x = commande.split(" ").at(1).toInt();
    y = commande.split(" ").at(2).toInt();

    if(SetCursorPos(x,y) == false) { return false; }
    if(commande.split(" ").at(3) == "oui")
    {
        mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
        mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
    }
    DEBUG << "Fin Esabora::Souris()";
    return true;
}

int EsaboraElec::GetEtat()
{
    return etat;
}

void EsaboraElec::Control_Window(QString name, bool &find, bool &focus, bool isFocus)
{
    DEBUG << "Esabora::Control_Window";
    static int lvl(0);
    QEventLoop loop;
    QTimer t;
    connect(&t,SIGNAL(timeout()),&loop,SLOT(quit()));

    lvl++;

    DEBUG << "Find Window:" << name;
    HWND hWnds = FindWindow(NULL,name.toStdWString().c_str());
    if(hWnds == NULL)
    {
        if(lvl > 1)
        {
            find = false;
            return;
        }
        QStringList listWin = Get_Windows_List(name);
        for(int i = 0;i<listWin.count();i++)
        {
            //boucle controle fenetre intermédiaire
            bool bFind(false),bFocus(false);
            QStringList t = listWin.at(i).split("|");
            Control_Window(t.at(0),bFind,bFocus,true);
            if(bFind && bFocus)
            {
                Clavier("Entrée");
                t.start(2000);
                loop.exec();
                bFind = false;
                bFocus = false;
                Control_Window(name,bFind,bFocus,true);
                if(!bFind)
                {
                    find = false;
                }
                else
                    find = true;
            }
        }
    }
    else
    {
        find = true;
    }
    if(find == false)
    {
        return;
    }

    //Control focus
    DEBUG << "Control focus window";
    hWnds = FindWindow(NULL,name.toStdWString().c_str());
    HWND h = GetForegroundWindow();
    if(hWnds == h && isFocus) {
        DEBUG << "La fenêtre '" << name << "' est toujours au premier plan(Ok)";
        focus = true;
    }
    else if(hWnds != h && isFocus == false)
    {
        QString message;
        DEBUG << "Détection fenêtre : " << Verification_Message_Box(message);
        DEBUG << "La fenêtre '" << name << "' n'est plus au premier plan(Ok)";
        focus = true;
    }
    else if(hWnds != h && isFocus)
    {
        if(lvl > 1)
        {
            focus = false;
            return;
        }
        QStringList listWin = Get_Windows_List(name);
        for(int i = 0;i<listWin.count();i++)
        {
            bool bFind (false),bFocus(false);
            Control_Window(listWin.at(i),bFind,bFocus,true);
            if(bFind && bFocus)
            {
                Clavier("Entrée");
                t.start(2000);
                loop.exec();
                bFind = false;
                bFocus = false;
                Control_Window(name,bFind,bFocus,true);
                if(bFocus)
                {
                    focus = true;
                }
                else
                {
                    focus = false;
                }
            }
        }
    }
    lvl--;
}

QStringList EsaboraElec::Get_Windows_List(QString name)
{
    QStringList l;
    QString n = "%2 - SESSION : 1 - REPERTOIRE : %1";
    QSqlQuery req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='12'");
    req.next();
    n.replace("%1",req.value(0));
    req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='23'");
    req.next();
    n.replace("%2",req.value(0));

    if(name == n)
    {
        l.append("AVERTISSEMENT|Entrée");
        l.append("RX - Agent de Mise à Jour|Stop");
    }
    else if(name == "")
}

void EsaboraElec::Set_Liste_Matos(QStringList liste)
{
    qDebug() << "Esabora::Set_Liste_Matos()";
    liste_Matos = liste;
    qDebug() << liste_Matos;
    qDebug() << "Fin Esabora::Set_Liste_Matos()";
}

void EsaboraElec::Reset_Liste_Matos()
{
    liste_Matos.clear();
}

void EsaboraElec::Apprentissage(QString &entreprise,QString &BDD)
{
    qDebug() << "Esabora::Apprentissage()";   
    QProcess p;
    if(m_Lien_Esabora.contains("Simul_Esabora"))
    {
        p.setProgram(m_Lien_Esabora);
        p.start();
    }
    else
        QDesktopServices::openUrl(m_Lien_Esabora);

    if(Lancement_API())
    {
        HWND hwnd = GetForegroundWindow();
        LPWSTR buf;
        buf = new WCHAR[128];
        if(GetWindowTextW(hwnd,buf,350))
        {
            QString var;
            var = var.fromStdWString(buf);
            if(var.contains("REPERTOIRE") && var.contains("SESSION"))
            {
                QStringList v = var.split(" - ");
                entreprise = v.at(0);
                BDD = v.at(2).split(" : ").at(1);
            }
        }
    }
    Clavier("Alt+F4");
    qDebug() << "Fin Esabora::Apprentissage()";
}

void EsaboraElec::Set_Var_Esabora(QString lien,QString login,QString MDP)
{
    qDebug() << "Esabora::Set_Vat_Esabora()";
    m_Lien_Esabora = lien;
    m_Login = login;
    m_MDP = MDP;
    qDebug() << "Fin Esabora::Set_Var_Esabora()";
}

bool EsaboraElec::Verification_Message_Box(QString &message)
{
    qDebug() << "Esabora::Verification_Message_Box()";
    HWND Handle = GetForegroundWindow(); //Récupération du handle

    RECT rect;
    GetWindowRect(Handle,&rect);
    //Screen
    QPixmap originalPixmap;
    QScreen *screen = QGuiApplication::primaryScreen();
    if(screen)
    {
        originalPixmap = screen->grabWindow(0);
        originalPixmap = originalPixmap.copy(rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top);
    }
    //enregistrement screen
    QFile file(m_Lien_Work + "/test.PNG");
    file.open(QIODevice::WriteOnly);
    originalPixmap.save(&file,"JPG");
    file.close();
    //Accès screen comparatifs
    QDir dir;
    dir.setPath("imgMessageBox");
    QImage img(0),img2(0);
    originalPixmap.save("imgMessageBox/Temp.JPEG");
    img.load("imgMessageBox/Temp.JPEG");
    //Boucle suivant le nombre de fichiers dans le dossier
    for(int cpt=0;cpt<dir.entryList(QDir::NoDotAndDotDot | QDir::Files).count();cpt++)
    {
        img2.load(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt));
        if(img.operator ==(img2))//Si les screens correspondent
        {
            if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "Aucun_acticle_a_receptionner.PNG")
            {
                emit Info("DEBUG : Aucun article à réceptionner !");
            }
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "c_non_repertorie.PNG")
            {
                emit Info("DEBUG : Chantier non répertorié !");
            }
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "f_non_repertorie.PNG")
            {
                emit Info("DEBUG : Fournisseur non répertorié !");
            }
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "i_non_repertorie.PNG")
            {
                emit Info("DEBUG : Interlocuteur non répertorié !");
            }
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "l_deja_livre.PNG")
            {
                emit Info("DEBUG : Modification de ligne déjà livrée !");
            }
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "livraison_total.PNG")
            {
                emit Info("DEBUG : Livraison total");
            }
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "quitter.PNG")
            {
                emit Info("DEBUG : Quitter en perdant les données ?");
            }
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "Sauvegarder.PNG")
            {
                emit Info("DEBUG : Sauvegarder ?");
            }
            else
            {
                emit Info("DEBUG : MessageBox introuvable ! " + dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt));
                file.copy(QApplication::applicationDirPath() + "/imgMessageBox/inconnu.PNG");
            }
            cpt = dir.entryList(QDir::NoDotAndDotDot | QDir::Files).count();
            qDebug() << "Verification_Message_Box - MessageBox Trouvé";
            qDebug() << "Fin Esabora::Verification_Message_Box()";
            return true;
        }
    }
    //Si aucune screen ne correspond
    emit Info("DEBUG : MessageBox inconnue !");
    file.copy(QApplication::applicationDirPath() + "/imgMessageBox/inconnu.PNG");
    message = "Inconnu";
    qDebug() << "Verification_Message_Box - MessageBox inconnue";
    qDebug() << "Fin Esabora::Verification_Message_Box()";
    return false;
}

bool EsaboraElec::Get_List_Matos(QString invoice)
{
    emit DemandeListeMatos(invoice);
    return true;
}

void EsaboraElec::Stop()
{
    m_Arret = true;
}

QString EsaboraElec::Test_Find_Fabricant(QString fab)
{
    return Find_Fabricant(fab);
}

bool EsaboraElec::Copy()
{
    QEventLoop l;
    QTimer t;
    connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
    QClipboard *cb = QApplication::clipboard();
    cb->clear();
    t.start(1000);
    l.exec();
    bool test = Clavier("Ctrl+C");
    t.start(1000);
    l.exec();
    return test;
}

bool EsaboraElec::Paste()
{
    QEventLoop l;
    QTimer t;
    connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
    bool test = Clavier("Ctrl+V");
    t.start(1000);
    l.exec();
    return test;
}

QStringList EsaboraElec::Verif_List(QStringList list)
{
    DEBUG << "Esabora | Verif_List";
    DEBUG << list << list.count();
    if(list.isEmpty() || list.count() % 7 != 0)
    {
        DEBUG << "Liste matériels Vide !";
        return QStringList(0);
    }

    QFile file(m_Lien_Work + "/Config/Fab.esab");
    if(file.open(QIODevice::ReadOnly) == false)
    {
        err->Err(open_File,"Fab.esab",ESAB);
    }
    QTextStream flux(&file);

    QStringList final;
    for(int i=0;i<list.count()-1;i+=7)
    {
        final.append(list.at(i));
        final.append(list.at(i+1));
        final.append(list.at(i+2));
        if(list.at(i+3).isEmpty() && list.at(i+2).isEmpty() == false)//si Fabricant connu mais pas Fab
        {
            flux.seek(0);
            DEBUG << "If";
            while(flux.atEnd() == false)//vérif si Fab déjà connu
            {
                QString var = flux.readLine();
                if(var.split(";").count() == 2 && var.split(";").at(0) == list.at(i+2))
                {
                    final.append(var.split(";").at(1));
                    file.seek(0);
                    break;
                }
            }
            if(flux.atEnd())//Sinon rechercher Fab sur esabora
            {
                QString var = Find_Fabricant(list.at(i+2));
                if(var.isEmpty() == false)
                {
                    file.seek(SEEK_END);
                    flux << list.at(i+2) + ";" + var + "\r\n";
                    file.seek(0);
                }
                final.append(var);
            }
        }
        else
        {
            file.seek(0);
            DEBUG << "Else";
            QString var = flux.readAll();
            DEBUG << "Control existing row";
            if(list.at(i+2).isEmpty() == false)
            {
                if(var.contains(list.at(i+2) + ";" + list.at(i+3)) == false)
                {
                    DEBUG << "Add New entry";
                    file.close();
                    if(file.open(QIODevice::WriteOnly | QIODevice::Append) == false)
                        err->Err(open_File,"Fab.esab",ESAB);
                    flux << list.at(i+2) + ";" + list.at(i+3) + "\r\n";
                    file.waitForBytesWritten(10000);
                    file.close();
                    if(file.open(QIODevice::ReadOnly) == false)
                        err->Err(open_File,"Fab.esab",ESAB);
                }
            }
            else
            {
                DEBUG << "Erreur, fabricant non trouvé dans la commande";
            }
            final.append(list.at(i+3));
        }
        final.append(list.at(i+4));
        final.append(list.at(i+5));
        final.append(list.at(i+6));
    }
    DEBUG << "Esabora | Fin Verif_List";
    return final;
}

void EsaboraElec::Test_Add_BC(QString invoice)
{
    if(Get_List_Matos(invoice) == false)
    {
        DEBUG << "Echec, liste matériels vide !";
        return;
    }
    if(Lancement_API())
    {
        if(Ouverture_Liste_BC())
        {
            if(Ajout_BC(invoice))
            {
                DEBUG << "Test réussis !";
                m_DB->Requete("UPDATE En_Cours SET Ajout='" + QString::number(add) + "' WHERE Numero_Commande='" + invoice + "'");
            }
            else
            {
                DEBUG << "Echec Ajout BC";
                m_DB->Requete("UPDATE En_Cours SET Ajout='"+QString::number(error)+"' WHERE Numero_Commande='" + invoice + "'");
            }
        }
    }
    else
    {
        DEBUG << "Echec démarrage Esabora";
    }
    Abort();
    Fermeture_API();
}

void EsaboraElec::Test_Add_BL(QString invoice,QString bl)
{
    if(Lancement_API())
    {
        if(Ajout_BL(invoice,bl))
        {
            DEBUG << "Test réussis !";  
        }
        else
        {
            DEBUG << "Echec Ajout BC";

        }
    }
    else
    {
        DEBUG << "Echec démarrage Esabora";
    }
    Abort();
    Fermeture_API();
}
