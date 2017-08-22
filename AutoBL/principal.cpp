#include "principal.h"
#include "ui_principal.h"

/////////////////////////////////
QString version("1.42"); //Version De L'application
QString ver("1420");
/////////////////////////////////

//Chargement de l'application
Principal::Principal(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Principal)
{
    ui->setupUi(this);

    //ARG
    for(int i = 0;i<qApp->arguments().count();i++)
        if(qApp->arguments().at(i).contains("-UpdateBDD="))
            if(qApp->arguments().at(i).split("=").at(1) == "1")
                QMessageBox::warning(0,"AutoBL",tr("Une erreur s'est produite durant la Restauration/mise à jour de la base de données."));

    //Traitement des variables de l'application
    m_Lien_Work = QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0);
    ui->e_Path->setText("Work Path : " + QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0));
    qDebug() << "Lien dossier de travail : " <<m_Lien_Work;
    premierDemarrage = false;

    //Erreur
    m_Error = new Error(m_Lien_Work);
    connect(m_Error,SIGNAL(sError(QString)),this,SLOT(Affichage_Erreurs(QString)));
    connect(m_Error,SIGNAL(sError(QString)),this,SLOT(AddError(QString)));

    //DB
    m_DB = new DB(m_Error);
    m_DB->Init();

    //Création des Dossiers
    QDir dir;
    dir.mkdir(m_Lien_Work);
    dir.mkdir(m_Lien_Work + "/Config");
    dir.mkdir(m_Lien_Work + "/Logs");

    //Ouverture des fichiers logs et errors
    m_Logs.setFileName(m_Lien_Work + "/Logs/logs.txt");
    //m_Errors.setFileName(m_Lien_Work + "/Logs/errors.txt");
    qDebug() << "Ouverture Logs " << m_Logs.open(QIODevice::WriteOnly | QIODevice::Append);
    //m_Errors.open(QIODevice::WriteOnly | QIODevice::Append);

    //Chargement des classes de l'application et fonctions nécéssaires
    Affichage_Info("-----------------------------AutoBL " + version + "------------------------------------");
    //Affichage_Erreurs("-----------------------------AutoBL " + version + "------------------------------------");
    m_Erreurs = 0;
    ui->e_Erreurs->setText("");
    ui->e_Erreur2->setText(QString::number(m_Erreurs));
    connect(m_DB,SIGNAL(Info(QString)),this,SLOT(Affichage_Info(QString)));
    connect(m_DB,SIGNAL(sError(QString)),this,SLOT(Affichage_Erreurs(QString)));
    connect(m_DB,SIGNAL(sError(QString)),this,SLOT(AddError(QString)));

    //Tache
    m_Tache = new Tache(version);
    m_Tache->Affichage_Info("AutoBL V" + version);

    //Vérification mise à jour
    MAJ();

    m_Tri = 0;
    Afficher_tNomFichier();
    mdp = new QLineEdit;
    mdp->setEchoMode(QLineEdit::Password);
    Login_False();

    //Premier démarrage
    QFileInfo f(qApp->applicationDirPath() + "/Config.esab");
    QFileInfo f2(m_Lien_Work + "/Config/Config.esab");
    if(!QFile::exists(m_Lien_Work + "/Config/Config.esab"))
    {
        QFile::copy("Config.esab",m_Lien_Work + "/Config/Config.esab");
        qDebug() << "Premier démarrage initialisé";
        premierDemarrage = true;
    }
    else if(f.lastModified() != f2.lastModified() && f.exists())//Vérification nouvelle version Config.esab
    {
        DEBUG << "Copie du Config.esab en cours...";
        bool err(false);
        QFile file(m_Lien_Work + "/Config/Config.esab");
        err = file.remove();
        err = QFile::copy(qApp->applicationDirPath() + "/Config.esab",m_Lien_Work + "/Config/Config.esab");
        if(err)
            DEBUG << "Copie de Config.esab échoué";
        else
            DEBUG << "Copie de Config.esab réussis";
    }

    //Chargement des paramètres
    Init_Config();

    //Préparation Esabora + Fournisseur
    QSqlQuery req;
    m_Frn = new Fournisseur(m_Lien_Work,m_DB,m_Error);
    req = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom='FrnADD'");
    while(req.next())
        if(!m_Frn->Add(req.value(0).toString()))
            Affichage_Erreurs(tr("Ajout du fournisseur %1 échoué").arg(req.value(0).toString()));

    m_Esabora = new Esabora(this,ui->eLogin->text(),ui->eMDP->text(),ui->lienEsabora->text(),m_Lien_Work,m_DB,m_Error);
    QThread *thread = new QThread;
    ///Déplacement des classes dans un thread séparé
    m_Esabora->moveToThread(thread);

    Chargement_Parametres();

    //Création des connect
    connect(m_DB,SIGNAL(CreateTable()),this,SLOT(PurgeError()));

    connect(m_Tache,SIGNAL(temps_Restant()),this,SLOT(Affichage_Temps_Restant()));
    connect(m_Tache,SIGNAL(Ouvrir()),this,SLOT(show()));
    connect(m_Tache,SIGNAL(ajout_BC()),this,SLOT(Demarrage()));
    connect(m_Tache,SIGNAL(Arret()),this,SLOT(Arret()));
    connect(m_Tache,SIGNAL(Ouvrir()),this,SLOT(Affichage_Temps_Restant()));
    connect(m_Tache,SIGNAL(Quitter()),this,SLOT(Quitter()));
    connect(m_Tache,SIGNAL(MAJ_BC()),this,SLOT(Demarrer_Frn()));

    connect(m_Frn,SIGNAL(Info(QString)),this,SLOT(Update_Fen_Info(QString)));
    connect(m_Frn,SIGNAL(LoadProgress(int)),this,SLOT(LoadWeb(int)));
    connect(m_Frn,SIGNAL(En_Cours_Info(QString)),this,SLOT(Update_Fen_Info(QString)));
    connect(m_Frn,SIGNAL(En_Cours_Fournisseur(QString)),this,SLOT(Fournisseur_Actuel(QString)));
    connect(m_Frn,SIGNAL(Change_Load_Window(QString)),this,SLOT(Update_Load_Window(QString)));

    connect(m_Esabora,SIGNAL(Info(QString)),this,SLOT(Affichage_Info(QString)));
    connect(m_Esabora,SIGNAL(Erreur(QString)),this,SLOT(AddError(QString)));
    connect(m_Esabora,SIGNAL(Message(QString,QString,bool)),this,SLOT(Afficher_Message_Box(QString,QString,bool)));
    connect(m_Esabora,SIGNAL(DemandeListeMatos(QString)),this,SLOT(Get_Tableau_Matos(QString)));
    connect(this,SIGNAL(End_Get_Tableau_Matos()),m_Esabora,SIGNAL(ReceptionListeMatos()));

    connect(ui->bLienEsabora,SIGNAL(clicked(bool)),this,SLOT(Emplacement_Esabora()));
    connect(ui->bTestEsabora,SIGNAL(clicked(bool)),this,SLOT(Test_Esabora()));
    connect(ui->actionAbout_Qt,SIGNAL(triggered(bool)),qApp,SLOT(aboutQt()));
    connect(ui->actionAbout,SIGNAL(triggered(bool)),this,SLOT(About()));
    connect(ui->bLogInfo,SIGNAL(clicked(bool)),this,SLOT(Affichage_Dossier_Logs()));
    connect(ui->bTestRexel,SIGNAL(clicked(bool)),this,SLOT(Test_Rexel()));
    connect(ui->bDossier_Travail,SIGNAL(clicked(bool)),this,SLOT(Afficher_Dossier_Travail()));
    connect(ui->bPurge,SIGNAL(clicked(bool)),this,SLOT(Purge_Bl()));
    connect(ui->tNomFichier,SIGNAL(cellChanged(int,int)),this,SLOT(Modif_Cell_TNomFichier(int,int)));
    connect(ui->tNomFichier,SIGNAL(cellDoubleClicked(int,int)),this,SLOT(Dble_Clique_tNomFichier(int,int)));
    connect(ui->bSauvegarder,SIGNAL(clicked(bool)),this,SLOT(Sauvegarde_Parametres()));
    connect(ui->gAjoutAuto,SIGNAL(clicked(bool)),this,SLOT(Etat_Ajout_Auto()));   
    connect(ui->b_Help,SIGNAL(clicked(bool)),this,SLOT(Help()));
    connect(&m_Temps,SIGNAL(timeout()),this,SLOT(Demarrage()));
    connect(ui->actionConnection,SIGNAL(triggered(bool)),this,SLOT(Login()));
    connect(qApp,SIGNAL(lastWindowClosed()),this,SLOT(Login_False()));
    connect(ui->bTestBC,SIGNAL(clicked(bool)),this,SLOT(Test_BC()));
    connect(ui->bTestBL,SIGNAL(clicked(bool)),this,SLOT(Test_BL()));
    connect(ui->actionRapport_de_bug,SIGNAL(triggered(bool)),this,SLOT(Bug_Report())); 
    connect(ui->tNomFichier,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(menu_TNomFichier(QPoint)));
    connect(ui->mDPA,SIGNAL(textChanged(QString)),this,SLOT(Verif_MDP_API()));
    connect(ui->bEffacer,SIGNAL(clicked(bool)),this,SLOT(PurgeError()));
    connect(ui->bMAJRepertoire,SIGNAL(clicked(bool)),this,SLOT(MAJ_Repertoire_Esabora()));
    connect(ui->bFrnAjouter,SIGNAL(clicked(bool)),this,SLOT(Add_Fournisseur()));
    connect(ui->bFrnRetirer,SIGNAL(clicked(bool)),this,SLOT(Del_Fournisseur()));
    connect(ui->listFrnAjouter,SIGNAL(currentRowChanged(int)),this,SLOT(Load_Param_Fournisseur()));
    connect(ui->bFrnSave,SIGNAL(clicked(bool)),this,SLOT(Save_Param_Fournisseur()));
    connect(ui->bFrnTest,SIGNAL(clicked(bool)),this,SLOT(Test_Fournisseur()));
    connect(ui->listFrnAjouter,SIGNAL(clicked(QModelIndex)),this,SLOT(Load_Param_Fournisseur()));
    connect(ui->restaurerDB,SIGNAL(clicked(bool)),this,SLOT(Restaurer_DB()));

    qApp->setQuitOnLastWindowClosed(false);

    Affichage_Info(m_Lien_Work);

    if(premierDemarrage)//Init Premier démarrage
    {
        this->show();
        ui->tabWidget->setCurrentIndex(2);
        ui->eLogin->setText("1");
        ui->eMDP->setEchoMode(QLineEdit::Normal);
        ui->eMDP->setText("2");
        ui->lienEsabora->setText("<-3");
    }

    req = m_DB->Requete("SELECT * FROM Options WHERE ID='16'");
    req.next();
    if(req.value("Valeur").toInt() == 0)
    {
        Help(true);
        m_DB->Requete("UPDATE Options SET Valeur='1' WHERE ID='16'");
    }

    //Reset MDP API
    if(qApp->arguments().contains("-RSTMDP"))
    {
        m_DB->Requete("UPDATE Options SET Valeur='' WHERE ID='11'");
        QMessageBox::information(this,"","Le mot de passe de l'application à été réinitialisé !");
    }

    qDebug() << test();
}

