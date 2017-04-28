#include "db.h"

QString key = "ODBABL";//Clé ouverture BDD
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
/// ---------------
/// Table Options
/// ---------------
/// ID
/// Nom
/// Valeur
/////////////////////////////////
DB::DB()
{
}

QSqlQuery DB::Requete(QString req)
{
    QSqlQuery requete;

    if(requete.prepare(req))
    {
        if(!requete.exec())
            emit Error("DB | E101 | Execution de la requete échouée : " + req);
    }
    else
        emit Error("DB | E102 | Préparation de la requete échouée : " + req);

    return requete;
}

void DB::Init()
{
    //Ouverture de la DB
    QSqlDatabase db = QSqlDatabase::addDatabase("SQLITECIPHER");
    db.setDatabaseName(qApp->applicationDirPath() + "/bddInfo.db");
    db.setHostName("127.0.0.1");

    if(!db.open())
    {
        emit Error("DB | E001 | Ouverture de la Database échoué : " + db.hostName() + " " + db.driverName());
        return;
    }
    else
        emit Info("Ouverture de la Database réussi ! " + db.hostName() + " " + db.driverName());

    //test DB
    QSqlQuery req;
    req.exec("PRAGMA key = '" + key + "';");
    req = Requete("SELECT * FROM Options");
    if(!req.next())
        emit Error("DB | E002 | DB Inaccessible !");
    else
        emit Info(req.value("Nom").toString() + "=" + req.value("Valeur").toString());

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
        Requete("INSERT INTO Options VALUES('1','Minutes','22')");
        Requete("INSERT INTO Options VALUES('2','Heure','0')");
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
        Requete("INSERT INTO Options VALUES('13','ADD_Auto_BL','0')");
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
        Requete("INSERT INTO Options VALUES('24','','')");
        Requete("INSERT INTO Options VALUES('25','','')");
    }
    Requete("UPDATE En_Cours SET Etat='En préparation' WHERE Etat='En cours'");
    Requete("UPDATE En_Cours SET Etat='Livrée en totalité' WHERE Etat='Fermée'");
    Requete("UPDATE En_Cours SET Etat='Livrée Et Facturée' WHERE Etat='Livrée et facturée'");
    Requete("UPDATE En_Cours SET Etat='Livrée En Totalité' WHERE Etat='Livrée en totalité'");
    Requete("UPDATE En_Cours SET Etat='Partiellement Livrée' WHERE Etat='Partiellement livrée'");
}

void DB::Sav()
{
    qDebug() << "Sav";
    QFile sav(qApp->applicationDirPath() + "/bddInfoSav.db");
    QFile sav2(qApp->applicationDirPath() + "/bddInfoSav2.db");
    QFile sav3(qApp->applicationDirPath() + "/bddInfoSav3.db");
    QFileInfo f(sav);
    QFileInfo f1(sav2);
    QFileInfo f2(sav3);
    QFileInfo fBDD(qApp->applicationDirPath() + "/bddInfo.db");

    bool ok(true);

    if(f.lastModified().operator ==(fBDD.lastModified()) || f1.lastModified().operator ==(fBDD.lastModified()) || f2.lastModified().operator ==(fBDD.lastModified()))
        return;

    if(!sav.exists())
    {
        ok = sav.copy(qApp->applicationDirPath() + "/bddInfo.db",qApp->applicationDirPath() + "/bddInfoSav.db");
        emit Info("DB | Sauvegarde de la DB(sav)");
    }
    else if(!sav2.exists())
    {
        ok = sav.copy(qApp->applicationDirPath() + "/bddInfo.db",qApp->applicationDirPath() + "/bddInfoSav2.db");
        emit Info("DB | Sauvegarde de la DB(sav2)");
    }
    else if(!sav3.exists())
    {
        ok = sav.copy(qApp->applicationDirPath() + "/bddInfo.db",qApp->applicationDirPath() + "/bddInfoSav3.db");
        emit Info("DB | Sauvegarde de la DB(sav3)");
    }
    else
    {
        if(f.lastModified().operator <(f1.lastModified()) && f.lastModified().operator <(f2.lastModified()))
        {
            sav.remove();
            ok = sav.copy(qApp->applicationDirPath() + "/bddInfo.db",qApp->applicationDirPath() + "/bddInfoSav.db");
            emit Info("DB | Sauvegarde de la DB(sav)");
        }
        else if(f1.lastModified().operator <(f2.lastModified()))
        {
            sav2.remove();
            ok = sav2.copy(qApp->applicationDirPath() + "/bddInfo.db",qApp->applicationDirPath() + "/bddInfoSav2.db");
            emit Info("DB | Sauvegarde de la DB(sav2)");
        }
        else
        {
            sav3.remove();
            ok = sav3.copy(qApp->applicationDirPath() + "/bddInfo.db",qApp->applicationDirPath() + "/bddInfoSav3.db");
            emit Info("DB | Sauvegarde de la DB(sav3)");
        }
    }
    if(!ok)
        emit Error("DB | E301 | Echec de sauvegarde de la DB");
}

void DB::Purge()
{
    qDebug() << "Purge";
    QDate t;
    t = t.currentDate();
    t = t.addMonths(-2);
    Requete("DELETE FROM En_Cours WHERE Date < '" + t.toString("yyyy-MM-dd") + "' AND Ajout='Ok'");
}

