#include "db.h"

/////////////////////////////////
/// Table En_Cours
/// ---------------
/// ID
/// Date
/// Nom_Chantier
/// Info_Chantier
/// Numero_Commande
/// Numero_Livraison
/// Numero_BC_Esabora
/// Lien_Commande
/// Etat
/// Ajout
/// Info_Chantier
/// Ajout_BL
/// Fournisseur
/// ---------------
/// Table Options
/// ---------------
/// ID
/// Nom
/// Valeur
/////////////////////////////////
DB::DB(Error *err):
    m_Error(err)
{
    DEBUG << "Initialisation DB";

    QFile f(qApp->applicationDirPath() + "/bddInfo.db");
    if(f.exists())//si DB inaccessible, lancer la mise à jour de la DB
    {
        DEBUG << "Update de la BDD";
        QDesktopServices::openUrl(QUrl::fromLocalFile(qApp->applicationDirPath() + "/bin/MAJ_BDD.exe"));
        qApp->exit(0);
    }
}

QSqlQuery DB::Requete(QString r)
{
    QSqlQuery req;

    if(req.prepare(r))
    {
        if(req.exec() == false)
        {
            m_Error->Err(requete,r,"DB");
        }
        else if(req.lastQuery().contains("INSERT INTO En_Cours VALUES"))
        {
            QString frn = req.lastQuery().split(",").last().split("'").at(1);
            QString invoice = req.lastQuery().split(",").at(3).split("'").at(1);
            req = Requete("SELECT * FROM Options WHERE Nom='" + frn + "_Last'");
            if(req.next() == false)
            {
                req = Requete("SELECT MAX(ID) FROM Options");
                req.next();
                int id = req.value(0).toInt();
                Requete("INSERT INTO Options VALUES('" + QString::number(id+1) + "','" + frn + "_Last','" + invoice + "')");
            }
            else
            {
                Requete("UPDATE Options SET Valeur='" + invoice + "' WHERE Nom='" + frn + "_Last'");
            }
        }
    }
    else
        m_Error->Err(requete,r,"DB");

    return req;
}