Principal::~Principal()
{ 
    delete ui;
}

void Principal::Init_Config()
{
    ///Démarrage automatique
    QSqlQuery req = m_DB->Requete("SELECT * FROM Options WHERE Nom='Auto'");
    req.next();
    if(req.value("Valeur").toInt() == 1)
    {
        ui->gAjoutAuto->setChecked(true);
        ui->Heure->setEnabled(true);
    }
    else
    {
        ui->gAjoutAuto->setChecked(false);
        ui->Heure->setEnabled(false);
    }
    ///Variable Heure && minutes Démarrage
    req = m_DB->Requete("SELECT * FROM Options WHERE Nom='Heure'");
    req.next();
    QSqlQuery req2 = m_DB->Requete("SELECT * FROM Options WHERE Nom='Minutes'");
    req2.next();
    ui->Heure->setTime(QTime(req.value("Valeur").toInt(),req2.value("Valeur").toInt()));
    ///Variable login Esabora
    req = m_DB->Requete("SELECT * FROM Options WHERE Nom='Login'");
    req.next();
    ui->eLogin->setText(m_DB->Decrypt(req.value("Valeur").toString()));
    ///Variable MDP Esabora
    req = m_DB->Requete("SELECT * FROM Options WHERE Nom='MDP'");
    req.next();
    ui->eMDP->setText(m_DB->Decrypt(req.value("Valeur").toString()));
    ///Variable Emplacement Esabora
    req = m_DB->Requete("SELECT * FROM Options WHERE Nom='LienEsabora'");
    req.next();
    ui->lienEsabora->setText(req.value("Valeur").toString());
    ///Variable MDP Application

    ///Variable temps entre chaque commandes Esabora
    req = m_DB->Requete("SELECT * FROM Options WHERE ID='14'");
    req.next();
    ui->tmpCmd->setValue(req.value("Valeur").toDouble());
    ///Variable nom BDD Esabora
    req = m_DB->Requete("SELECT * FROM Options WHERE ID='12'");
    req.next();
    ui->nBDDEsab->setText(req.value("Valeur").toString());
    ///Variable Temps boucle Navigation Rexel
    req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='19'");
    req.next();
    ui->tmpBoucleRexel->setValue(req.value("Valeur").toDouble());
    ///Variable Purge auto BDD
    req = m_DB->Requete("SELECT * FROM Options WHERE ID='17'");
    req.next();
    if(req.value("Valeur").toInt() == 1)
        ui->autoPurgeDB->setChecked(true);
    else
        ui->autoPurgeDB->setChecked(false);
    ///Variable Ajout Auto des BL
    req = m_DB->Requete("SELECT * FROM Options WHERE ID='13'");
    req.next();
    if(req.value("Valeur").toInt() == 1)
        ui->ajoutAutoBL->setChecked(true);
    ///Variable démarrage avec windows
    QSettings settings2("Microsoft","Windows\\CurrentVersion\\Run");
    if(settings2.value("AutoBL").toString() != "")
        ui->cDWin->setChecked(true);
    ///Page Information
    /// Version
    ui->versionAPI->setText(version);
    ///Cycle total
    req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='20'");
    req.next();
    ui->totalBCCrees->setText(req.value("Valeur").toString());
    ///Semi Automatique
    req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='22'");
    req.next();
    if(req.value("Valeur").toInt() == 1 && 0)
        ui->semiAuto->setChecked(true);
    else
        ui->semiAuto->setChecked(false);
    ///Nom Entreprise Esabora
    req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='23'");
    req.next();
    ui->nEntrepriseEsab->setText(req.value("Valeur").toString());
    Init_Fournisseur();

    m_Arret = true;
    login = false,
    ui->tNomFichier->setContextMenuPolicy(Qt::CustomContextMenu);

    Show_List_Sav();
}

//Config//////////////////////////////
void Principal::Sauvegarde_Parametres()
{
    if(ui->gAjoutAuto->isChecked())
        m_DB->Requete("UPDATE Options SET Valeur='1' WHERE Nom='Auto'");
    else
        m_DB->Requete("UPDATE Options SET Valeur='0' WHERE Nom='Auto'");

    m_DB->Requete("UPDATE Options SET Valeur='" + ui->Heure->time().toString("HH") + "' WHERE Nom='Heure'");
    m_DB->Requete("UPDATE Options SET Valeur='" + ui->Heure->time().toString("mm") + "' WHERE Nom='Minutes'");
    m_DB->Requete("UPDATE Options SET Valeur='" + m_DB->Encrypt(ui->eLogin->text()) + "' WHERE Nom='Login'");
    m_DB->Requete("UPDATE Options SET Valeur='" + m_DB->Encrypt(ui->eMDP->text()) + "' WHERE Nom='MDP'");
    m_DB->Requete("UPDATE Options SET Valeur='" + ui->lienEsabora->text() + "' WHERE Nom='LienEsabora'");
    if(!ui->mDPA->text().isEmpty())
        m_DB->Requete("UPDATE Options SET Valeur='" + HashMDP(ui->mDPA->text()) + "' WHERE Nom='MDPA'");
    m_DB->Requete("UPDATE Options SET Valeur='" + ui->nBDDEsab->text() + "' WHERE ID='12'");
    m_DB->Requete("UPDATE Options SET Valeur='" + QString::number(ui->tmpCmd->value()) + "' WHERE ID='14'");
    m_DB->Requete("UPDATE Options SET Valeur='" + QString::number(ui->tmpBoucleRexel->value()) + "' WHERE ID='19'");
    if(ui->autoPurgeDB->isChecked())
        m_DB->Requete("UPDATE Options SET Valeur='1' WHERE ID='17'");
    else
        m_DB->Requete("UPDATE Options SET Valeur='0' WHERE ID='17'");
    if(ui->ajoutAutoBL->isChecked())
    {
        m_DB->Requete("UPDATE Options SET Valeur='1' WHERE ID='13'");
        m_DB->Requete("UPDATE En_Cours SET Ajout_BL='1'");
    }
    else
    {
        m_DB->Requete("UPDATE Options SET Valeur='0' WHERE ID='13'");
        m_DB->Requete("UPDATE En_Cours SET Ajout_BL='0'");
    }
    if(ui->semiAuto->isChecked())
        m_DB->Requete("UPDATE Options SET Valeur='1' WHERE ID='22'");
    else
        m_DB->Requete("UPDATE Options SET Valeur='0' WHERE ID='22'");
    m_DB->Requete("UPDATE Options SET Valeur='" + ui->nEntrepriseEsab->text() + "' WHERE ID='23'");

    QSettings settings2("Microsoft","Windows\\CurrentVersion\\Run");
    if(ui->cDWin->isChecked())
    {
        if(settings2.value("AutoBL").toString() == "")
            settings2.setValue("AutoBL",qApp->applicationFilePath().replace("/","\\"));
    }
    else
        settings2.remove("AutoBL");

    QSqlQuery r = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='7'");
    QSqlQuery r2 = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='8'");
    r.next();
    r2.next();

    if(premierDemarrage)
    {
        if(!ui->ajoutAutoBL->isChecked())
            if(QMessageBox::question(this,"","Souhaitez-vous que les bons de livraisons soit ajoutés automatiquement ?") == QMessageBox::Yes)
                ui->ajoutAutoBL->setChecked(true);
        if(!ui->autoPurgeDB->isChecked())
            if(QMessageBox::question(this,"","Souhaitez-vous que la base de données d'AutoBL supprime les bons validés de plus de 2 mois ?") == QMessageBox::Yes)
                ui->autoPurgeDB->setChecked(true);
        if(!ui->rapport_Erreur->isChecked())
            if(QMessageBox::question(this,"","Souhaitez-vous qu'AutoBL envoie les rapports d'erreurs pour faciliter le débuggage(aucune information personnel n'est transmise) ?") == QMessageBox::Yes)
                ui->rapport_Erreur->setChecked(true);
        premierDemarrage = false;
        Sauvegarde_Parametres();
        Afficher_tNomFichier();
    }

    Affichage_Info("Sauvegarde des paramètres terminés",true);
    Chargement_Parametres();
}

