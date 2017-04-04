#include "db.h"

QString key = "5269856472300456";//Clé ouverture BDD

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
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(qApp->applicationDirPath() + "/bdd.db");
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
        Requete("INSERT INTO Options VALUES('24','Fournisseurs','Rexel.fr|')");
        Requete("INSERT INTO Options VALUES('25','','')");
    }

    if(query.prepare("ALTER TABLE En_Cours ADD Fournisseur TEXT"))
        if(!query.exec())
        {
          emit Error("Echec de modification de la base de données");
        }
    Requete("UPDATE En_Cours SET Fournisseur='Rexel.fr' WHERE Fournisseur=''");
    Requete("UPDATE En_Cours SET Etat='En préparation' WHERE Etat='En cours' AND Fournisseur='Rexel.fr'");
    Requete("UPDATE En_Cours SET Etat='Livrée en totalité' WHERE Etat='Fermée' AND Fournisseur='Rexel.fr'");
    Requete("UPDATE En_COurs SET Etat='Livrée Et Facturée' WHERE Etat='Livrée et facturée'");
    Requete("UPDATE En_Cours SET Etat='Livrée En Totalité' WHERE Etat='Livrée en totalité'");
    Requete("UPDATE En_Cours SET Etat='Partiellement Livrée' WHERE Etat='Partiellement livrée'");

    //Test DB
    req = Requete("SELECT * FROM Options");
    if(!req.next())//si DB inaccessible, lancer la mise à jour de la DB
    {
        db.close();
        QProcess p;
        p.start("MAJ_BDD.exe");
        p.waitForFinished();
        if(p.exitCode() == 1)
           emit Error("DB | E015 | Echec de mise à jour de la DB");
        else if(p.exitCode() != 0)
            emit Error("DB | E002 | DB Inaccessible !");
        Init();
        return;
    }
    else
        emit Info(req.value("Nom").toString() + "=" + req.value("Valeur").toString());
}

void DB::Close()
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

    if(f.lastModified().operator ==(fBDD.lastModified()) || f1.lastModified().operator ==(fBDD.lastModified()) || f2.lastModified().operator ==(fBDD.lastModified()))
        return;

    if(!sav.exists())
    {
        ok = sav.copy(qApp->applicationDirPath() + "/bdd.db",qApp->applicationDirPath() + "/bddSav.db");
        emit Info("DB | Sauvegarde de la DB(sav)");
    }
    else if(!sav2.exists())
    {
        ok = sav.copy(qApp->applicationDirPath() + "/bdd.db",qApp->applicationDirPath() + "/bddSav2.db");
        emit Info("DB | Sauvegarde de la DB(sav2)");
    }
    else if(!sav3.exists())
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

QStringList DB::Find_Fournisseur_From_Invoice(QString invoice)
{
    QStringList list;
    QSqlQuery req = Requete("SELECT Fournisseur FROM En_Cours WHERE Numero_Commande='" + invoice + "'");
    while(req.next())
        list.append(req.value(0).toString());
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
            idk = 0;
        int t = text.at(i).unicode();
        t -= k.at(idk).toInt();
        if(t > 250)
            t = t - 250;
        else if(t < 0)
            t = t + 250;
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
            idk = 0;
        int t = text.at(i).unicode();
        t += k.at(idk).toInt();
        if(t < 0)
            t = t + 250;
        else if(t > 250)
            t = t - 250;
        decrypt += QChar(t).toLatin1();
        idk++;
    }
    return decrypt;
}
