#include "esabora.h"

Esabora::Esabora(QWidget *fen, QString Login, QString MDP, QString Lien_Esabora, QString Lien_Travail):
    m_fen(fen),m_Login(Login),m_MDP(MDP),m_Lien_Esabora(Lien_Esabora),m_Lien_Work(Lien_Travail)
{
    qDebug() << "Init Class Esabora";
}

Esabora::~Esabora()
{
    Clavier("Purge",true);
}
///Erreur 2xx
bool Esabora::Lancement_API()
{
    qDebug() << "Esabora::Lancement_API()";
    if(QDesktopServices::openUrl(m_Lien_Esabora))
    {
        if(!Traitement_Fichier_Config("Open"))
        {
            EmitErreur(202,2);
            qDebug() << "Lancement_API - Echec du traitement du login sur Esabora.elec";
            qDebug() << "Fin Esabora::Lancement_API()";
            return false;
        }
        qDebug() << "Fin Esabora::Lancement_API()";
        return true;
    }
    else
    {
        EmitErreur(203,3);
        qDebug() << "Lancement_API - Echec ouverture d'Esabora.elec";
        qDebug() << "Fin Esabora::Lancement_API()";
        return false;
    }
}
///Erreur 3xx

bool Esabora::Ouverture_Liste_BC()
{
    qDebug() << "Esabora::Ouverture_Liste_BC()";
    if(Traitement_Fichier_Config("Liste_BC")) return true;
    return false;
}