void Principal::Chargement_Parametres()
{
    if(ui->gAjoutAuto->isChecked())
        Demarrage_Auto_BC(true);
    else
        m_Temps.stop();

    //Affichage colonne BL
    QSqlQuery val = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='13'");
    val.next();
    if(val.value("Valeur").toInt() == 1)
        ui->tNomFichier->hideColumn(7);
    else
        ui->tNomFichier->showColumn(7);

    //Mise à jour des variables membres
    m_Esabora->Set_Var_Esabora(ui->lienEsabora->text(),ui->eLogin->text(),ui->eMDP->text());

    Etat_Ajout_Auto();
}

void Principal::Demarrage_Auto_BC(bool Demarrage_Timer)
{
    if(Demarrage_Timer && ui->gAjoutAuto->isChecked())
    {
        //Vérification de mise à jour dispo
        MAJ();

        //Préparation de m_temps
        Affichage_Info("DEBUG | Demarrage Auto | Début");
        QTime temps2;
        QStringList var = temps2.currentTime().toString("H-m").split("-");
        QStringList var2 = ui->Heure->text().split(":");
        int heure(0),minutes(0);
        if(var.at(0).toInt() > var2.at(0).toInt())
            heure = 24 - (var.at(0).toInt() - var2.at(0).toInt());
        else
            heure = var2.at(0).toInt() - var.at(0).toInt();
        if(var.at(1).toInt() > var2.at(1).toInt())
        {
            minutes = 60 - (var.at(1).toInt() - var2.at(1).toInt());
            if(heure == 0)
                heure = 24;
            heure--;
        }
        else
            minutes = var2.at(1).toInt() - var.at(1).toInt();

            qDebug() << "temps " << (heure * 60 * 60 + minutes * 60) * 1000 << heure << minutes;
         m_Temps.start((heure * 60 * 60 + minutes * 60) * 1000);
         Affichage_Info("DEBUG | Demarrage Auto | test temps : " + QString::number(m_Temps.remainingTime()));

         //Envoie rapport de bugs
         if(ui->e_Erreur2->text().toInt() > 0)
             if(Post_Report())
             {
                 m_Errors.resize(0);
                 m_Logs.resize(0);
             }
         if(ui->autoPurgeDB->isChecked())
             m_DB->Purge();
    }
    else if(!Demarrage_Timer)
    {
        DEBUG << "DEBUG | Demarrage Auto | Boucle";
        //boucle de 1mn pour attente avant redemarrage
        QTimer t;
        QEventLoop l;
        connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
        t.start(60000);
        l.exec();
        DEBUG << "DEBUG | Demarrage Auto | Fin Boucle";
        Demarrage_Auto_BC(true);
    }
}

void Principal::Etat_Ajout_Auto()
{
    if(ui->gAjoutAuto->isChecked())
        ui->Heure->setEnabled(true);
    else
        ui->Heure->setEnabled(false);
}

void Principal::PurgeError()
{
    ui->e_Erreur2->setText("0");
    ui->eArgErreurs->clear();
    ui->aff_Erreur->clear();
    ui->tabWidget->setTabText(1,"Information");
}

void Principal::Show_List_Sav()
{
    ui->listSavDB->clear();

    QFile s(qApp->applicationDirPath() + "/bddSav.db");
    QFile s2(qApp->applicationDirPath() + "/bddSav2.db");
    QFile s3(qApp->applicationDirPath() + "/bddSav3.db");
    QFileInfo i(s);
    QFileInfo i2(s2);
    QFileInfo i3(s3);
    if(i.lastModified().toString("dd/MM/yyyy") != "")
        ui->listSavDB->addItem(i.lastModified().toString("dd/MM/yyyy"));
    if(i2.lastModified().toString("dd/MM/yyyy") != "")
        ui->listSavDB->addItem(i2.lastModified().toString("dd/MM/yyyy"));
    if(i3.lastModified().toString("dd/MM/yyyy") != "")
        ui->listSavDB->addItem(i3.lastModified().toString("dd/MM/yyyy"));
}

void Principal::Verif_MDP_API()
{
    QPalette v,r;
    v.setColor(QPalette::Text,Qt::black);
    r.setColor(QPalette::Text,Qt::red);
    if(ui->mDPA->text().contains(" "))
        ui->mDPA->setPalette(r);
    else
        ui->mDPA->setPalette(v);
}