void DB::Init()
{
    //Ouverture de la DB
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    qDebug() << db.drivers();
    db.setDatabaseName(qApp->applicationDirPath() + "/bdd.db");
    db.setHostName("127.0.0.1");

    if(db.open() == false)
    {
        m_Error->Err(openDB);
        return;
    }
    else
    {
        emit Info("Ouverture de la Database réussi ! " + db.hostName() + " " + db.driverName());
    }

    //test DB
    QSqlQuery req;


    //Création Tableau DB si inexistant
    QSqlQuery query;
    query.prepare("CREATE TABLE En_Cours ('ID' SMALLINT, 'Date' TEXT, 'Nom_Chantier' TEXT, 'Numero_Commande' TEXT, 'Numero_Livraison' TEXT, 'Lien_Commande' TEXT, 'Etat' TEXT, 'Ajout' TEXT, 'Info_Chantier' TEXT, 'Ajout_BL' SMALLINT, 'Numero_BC_Esabora' TEXT)");
    query.exec();
    query.clear();
    query.prepare("CREATE TABLE Options ('ID' SMALLINT, 'Nom' TEXT, 'Valeur' TEXT)");
    if(query.exec())
    {
        emit Info("Création BDD");
        emit CreateTable();
        Requete("INSERT INTO Options VALUES('0','Auto','0')");
        Requete("INSERT INTO Options VALUES('1','Minutes','0')");
        Requete("INSERT INTO Options VALUES('2','Heure','22')");
        Requete("INSERT INTO Options VALUES('3','Login','')");
        Requete("INSERT INTO Options VALUES('4','MDP','')");
        Requete("INSERT INTO Options VALUES('5','LienEsabora','')");
        Requete("INSERT INTO Options VALUES('6','NDCR','')");
        Requete("INSERT INTO Options VALUES('7','NUR','')");
        Requete("INSERT INTO Options VALUES('8','MDPR','')");
        Requete("INSERT INTO Options VALUES('9','EDE','')");
        Requete("INSERT INTO Options VALUES('10','APTSG','0')");
        Requete("INSERT INTO Options VALUES('11','MDPA','')");
        Requete("INSERT INTO Options VALUES('12','Nom_BDD','')");
        Requete("INSERT INTO Options VALUES('13','ADD_Auto_BL','1')");
        Requete("INSERT INTO Options VALUES('14','Tmp_Cmd','0.5')");
        Requete("INSERT INTO Options VALUES('15','NUR2','')");
        Requete("INSERT INTO Options VALUES('16','Help','0')");
        Requete("INSERT INTO Options VALUES('17','PurgeAuto','1')");
        Requete("INSERT INTO Options VALUES('18','LastPurge','00/00/0000')");
        Requete("INSERT INTO Options VALUES('19','Tmp_Rexel','5')");
        Requete("INSERT INTO Options VALUES('20','totalBL','0')");
        Requete("INSERT INTO Options VALUES('21','VersionAPI','')");
        Requete("INSERT INTO Options VALUES('22','SemiAuto','0')");
        Requete("INSERT INTO Options VALUES('23','Nom_Entreprise_Esabora','')");
        Requete("INSERT INTO Options VALUES('24','Fournisseurs','Rexel.fr|')");
        Requete("INSERT INTO Options VALUES('25','','')");
    }

    if(query.prepare("ALTER TABLE En_Cours ADD Fournisseur TEXT"))
    {
        if(query.exec() == false) { m_Error->Err(updateDB,"","DB"); }
    }

    Requete("UPDATE En_Cours SET Fournisseur='Rexel.fr' WHERE Fournisseur=''");
    Requete("UPDATE En_Cours SET Etat='0' WHERE Etat='En préparation' AND Fournisseur='Rexel.fr'");
    Requete("UPDATE En_Cours SET Etat='2' WHERE Etat='Livrée en totalité' AND Fournisseur='Rexel.fr'");
    Requete("UPDATE En_Cours SET Etat='2' WHERE Etat='Livrée Et Facturée'");
    Requete("UPDATE En_Cours SET Etat='2' WHERE Etat='Livrée En Totalité'");
    Requete("UPDATE En_Cours SET Etat='1' WHERE Etat='Partiellement Livrée'");
    Requete("UPDATE En_Cours SET Etat='2' WHERE Etat='Facturée'");

    //Test DB
    req = Requete("SELECT * FROM Options");

    if(req.next() == false)//si DB inaccessible, lancer la mise à jour de la DB
    {
        db.close();
        QDesktopServices::openUrl(QUrl("bin/MAJ_BDD.exe"));
        qApp->exit(0);
    }
    else
    {
        emit Info(req.value("Nom").toString() + "=" + req.value("Valeur").toString());
    }

    //Update variable Ajout
    Requete("UPDATE En_Cours SET Ajout='0' WHERE Ajout=''");
    Requete("UPDATE En_Cours SET Ajout='0' WHERE Ajout='Télécharger'");
    Requete("UPDATE En_Cours SET Ajout='1' WHERE Ajout='Erreur'");
    Requete("UPDATE En_Cours SET Ajout='2' WHERE Ajout='Modifier'");
    Requete("UPDATE En_Cours SET Ajout='3' WHERE Ajout='Bon Ajouté'");
    Requete("UPDATE En_Cours SET AJout='4' WHERE Ajout='Ok'");


    DEBUG << "DB initialisée";
}

void DB::Close_DB()
{
    QSqlDatabase::removeDatabase("qt_sql_default_connection");
}

void DB::Sav()
{
    qDebug() << "Sav";
    QFile sav(qApp->applicationDirPath() + "/bddSav.db");
    QFile sav2(qApp->applicationDirPath() + "/bddSav2.db");
    QFile sav3(qApp->applicationDirPath() + "/bddSav3.db");
    QFileInfo f(sav);
    QFileInfo f1(sav2);
    QFileInfo f2(sav3);
    QFileInfo fBDD(qApp->applicationDirPath() + "/bdd.db");

    bool ok(true);

    if(f.lastModified().operator ==(fBDD.lastModified()) || f1.lastModified().operator ==(fBDD.lastModified()) || f2.lastModified().operator ==(fBDD.lastModified())) { return; }

    if(sav.exists() == false)
    {
        ok = sav.copy(qApp->applicationDirPath() + "/bdd.db",qApp->applicationDirPath() + "/bddSav.db");
        emit Info("DB | Sauvegarde de la DB(sav)");
    }
    else if(sav2.exists() == false)
    {
        ok = sav.copy(qApp->applicationDirPath() + "/bdd.db",qApp->applicationDirPath() + "/bddSav2.db");
        emit Info("DB | Sauvegarde de la DB(sav2)");
    }
    else if(sav3.exists() == false)
    {
        ok = sav.copy(qApp->applicationDirPath() + "/bdd.db",qApp->applicationDirPath() + "/bddSav3.db");
        emit Info("DB | Sauvegarde de la DB(sav3)");
    }
    else
    {
        if(f.lastModified().operator <(f1.lastModified()) && f.lastModified().operator <(f2.lastModified()) && f.lastModified().toString("dd/MM/yyyy") != QDate::currentDate().toString("dd/MM/yyyy"))
        {
            sav.remove();
            ok = sav.copy(qApp->applicationDirPath() + "/bdd.db",qApp->applicationDirPath() + "/bddSav.db");
            emit Info("DB | Sauvegarde de la DB(sav)");
        }
        else if(f1.lastModified().operator <(f2.lastModified()) && f1.lastModified().toString("dd/MM/yyyy") != QDate::currentDate().toString("dd/MM/yyyy"))
        {
            sav2.remove();
            ok = sav2.copy(qApp->applicationDirPath() + "/bdd.db",qApp->applicationDirPath() + "/bddSav2.db");
            emit Info("DB | Sauvegarde de la DB(sav2)");
        }
        else if(f2.lastModified().toString("dd/MM/yyyy") != QDate::currentDate().toString("dd/MM/yyyy"))
        {
            sav3.remove();
            ok = sav3.copy(qApp->applicationDirPath() + "/bdd.db",qApp->applicationDirPath() + "/bddSav3.db");
            emit Info("DB | Sauvegarde de la DB(sav3)");
        }
    }
    if(ok == false)
    {
        m_Error->Err(saveDB);
    }
}