bool Esabora::Ajout_BC(QString Numero_Commande)
{
    qDebug() << "Esabora::Ajout_BC()";
    QSqlQuery req = m_DB.Requete("SELECT * FROM En_Cours WHERE Numero_Commande='" + Numero_Commande + "'");
    req.next();
    QString bL;
    bL = m_Lien_Work + "/Pj/" + Numero_Commande + ".xlsx";

    if(Traitement_Fichier_Config("New_BC",bL))
    {
        qDebug() << "Ajout_BC - Traitement d'ajout de BC terminé";
        qDebug() << "Fin Esabora::Ajout_BC()";
        return true;
    }
    else
    {
        qDebug() << "Ajout_BC - Echec Traitement d'ajout de BC";
        qDebug() << "Fin Esabora::Ajout_BC()";
        return false;
    }
}
///Erreur 4xx
bool Esabora::Ajout_BL(QString Numero_Commande_Esab, QString Numero_BL)
{
    qDebug() << "Esabora::Ajout_BL()";
    if(!Traitement_Fichier_Config("Valid_BL",Numero_Commande_Esab + " " + Numero_BL))
    {
        EmitErreur(402,6,Numero_Commande_Esab);
        qDebug() << "Ajout_BL - Echec Traitement Valid_BL";
        qDebug() << "Fin Esabora::Ajout_BL()";
        return false;
    }
    qDebug() << "Ajout_BL - Traitement Valid_BL terminé";
    qDebug() << "Fin Esabora::Ajout_BL()";
    return true;
}
///Erreur 5xx
bool Esabora::Traitement_Fichier_Config(const QString file, const QString bL)
{
    qDebug() << "Esabora::Traitement_Fichier_Config()";
    etat = 0;
    QFile fichier(m_Lien_Work + "/Config/Config.esab");
    emit Info("Ouverture fichier Config.esab");
    if(!fichier.exists())
    {
        EmitErreur(501,5,fichier.fileName());
        qDebug() << "Traitement_Fichier_Config - Echec le fichier " << fichier.fileName() << "n'existe pas";
        return false;
    }
    if(!fichier.open(QIODevice::ReadOnly))
    {
        EmitErreur(502,1,fichier.fileName());
        qDebug() << "Traitement_Fichier_Config - Echec d'ouverture du fichier " << fichier.fileName();
        return false;
    }

    QTextStream flux(&fichier);

    QString temp;
    QEventLoop loop;
    QTimer tmp;
    connect(&tmp,SIGNAL(timeout()),&loop,SLOT(quit()));
    QSqlQuery req = m_DB.Requete("SELECT Nom_Chantier FROM En_Cours WHERE Numero_Commande='" + bL.split("/").last().split(".").at(0) + "'");
    req.next();
    QSqlQuery r = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='14'");
    r.next();
    int varTmp = r.value("Valeur").toDouble() * 1000;
    qDebug() << "Esabora::Traitement_Fichier_Config - Recherche de " << file << "dans le fichier Config";
    while(!flux.atEnd() && !flux.readLine().contains(file));

    if(flux.atEnd()) return false;
    qDebug() << "Esabora::Traitement_Fichier_Config - " << file << "trouvé dans le fichier";
    while(!flux.atEnd())
    {
        tmp.start(varTmp);
        loop.exec();
        tmp.stop();

        temp = flux.readLine();
        qDebug() << "Traitement_Fichier_Config - Commande " << temp;
        emit Info("Préparation de la commande " + temp);

        if(temp.contains("/*"))
        {
            while(!flux.readLine().contains("*/"));
            temp = flux.readLine();
        }
        else if(temp == "{LOGIN}") Clavier("-" + m_Login);
        else if(temp == "{MDP}") Clavier("-" + m_MDP);
        else if(temp == "{CHANTIER}")
        {///Si le chantier est trouvé, sinon si la variable contient Stock
            if(req.value("Nom_Chantier").toString() != "" && req.value("Nom_Chantier").toString() != "0") Clavier("-" + req.value("Nom_Chantier").toString());
            else if(req.value("Nom_Chantier").toString() == "0") Clavier("-Stock");
        }
        else if(temp == "{INTERLOCUTEUR}") Clavier("-Autobl");
        else if(temp == "{BC}") Clavier("-" + bL);
        else if(temp == "{NUMERO_COMMANDE_ESABORA}") Clavier("-" + bL.split(" ").at(0));
        else if(temp == "{BL}") Clavier("-" + bL.split(" ").at(1));
        else if(temp == "{COMMENTAIRE}")
        {
            QSqlQuery req = m_DB.Requete("SELECT * FROM En_Cours WHERE Numero_BC_Esabora='" + bL.split(" ").at(0) + "'");
            req.next();
            Clavier("-AutoBL : Bon de commande " + req.value("Numero_Commande").toString());
        }
        else if(temp == "{FOURNISSEUR}") Clavier("-rex");
        else if(temp == "{BOUCLE}")
        {
            //boucle de 6 strings designation,reference,fabricant,prix unitaire,quantité livré,quantité restante
            if(liste_Matos.isEmpty())
            {
                Abort();
                Ouverture_Liste_BC();
                return false;
            }
            QClipboard *pp = QApplication::clipboard();
            for(int cpt=0;cpt<liste_Matos.count();cpt++)
            {
                if(liste_Matos.count()-cpt < 7)
                    EmitErreur(514,10,"Ligne incomplète : " + liste_Matos.at(cpt));
                QEventLoop lp;
                QTimer t;
                connect(&t,SIGNAL(timeout()),&lp,SLOT(quit()));
                pp->clear();
                t.start(1000);
                lp.exec();
                Clavier("Ctrl+C");
                t.start(1000);
                lp.exec();
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
                    EmitErreur(512,10,"Ref=" + liste_Matos.at(cpt) + " Chantier=" + req.value("Nom_Chantier").toString());
                Clavier("-" + liste_Matos.at(cpt+1));//Ref
                Clavier("Tab");
                pp->clear();
                t.start(1000);
                lp.exec();
                Clavier("Ctrl+C");
                t.start(1000);
                lp.exec();
                if(pp->text() == "" || pp->text() == liste_Matos.at(cpt+1))//Si La désignation n'a pas été trouvé
                {
                    EmitErreur(513,10,"Ref=" + liste_Matos.at(cpt+1) + " Chantier=" + req.value("Nom_Chantier").toString());
                    pp->clear();
                    t.start(1000);
                    lp.exec();
                    pp->setText(liste_Matos.at(cpt));
                    t.start(1000);
                    lp.exec();
                    Clavier("Ctrl+V");
                }
                Clavier("Tab");
                Clavier("-" + liste_Matos.at(cpt+5));//Quantité
                Clavier("Tab");
                Clavier("-" + liste_Matos.at(cpt+4));//Prix
                Clavier("Tab");
                Clavier("Tab");
                Clavier("Tab");
                cpt += 6;
            }
        }
        else if(temp[0] == '=')
        {
            QString t;
            if(bL.split("/").count() > 1)
            {
                QSqlQuery r = m_DB.Requete("SELECT * FROM En_Cours WHERE Numero_Commande='" + bL.split("/").last().split(".").at(0) + "'");
                if(!r.next())
                    EmitErreur(510,2,r.lastError().text());

                t = r.value("Numero_BC_Esabora").toString();
                temp.replace("%0",t);
            }
            r = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='12'");
            r.next();
            temp.replace("%1",r.value("Valeur").toString());
            r = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='23'");
            r.next();
            temp.replace("%2",r.value("Valeur").toString());
            qDebug() << temp;

            QString fen = temp.split("=").at(1);
            fen.replace("Focus","");
            if(fen.at(fen.count()-1) == ' ')
                fen.remove(fen.count()-1,fen.count()-1);
            emit Info(fen);
            if(!Verification_Fenetre(fen))
            {
                EmitErreur(503,7,fen);
                return false;
            }
            else
            {
                bool t = false;
                if(temp.split(" ").last() == "Focus")
                    t = true;
                if(!Verification_Focus(fen,t))
                {
                    EmitErreur(506,9);
                    Clavier("Entrée");
                    Abort();
                    if(file == "New_BC")
                        Ouverture_Liste_BC();
                    return false;
                }
                emit Info("Esabora | Verification de fenêtre Réussis");
            }
        }
        else if(temp.split(" ").at(0) == "Pause" && temp.split(" ").count() == 2)
        {
            tmp.start(temp.split(" ").at(1).toInt() * 1000);
            loop.exec();
            tmp.stop();
            m_Tmp = "";
        }
        else if(temp.split(" ").at(0) == "Souris")
        {
            if(!Souris(temp))
               EmitErreur(504,8);
            m_Tmp = "";
        }
        else if(temp.split(" ").at(0) == "Copier")
        {
            Clavier("Ctrl+C");
            QClipboard *pP = QApplication::clipboard();
            tmp.start(1000);
            loop.exec();
            tmp.stop();
            emit Info("Copie de " + pP->text());
            if(pP->text().count() != 7)
            {
                EmitErreur(511,2,"Copie PP échouée ! " + pP->text());
                return false;
            }
            if(temp.split(" ").at(1) == "Numero_BC_Esabora")
                m_DB.Requete("UPDATE En_Cours SET Numero_BC_Esabora='" + pP->text() + "' WHERE Numero_Commande='" + bL.split("/").last().split(".").at(0) + "'");
        }
        else if(temp.at(0) == '|')
        {
            etat = temp.split(" ").at(1).toInt();
        }
        else if(temp.contains(file)) return true;
        else if(temp != "")
        {
            bool KU(true);
            if(temp.split(" ").last() == "oui" || temp.split(" ").last() == "non")
            {
                if(temp.split(" ").last() == "non")
                    KU = false;

                temp = temp.remove(temp.count() - (temp.split(" ").last().count() + 1),temp.count());
            }
            qDebug() << temp << KU;
            Clavier(temp,KU);
        }
        else EmitErreur(505,2,temp);
    }
    qDebug() << "Fin Esabora::Traitement_Fichier_Config()";
    return true;
}