//Affichage///////////////////////////
void Principal::Affichage_Erreurs(QString texte, bool visible)
{
    QTextStream flux(&m_Errors);
    flux << QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm - ") << texte << "\r\n";
    ui->aff_Erreur->append(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm - ") + texte + "\r\n");

    m_Erreurs++;
    ui->e_Erreurs->setText(tr("%n Erreur(s)","",ui->e_Erreurs->text().split(" ").at(0).toInt()+1));
    ui->e_Erreur2->setText(tr("%n","",m_Erreurs));
    if(ui->e_Erreur2->text().toInt() > 0)
        ui->tabWidget->setTabText(1,tr("Information(%n)","",m_Erreurs));

    if(visible)
        m_Tache->Affichage_Info(texte);
}

void Principal::Affichage_Info(QString texte, bool visible)
{
    QTextStream flux(&m_Logs);
    flux << QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm - ") << texte << "\r\n";

    if(visible)
        m_Tache->Affichage_Info(texte);
}

void Principal::Affichage_Dossier_Logs()
{
    QDesktopServices::openUrl(QUrl(m_Lien_Work + "/Logs",QUrl::TolerantMode));
}

void Principal::Afficher_Dossier_Travail()
{
    QDesktopServices::openUrl(QUrl(m_Lien_Work,QUrl::TolerantMode));
}

void Principal::Affichage_Temps_Restant()
{
    if(m_Temps.remainingTime() != -1)
    {
        int minutes = m_Temps.remainingTime() / 1000 / 60;
        int heures = minutes / 60;
        minutes = minutes % 60;
        qDebug() << heures << minutes;

        QEventLoop loop;
        QTimer timer;
        connect(&timer,SIGNAL(timeout()),&loop,SLOT(quit()));
        timer.start(1000);
        loop.exec();
        if(this->isVisible())
            ui->eTimer->setText(tr("%n Heures ","",heures) + tr("%n Minutes","",minutes));
        else
            Affichage_Info("Prochaine recherche dans :\n" + QString::number(heures) + " Heure(s) " + QString::number(minutes) + " Minute(s)",true);
    }
    else
    {
        qDebug() << "Desactiver";
        if(this->isVisible())
            ui->eTimer->setText(tr("Inactif"));
        else
            Affichage_Info("Recherche automatique désactivé !",true);
    }
    if(ui->e_Erreur2->text().toInt() > 0)
        ui->tabWidget->setTabText(1,tr("Information(") + ui->e_Erreur2->text() + ")");
    else
        ui->tabWidget->setTabText(1,tr("Information"));
}

void Principal::Afficher_tNomFichier(int l,int c,int tri)
{
    DEBUG << "Traitement affichage tableau";

    ui->tNomFichier->blockSignals(true);
    ui->tNomFichier->clearContents();
    QFlag flag = ~Qt::ItemIsEditable;
    QSqlQuery req;
    if(tri == 10)
        tri = m_Tri;
    if(tri == 0)///Tri par Date
        req = m_DB->Requete("SELECT * FROM En_Cours ORDER BY Date");
    else if(tri == 1)///Tri par Etat Esabora
        req = m_DB->Requete("SELECT * FROM En_Cours ORDER BY Ajout");
    else if(tri == 2)///Tri par Référence
        req = m_DB->Requete("SELECT * FROM En_Cours ORDER BY Nom_Chantier");

    if(!req.exec())
        Affichage_Erreurs("Requete affichage des fichiers échoué !",true);

    while(ui->tNomFichier->rowCount() > 0)
        ui->tNomFichier->removeRow(0);

    //Affichage de la colonne numéro BC esabora si logué
    if(login) ui->tNomFichier->showColumn(8);
    else ui->tNomFichier->hideColumn(8);

    while(req.next())
    {
        //Contrôle des nouveau bons
        if(req.value("Ajout").toInt() == download && !req.value("Nom_Chantier").toString().at(0).isDigit() && !req.value("Nom_Chantier").toString().at(req.value("Nom_Chantier").toString().count()-1).isDigit())
            m_DB->Requete("UPDATE En_Cours SET Ajout='"+QString::number(error)+"' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'AND Fournisseur='" + req.value("Fournisseur").toString() + "'");

        //Création de la ligne
        if(req.value("Ajout").toInt() != endAdd || login)
        {
            ui->tNomFichier->insertRow(0);
            QStringList d = req.value("Date").toString().split("-");
            ui->tNomFichier->setItem(0,0,new QTableWidgetItem(d.at(2) + "-" + d.at(1) + "-" + d.at(0)));
            ui->tNomFichier->setItem(0,6,new QTableWidgetItem(req.value("Etat").toString()));
            ui->tNomFichier->setItem(0,1,new QTableWidgetItem(req.value("Ajout").toString()));
            ui->tNomFichier->setItem(0,2,new QTableWidgetItem(req.value("Nom_Chantier").toString()));
            ui->tNomFichier->setItem(0,3,new QTableWidgetItem(req.value("Info_Chantier").toString()));
            ui->tNomFichier->setItem(0,5,new QTableWidgetItem(req.value("Numero_Commande").toString()));
            ui->tNomFichier->setItem(0,4,new QTableWidgetItem(req.value("Numero_Livraison").toString()));
            ui->tNomFichier->setItem(0,3,new QTableWidgetItem(req.value("Info_Chantier").toString()));
            ui->tNomFichier->setItem(0,7,new QTableWidgetItem());
            ui->tNomFichier->setItem(0,8,new QTableWidgetItem(req.value("Numero_BC_Esabora").toString()));
            ui->tNomFichier->setItem(0,9,new QTableWidgetItem(req.value("Fournisseur").toString()));
            if(ui->tNomFichier->item(0,4)->text() != "" && ui->tNomFichier->item(0,1)->text() == "Bon Ajouté")
            {
                if(req.value("Ajout_BL").toInt() == 0)
                    ui->tNomFichier->item(0,7)->setCheckState(Qt::Unchecked);
                else
                    ui->tNomFichier->item(0,7)->setCheckState(Qt::Checked);

                //Force Check BL
                QSqlQuery bl = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='13'");
                bl.next();
                if(bl.value("Valeur").toInt() == 1)
                {
                    ui->tNomFichier->item(0,7)->setCheckState(Qt::Checked);
                    ui->tNomFichier->hideColumn(7);
                }
            }

            ///Ajout des verrous items + couleur
            ui->tNomFichier->item(0,0)->setFlags(flag);
            ui->tNomFichier->item(0,1)->setFlags(flag);

            //Couleur des items(Etat)
            if(ui->tNomFichier->item(0,6)->text().toInt() == Close)
            {
                ui->tNomFichier->item(0,6)->setText("Livrée en totalité");
                ui->tNomFichier->item(0,6)->setTextColor(QColor(0,255,0));
            }
            else if(ui->tNomFichier->item(0,6)->text().toInt() == partial)
            {
                ui->tNomFichier->item(0,6)->setText("Partiellement livrée");
                ui->tNomFichier->item(0,6)->setTextColor(QColor(200,200,0));
            }
            else
            {
                ui->tNomFichier->item(0,6)->setText(tr("En préparation"));
                ui->tNomFichier->item(0,6)->setTextColor(QColor(255,0,0));
            }

            //Couleur et verrouillage des items(Ajout)
            if(ui->tNomFichier->item(0,1)->text().toInt() == error)
                ui->tNomFichier->item(0,1)->setTextColor(QColor(255,0,0));
            else if(ui->tNomFichier->item(0,1)->text().toInt() == download || ui->tNomFichier->item(0,1)->text().toInt() == updateRef)
                ui->tNomFichier->item(0,1)->setTextColor(QColor(200,200,0));
            else
                ui->tNomFichier->item(0,1)->setTextColor(QColor(0,255,0));
            ui->tNomFichier->item(0,1)->setFlags(flag);
            if(ui->tNomFichier->item(0,1)->text().toInt() != error && ui->tNomFichier->item(0,1)->text().toInt() != updateRef && !login
                    && ui->tNomFichier->item(0,1)->text().toInt() != download)
                ui->tNomFichier->item(0,2)->setFlags(flag);
            ui->tNomFichier->item(0,4)->setFlags(flag);
            ui->tNomFichier->item(0,5)->setFlags(flag);
            ui->tNomFichier->item(0,6)->setFlags(flag);

            //Mise à jour colonne 1 int > string
            ui->tNomFichier->item(0,1)->setText(m_DB->enum_State(ui->tNomFichier->item(0,1)->text().toInt()));
        }
    }

    if(l > 0)//retour à la dernière ligne selectionnée
        ui->tNomFichier->setCurrentCell(l,c);

    //Paramètres tableau
    ui->tNomFichier->setAlternatingRowColors(true);
    ui->tNomFichier->blockSignals(false);
    ui->tNomFichier->resizeColumnsToContents();
    ui->tNomFichier->resizeRowsToContents();
    ui->tNomFichier->resize(ui->tNomFichier->columnWidth(0) + ui->tNomFichier->columnWidth(1) +
                            ui->tNomFichier->columnWidth(2) + ui->tNomFichier->columnWidth(3) +
                            ui->tNomFichier->columnWidth(4) + ui->tNomFichier->columnWidth(5) +
                            ui->tNomFichier->columnWidth(6) + ui->tNomFichier->columnWidth(7) +
                            ui->tNomFichier->columnWidth(8) + ui->tNomFichier->columnWidth(9) +
                            105,ui->tNomFichier->size().height());

    this->resize(ui->tNomFichier->size());

    DEBUG << "Affichage tableau terminé";
}

//Esabora/////////////////////////////
void Principal::Emplacement_Esabora()
{
    ui->lienEsabora->setText(QFileDialog::getOpenFileName(this,"Emplacement Esabora",ui->lienEsabora->text(),"*.ink",0,QFileDialog::DontResolveSymlinks));

    QSqlQuery req = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom='LienEsabora'");
    req.next();
    if(req.value("Valeur").toString() != ui->lienEsabora->text())
    {
        m_Esabora->Set_Var_Esabora(ui->lienEsabora->text(),ui->eLogin->text(),ui->eMDP->text());
        if((ui->nBDDEsab->text() == "" || ui->nEntrepriseEsab->text() == "") && ui->eLogin->text() != "" && ui->eMDP->text() != "")
            if(QMessageBox::question(this,"","Souhaitez vous qu'AutoBL tente de remplir les champs Répertoire et nom d'entreprise automatiquement ?") ==
                    QMessageBox::Yes)
                MAJ_Repertoire_Esabora();
    }
}

void Principal::MAJ_Repertoire_Esabora()
{
    QString entreprise,BDD;
    m_Esabora->Apprentissage(entreprise,BDD);
    ui->nBDDEsab->setText(BDD);
    ui->nEntrepriseEsab->setText(entreprise);

    Sauvegarde_Parametres();
}

//Traitement/////////////////////////
void Principal::Demarrage()
{
    QSqlQuery req;

    m_DB->Sav();
    Affichage_Info("Recherche de nouveau BL...",true);
    m_Temps.stop();
    m_Arret = false;///Début d'ajout BC

    ///Verif Affichage popup TV
    HWND hWnds = FindWindow(NULL,L"Sessions sponsorisées");
    if(hWnds != NULL)
    {
        SetForegroundWindow(hWnds);
        keybd_event(VK_EXECUTE,0,0,0);
        keybd_event(VK_EXECUTE,0,KEYEVENTF_KEYUP,0);
    }

    ///Récuperation des BC
    Create_Fen_Info("Fournisseur","Info");
    if(ui->activ_Rexel->isChecked())
        if(!m_Frn->Start())
            Affichage_Erreurs("Des erreurs se sont produites durant la recherche de bons");
    Update_Fen_Info();

    int nbBC(0),nbBL(0);
    ///Démarrage d'esabora
    bool automatic = ui->ajoutAutoBL->isChecked();
    if(!m_Arret && ui->activ_Esab->isChecked())
        if(!m_Esabora->Start(automatic,nbBC,nbBL))
            Affichage_Erreurs(tr("Des erreurs se sont produite durant la procédure d'ajout de bon"));


    Afficher_tNomFichier();
    req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='20'");
    req.next();
    m_DB->Requete("UPDATE Options SET Valeur='" + QString::number(req.value("Valeur").toInt()+nbBC) + "' WHERE ID='20'");
    DEBUG << "nb BC créer : " << nbBC << " nb BC validé : " << nbBL;
    ui->totalBCCrees->setText(QString::number(ui->totalBCCrees->text().toInt()+nbBC));
    ui->lastCycleBCCrees->setText(QString::number(nbBC));
    ui->lastCycleBCValides->setText(QString::number(nbBL));

    m_Tache->Affichage_Info("Ajout de bon terminé");
    m_Arret = true;///Fin d'ajout BC
    emit FinAjout();///envoie signal en cas d'arret de l'API

    if(ui->autoPurgeDB->isChecked())
        m_DB->Purge();
    if(ui->e_Erreur2->text().toInt() > 0)
        Post_Report();
    Demarrage_Auto_BC();
}

void Principal::Demarrer_Frn()
{
    Create_Fen_Info("Fournisseur","Info");
    if(!m_Frn->Start())
        Affichage_Erreurs("Des erreurs se sont produites durant la recherche de bons");
    Update_Fen_Info();
    Afficher_tNomFichier();
}

void Principal::Arret()
{
    qDebug() << "Arret demandé !";
    m_Arret = true;
    m_Esabora->Stop();
    m_Tache->Affichage_Info("Procédure en cours d'arret, veuiller patientez...");
}

//Fenêtre de traitement//////////////
void Principal::InfoTraitementBL()
{
    QDialog *f = new QDialog(this);
    f->setObjectName("traitement");
    f->setWindowTitle("AutoBL : Traitement...");
    f->setWindowIcon(QIcon(":/icone/Doc.png"));
    QFormLayout l;
    QLabel *rexel = new QLabel;
    rexel->setObjectName("connexion");
    rexel->setText("En Cours");
    QLabel *rexel2 = new QLabel;
    rexel2->setObjectName("navigation");
    rexel2->setText("En Attente");
    QLabel *esabora = new QLabel;
    esabora->setObjectName("BC");
    esabora->setText("En Attente");
    QLabel *esabora2 = new QLabel;
    esabora2->setObjectName("BL");
    esabora2->setText("En Attente");
    QLabel *info = new QLabel;
    info->setObjectName("Info");
    QLabel *info2 = new QLabel("Appuyer sur 'Echap' pour stopper la procédure.");
    QVBoxLayout *vL = new QVBoxLayout;
    vL->addWidget(info2);

    l.addRow(info2);
    l.addRow("Connexion Rexel",rexel);
    l.addRow("Récupération des informations",rexel2);
    l.addRow("Ajout bon de commande",esabora);
    l.addRow("Validation bon de commande",esabora2);
    l.addRow(info);
    l.addRow(info);
    f->setLayout(&l);
    f->setWindowFlags(Qt::Tool | Qt::CustomizeWindowHint);
    f->show();
    connect(f,SIGNAL(rejected()),this,SLOT(Arret()));
}

void Principal::ModifInfoTraitementBL(QString label,QString etat)
{
    if(this->findChild<QDialog *>("traitement") != NULL)
    {
        this->findChild<QDialog *>("traitement")->findChild<QLabel *>(label)->setText(etat);

        if(label == "connexion")
            this->findChild<QDialog *>("traitement")->findChild<QLabel *>("navigation")->setText("En cours");
        else if(label == "navigation")
            this->findChild<QDialog *>("traitement")->findChild<QLabel *>("BC")->setText("En cours");
        else if(label == "BC")
            this->findChild<QDialog *>("traitement")->findChild<QLabel *>("BL")->setText("En cours");
        else if(label == "BL")
        {
            while(this->findChild<QDialog *>("traitement")->findChildren<QLabel *>().count() > 0)
                delete this->findChild<QDialog *>("traitement")->findChildren<QLabel *>().at(0);
            delete this->findChild<QDialog *>("traitement");
        }
    }
}

//Fenêtres//////////////////////////
void Principal::About()
{
    QFormLayout *layout = new QFormLayout;
    QLabel vVersion(version);
    QLabel auteur("Kévin BRIAND");
    QLabel contact("briand-kevin@hotmail.fr");
    QLabel licence("Ce logiciel est sous licence GNU LGPLv3");
    QLabel github("<a href='https://github.com/firedream89/AutoBL'>ici</a>");
    github.setOpenExternalLinks(true);
    layout->addRow("Version",&vVersion);
    layout->addRow("Auteur",&auteur);
    layout->addRow("Contact",&contact);
    layout->addRow("Licence",&licence);
    layout->addRow("Sources",&github);
    QDialog *fen = new QDialog;
    fen->setLayout(layout);
    fen->setWindowTitle("About AutoBL");
    fen->exec();
}

void Principal::Afficher_Message_Box(QString header,QString texte, bool warning)
{

    if(!warning)
        QMessageBox::information(this,header,texte);
    else
        QMessageBox::warning(this,header,texte);

}

void Principal::Help(bool p)
{
    QString user;
    if(p)
    {
        QFile f("Help/accueil.txt");
        if(!f.open(QIODevice::ReadOnly))
            Affichage_Erreurs("Principal | E080 | Erreur dans l'ouverture du fichier general.txt");
        else
        {
            user = QTextStream(&f).readAll();
        }
    }
    else
    {
        QFile f("Help/accueil.txt");
        if(!f.open(QIODevice::ReadOnly))
            Affichage_Erreurs("Principal | E080 | Erreur dans l'ouverture du fichier general.txt");
        else
        {
            user = QTextStream(&f).readAll();
        }
    }
    QTextBrowser *text = new QTextBrowser;
    text->setWindowTitle("Aide");

    if(ui->tabWidget->currentIndex() == 0)
        text->setHtml(user);
    else if(ui->tabWidget->currentIndex() == 1)
        text->setHtml(user);
    text->resize(this->size());
    text->show();
}

//Test/////////////////////////////
void Principal::Test_Rexel()
{
    m_Frn->Show_Web();
}

void Principal::Test_Esabora()
{
    return;
    if(m_Esabora->Lancement_API())
    {
        QSqlQuery req = m_DB->Requete("SELECT * FROM En_Cours WHERE Ajout='"+QString::number(download)+"' OR Ajout='"+QString::number(updateRef)+"'");
        while(req.next())
        {
            if(!m_Esabora->Ajout_BC(req.value("Numero_Commande").toString()))
            {
                Affichage_Erreurs(tr("Ajout du bon de commande %0 échoué").arg(req.value("Numero_Commande").toString()));
                m_DB->Requete("UPDATE En_Cours SET Ajout='"+QString::number(error)+"' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");

                if(req.value("Ajout").toInt() != updateRef)
                {
                    QFile file(m_Lien_Work + "/Pj/" + req.value("Numero_Commande").toString() + ".xlsx");
                    if(QFile::copy(m_Lien_Work + "/Pj/" + file.fileName(),m_Lien_Work + "/Pj/" + file.fileName()))
                        file.remove();
                }
            }
            else
            {
                m_DB->Requete("UPDATE En_Cours SET Ajout='"+QString::number(endAdd)+"' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
            }
        }
        if(!m_Esabora->Fermeture_API())
            Affichage_Erreurs("Fermeture d'Esabora échoué");
    }
    else
        Affichage_Erreurs("Démarrage d'Esabora échoué");
}

bool Principal::test()
{
    return true;
}

void Principal::Test_BC()
{
    if(m_Esabora->Lancement_API())
    {
        QSqlQuery req = m_DB->Requete("SELECT * FROM En_Cours WHERE Ajout='"+QString::number(download)+"' OR Ajout='"+QString::number(updateRef)+"'");
        if(req.next())
        {
            m_Tache->Affichage_En_Cours();
            QString fxlsx = req.value("Numero_Commande").toString() + ".xlsx";
            if(!m_Esabora->Ajout_BC(req.value("Numero_Commande").toString()))
            {
                if(m_Esabora->GetEtat() == 0)
                {
                    Affichage_Erreurs(tr("Ajout du bon de commande %0 échoué, BC non créé !").arg(req.value("Numero_Commande").toString()));
                }
                if(m_Esabora->GetEtat() == 1)
                {
                    Affichage_Erreurs(tr("Ajout du bon de commande %0 échoué, BC Vide à supprimer !").arg(req.value("Numero_Commande").toString()));
                    m_DB->Requete("UPDATE En_Cours SET Ajout='"+QString::number(error)+"' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
                }
            }
            else
            {
                m_DB->Requete("UPDATE En_Cours SET Ajout='Bon Ajouté' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
            }
        }
        m_Esabora->Fermeture_API();
    }
}

void Principal::Test_BL()
{
    if(m_Esabora->Lancement_API())
        for(int cpt=0;cpt<ui->tNomFichier->rowCount();cpt++)
        {
            if(ui->tNomFichier->item(cpt,7)->checkState() == Qt::Checked)
            {
                QSqlQuery req = m_DB->Requete("SELECT * FROM En_Cours WHERE Numero_Commande='" + ui->tNomFichier->item(cpt,5)->text() + "'");
                if(req.next())
                {
                    if(!m_Esabora->Ajout_BL(req.value("Numero_BC_Esabora").toString(),ui->tNomFichier->item(cpt,4)->text()))
                    {
                        Affichage_Erreurs(tr("Principal | Ajout BL N°%0 échoué").arg(ui->tNomFichier->item(cpt,4)->text()));
                    }
                    else
                    {
                        Affichage_Info("Principal | Ajout BL N°" + ui->tNomFichier->item(cpt,3)->text() + " Réussi");
                        m_DB->Requete("UPDATE En_Cours SET Ajout='"+QString::number(endAdd)+"' WHERE Numero_Commande='" + ui->tNomFichier->item(cpt,4)->text() + "'");
                    }
                }
                else
                {
                    Affichage_Erreurs("Principal | BL non trouvé dans la DB !");
                }
                cpt = ui->tNomFichier->rowCount();
            }
        }
    m_Esabora->Fermeture_API();
}

//DB//////////////////////////////
void Principal::Purge_Bl()
{
    if(QMessageBox::question(this,"Purge","Voulez vous vraiment purger la base de données ?"))
    {
        m_DB->Requete("DELETE FROM En_Cours");
    }
}

void Principal::Remove_Row_DB()
{
    int l = ui->tNomFichier->currentRow();
    if(QMessageBox::question(this,"","Voulez-vous vraiment placer ce bon en terminé ?") == QMessageBox::Yes)
        m_DB->Requete("UPDATE En_Cours SET Ajout='"+QString::number(endAdd)+"' WHERE Numero_Commande='" + ui->tNomFichier->item(l,5)->text() + "'");
}

void Principal::Restaurer_DB()
{
    m_DB->Close();

    QString var;
    QFile s(qApp->applicationDirPath() + "/bddSav.db");
    QFile s2(qApp->applicationDirPath() + "/bddSav2.db");
    QFile s3(qApp->applicationDirPath() + "/bddSav3.db");
    QFileInfo i(s);
    QFileInfo i2(s2);
    QFileInfo i3(s3);
    QListWidgetItem *item = ui->listSavDB->selectedItems().at(0);
    if(item->text() == i.lastModified().toString("dd/MM/yyyy"))
    {
        var = "/bddSav.db";
    }
    else if(item->text() == i2.lastModified().toString("dd/MM/yyyy"))
    {
        var = "/bddSav2.db";
    }
    else if(item->text() == i3.lastModified().toString("dd/MM/yyyy"))
    {
        var = "/bddSav3.db";
    }

    if(!var.isEmpty())
    {
        QDesktopServices::openUrl(QUrl(qApp->applicationDirPath() + "/MAJ_BDD.exe -restore=" + var));
        qApp->exit(0);
    }

    m_DB->Init();
    Show_List_Sav();
}

//tableau tNomFichier/////////////
void Principal::Dble_Clique_tNomFichier(int l,int c)
{
    if(c == 1 && ui->semiAuto->isChecked())//Semi auto
    {
        if(QMessageBox::question(this,"Saisie semi automatique","Attention l'application Esabora doit être lancée sur la fenêtre de démarrage"
                                                            ", si vous continuez la procédure l'ajout du bon de commande commencera.\n"
                                                            "A la fin de la procédure vous devrez vérifier et enregistrer manuellement le bon.\n"
                                                            "Merci de ne pas toucher au PC durant la procédure !\n"
                                                            "Voulez-vous continuer ?") == QMessageBox::No) return;
        Affichage_Info("Récupération de la liste de matériel...",true);
        m_Esabora->Reset_Liste_Matos();
        Get_Tableau_Matos(l);
        Affichage_Info("Préparation de l'ajout sur Esabora...",true);
        m_Esabora->Semi_Auto(ui->tNomFichier->item(l,5)->text());
    }   
    else if(c == 5)//Affichage Tableau matos
    {
        //Affichage chargement
        InfoWindow *load = new InfoWindow(this,"",1);
        load->Add_Label("Update",false);
        load->Update_Label("Update",tr("Préparation du tableau\nVeuillez patienter..."));
        load->Show();

        QStringList list = m_Frn->Get_Invoice_List(ui->tNomFichier->item(l,9)->text(),ui->tNomFichier->item(l,5)->text());
        DEBUG << list;

        load->Close();
        QTableWidget *tbl = new QTableWidget;
        tbl->insertColumn(0);
        tbl->insertColumn(0);
        tbl->insertColumn(0);
        tbl->insertColumn(0);
        tbl->insertColumn(0);
        tbl->insertColumn(0);
        tbl->insertColumn(0);
        tbl->setHorizontalHeaderItem(0,new QTableWidgetItem("Marque"));
        tbl->setHorizontalHeaderItem(1,new QTableWidgetItem("Référence"));
        tbl->setHorizontalHeaderItem(2,new QTableWidgetItem("Désignation"));
        tbl->setHorizontalHeaderItem(3,new QTableWidgetItem("Prix unitaire"));
        tbl->setHorizontalHeaderItem(4,new QTableWidgetItem("Quantité"));
        tbl->setHorizontalHeaderItem(5,new QTableWidgetItem("Reste à livrer"));
        tbl->setHorizontalHeaderItem(6,new QTableWidgetItem("Prix total"));
        tbl->setWindowTitle("");
        for(int cpt = 0;cpt<list.count();cpt++)
        {
            if(list.count() - cpt >= 7)
            {
                tbl->insertRow(0);
                tbl->setItem(0,2,new QTableWidgetItem(list.at(cpt)));//Designation
                cpt++;
                tbl->setItem(0,1,new QTableWidgetItem(list.at(cpt)));//reference
                cpt++;
                tbl->setItem(0,0,new QTableWidgetItem(list.at(cpt)));//fabricant
                cpt += 2;
                tbl->setItem(0,3,new QTableWidgetItem(list.at(cpt)));//Prix unitaire
                cpt++;
                tbl->setItem(0,4,new QTableWidgetItem(list.at(cpt)));//Quantité
                cpt++;
                tbl->setItem(0,5,new QTableWidgetItem(list.at(cpt)));//Quantité restante
                QString p = tbl->item(0,3)->text().replace("€","");
                double v = p.replace(",",".").toDouble();
                double v2 = tbl->item(0,4)->text().replace(",",".").toDouble();
                double tmp = v * v2;
                tbl->setItem(0,6,new QTableWidgetItem(QString::number(tmp).replace(".",",") + "€"));//Prix total
            }
        }
        tbl->resizeColumnsToContents();
        tbl->resizeRowsToContents();
        tbl->resize(tbl->columnWidth(0)+tbl->columnWidth(1)+tbl->columnWidth(2)+tbl->columnWidth(3)+
                    tbl->columnWidth(4)+tbl->columnWidth(5)+tbl->columnWidth(6)+100,tbl->height());

        //Create table window
        QDialog *f = new QDialog(this);
        QGridLayout *l = new QGridLayout(f);
        l->addWidget(tbl);
        f->resize(tbl->width(),tbl->height());
        f->exec();
        while(!f->isHidden())
        {
            QTimer t;
            QEventLoop l;
            connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
            t.start(500);
            l.exec();
        }
        tbl->clearContents();
        f->deleteLater();
    }
    else if(c == 7)//MAJ BDD Ajout Manu BL
    {
        if(ui->tNomFichier->item(l,c)->checkState() == Qt::Checked)
        {
            m_DB->Requete("UPDATE En_Cours SET Ajout_BL='1' WHERE Numero_Commande='" + ui->tNomFichier->item(l,5)->text() + "'");
        }
        else
        {
            m_DB->Requete("UPDATE En_Cours SET Ajout_BL='0' WHERE Numero_Commande='" + ui->tNomFichier->item(l,5)->text() + "'");
        }
    }

}

void Principal::Modif_Cell_TNomFichier(int l,int c)
{
    if(c == 2)//Modif nom chantier
    {
        if(QMessageBox::question(this,"",tr("Voulez vous vraiment modifier la référence de chantier ?")) == 16384)
        {
            if(ui->tNomFichier->item(l,2)->text().at(0).isDigit() && ui->tNomFichier->item(l,2)->text().at(ui->tNomFichier->item(l,2)->text().count()-1).isDigit())
                m_DB->Requete("UPDATE En_Cours SET Nom_Chantier='" + ui->tNomFichier->item(l,2)->text() + "', Ajout='"+QString::number(updateRef)+"' WHERE Numero_Commande='" + ui->tNomFichier->item(l,5)->text() + "'");
            else
                QMessageBox::information(this,"",tr("La référence de chantier doit être un nombre !"));
        }
        Afficher_tNomFichier(l,c);
    }
    else if(c == 8)//Modif numero esabora
    {
        if(QMessageBox::question(this,"",tr("Voulez vous vraiment modifier le numéro esabora ?")) == 16384)
        {
            m_DB->Requete("UPDATE En_Cours SET Numero_BC_Esabora='" + ui->tNomFichier->item(l,8)->text() + "' WHERE Numero_Commande='" + ui->tNomFichier->item(l,5)->text() + "'");
        }
        Afficher_tNomFichier();
    }

}

void Principal::menu_TNomFichier(QPoint point)
{
    QMenu *menuTri = new QMenu("Trier par");
    QAction *action = new QAction("Date",this);
    QAction *b = new QAction("Etat",this);
    QAction *c = new QAction("Référence",this);
    connect(action,SIGNAL(triggered(bool)),this,SLOT(Tri_TNomFichier_Date()));
    connect(b,SIGNAL(triggered(bool)),this,SLOT(Tri_TNomFichier_Nom()));
    connect(c,SIGNAL(triggered(bool)),this,SLOT(Tri_TNomFichier_Ref()));
    menuTri->addAction(action);
    menuTri->addAction(b);
    menuTri->addAction(c);

    QMenu mMenu(this);
    mMenu.addMenu(menuTri);
    QAction *mb = new QAction("Supprimer",this);
    connect(mb,SIGNAL(triggered(bool)),this,SLOT(Remove_Row_DB()));
    mMenu.addAction(mb);

    if(ui->tabWidget->tabText(2) != "Configuration")
        mb->setEnabled(false);

    mMenu.exec(ui->tNomFichier->mapToGlobal(point));
}

void Principal::Tri_TNomFichier_Date()
{
    Afficher_tNomFichier(ui->tNomFichier->currentRow(),ui->tNomFichier->currentColumn(),0);
}

void Principal::Tri_TNomFichier_Nom()
{
    Afficher_tNomFichier(ui->tNomFichier->currentRow(),ui->tNomFichier->currentColumn(),1);
}

void Principal::Tri_TNomFichier_Ref()
{
    Afficher_tNomFichier(ui->tNomFichier->currentRow(),ui->tNomFichier->currentColumn(),2);
}

//Fenêtre login//////////////////
void Principal::Login()
{
    QDialog *d = new QDialog;
    QGridLayout *l = new QGridLayout;
    QPushButton *b = new QPushButton("Envoyer");
    d->setWindowTitle("Mot de passe");
    l->addWidget(mdp);
    l->addWidget(b);
    d->setLayout(l);
    mdp->setText("");
    connect(b,SIGNAL(clicked(bool)),d,SLOT(accept()));
    connect(d,SIGNAL(accepted()),this,SLOT(Login_True()));
    d->exec();
}

void Principal::Login_False()
{
    QSqlQuery req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='11'");
    if(req.next())
    {
        if(req.value("Valeur").toString() != "")
        {
            ui->tabWidget->removeTab(3);
            ui->tabWidget->removeTab(2);
            login = false;
            Afficher_tNomFichier();
            m_Tache->Login(false);
        }
        else if(ui->tabWidget->isTabEnabled(3))
            ui->tabWidget->removeTab(3);
    }
    ui->tabWidget->setCurrentIndex(0);
}

void Principal::Login_True()
{
    QString mdph = mdp->text().split(" ").at(0);
    QByteArray mdpb = QCryptographicHash::hash(mdph.toLatin1(),QCryptographicHash::Sha256);
    DEBUG << mdpb.toHex();
    QSqlQuery req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='11'");
    req.next();
    DEBUG << req.value(0);
    if(req.value("Valeur").toString() != mdpb.toHex())
        QMessageBox::information(this,"Erreur","Mot de passe faux !");
    else
    {
        ui->tabWidget->addTab(ui->Configuration,"Configuration");
        if(mdp->text().split(" ").count() == 2)
            if(mdp->text().split(" ").at(1) == "dbg")
                ui->tabWidget->addTab(ui->Debug,"Débuggage");
        login = true;
        Afficher_tNomFichier();
        m_Tache->Login(true);
    }
    ui->tabWidget->setCurrentIndex(0);
}

//Recherche de mise à jour//////
void Principal::MAJ()
{
    QTimer *timer = new QTimer;
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(MAJLINK))); // Url vers le fichier version.txt
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    timer->start(10000);
    QObject::connect(timer,SIGNAL(timeout()), timer, SLOT(stop()));
    QObject::connect(timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    loop.exec();
    qDebug() << reply->errorString();
    timer->stop();
    QString retour1 = reply->readAll();
    QStringList retour = retour1.split("\n");
    qDebug() << "MAJ - Dernière version = " << retour;
    QStringList MAJDispo;
    for(int cpt=0;cpt<retour.count();cpt++)
        if(retour.at(cpt).toDouble() > ver.toDouble())
            MAJDispo.append("ver=" + retour.at(cpt));
    QSqlQuery req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='21'");
    req.next();
    if(MAJDispo.count() == 1 && MAJDispo.at(0) != req.value("Valeur").toString())
    {
        qDebug() << "MAJ - Une mise à jour est disponible !";
        m_DB->Requete("UPDATE Options SET Valeur='" + MAJDispo.at(0) + "' WHERE ID='21'");
        MAJDispo.append("API=" + QCoreApplication::applicationDirPath() + "/" + "AutoBL.exe");
        MAJDispo.append("ftp=" + QString(MAJLINK));
        MAJDispo.append("RA=oui");
        MAJDispo.append("name=AutoBL");
        MAJDispo.append("icon=");
        MAJDispo.append("webSite=https://github.com/firedream89/AutoBL/releases");
        QProcess MAJ;
        MAJ.start(QCoreApplication::applicationDirPath() + "/" + "MAJ.exe",MAJDispo);
        exit(0);
    }
}

//Envoie de rapport/////////////
void Principal::Bug_Report()
{
    QDialog *f = new QDialog(this);
    f->setWindowTitle("Rapport de bug");
    f->setFixedHeight(500);
    f->setFixedWidth(500);
    f->setObjectName("rapport de bug");
    QGridLayout *l = new QGridLayout(f);
    QLabel *lbl1 = new QLabel("Commentaire");
    QTextEdit *text = new QTextEdit;
    QPushButton *btn = new QPushButton("Envoyer le rapport");
    l->addWidget(lbl1,0,0);
    l->addWidget(text,0,1);
    l->addWidget(btn,1,0);
    connect(btn,SIGNAL(clicked(bool)),this,SLOT(Post_Report()));
    connect(f,SIGNAL(finished(int)),f,SLOT(deleteLater()));
    f->show();
}

bool Principal::Post_Report()
{
    qDebug() << "Post Report";
    QSqlQuery r = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom='Nom_BDD'");
    r.next();
    QString nur = r.value(0).toString();
    if(this->findChildren<QDialog *>("rapport de bug").count() > 0)
    {
        if(this->findChildren<QDialog *>().at(0)->windowTitle() == "Rapport de bug")
        {
            this->findChildren<QDialog *>().at(0)->findChildren<QPushButton *>().at(0)->setEnabled(false);
            this->findChildren<QDialog *>().at(0)->findChildren<QPushButton *>().at(0)->setText("Création du rapport...");
            QFile rapport("Rapport_" + QDateTime::currentDateTime().toString("dd-MM-yyyy hh-mm") + ".esab");
            rapport.open(QIODevice::WriteOnly);
            QTextStream flux(&rapport);
            QFile f(m_Lien_Work + "/Logs/logs.txt");
            f.open(QIODevice::ReadOnly);
            flux << "---------------- Rapport AutoBL " << nur << " " << QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm") << " ----------------\r\n";
            flux << "Commentaire : " << this->findChildren<QDialog *>().at(0)->findChildren<QTextEdit *>().at(0)->toPlainText();
            flux << "- Logs -\r\n" << f.readAll() << "\r\n";
            f.close();
            f.setFileName(m_Lien_Work + "/Logs/errors.txt");
            f.open(QIODevice::ReadOnly);
            flux << "- Erreurs -\r\n" << f.readAll() << "\r\n";
            f.close();

            this->findChildren<QDialog *>().at(0)->findChildren<QPushButton *>().at(0)->setText("Envoi du rapport...");
            rapport.close();
            rapport.open(QIODevice::ReadOnly);

            QWebEngineView w;
            w.load(QString(MAJLINK) + "mail.php");
            QTimer t;
            QEventLoop l;
            connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
            connect(&w,SIGNAL(loadFinished(bool)),&l,SLOT(quit()));
            t.start(30000);
            l.exec();
            w.page()->runJavaScript("document.getElementById('sj').value='" + rapport.fileName().split(".").at(0) + "';",[&l](const QVariant r){l.quit();});
            l.exec();
            QString text;
            while(!rapport.atEnd())
                text += rapport.readLine();
            text.replace("\r\n","<br/>");
            w.page()->runJavaScript("document.getElementById('msg').value='<html><head></head><body>" + text + "</body></html>';",[&l](const QVariant r){l.quit();});
            l.exec();
            w.page()->runJavaScript("document.getElementById('frm').submit();",[&l](const QVariant r){l.quit();});
            l.exec();

            this->findChildren<QDialog *>().at(0)->findChildren<QPushButton *>().at(0)->setText("Rapport envoyé !");
            l.exec();

            rapport.close();
            rapport.remove();
        }
    }
    else
    {
        QFile rapport("Rapport_" + QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm") + ".esab");
        rapport.open(QIODevice::WriteOnly);
        QTextStream flux(&rapport);
        QFile f(m_Lien_Work + "/Logs/logs.txt");
        f.open(QIODevice::ReadOnly);
        flux << "---------------- Rapport AutoBL " << nur << " " << QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm") << " ----------------\r\n";
        flux << "- Logs -\r\n" << f.readAll() << "\r\n";
        f.close();
        f.setFileName(m_Lien_Work + "/Logs/errors.txt");
        f.open(QIODevice::ReadOnly);
        flux << "- Erreurs -\r\n" << f.readAll() << "\r\n";
        f.close();
        f.setFileName(m_Lien_Work + "/Logs/debug.log");
        f.open(QIODevice::ReadOnly);
        flux << "- Debug -\r\n";
        while(!f.atEnd())
        {
            QString v = f.readLine();
            if(!v.contains("Cannot create accessible interface for object:"))
                flux << v << "\r\n";
        }
        f.close();

        rapport.close();
        rapport.open(QIODevice::ReadOnly);

        QWebEngineView w;
        w.load(QString(MAJLINK) + "mail.php");
        QTimer t;
        QEventLoop l;
        connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
        connect(&w,SIGNAL(loadFinished(bool)),&l,SLOT(quit()));
        t.start(30000);
        l.exec();
        w.page()->runJavaScript("document.getElementById('sj').value=" + rapport.fileName().split(".").at(0) + ";",[&l](const QVariant r){l.quit();});
        l.exec();
        QString text;
        while(!rapport.atEnd())
            text += rapport.readLine();
        text.replace("\r\n","<br/>");
        w.page()->runJavaScript("document.getElementById('msg').value='<html><head></head><body>" + text + "</body></html>';",[&l](const QVariant r){l.quit();});
        l.exec();
        w.page()->runJavaScript("document.getElementById('frm').submit();",[&l](const QVariant r){l.quit();});
        l.exec();

        rapport.close();
        rapport.remove();
    }
    m_Logs.resize(0);
    m_Errors.resize(0);
    QFile f(m_Lien_Work + "/Logs/debug.log");
    f.resize(0);
    ui->e_Erreur2->setText("0");
    return true;
}

//Fournisseur///////////////////
void Principal::Get_Tableau_Matos(int l)
{
    m_Esabora->Set_Liste_Matos(m_Frn->Get_Invoice_List(ui->tNomFichier->item(l,9)->text(),ui->tNomFichier->item(l,3)->text()));
}
//Erreur 9xx
void Principal::Get_Tableau_Matos(QString Numero_Commande)
{
    QStringList list = m_DB->Find_Fournisseur_From_Invoice(Numero_Commande);
    if(list.count() != 1)
    {
        if(list.count() == 0)
            Affichage_Erreurs(tr("Aucun fournisseur trouvé"));
        else
            Affichage_Erreurs(tr("Plusieurs fournisseurs ont été trouvés"));
        return;
    }
    m_Esabora->Set_Liste_Matos(m_Frn->Get_Invoice_List(list.at(0),Numero_Commande));
    emit End_Get_Tableau_Matos();
}

void Principal::Init_Fournisseur()
{
    QSqlQuery req = m_DB->Requete("SELECT Valeur FROM Options WHERE ID='24'");
    req.next();
    if(req.value(0).toString() != m_Frn->List_Frn())
        m_DB->Requete("UPDATE Options SET Valeur='" + m_Frn->List_Frn() + "', Nom='Fournisseurs' WHERE ID='24'");

    req = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom='Fournisseurs' OR Nom='FrnADD'");

    req.next();
    DEBUG << req.value("Valeur");
    if(req.value("Valeur").toString().split("|").count() > 1)
    {
        QStringList list = req.value("Valeur").toString().split("|");
        while(req.next())
        {
            list.removeOne(req.value("Valeur").toString());
            ui->listFrnAjouter->addItem(req.value("Valeur").toString());
        }
        if(list.last().isEmpty() && list.count() > 0)
            list.removeLast();
        ui->listFrnDispo->addItems(list);
    }
    else
        DEBUG << "Erreur Liste Fournisseurs non trouvé !";
    DEBUG << "FIN Init_Fournisseurs";
}

void Principal::Add_Fournisseur()
{
    ui->listFrnAjouter->blockSignals(true);
    if(ui->listFrnDispo->selectedItems().isEmpty())
        return;
    ui->listFrnAjouter->addItem(ui->listFrnDispo->selectedItems().at(0)->text());
    ui->listFrnAjouter->setCurrentRow(ui->listFrnAjouter->count()-1);
    QSqlQuery req = m_DB->Requete("SELECT MAX(ID) FROM Options");
    req.next();
    m_DB->Requete("INSERT INTO Options VALUES('" + QString::number(req.value(0).toInt()+1) + "','FrnADD','" + ui->listFrnDispo->selectedItems().at(0)->text() + "')");

    req = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom='" + ui->listFrnDispo->selectedItems().at(0)->text() + "'");
    QStringList f;
    if(req.next())
    {
        f = m_DB->Decrypt(req.value("Valeur").toString()).split("|");
        m_Frn->Add(ui->listFrnDispo->selectedItems().at(0)->text(),f.at(0),f.at(1),f.at(2));
    }
    else
    {
        DEBUG << "ADD Fournisseur : " << ui->listFrnDispo->currentItem()->text() << " : variables non défini";
        Load_Param_Fournisseur();
    }
    ui->listFrnDispo->clear();
    req = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom='Fournisseurs' OR Nom ='FrnADD' ORDER BY ID ASC");
    req.next();
    if(req.value("Valeur").toString().split("|").count() > 1)
    {
        QStringList list = req.value("Valeur").toString().split("|");
        while(req.next())
            list.removeOne(req.value("Valeur").toString());
        ui->listFrnDispo->addItems(list);
    }
    ui->listFrnAjouter->blockSignals(false);
}

void Principal::Del_Fournisseur()
{
    ui->listFrnAjouter->blockSignals(true);
    if(ui->listFrnAjouter->selectedItems().isEmpty())
        return;
    ui->listFrnDispo->addItem(ui->listFrnAjouter->selectedItems().at(0)->text());
    m_DB->Requete("DELETE FROM Options WHERE Valeur='" + ui->listFrnAjouter->selectedItems().at(0)->text() + "'");
    QSqlQuery req = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom ='FrnADD'");
    m_Frn->Del(ui->listFrnAjouter->selectedItems().at(0)->text());
    ui->listFrnAjouter->clear();

    QStringList list;
    while(req.next())
        list.append(req.value("Valeur").toString());
    ui->listFrnAjouter->addItems(list);
    ui->listFrnAjouter->blockSignals(false);
}

void Principal::Save_Param_Fournisseur()
{
    if(ui->eFrnMail->text().isEmpty() || ui->eFrnMDP->text().isEmpty() || ui->eFrnUserName->text().isEmpty() || ui->eFrnRcc->text().isEmpty())
    {
        QMessageBox::information(this,"","Tout les champs doivent être remplis !");
        return;
    }
    QString r = ui->eFrnUserName->text() + "|" + ui->eFrnMail->text() + "|" + ui->eFrnMDP->text();
    QSqlQuery req = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom='" + ui->listFrnAjouter->currentItem()->text() + "'");
    if(req.next())
        m_DB->Requete("UPDATE Options SET Valeur='" + m_DB->Encrypt(r) + "' WHERE Nom ='" + ui->lFrn->text() + "'");
    else
    {
        req = m_DB->Requete("SELECT MAX(ID) FROM Options");
        req.next();
        m_DB->Requete("INSERT INTO Options VALUES('" + QString::number(req.value(0).toInt()+1) + "','" + ui->lFrn->text() + "','" + m_DB->Encrypt(r) + "')");
    }
    req = m_DB->Requete("SELECT * FROM Options WHERE Nom='" + ui->lFrn->text() + "Rcc'");
    if(req.next())
        m_DB->Requete("UPDATE Options SET Valeur='" + ui->eFrnRcc->text() + "' WHERE Nom='" + ui->lFrn->text() + "Rcc'");
    else
    {
        req = m_DB->Requete("SELECT MAX(ID) FROM Options");
        req.next();
        m_DB->Requete("INSERT INTO Options VALUES('" + QString::number(req.value(0).toInt()+1) + "','" + ui->lFrn->text() + "Rcc','" + ui->eFrnRcc->text() + "')");
    }
    if(m_Frn->Update_Var(ui->lFrn->text(),ui->eFrnMail->text(),ui->eFrnMDP->text(),ui->eFrnUserName->text()))
        Affichage_Info("Informations " + ui->listFrnAjouter->currentItem()->text() + " mise à jour",true);
    else
    {
        m_Frn->Add(ui->lFrn->text(),ui->eFrnMail->text(),ui->eFrnMDP->text(),ui->eFrnUserName->text());
        Affichage_Info("Informations " + ui->listFrnAjouter->currentItem()->text() + " mise à jour",true);
    }
}

void Principal::Test_Fournisseur()
{
    Create_Fen_Info("Info");
    QTimer *t = new QTimer;
    connect(t,SIGNAL(timeout()),this,SLOT(Update_Fen_Info()));
    connect(t,SIGNAL(timeout()),t,SLOT(deleteLater()));
    if(m_Frn->Test_Connexion(ui->lFrn->text()))
        t->start(5000);
    else
        t->start(10000);
}

void Principal::Load_Param_Fournisseur()
{
    ui->lFrn->clear();
    ui->lFrnUserName->clear();
    ui->lFrnMail->clear();
    ui->lFrnMDP->clear();
    ui->eFrnUserName->clear();
    ui->eFrnMail->clear();
    ui->eFrnMDP->clear();
    ui->eFrnRcc->clear();
    if(ui->listFrnAjouter->selectedItems().isEmpty())
        return;
    QStringList l = m_Frn->Get_Frn_Inf(ui->listFrnAjouter->currentItem()->text()).split("|");

    if(l.count() == 3)
    {
        ui->lFrnUserName->setText(l.at(0));
        ui->lFrnMail->setText(l.at(1));
        ui->lFrnMDP->setText(l.at(2));
    }

    if(ui->lFrnUserName->text() == "")
        ui->lFrnUserName->setText("Nom d'utilisateur");
    if(ui->lFrnMail->text() == "")
        ui->lFrnMail->setText("Mail");
    if(ui->lFrnMDP->text() == "")
        ui->lFrnMDP->setText("Mot de passe");


    QSqlQuery req = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom='" + ui->listFrnAjouter->currentItem()->text() + "'");
    ui->lFrn->setText(ui->listFrnAjouter->currentItem()->text());
    if(req.next())
    {
        l = m_DB->Decrypt(req.value("Valeur").toString()).split("|");
        if(l.count() == 3)
        {
            ui->eFrnUserName->setText(l.at(0));
            ui->eFrnMail->setText(l.at(1));
            ui->eFrnMDP->setText(l.at(2));

            req = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom='" + ui->lFrn->text() + "Rcc'");
            if(req.next())
                ui->eFrnRcc->setText(req.value("Valeur").toString());
        }
    }
}

//Fenêtre Info/////////////////
void Principal::Create_Fen_Info(QString label1, QString label2, QString label3, QString label4)
{
    InfoWindow *f = new InfoWindow(this,"",1);

    if(label1.isEmpty())
    {
        DEBUG << "Principal | Error label1 is empty";
        f->Close();
    }
    else
    {
        if(label1 == "Info")
            f->Add_Label(label1,false);
        else
            f->Add_Label(label1);
    }
    if(!label2.isEmpty())
        f->Add_Label(label2);
    if(!label3.isEmpty())
        f->Add_Label(label3);
    if(!label4.isEmpty())
        f->Add_Label(label4);

    f->Show();
}

void Principal::Update_Fen_Info(QString label, QString info)
{
    InfoWindow *f = new InfoWindow(this,"",1);
    f->Update_Label(label,info);
}

void Principal::Update_Fen_Info(QString info)
{
    InfoWindow *f = new InfoWindow(this,"",1);
    if(info.isEmpty())
        f->Close();
    else
        f->Update_Label("Info",info);
}

void Principal::Fournisseur_Actuel(QString nom)
{
    Update_Fen_Info("Fournisseur",nom);
}

//Fenêtre chargement///////////
void Principal::Update_Load_Window(QString text)
{
    InfoWindow *f = new InfoWindow(this,"",1);
    if(f->Get_Label_Text("Update").split("\n").count() != 2)
        return;
    f->Update_Label("Update",text + "\n" + f->Get_Label_Text("Update").split("\n").at(1));
}

void Principal::Destroy_Chargement()
{
    InfoWindow *f = new InfoWindow(this,"",1);
    f->Close();
}

//Fonctions////////////////////
QString Principal::HashMDP(QString mdp)
{
    QByteArray mdpb = QCryptographicHash::hash(mdp.toLatin1(),QCryptographicHash::Sha256);
    return QString(mdpb.toHex());
}

void Principal::AddError(QString error)
{
    QDateTime t;
    ui->eArgErreurs->addItem(t.currentDateTime().toString("dd/MM/yyyy hh:mm ") + error);
}

void Principal::Quitter()
{
    if(!m_Arret)
    {
        m_Arret = true;
        QEventLoop l;
        QTimer t;
        connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
        connect(this,SIGNAL(FinAjout()),&l,SLOT(quit()));
        t.start(20000);
        l.exec();
    }
    qApp->exit(0);
}

void Principal::LoadWeb(int valeur)
{
    InfoWindow *f = new InfoWindow(this,"",0);
    QString text = f->Get_Label_Text("Update");
    if(!text.isEmpty())
    {
        if(text.contains("%"))
            text.remove(text.count()-text.split(" ").last().count()-1,text.count()-1);
        f->Update_Label("Update",text + " " + QString::number(valeur) + "%");
    }

    text.clear();
    text = f->Get_Label_Text("Info");
    if(text.isEmpty())
        return;
    if(text.contains("%"))
        text.remove(text.count()-text.split(" ").last().count()-1,text.count()-1);
    f->Update_Label("Info",text + " " + QString::number(valeur) + "%");
}