void DB::Purge()
{
    qDebug() << "Purge";
    QDate t;
    t = t.currentDate();
    t = t.addMonths(-2);
    /*
    //Liste BC < 2 mois
    QStringList var;
    QSqlQuery req = Requete("SELECT * FROM En_Cours WHERE Date > '" + t.toString("yyyy-MM-dd") + "'");
    while(req.next())
    {
        bool test(false);
        for(int i=0;i<var.count();i++)
        {
            if(var.at(i) == req.value("Fournisseur").toString())
            {
                test = true;
            }
        }
        if(test == false)
        {
            var.append(req.value("Fournisseur").toString());
        }
    }

    //Liste BC > 2 mois
    req = Requete("SELECT * FROM En_Cours WHERE Date > '" + t.toString("yyyy-MM-dd") + "'");
    QStringList var2,var3;
    while(req.next())
    {
        bool test(false);
        for(int i=0;i<var2.count();i++)
        {
            if(var2.at(i) == req.value("Fournisseur").toString())
            {
                test = true;
            }
        }
        if(test == false)
        {
            var2.append(req.value("Fournisseur").toString());
        }
    }

    bool test(true);
    for(int i=0;i<var2.count();i++)
    {
        if(var.contains(var2.at(i)) == false)
        {
            test = false;
            var3.append(var2.at(i));
        }
    }
    if(test == false)
    {
        //mise à jour date dernier BC
        for(int i = 0;i<var3.count();i++)
        {
            req = Requete("SELECT * FROM En_Cours WHERE Fournisseur='" + var3.at(i) + "' ORDER BY Date DESC");
            if(!req.next())
            {
                DEBUG << "Purge | Aucune ligne disponible";
                return;
            }
            Requete("UPDATE En_Cours SET Date='" + QDate::currentDate().toString("yyyy-MM-dd") + "' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "' AND Fournisseur='" + var3.at(i) + "'");

        }
    }
*/
    Requete("DELETE FROM En_Cours WHERE Date < '" + t.toString("yyyy-MM-dd") + "' AND Ajout='"+QString::number(endAdd)+"'");
}

QStringList DB::Find_Fournisseur_From_Invoice(QString invoice)
{
    QStringList list;
    QSqlQuery req = Requete("SELECT Fournisseur FROM En_Cours WHERE Numero_Commande='" + invoice + "'");
    while(req.next()) { list.append(req.value(0).toString()); }
    return list;
}

QString DB::Encrypt(QString text)
{
    QString crypt;
    QStringList k = QString(PKEY).split(" ");
    int idk(0);
    for(int i = 0;i<text.count();i++)
    {
        if(idk == k.count())
        {
            idk = 0;
        }
        int t = text.at(i).unicode();
        t -= k.at(idk).toInt();
        if(t > 250)
        {
            t = t - 250;
        }
        else if(t < 0)
        {
            t = t + 250;
        }
        if(t == 34)//si '
        {
            t = 251;
        }
        else if(t == 39)//ou "
        {
            t = 252;
        }
        crypt += QChar(t).toLatin1();
        idk++;
    }
    return crypt;
}

QString DB::Decrypt(QString text)
{
    QString decrypt;
    QStringList k = QString(PKEY).split(" ");
    int idk(0);
    for(int i = 0;i<text.count();i++)
    {
        if(idk == k.count())
        {
            idk = 0;
        }
        int t = text.at(i).unicode();
        if(t == 251)//retour a '
        {
            t = 34;
        }
        else if(t == 252)//retour a "
        {
            t = 39;
        }
        t += k.at(idk).toInt();
        if(t < 0)
        {
            t = t + 250;
        }
        else if(t > 250)
        {
            t = t - 250;
        }
        decrypt += QChar(t).toLatin1();
        idk++;
    }
    return decrypt;
}