void Esabora::Abort()
{
    qDebug() << "Esabora::Abort()";
    Traitement_Fichier_Config("Cancel");
    qDebug() << "Fin Esabora::Abort()";
}

bool Esabora::Fermeture_API()
{
    qDebug() << "Esabora::Fermeture_API()";
    Clavier("Echap");
    Clavier("Echap");
    Clavier("Echap");
    Clavier("Echap");
    Clavier("Echap");
    Clavier("Alt+F4");
    Clavier("Purge");
    qDebug() << "Fin Esabora::Fermeture_API()";
    return true;
}

bool Esabora::Clavier(QString commande,bool keyUp)
{
    //OK, fonctionne uniquement au premier plan !
    //Lettre tjrs en majuscule !
    //keybd_event('O',0,0,0);
    //keybd_event('K',0,0,0);
    //keybd_event(VK_SPACE,0,0,0);
    //keybd_event(VK_SPACE,0,KEYEVENTF_KEYUP,0);
    qDebug() << "Esabora::Clavier()";
    if(commande == "Purge")
    {
        if(GetKeyState(VK_LMENU) < 0)
            keybd_event(VK_LMENU,0,KEYEVENTF_KEYUP,0);
        if(GetKeyState(VK_SHIFT) < 0)
            keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
        if(GetKeyState(VK_LCONTROL) < 0)
            keybd_event(VK_LCONTROL,0,KEYEVENTF_KEYUP,0);
        return true;
    }

    QStringList ligne = commande.split("+");
    if(commande.at(0) == '-' && ligne.count() == 1)
    {
        ligne.clear();
        for(int cpt=1;cpt<=commande.count();cpt++)
            ligne.append(commande.at(cpt));
    }

    int nbLigne = 0;

    while(ligne.count() > nbLigne)
    {
        QString var = ligne.at(nbLigne);
        //emit Info(var + " Shift=" + GetKeyState(VK_SHIFT) + " Alt=" + GetKeyState(VK_LMENU) + " Ctrl=" + GetKeyState(VK_LCONTROL));
        //Verif majuscule
        if((var.at(0).isUpper() || var.at(0) == '/' || var.at(0) == '.') && GetKeyState(VK_SHIFT) >= 0 && GetKeyState(VK_LMENU) >= 0)
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
            if(GetKeyState(VK_SHIFT) < 0)
                keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
            if(GetKeyState(VK_LMENU) >= 0)
                keybd_event(VK_LMENU,0,0,0);
        }
        else if(var == "Tab")
        {
            if(GetKeyState(VK_SHIFT) < 0)
                keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
            keybd_event(VK_TAB,0,0,0);
            keybd_event(VK_TAB,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Entrée")
        {
            if(GetKeyState(VK_SHIFT) < 0)
                keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
            keybd_event(VK_RETURN,0,0,0);
            keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Echap")
        {
            if(GetKeyState(VK_SHIFT) < 0)
                    keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
            keybd_event(VK_ESCAPE,0,0,0);
            keybd_event(VK_ESCAPE,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Gauche")
        {
            if(GetKeyState(VK_SHIFT) < 0)
                keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
            keybd_event(VK_LEFT,0,0,0);
            keybd_event(VK_LEFT,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Droite")
        {
            if(GetKeyState(VK_SHIFT) < 0)
                keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
            keybd_event(VK_RIGHT,0,0,0);
            keybd_event(VK_RIGHT,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Haut")
        {
            if(GetKeyState(VK_SHIFT) < 0)
                keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
            keybd_event(VK_UP,0,0,0);
            keybd_event(VK_UP,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "Bas")
        {
            if(GetKeyState(VK_SHIFT) < 0)
                keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
            keybd_event(VK_DOWN,0,0,0);
            keybd_event(VK_DOWN,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "F4")
        {
            keybd_event(VK_F4,0,0,0);
            keybd_event(VK_F4,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "A" || var == "a")
            keybd_event('A',0,0,0);
        else if(var == "B" || var == "b")
            keybd_event('B',0,0,0);
        else if(var == "C" || var == "c")
            keybd_event('C',0,0,0);
        else if(var == "D" || var == "d")
            keybd_event('D',0,0,0);
        else if(var == "E" || var == "e")
            keybd_event('E',0,0,0);
        else if(var == "é")
            keybd_event('2',0,0,0);
        else if(var == "F" || var == "f")
            keybd_event('F',0,0,0);
        else if(var == "G" || var == "g")
            keybd_event('G',0,0,0);
        else if(var == "H" || var == "h")
            keybd_event('H',0,0,0);
        else if(var == "I" || var == "i")
            keybd_event('I',0,0,0);
        else if(var == "J" || var == "j")
            keybd_event('J',0,0,0);
        else if(var == "K" || var == "k")
            keybd_event('K',0,0,0);
        else if(var == "L" || var == "l")
            keybd_event('L',0,0,0);
        else if(var == "M" || var == "m")
            keybd_event('M',0,0,0);
        else if(var == "N" || var == "n")
            keybd_event('N',0,0,0);
        else if(var == "O" || var == "o")
            keybd_event('O',0,0,0);
        else if(var == "P" || var == "p")
            keybd_event('P',0,0,0);
        else if(var == "Q" || var == "q")
            keybd_event('Q',0,0,0);
        else if(var == "R" || var == "r")
            keybd_event('R',0,0,0);
        else if(var == "S" || var == "s")
            keybd_event('S',0,0,0);
        else if(var == "T" || var == "t")
            keybd_event('T',0,0,0);
        else if(var == "U" || var == "u")
            keybd_event('U',0,0,0);
        else if(var == "V" || var == "v")
            keybd_event('V',0,0,0);
        else if(var == "W" || var == "w")
            keybd_event('W',0,0,0);
        else if(var == "X" || var == "x")
            keybd_event('X',0,0,0);
        else if(var == "Y" || var == "y")
            keybd_event('Y',0,0,0);
        else if(var == "Z" || var == "z")
            keybd_event('Z',0,0,0);
        else if(var == " ")
            keybd_event(' ',0,0,0);
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
        else if(var == "0")
        {
            keybd_event(VK_NUMPAD0,0,0,0);
            keybd_event(VK_NUMPAD0,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "1")
        {
            keybd_event(VK_NUMPAD1,0,0,0);
            keybd_event(VK_NUMPAD1,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "2")
        {
            keybd_event(VK_NUMPAD2,0,0,0);
            keybd_event(VK_NUMPAD2,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "3")
        {
            keybd_event(VK_NUMPAD3,0,0,0);
            keybd_event(VK_NUMPAD3,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "4")
        {
            keybd_event(VK_NUMPAD4,0,0,0);
            keybd_event(VK_NUMPAD4,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "5")
        {
            keybd_event(VK_NUMPAD5,0,0,0);
            keybd_event(VK_NUMPAD5,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "6")
        {
            keybd_event(VK_NUMPAD6,0,0,0);
            keybd_event(VK_NUMPAD6,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "7")
        {
            keybd_event(VK_NUMPAD7,0,0,0);
            keybd_event(VK_NUMPAD7,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "8")
        {
            keybd_event(VK_NUMPAD8,0,0,0);
            keybd_event(VK_NUMPAD8,0,KEYEVENTF_KEYUP,0);
        }
        else if(var == "9")
        {
            keybd_event(VK_NUMPAD9,0,0,0);
            keybd_event(VK_NUMPAD9,0,KEYEVENTF_KEYUP,0);
        }
        else
        {
            emit Info("Erreur de lecture de " + var);
            qDebug() << "Clavier - Erreur de lecture de " << var << ", annulation";
            return false;
        }

        nbLigne++;
    }
    nbLigne = 0;

    if(1)
    {
        if(GetKeyState(VK_SHIFT) < 0)
            keybd_event(VK_SHIFT,0,KEYEVENTF_KEYUP,0);
        if(GetKeyState(VK_LMENU) < 0)
            keybd_event(VK_LMENU,0,KEYEVENTF_KEYUP,0);
        if(GetKeyState(VK_LCONTROL) < 0)
            keybd_event(VK_LCONTROL,0,KEYEVENTF_KEYUP,0);
    }
    qDebug() << "Fin Esabora::Clavier()";
    return true;
}

bool Esabora::Souris(QString commande)
{
    qDebug() << "Esabora::Souris()";
    int x = 0,y = 0;
    bool clique = false;
    if(commande.split(" ").count() != 4)
        return false;
    x = commande.split(" ").at(1).toInt();
    y = commande.split(" ").at(2).toInt();
    if(commande.split(" ").at(3) == "oui")
        clique = true;
    if(!SetCursorPos(x,y))
        return false;
    if(clique)
    {
        mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
        mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
    }
    qDebug() << "Fin Esabora::Souris()";
    return true;
}

int Esabora::GetEtat()
{
    return etat;
}

bool Esabora::Verification_Fenetre(QString fenetre)
{
    qDebug() << "Esabora::Verification_Fenetre()";
    HWND hWnds = FindWindow(NULL,fenetre.toStdWString().c_str());
    if(hWnds == NULL)
    {
        hWnds = FindWindow(NULL,L"AVERTISSEMENT");
        if(hWnds == NULL) return false;
        else
        {
            Clavier("Entrée");
            QEventLoop loop;
            QTimer t;
            connect(&t,SIGNAL(timeout()),&loop,SLOT(quit()));
            t.start(2000);
            loop.exec();
            if(Verification_Fenetre(fenetre)) return true;
            else return false;
        }
    }
    else return true;
}

void Esabora::EmitErreur(int code,int string,QString info)
{
    QString ret;
    switch(string)
    {
    case 0:
        ret = "Apprentissage Echoué";
        break;
    case 1:
        ret = "Ouverture fichier échoué";
        break;
    case 2:
        ret = "Erreur dans le traitement";
        break;
    case 3:
        ret = "Lancement d'Esabora échoué";
        break;
    case 4:
        ret = "Les images ne correspondent pas";
        break;
    case 5:
        ret = "Le fichier n'existe pas";
        break;
    case 6:
        ret = "Ajout du BL échoué";
        break;
    case 7:
        ret = "Vérification de fenêtre échouée";
        break;
    case 8:
        ret = "Erreur dans le déplacement de la souris";
        break;
    case 9:
        ret = "Vérification Focus échouée";
        break;
    case 10:
        ret = "Référence inconnue";
        break;
    default:
        ret = "Erreur inconnue";
        break;
    }
    emit Erreur("Esabora | E" + QString::number(code) + " | " + ret + " : " + info);
}

bool Esabora::Verification_Focus(QString fen, bool focus)
{
    qDebug() << "Esabora::Verification_Focus()";
    HWND hWnds = FindWindow(NULL,fen.toStdWString().c_str());
    HWND h = GetForegroundWindow();
    if(hWnds == h && focus) return true;
    else if(hWnds != h && !focus)
    {
        QString message;
        Verification_Message_Box(message);
        return true;
    }
    else return false;
}
///Erreur 6xx
bool Esabora::Ajout_Stock(QString numero_Commande)
{
    qDebug() << "Esabora::Ajout_Stock()";
    if(!Traitement_Fichier_Config("Ajout_Stock","///" + numero_Commande + ".tmp"))
    {
        EmitErreur(600,2);
        qDebug() << "Ajout_Stock - Echec dans le traitement du Bon de commande";
        return false;
    }
    qDebug() << "Fin Esabora::Ajout_Stock()";
    return true;
}

void Esabora::Semi_Auto(QString NumeroCommande)
{
    qDebug() << "Esabora::Semi_Auto()";
    QSqlQuery r = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='23'");
    QSqlQuery r2 = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='12'");
    r.next();
    r2.next();
    if(!Verification_Fenetre(r.value("Valeur").toString() + " - SESSION : 1 - REPERTOIRE : " + r2.value("Valeur").toString()))
    {
        emit Message("Erreur","Esabora n'est pas ouvert !",true);
        return;
    }
    else
    {
        QString fen(r.value("Valeur").toString() + " - SESSION : 1 - REPERTOIRE : " + r2.value("Valeur").toString());
        HWND hWnds = FindWindow(NULL,fen.toStdWString().c_str());
        SetForegroundWindow(hWnds);
    }
    Traitement_Fichier_Config("Semi_Auto","///" + NumeroCommande + ".");

    if(QMessageBox::question(m_fen,"","L'ajout du bon de commande à t'il réussi ?") == QMessageBox::Yes)
        m_DB.Requete("UPDATE En_Cours SET Ajout='Ok' WHERE Numero_Commande='" + NumeroCommande + "'");

    emit Message("","Le bon de livraison doit être validé manuellement.",false);
    qDebug() << "Fin Esabora::Semi_Auto()";
}

void Esabora::Set_Liste_Matos(QStringList liste)
{
    qDebug() << "Esabora::Set_Liste_Matos()";
    liste_Matos = liste;
    qDebug() << liste_Matos;
    qDebug() << "Fin Esabora::Set_Liste_Matos()";
}

void Esabora::Reset_Liste_Matos()
{
    liste_Matos.clear();
}

void Esabora::Apprentissage(QString &entreprise,QString &BDD)
{
    qDebug() << "Esabora::Apprentissage()";
    QDesktopServices::openUrl(m_Lien_Esabora);
    QEventLoop l;
    QTimer t;
    connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
    t.start(3000);
    l.exec();
    Clavier("-" + m_Login);
    Clavier("Tab");
    Clavier("-" + m_MDP);
    Clavier("Entrée");
    t.start(2000);
    l.exec();
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
    Clavier("Alt+F4");
    qDebug() << "Fin Esabora::Apprentissage()";
}

void Esabora::Set_Var_Esabora(QString lien,QString login,QString MDP)
{
    qDebug() << "Esabora::Set_Vat_Esabora()";
    m_Lien_Esabora = lien;
    m_Login = login;
    m_MDP = MDP;
    qDebug() << "Fin Esabora::Set_Var_Esabora()";
}

bool Esabora::Verification_Message_Box(QString &message)
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
                emit Info("DEBUG : Aucun article à réceptionner !");
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "c_non_repertorie.PNG")
                emit Info("DEBUG : Chantier non répertorié !");
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "f_non_repertorie.PNG")
                emit Info("DEBUG : Fournisseur non répertorié !");
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "i_non_repertorie.PNG")
                emit Info("DEBUG : Interlocuteur non répertorié !");
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "l_deja_livre.PNG")
                emit Info("DEBUG : Modification de ligne déjà livrée !");
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "livraison_total.PNG")
                emit Info("DEBUG : Livraison total");
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "quitter.PNG")
                emit Info("DEBUG : Quitter en perdant les données ?");
            else if(dir.entryList(QDir::NoDotAndDotDot | QDir::Files).at(cpt) == "Sauvegarder.PNG")
                emit Info("DEBUG : Sauvegarder ?");
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