QString DB::enum_State(int state)
{
    QString var;

    if(state == download) { var = "Télécharger"; }
    else if(state == error) { var = "Erreur"; }
    else if(state == updateRef) { var = "Modifier"; }
    else if(state == add) { var = "Bon ajouté"; }
    else if(state == endAdd) { var = "Ok"; }
    else
    {
        m_Error->Err(variable,"enum_State","DB");
        return QString();
    }
    return var;
}

QString DB::Get_Last_Invoice(QString frn)
{
    QSqlQuery req = Requete("SELECT Valeur FROM Options WHERE Nom='" + frn + "_Last'");
    if(req.next() == false)
    {
        m_Error->Err(requete,"Get_Last_Invoice:"+frn,"DB");
    }
    else
    {
        return req.value(0).toString();
    }
    return QString();
}

bool DB::Insert_Into_En_Cours(int id,QString date,QString Nom_Chantier,QString Numero_Commande,QString Numero_Livraison,QString Lien_Commande,int Etat,
                              int Ajout,QString Info_Chantier,int Ajout_BL,QString Numero_BC_Esab,QString Fournisseur)
{
    if(id <= 0)//ID
    {
        m_Error->Err(variable,"id <= 0",DB);
    }
    if(date.split("-").count() == 3)//Date
    {
        QStringList var = date.split("-");
        if(var.at(0).count() != 4 && var.at(1).count() != 2 && var.at(2).count() != 2)
        {
            m_Error->Err(variable,"Format date invalide: " + date,DB);
            return false;
        }
        if(var.at(1).toInt() > 0 && var.at(1).toInt() < 13 && var.at(2).toInt() > 0 && var.at(2).toInt() < 32)
        {
            m_Error->Err(variable,"Date invalide: " + date,DB);
            return false;
        }
    }
    else
    {
        m_Error->Err(variable,"Format date invalide: " + date,DB);
        return false;
    }
    if(Nom_Chantier.count() != 6 || Nom_Chantier.at(0).isNumber() || Nom_Chantier.at(Nom_Chantier.count()-1).isNumber())//Nom_Chantier
    {
        DEBUG << "DB | Nom_Chantier invalide : " << Nom_Chantier;
        Ajout = error;
    }
    if(Etat < open || Etat > Close)//Etat
    {
        m_Error->Err(variable,"Valeur Etat incorrect",DB);
        return false;
    }
    QSqlQuery req = Requete("SELECT Valeur FROM Options WHERE ID='13'");
    req.next();
    if(req.value(0).toInt() == 1)//Ajout_BL
    {
        Ajout_BL = 1;
    }
    //Fournisseur
    req = Requete("SELECT Valeur FROM Options WHERE ID='24'");
    req.next();
    if(req.value(0).toString().contains(Fournisseur) == false)
    {
        m_Error->Err(variable,"Founisseur inconnu",DB);
        return false;
    }
    Requete("INSERT INTO En_Cours VALUES('" + QString::number(id) + "','" + date + "','" + Nom_Chantier + "','" + Numero_Commande + "','" +
            Numero_Livraison + "','" + Lien_Commande + "','" + QString::number(Etat) + "','" + QString::number(Ajout) + "','" +
            Info_Chantier + "','" + QString::number(Ajout_BL) + "','" + Numero_BC_Esab + "','" + Fournisseur + "')");
    return true;
}

QSqlQuery DB::Get_Download_Invoice()
{
    return Requete("SELECT * FROM En_Cours WHERE Ajout='"+QString::number(download)+"' OR Ajout='"+QString::number(updateRef)+"'");
}

QSqlQuery DB::Get_Added_Invoice()
{
    return Requete("SELECT * FROM En_Cours WHERE Ajout_BL='1' AND Ajout='" + QString(add) + "'");
}

QSqlQuery DB::Get_No_Closed_Invoice(QString frn)
{
    return Requete("SELECT * FROM En_Cours WHERE Etat!='" + QString::number(Close) + "' AND Fournisseur='" + frn + "'");
}

QSqlQuery DB::Get_Delivery_Invoice(QString frn)
{
    return Requete("SELECT * FROM En_Cours WHERE Etat='" + QString::number(Close) + "' AND Fournisseur='" + QString(REXEL) + "' AND (Numero_Livraison='' OR Numero_Livraison=' ')");
}
