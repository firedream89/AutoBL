#include "principal.h"
#include "ui_principal.h"

/////////////////////////////////
QString version("1.37"); //Version De L'application
QString maj("http://37.187.104.80/");//Serveur MAJ
/////////////////////////////////

/// ///////////MAJ///////////////
///
/// Post_Report :
/// -Ajout du log Debug
///
////////////////////////////////
Principal::Principal(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Principal)
{
    ui->setupUi(this);

    MAJ();

    //Traitement des variables de l'application
    m_Lien_Work = QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0);
    ui->e_Path->setText("Work Path : " + QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0));
    qDebug() << "Lien dossier de travail : " <<m_Lien_Work;
    this->setFixedWidth(1000);
    premierDemarrage = false;

    //Création des Dossiers
    QDir dir;
    dir.mkdir(m_Lien_Work);
    dir.mkdir(m_Lien_Work + "/Config");
    dir.mkdir(m_Lien_Work + "/Logs");

    //Ouverture des fichiers logs et errors
    m_Logs.setFileName(m_Lien_Work + "/Logs/logs.txt");
    m_Errors.setFileName(m_Lien_Work + "/Logs/errors.txt");
    qDebug() << "Ouverture Logs " << m_Logs.open(QIODevice::WriteOnly | QIODevice::Append);
    m_Errors.open(QIODevice::WriteOnly | QIODevice::Append);

    //Chargement des classes de l'application et fonctions nécéssaires
    Affichage_Info("-----------------------------AutoBL " + version + "------------------------------------");
    Affichage_Erreurs("-----------------------------AutoBL " + version + "------------------------------------");
    m_Erreurs = 0;
    ui->e_Erreurs->setText("");
    ui->e_Erreur2->setText(QString::number(m_Erreurs));
    connect(&m_DB,SIGNAL(Info(QString)),this,SLOT(Affichage_Info(QString)));
    connect(&m_DB,SIGNAL(Error(QString)),this,SLOT(Affichage_Erreurs(QString)));
    connect(&m_DB,SIGNAL(Error(QString)),this,SLOT(AddError(QString)));
    m_DB.Init();
    m_Tache = new Tache(version);
    m_Tache->Affichage_Info("AutoBL V" + version);
    m_Rexel = new Rexel(m_Lien_Work);
    m_Tri = 0;
    Afficher_Fichiers_Excel();
    mdp = new QLineEdit;
    mdp->setEchoMode(QLineEdit::Password);
    Login_False();
    //Premier démarrage
    if(!QFile::exists(m_Lien_Work + "/Config/Config.esab"))
    {
        QFile::copy("Config.esab",m_Lien_Work + "/Config/Config.esab");
        qDebug() << "Premier démarrage initialisé";
        premierDemarrage = true;
    }

    //Chargement des paramètres
    ///Démarrage automatique
    QSqlQuery req = m_DB.Requete("SELECT * FROM Options WHERE Nom='Auto'");
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
    req = m_DB.Requete("SELECT * FROM Options WHERE Nom='Heure'");
    req.next();
    QSqlQuery req2 = m_DB.Requete("SELECT * FROM Options WHERE Nom='Minutes'");
    req2.next();
    ui->Heure->setTime(QTime(req.value("Valeur").toInt(),req2.value("Valeur").toInt()));
    ///Variable login Esabora
    req = m_DB.Requete("SELECT * FROM Options WHERE Nom='Login'");
    req.next();
    ui->eLogin->setText(req.value("Valeur").toString());
    ///Variable MDP Esabora
    req = m_DB.Requete("SELECT * FROM Options WHERE Nom='MDP'");
    req.next();
    ui->eMDP->setText(req.value("Valeur").toString());
    ///Variable Emplacement Esabora
    req = m_DB.Requete("SELECT * FROM Options WHERE Nom='LienEsabora'");
    req.next();
    ui->lienEsabora->setText(req.value("Valeur").toString());
    ///Variable Mail Rexel
    req = m_DB.Requete("SELECT * FROM Options WHERE Nom='NUR'");
    req.next();
    ui->eNUR->setText(req.value("Valeur").toString());
    ///Variable MDP Application
    req = m_DB.Requete("SELECT * FROM Options WHERE Nom='MDPA'");
    req.next();
    ui->mDPA->setText(req.value("Valeur").toString());
    ///Variable MDP Rexel
    req = m_DB.Requete("SELECT * FROM Options WHERE Nom='MDPR'");
    req.next();
    ui->eMDPR->setText(req.value("Valeur").toString());
    ///Variable temps entre chaque commandes Esabora
    req = m_DB.Requete("SELECT * FROM Options WHERE ID='14'");
    req.next();
    ui->tmpCmd->setValue(req.value("Valeur").toDouble());
    ///Variable nom BDD Esabora
    req = m_DB.Requete("SELECT * FROM Options WHERE ID='12'");
    req.next();
    ui->nBDDEsab->setText(req.value("Valeur").toString());
    ///Variable Nom D'utilisateur Rexel
    req = m_DB.Requete("SELECT * FROM Options WHERE ID='15'");
    req.next();
    ui->eNUR2->setText(req.value("Valeur").toString());
    ///Variable Temps boucle Navigation Rexel
    req = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='19'");
    req.next();
    ui->tmpBoucleRexel->setValue(req.value("Valeur").toDouble());
    ///Variable Purge auto BDD
    req = m_DB.Requete("SELECT * FROM Options WHERE ID='17'");
    req.next();
    if(req.value("Valeur").toInt() == 1)
        ui->autoPurgeDB->setChecked(true);
    else
        ui->autoPurgeDB->setChecked(false);
    ///Variable Ajout Auto des BL
    req = m_DB.Requete("SELECT * FROM Options WHERE ID='13'");
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
    req = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='20'");
    req.next();
    ui->totalBCCrees->setText(req.value("Valeur").toString());
    ///Semi Automatique
    req = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='22'");
    req.next();
    if(req.value("Valeur").toInt() == 1 && 0)
        ui->semiAuto->setChecked(true);
    else
        ui->semiAuto->setChecked(false);
    ///Nom Entreprise Esabora
    req = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='23'");
    req.next();
    ui->nEntrepriseEsab->setText(req.value("Valeur").toString());

    m_Arret = true;
    login = false,
    Chargement_Parametres();
    ui->tNomFichier->setContextMenuPolicy(Qt::CustomContextMenu);

    //Préparation Esabora
    m_Esabora = new Esabora(this,ui->eLogin->text(),ui->eMDP->text(),ui->lienEsabora->text(),m_Lien_Work);
    QThread *thread = new QThread;
    ///Déplacement des classes dans un thread séparé
    qDebug() << m_Esabora->thread() << m_Rexel->thread();
    m_Esabora->moveToThread(thread);
    //m_Rexel->moveToThread(thread);
    qDebug() << m_Esabora->thread() << m_Rexel->thread();

    //Création des connect
    connect(&m_DB,SIGNAL(CreateTable()),this,SLOT(PurgeError()));

    connect(m_Tache,SIGNAL(temps_Restant()),this,SLOT(Affichage_Temps_Restant()));
    connect(m_Tache,SIGNAL(Ouvrir()),this,SLOT(show()));
    connect(m_Tache,SIGNAL(ajout_BC()),this,SLOT(Demarrage()));
    connect(m_Tache,SIGNAL(Arret()),this,SLOT(Arret()));
    connect(m_Tache,SIGNAL(Ouvrir()),this,SLOT(Affichage_Temps_Restant()));
    connect(m_Tache,SIGNAL(Quitter()),this,SLOT(Quitter()));

    connect(m_Rexel,SIGNAL(Info(QString)),this,SLOT(Affichage_Info(QString)));
    connect(m_Rexel,SIGNAL(Erreur(QString)),this,SLOT(Affichage_Erreurs(QString)));
    connect(m_Rexel,SIGNAL(Erreur(QString)),this,SLOT(AddError(QString)));
    connect(m_Rexel,SIGNAL(LoadProgress(int)),this,SLOT(LoadWeb(int)));
    connect(m_Rexel,SIGNAL(InfoFen(QString,QString)),this,SLOT(ModifInfoTraitementBL(QString,QString)));

    connect(m_Esabora,SIGNAL(Info(QString)),this,SLOT(Affichage_Info(QString)));
    connect(m_Esabora,SIGNAL(Erreur(QString)),this,SLOT(Affichage_Erreurs(QString)));
    connect(m_Esabora,SIGNAL(Erreur(QString)),this,SLOT(AddError(QString)));
    connect(m_Esabora,SIGNAL(Message(QString,QString,bool)),this,SLOT(Afficher_Message_Box(QString,QString,bool)));

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

    qApp->setQuitOnLastWindowClosed(false);

    this->resize(ui->tNomFichier->width(),this->height());

    Affichage_Info(m_Lien_Work);

    if(premierDemarrage)//Init Premier démarrage
    {
        this->show();
        ui->tabWidget->setCurrentIndex(2);
        ui->eLogin->setText("1");
        ui->eMDP->setEchoMode(QLineEdit::Normal);
        ui->eMDP->setText("2");
        ui->lienEsabora->setText("<-3");
        ui->eNUR->setText("4");
        ui->eNUR2->setText("6");
        ui->eMDPR->setEchoMode(QLineEdit::Normal);
        ui->eMDPR->setText("5");
    }

    req = m_DB.Requete("SELECT * FROM Options WHERE ID='16'");
    req.next();
    if(req.value("Valeur").toInt() == 0)
    {
        Help(true);
        m_DB.Requete("UPDATE Options SET Valeur='1' WHERE ID='16'");
    }

    //Reset MDP API
    if(qApp->arguments().contains("-RSTMDP"))
    {
        m_DB.Requete("UPDATE Options SET Valeur='' WHERE ID='11'");
        QMessageBox::information(this,"","Le mot de passe de l'application à été réinitialisé !");
    }

    qDebug() << test();
}

Principal::~Principal()
{ 
    delete ui;
}

bool Principal::test()
{

}

void Principal::Sauvegarde_Parametres()
{
    if(ui->gAjoutAuto->isChecked())
        m_DB.Requete("UPDATE Options SET Valeur='1' WHERE Nom='Auto'");
    else
        m_DB.Requete("UPDATE Options SET Valeur='0' WHERE Nom='Auto'");

    m_DB.Requete("UPDATE Options SET Valeur='" + ui->Heure->time().toString("HH") + "' WHERE Nom='Heure'");
    m_DB.Requete("UPDATE Options SET Valeur='" + ui->Heure->time().toString("mm") + "' WHERE Nom='Minutes'");
    m_DB.Requete("UPDATE Options SET Valeur='" + ui->eLogin->text() + "' WHERE Nom='Login'");
    m_DB.Requete("UPDATE Options SET Valeur='" + ui->eMDP->text() + "' WHERE Nom='MDP'");
    m_DB.Requete("UPDATE Options SET Valeur='" + ui->lienEsabora->text() + "' WHERE Nom='LienEsabora'");
    m_DB.Requete("UPDATE Options SET Valeur='" + ui->eNUR->text() + "' WHERE Nom='NUR'");
    m_DB.Requete("UPDATE Options SET Valeur='" + ui->eMDPR->text() + "' WHERE Nom='MDPR'");
    m_DB.Requete("UPDATE Options SET Valeur='" + ui->mDPA->text() + "' WHERE Nom='MDPA'");
    m_DB.Requete("UPDATE Options SET Valeur='" + ui->nBDDEsab->text() + "' WHERE ID='12'");
    m_DB.Requete("UPDATE Options SET Valeur='" + QString::number(ui->tmpCmd->value()) + "' WHERE ID='14'");
    m_DB.Requete("UPDATE Options SET Valeur='" + ui->eNUR2->text() + "' WHERE ID='15'");
    m_DB.Requete("UPDATE Options SET Valeur='" + QString::number(ui->tmpBoucleRexel->value()) + "' WHERE ID='19'");
    if(ui->autoPurgeDB->isChecked())
        m_DB.Requete("UPDATE Options SET Valeur='1' WHERE ID='17'");
    else
        m_DB.Requete("UPDATE Options SET Valeur='0' WHERE ID='17'");
    if(ui->ajoutAutoBL->isChecked())
        m_DB.Requete("UPDATE Options SET Valeur='1' WHERE ID='13'");
    else
        m_DB.Requete("UPDATE Options SET Valeur='0' WHERE ID='13'");
    if(ui->semiAuto->isChecked())
        m_DB.Requete("UPDATE Options SET Valeur='1' WHERE ID='22'");
    else
        m_DB.Requete("UPDATE Options SET Valeur='0' WHERE ID='22'");
    m_DB.Requete("UPDATE Options SET Valeur='" + ui->nEntrepriseEsab->text() + "' WHERE ID='23'");

    QSettings settings2("Microsoft","Windows\\CurrentVersion\\Run");
    if(ui->cDWin->isChecked())
    {
        if(settings2.value("AutoBL").toString() == "")
            settings2.setValue("AutoBL",qApp->applicationFilePath().replace("/","\\"));
    }
    else
        settings2.remove("AutoBL");

    Affichage_Info("Sauvegarde des paramètres terminés",true);

    InfoTraitementBL();
    bool connexionRexel(false);
    if(!m_Rexel->Connexion(ui->eNUR->text(),ui->eMDPR->text()))
        Afficher_Message_Box("Erreur","Echec de connexion au site Rexel.fr, veuillez vérifier vos identifiants saisie",true);
    else
    {
        ModifInfoTraitementBL("Info","Test de connexion...");
        Afficher_Message_Box("","Connexion au site de Rexel.fr réussis");
        connexionRexel = true;
    }
    if(premierDemarrage && connexionRexel)
    {
        ModifInfoTraitementBL("connexion","Réussis");
        ModifInfoTraitementBL("Info","Préparation de la base de données...");
        if(!m_Rexel->Navigation())
        {
            Afficher_Message_Box("","Echec de chargement de la page",true);
            Sauvegarde_Parametres();
            return;
        }
        Afficher_Message_Box("","Le dernier bon de commande est sauvegardé et considéré comme ajouté à Esabora(il permettra d'éviter les doublons de bon de commande)");

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
        ModifInfoTraitementBL("BL","Réussis");
        Sauvegarde_Parametres();
        Afficher_Fichiers_Excel();
    }
    else
        ModifInfoTraitementBL("BL","Réussis");

    Chargement_Parametres();
}

void Principal::Chargement_Parametres()
{
    QString aAuto("false"),PC("false");
    if(ui->gAjoutAuto->isChecked())
        aAuto = "true";
    Affichage_Info("Auto=" + aAuto + ",PConfig=" + PC);
    if(ui->gAjoutAuto->isChecked())
        Demarrage_Auto_BC(true);
    else
        m_Temps.stop();

    Etat_Ajout_Auto();
}

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

void Principal::Emplacement_Esabora()
{
    ui->lienEsabora->setText(QFileDialog::getOpenFileName(this,"Emplacement Esabora",ui->lienEsabora->text(),"*.ink",0,QFileDialog::DontResolveSymlinks));

    QSqlQuery req = m_DB.Requete("SELECT Valeur FROM Options WHERE Nom='LienEsabora'");
    req.next();
    if(req.value("Valeur").toString() != ui->lienEsabora->text())
    {
        m_Esabora->Set_Var_Esabora(ui->lienEsabora->text(),ui->eLogin->text(),ui->eMDP->text());
        if((ui->nBDDEsab->text() == "" || ui->nEntrepriseEsab->text() == "") && ui->eLogin->text() != "" && ui->eMDP->text() != "")
            if(QMessageBox::question(this,"","Souhaitez vous qu'AutoBL tente de remplir les champs Répertoire et nom d'entreprise automatiquement ?") ==
                    QMessageBox::Yes)
            {
                QString entreprise,BDD;
                m_Esabora->Apprentissage(entreprise,BDD);
                ui->nBDDEsab->setText(BDD);
                ui->nEntrepriseEsab->setText(entreprise);
            }
    }
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
             m_DB.Purge();
    }
    else if(!Demarrage_Timer)
    {
        Affichage_Info("DEBUG | Demarrage Auto | Boucle");
        //boucle de 1mn pour attente avant redemarrage
        QTimer t;
        QEventLoop l;
        connect(&t,SIGNAL(timeout()),&l,SLOT(quit()));
        t.start(60000);
        l.exec();
        Affichage_Info("DEBUG | Demarrage Auto | Fin Boucle");
        Demarrage_Auto_BC(true);
    }
}

void Principal::Demarrage()
{
    m_DB.Sav();
    Affichage_Info("Recherche de nouveau BL...",true);
    InfoTraitementBL();
    m_Temps.stop();
    m_Arret = false;///Début d'ajout BC
    m_Rexel->ResetWeb();

    ///Verif Affichage popup TV
    HWND hWnds = FindWindow(NULL,L"Sessions sponsorisées");
    if(hWnds != NULL)
    {
        SetForegroundWindow(hWnds);
        keybd_event(VK_EXECUTE,0,0,0);
    }

    ///Récuperation des BC sur rexel.fr
    m_Tache->Affichage_En_Cours();
    qDebug() << "Demarrage 0";
    if(ui->eNUR2->text() == "")
    {
        QMessageBox::information(this,"Erreur","Le nom d'utilisateur Rexel est nécéssaire pour le fonctionnement de l'application !");
        return;
    }
    if(ui->activ_Rexel->isChecked() && !m_Arret)
    {
        if(m_Rexel->Connexion(ui->eNUR->text(),ui->eMDPR->text()))
        {
            ModifInfoTraitementBL("connexion","Réussi");
            m_Tache->Affichage_En_Cours();
            if(m_Rexel->Navigation() && !m_Arret)
            {
                qDebug() << "Demarrage 0.1";
                QSqlQuery req = m_DB.Requete("SELECT * FROM En_Cours WHERE Ajout=''");
                QEventLoop loop;
                QTimer timer;
                connect(&timer,SIGNAL(timeout()),&loop,SLOT(quit()));
                connect(m_Rexel,SIGNAL(TelechargementTerminer()),&loop,SLOT(quit()));
                while(req.next() && !m_Arret)
                {
                    m_Tache->Affichage_En_Cours();
                    if(m_Rexel->Exportation(req.value("Numero_Commande").toString()))
                    {
                        timer.start(10000);
                        //loop.exec();
                        if(timer.isActive())
                        {
                            timer.stop();
                            if(req.value("Nom_Chantier").toString().at(0).isDigit() && req.value("Nom_Chantier").toString().at(req.value("Nom_Chantier").toString().count()-1).isDigit())
                                m_DB.Requete("UPDATE En_Cours SET Ajout='Telecharger' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
                            else
                                m_DB.Requete("UPDATE En_Cours SET Ajout='Erreur' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
                        }
                        else
                        {
                            timer.stop();
                            Affichage_Erreurs(tr("Téléchargement de %0 échoué(temps écoulé)").arg(req.value("Numero_Commande").toString()));
                        }
                    }
                }
                m_Tache->Affichage_En_Cours();qDebug() << "Demarrage 1";

                QSqlQuery req2 = m_DB.Requete("SELECT * FROM En_Cours WHERE Etat='En préparation' OR Etat='Partiellement livrée'");
                while(req2.next() && !m_Arret)
                {
                    m_Rexel->VerificationEtatBC(req2.value("Numero_Commande").toString());
                }
                req2 = m_DB.Requete("SELECT * FROM En_Cours WHERE Etat='Livrée en totalité'");
                while(req2.next() && !m_Arret)
                {
                    if(req2.value("Numero_Livraison").toString().isEmpty())
                    {
                        m_Rexel->Recuperation_BL(req2.value("Numero_Commande").toString());
                    }
                }
                ModifInfoTraitementBL("navigation","Réussi");
            }
            else
                ModifInfoTraitementBL("navigation","Echouée");
        }
        else
            ModifInfoTraitementBL("connexion","Echouée");
    }
qDebug() << "Demarrage 2";
    int nbBC(0),nbBL(0);
    ///Démarrage d'esabora
    if(ui->activ_Esab->isChecked() && !m_Arret)
    {
        if(m_Esabora->Lancement_API() && !m_Arret)
        {
            ///Ajout des BC sur esabora
            QSqlQuery req = m_DB.Requete("SELECT * FROM En_Cours WHERE Ajout='Telecharger' OR Ajout='Modifier'");
            m_Esabora->Ouverture_Liste_BC();
            while(req.next() && !m_Arret)
            {
                m_Tache->Affichage_En_Cours();
                Get_Tableau_Matos(req.value("Numero_Commande").toString());
                if(req.value("Nom_Chantier").toString() == "0")//Ajout BC au Stock
                {
                    if(!m_Esabora->Ajout_Stock(req.value("Numero_Commande").toString()))
                    {
                        Affichage_Erreurs(tr("Ajout du bon de commande %0 échoué, BC non créé !").arg(req.value("Numero_Commande").toString()));
                    }
                    else
                    {
                        m_DB.Requete("UPDATE En_Cours SET Ajout='Bon Ajouté' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
                        nbBC++;
                    }
                }
                else//Ajout BC sur numéro de chantier
                {
                    if(!m_Esabora->Ajout_BC(req.value("Numero_Commande").toString()))
                    {
                        if(m_Esabora->GetEtat() == 0)
                        {
                            Affichage_Erreurs(tr("Ajout du bon de commande %0 échoué, BC non créé !").arg(req.value("Numero_Commande").toString()));
                        }
                        if(m_Esabora->GetEtat() == 1)
                        {
                            Affichage_Erreurs(tr("Ajout du bon de commande %0 échoué, BC Vide à supprimer !").arg(req.value("Numero_Commande").toString()));
                            m_DB.Requete("UPDATE En_Cours SET Ajout='Erreur' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
                        }
                    }
                    else
                    {
                        m_DB.Requete("UPDATE En_Cours SET Ajout='Bon Ajouté' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
                        nbBC++;
                    }
                }
            }
            m_Esabora->Abort();
            ModifInfoTraitementBL("BC","Terminer");
            ///Ajout des BL sur esabora
            Afficher_Fichiers_Excel();///Refresh liste pour validation des bons ajouté sur ce cycle
            for(int cpt=0;cpt<ui->tNomFichier->rowCount();cpt++)
            {
                if(m_Arret)
                    cpt = ui->tNomFichier->rowCount();
                if(ui->tNomFichier->item(cpt,7)->checkState() == Qt::Checked)
                {
                    QSqlQuery req = m_DB.Requete("SELECT * FROM En_Cours WHERE Numero_Commande='" + ui->tNomFichier->item(cpt,5)->text() + "'");
                    if(req.next())
                    {
                        if(!m_Esabora->Ajout_BL(req.value("Numero_BC_Esabora").toString(),ui->tNomFichier->item(cpt,4)->text()))
                        {
                            Affichage_Erreurs(tr("Principal | Ajout BL N°%0 échoué").arg(ui->tNomFichier->item(cpt,4)->text()));
                        }
                        else
                        {
                            nbBL++;
                            Affichage_Info("Principal | Ajout BL N°" + ui->tNomFichier->item(cpt,4)->text() + " Réussi");
                            m_DB.Requete("UPDATE En_Cours SET Ajout='Ok' WHERE Numero_Commande='" + ui->tNomFichier->item(cpt,5)->text() + "'");
                        }
                    }
                    else
                    {
                        Affichage_Erreurs("Principal | BL non trouvé dans la DB !");
                    }
                }
            }
            ModifInfoTraitementBL("BL","Terminer");
            //Fermeture d'esabora
            if(!m_Esabora->Fermeture_API())
                Affichage_Erreurs("Fermeture d'Esabora échoué");
        }
        else
        {
            Affichage_Erreurs("Démarrage d'Esabora échoué");
            ModifInfoTraitementBL("BC","Echouée");
            ModifInfoTraitementBL("BL","Echouée");

            ///Verif modification DB
            QString entreprise,BDD;
            m_Esabora->Apprentissage(entreprise,BDD);
            if(ui->nBDDEsab->text() != BDD || ui->nEntrepriseEsab->text() != entreprise)
            {
                ui->nBDDEsab->setText(BDD);
                ui->nEntrepriseEsab->setText(entreprise);
                Sauvegarde_Parametres();
            }
        }
    }

    Afficher_Fichiers_Excel();
    QSqlQuery req = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='20'");
    req.next();
    m_DB.Requete("UPDATE Options SET Valeur='" + QString::number(req.value("Valeur").toInt()+nbBC) + "' WHERE ID='20'");
    ui->totalBCCrees->setText(QString::number(ui->totalBCCrees->text().toInt()+nbBC));
    ui->lastCycleBCCrees->setText(QString::number(nbBC));
    ui->lastCycleBCValides->setText(QString::number(nbBL));

    m_Tache->Affichage_Info("Ajout de bon terminé");
    m_Arret = true;///Fin d'ajout BC
    emit FinAjout();///envoie signal en cas d'arret de l'API

    if(ui->autoPurgeDB->isChecked())
        m_DB.Purge();
    if(ui->e_Erreur2->text().toInt() > 0)
        Post_Report();
    Demarrage_Auto_BC();
}

void Principal::Test_Esabora()
{
    return;
    if(m_Esabora->Lancement_API())
    {
        QSqlQuery req = m_DB.Requete("SELECT * FROM En_Cours WHERE Ajout='Telecharger' OR Ajout='Modifier'");
        while(req.next())
        {
            if(!m_Esabora->Ajout_BC(req.value("Numero_Commande").toString()))
            {
                Affichage_Erreurs(tr("Ajout du bon de commande %0 échoué").arg(req.value("Numero_Commande").toString()));
                m_DB.Requete("UPDATE En_Cours SET Ajout='Erreur' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");

                if(req.value("Ajout").toString() != "Modifier")
                {
                    QFile file(m_Lien_Work + "/Pj/" + req.value("Numero_Commande").toString() + ".xlsx");
                    if(QFile::copy(m_Lien_Work + "/Pj/" + file.fileName(),m_Lien_Work + "/Pj/" + file.fileName()))
                        file.remove();
                }
            }
            else
            {
                m_DB.Requete("UPDATE En_Cours SET Ajout='Ok' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");

                if(req.value("Ajout") != "Modifier")
                {
                    QFile file(m_Lien_Work + "/Pj/" + req.value("Numero_Commande").toString() + ".xlsx");
                    if(QFile::copy(m_Lien_Work + "/Pj/" + file.fileName(),m_Lien_Work + "/Pj/Terminer/" + file.fileName()))
                        file.remove();
                }
            }
        }
        if(!m_Esabora->Fermeture_API())
            Affichage_Erreurs("Fermeture d'Esabora échoué");
    }
    else
        Affichage_Erreurs("Démarrage d'Esabora échoué");
}

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

void Principal::Test_Rexel()
{
    m_Rexel->Affichage();
}

void Principal::Afficher_Message_Box(QString header,QString texte, bool warning)
{

    if(!warning)
        QMessageBox::information(this,header,texte);
    else
        QMessageBox::warning(this,header,texte);

}

void Principal::Afficher_Fichiers_Excel(int l,int c,int tri)
{
    ui->tNomFichier->blockSignals(true);
    QFlag flag = ~Qt::ItemIsEditable;
    QSqlQuery req;
    if(tri == 10)
        tri = m_Tri;
    if(tri == 0)///Tri par Date
        req = m_DB.Requete("SELECT * FROM En_Cours ORDER BY Date");
    else if(tri == 1)///Tri par Etat Esabora
        req = m_DB.Requete("SELECT * FROM En_Cours ORDER BY Ajout");
    else if(tri == 2)///Tri par Référence
        req = m_DB.Requete("SELECT * FROM En_Cours ORDER BY Nom_Chantier");

    if(!req.exec())
        Affichage_Erreurs("Requete affichage des fichiers échoué !",true);

    while(ui->tNomFichier->rowCount() > 0)
        ui->tNomFichier->removeRow(0);

    //Affichage de la colonne numéro BC esabora si logué
    if(login) ui->tNomFichier->showColumn(8);
    else ui->tNomFichier->hideColumn(8);

    QColor setColor;
    setColor.setNamedColor("#9bffff");
    while(req.next())
    { 
        bool t(false);
        if(req.value("Ajout").toString() != "Ok")
            t = true;
        if(login)
            t = true;
        if(t)
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
            if(ui->tNomFichier->item(0,4)->text() != "" && ui->tNomFichier->item(0,1)->text() == "Bon Ajouté")
            {
                if(req.value("Ajout_BL").toInt() == 0)
                    ui->tNomFichier->item(0,7)->setCheckState(Qt::Unchecked);
                else
                    ui->tNomFichier->item(0,7)->setCheckState(Qt::Checked);

                //Force Check BL
                QSqlQuery bl = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='13'");
                bl.next();
                if(bl.value("Valeur").toInt() == 1)
                {
                    ui->tNomFichier->item(0,7)->setCheckState(Qt::Checked);
                    ui->tNomFichier->hideColumn(7);
                }
            }

            //Ajout couleur une ligne sur 2
            ui->tNomFichier->setAlternatingRowColors(true);
        }
    }

    for(int cpt = 0;cpt < ui->tNomFichier->rowCount();cpt++)
    {
        ui->tNomFichier->item(cpt,0)->setFlags(flag);
        ui->tNomFichier->item(cpt,1)->setFlags(flag);
        if(ui->tNomFichier->item(cpt,6)->text() == "Livrée en totalité")
            ui->tNomFichier->item(cpt,6)->setTextColor(QColor(0,255,0));
        else
            ui->tNomFichier->item(cpt,6)->setTextColor(QColor(255,0,0));
        if(ui->tNomFichier->item(cpt,1)->text() == "Erreur")
            ui->tNomFichier->item(cpt,1)->setTextColor(QColor(255,0,0));
        else if(ui->tNomFichier->item(cpt,1)->text() == "Telecharger" || ui->tNomFichier->item(cpt,1)->text() == "Modifier")
            ui->tNomFichier->item(cpt,1)->setTextColor(QColor(200,200,0));
        else
            ui->tNomFichier->item(cpt,1)->setTextColor(QColor(0,255,0));
        ui->tNomFichier->item(cpt,1)->setFlags(flag);
        if(ui->tNomFichier->item(cpt,1)->text() != "Erreur" && ui->tNomFichier->item(cpt,1)->text() != "Modifier" && !login &&
                ui->tNomFichier->item(cpt,1)->text() != ""  && ui->tNomFichier->item(cpt,1)->text() != "Telecharger")
            ui->tNomFichier->item(cpt,2)->setFlags(flag);
        ui->tNomFichier->item(cpt,4)->setFlags(flag);
        ui->tNomFichier->item(cpt,5)->setFlags(flag);
        ui->tNomFichier->item(cpt,6)->setFlags(flag);
    }

    if(l > 0)//retour à la dernière ligne selectionnée
        ui->tNomFichier->setCurrentCell(l,c);

    ui->tNomFichier->blockSignals(false);
    ui->tNomFichier->resizeColumnsToContents();
    ui->tNomFichier->resizeRowsToContents();
}

void Principal::Purge_Bl()
{
    if(QMessageBox::question(this,"Purge","Voulez vous vraiment purger la base de données ?"))
    {
        m_DB.Requete("DELETE FROM En_Cours");
    }
}

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
        QDialog *d = new QDialog(this);
        QLabel *label = new QLabel("Connection au site de Rexel\nVeuillez patienter...");
        QGridLayout *gl = new QGridLayout;
        gl->addWidget(label);
        d->setLayout(gl);
        d->setWindowFlags(Qt::Tool | Qt::CustomizeWindowHint);
        d->setObjectName("Chargement");
        d->show();
        if(m_Rexel->Connexion(ui->eNUR->text(),ui->eMDPR->text()) && d->isVisible())
        {
            label->setText("Chargement de la page web\nVeuillez patienter...");
            if(m_Rexel->webLoad("https://www.rexel.fr/frx/my-account/orders/" + ui->tNomFichier->item(l,c)->text()) && d->isVisible())
            {
                d->close();
                QStringList list = m_Rexel->AffichageTableau();
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
                        double v = tbl->item(0,3)->text().replace(",",".").toDouble();
                        double v2 = tbl->item(0,4)->text().replace(",",".").toDouble();
                        double tmp = v * v2;
                        tbl->setItem(0,6,new QTableWidgetItem(QString::number(tmp).replace(".",",")));//Prix total
                    }
                }
                tbl->resizeColumnsToContents();
                tbl->resizeRowsToContents();
                tbl->resize(tbl->width(),tbl->height());
                tbl->show();
            }
            else if(d->isVisible())
                QMessageBox::information(this,"Erreur","Impossible d'accèder au bon de commande !");
        }
        else if(d->isVisible())
            QMessageBox::information(this,"Erreur","Le site de rexel est actuellement indisponible !");

        d->close();
        delete label;
        delete gl;
        delete d;
    }
    else if(c == 7)//MAJ BDD Ajout Manu BL
    {
        if(ui->tNomFichier->item(l,c)->checkState() == Qt::Checked)
        {
            m_DB.Requete("UPDATE En_Cours SET Ajout_BL='1' WHERE Numero_Commande='" + ui->tNomFichier->item(l,5)->text() + "'");
        }
        else
        {
            m_DB.Requete("UPDATE En_Cours SET Ajout_BL='0' WHERE Numero_Commande='" + ui->tNomFichier->item(l,5)->text() + "'");
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
                m_DB.Requete("UPDATE En_Cours SET Nom_Chantier='" + ui->tNomFichier->item(l,2)->text() + "', Ajout='Modifier' WHERE Numero_Commande='" + ui->tNomFichier->item(l,5)->text() + "'");
            else
                QMessageBox::information(this,"",tr("La référence de chantier doit être un nombre !"));
        }
        Afficher_Fichiers_Excel(l,c);
    }
    else if(c == 8)//Modif numero esabora
    {
        if(QMessageBox::question(this,"",tr("Voulez vous vraiment modifier le numéro esabora ?")) == 16384)
        {
            m_DB.Requete("UPDATE En_Cours SET Numero_BC_Esabora='" + ui->tNomFichier->item(l,8)->text() + "' WHERE Numero_Commande='" + ui->tNomFichier->item(l,5)->text() + "'");
        }
        Afficher_Fichiers_Excel();
    }

}

void Principal::Arret()
{
    qDebug() << "Arret demandé !";
    m_Arret = true;
    m_Tache->Affichage_Info("Procédure en cours d'arret, veuiller patientez...");
}

void Principal::Etat_Ajout_Auto()
{
    if(ui->gAjoutAuto->isChecked())
        ui->Heure->setEnabled(true);
    else
        ui->Heure->setEnabled(false);
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
    QSqlQuery req = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='11'");
    if(req.next())
    {
        if(req.value("Valeur").toString() != "")
        {
            ui->tabWidget->removeTab(3);
            ui->tabWidget->removeTab(2);
            login = false;
            Afficher_Fichiers_Excel();
            m_Tache->Login(false);
        }
        else if(ui->tabWidget->isTabEnabled(3))
            ui->tabWidget->removeTab(3);
    }
    ui->tabWidget->setCurrentIndex(0);
}

void Principal::Login_True()
{
    QSqlQuery req = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='11'");
    req.next();
    if(req.value("Valeur").toString() != mdp->text().split(" ").at(0))
        QMessageBox::information(this,"Erreur","Mot de passe faux !");
    else
    {
        ui->tabWidget->addTab(ui->Configuration,"Configuration");
        if(mdp->text().split(" ").count() == 2)
            if(mdp->text().split(" ").at(1) == "dbg")
                ui->tabWidget->addTab(ui->Debug,"Débuggage");
        login = true;
        Afficher_Fichiers_Excel();
        m_Tache->Login(true);
    }
    ui->tabWidget->setCurrentIndex(0);
}

void Principal::Test_BC()
{
    if(m_Esabora->Lancement_API())
    {
        QSqlQuery req = m_DB.Requete("SELECT * FROM En_Cours WHERE Ajout='Telecharger' OR Ajout='Modifier'");
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
                    m_DB.Requete("UPDATE En_Cours SET Ajout='Erreur' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");
                }
            }
            else
            {
                m_DB.Requete("UPDATE En_Cours SET Ajout='Bon Ajouté' WHERE Numero_Commande='" + req.value("Numero_Commande").toString() + "'");

                if(req.value("Ajout") != "Modifier")
                {
                    QFile file(m_Lien_Work + "/Pj/" + fxlsx);
                    if(QFile::copy(m_Lien_Work + "/Pj/" + fxlsx,m_Lien_Work + "/Pj/Terminer/" + fxlsx))
                        file.remove();
                }
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
                QSqlQuery req = m_DB.Requete("SELECT * FROM En_Cours WHERE Numero_Commande='" + ui->tNomFichier->item(cpt,5)->text() + "'");
                if(req.next())
                {
                    if(!m_Esabora->Ajout_BL(req.value("Numero_BC_Esabora").toString(),ui->tNomFichier->item(cpt,4)->text()))
                    {
                        Affichage_Erreurs(tr("Principal | Ajout BL N°%0 échoué").arg(ui->tNomFichier->item(cpt,4)->text()));
                    }
                    else
                    {
                        Affichage_Info("Principal | Ajout BL N°" + ui->tNomFichier->item(cpt,3)->text() + " Réussi");
                        m_DB.Requete("UPDATE En_Cours SET Ajout='Ok' WHERE Numero_Commande='" + ui->tNomFichier->item(cpt,4)->text() + "'");
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

void Principal::MAJ()
{
    QTimer *timer = new QTimer;
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl(maj + "/MAJ/AutoBL/MAJ.txt"))); // Url vers le fichier version.txt
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
    QStringList MAJDispo;
    for(int cpt=0;cpt<retour.count();cpt++)
        if(retour.at(cpt).toDouble() > version.toDouble())
            MAJDispo.append("ver=" + retour.at(cpt));
    QSqlQuery req = m_DB.Requete("SELECT Valeur FROM Options WHERE ID='21'");
    req.next();
    if(MAJDispo.count() == 1 && MAJDispo.at(0) != req.value("Valeur").toString())
    {
        m_DB.Requete("UPDATE Options SET Valeur='" + MAJDispo.at(0) + "' WHERE ID='21'");
        MAJDispo.append("API=" + QCoreApplication::applicationDirPath() + "/" + "AutoBL.exe");
        MAJDispo.append("ftp=" + maj + "/MAJ/AutoBL/");
        MAJDispo.append("RA=oui");
        QProcess MAJ;
        MAJ.start(QCoreApplication::applicationDirPath() + "/" + "MAJ.exe",MAJDispo);
        exit(0);
    }
}

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
            flux << "---------------- Rapport AutoBL " << QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm") << " ----------------\r\n";
            flux << "Commentaire : " << this->findChildren<QDialog *>().at(0)->findChildren<QTextEdit *>().at(0)->toPlainText();
            flux << "- Logs -\r\n" << f.readAll() << "\r\n";
            f.close();
            f.setFileName(m_Lien_Work + "/Logs/errors.txt");
            f.open(QIODevice::ReadOnly);
            flux << "- Erreurs -\r\n" << f.readAll() << "\r\n";
            f.close();
            f.setFileName(m_Lien_Work + "/Logs/debug.log");
            f.open(QIODevice::ReadOnly);
            flux << "- Debug -\r\n" << f.readAll() << "\r\n";
            f.close();

            this->findChildren<QDialog *>().at(0)->findChildren<QPushButton *>().at(0)->setText("Envoi du rapport...");
            rapport.close();
            rapport.open(QIODevice::ReadOnly);
            QNetworkAccessManager manager;
            QEventLoop loop;
            QNetworkRequest requete(QUrl(maj + "/Rapport/" + rapport.fileName().split("/").last()));
            QNetworkReply *reply = manager.put(requete,rapport.readAll());
            connect(&manager,SIGNAL(finished(QNetworkReply*)),&loop,SLOT(quit()));
            loop.exec();
            if(reply->error() == QNetworkReply::NoError)
                this->findChildren<QDialog *>().at(0)->findChildren<QPushButton *>().at(0)->setText("Rapport envoyé !");
            else
                this->findChildren<QDialog *>().at(0)->findChildren<QPushButton *>().at(0)->setText("Echec envoi !");

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
        flux << "---------------- Rapport AutoBL " << QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm") << " ----------------\r\n";
        flux << "- Logs -\r\n" << f.readAll() << "\r\n";
        f.close();
        f.setFileName(m_Lien_Work + "/Logs/errors.txt");
        f.open(QIODevice::ReadOnly);
        flux << "- Erreurs -\r\n" << f.readAll() << "\r\n";
        f.close();
        f.setFileName(m_Lien_Work + "/Logs/debug.log");
        f.open(QIODevice::ReadOnly);
        flux << "- Debug -\r\n" << f.readAll() << "\r\n";
        f.close();

        rapport.close();
        rapport.open(QIODevice::ReadOnly);
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkRequest requete(QUrl(maj + "/Rapport/" + rapport.fileName().split("/").last()));
        QNetworkReply *r = manager.put(requete,rapport.readAll());
        connect(&manager,SIGNAL(finished(QNetworkReply*)),&loop,SLOT(quit()));
        loop.exec();
        rapport.close();
        rapport.remove();

        if(r->error()!= QNetworkReply::NoError)
            return false;
    }
    m_Logs.resize(0);
    m_Errors.resize(0);
    ui->e_Erreur2->setText("0");
    return true;
}

void Principal::LoadWeb(int valeur)
{
    if(this->findChildren<QDialog *>().count() > 0)
        if(this->findChildren<QDialog *>().at(0)->objectName() == "Chargement")
        {
            QLabel *l = this->findChildren<QDialog *>().at(0)->findChildren<QLabel *>().at(0);
            QString s = l->text();
            if(l->text().contains("%"))
                s.remove(l->text().count()-l->text().split(" ").last().count()-1,l->text().count()-1);

            l->setText(s + " " + QString::number(valeur) + "%");
        }
}

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

void Principal::Get_Tableau_Matos(int l)
{
    QStringList list;
    if(m_Rexel->Connexion(ui->eNUR->text(),ui->eMDPR->text()))
    {
        if(m_Rexel->webLoad("https://www.rexel.fr/frx/my-account/orders/" + ui->tNomFichier->item(l,5)->text()))
            list = m_Rexel->AffichageTableau();

        else
            QMessageBox::information(this,"Erreur","Impossible d'accèder au bon de commande !");
    }
    else
        QMessageBox::information(this,"Erreur","Le site de rexel est actuellement indisponible !");

    m_Esabora->Set_Liste_Matos(list);
}
//Erreur 9xx
void Principal::Get_Tableau_Matos(QString Numero_Commande)
{
    QStringList list;
    if(m_Rexel->Connexion(ui->eNUR->text(),ui->eMDPR->text()))
    {
        if(m_Rexel->webLoad("https://www.rexel.fr/frx/my-account/orders/" + Numero_Commande))
            list = m_Rexel->AffichageTableau();

        else
            Affichage_Erreurs("Principal | 901 | Chargement de la page échouée");
    }
    else
        Affichage_Erreurs("Principal | 902 | Connexion échouée");

    m_Esabora->Set_Liste_Matos(list);
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
    Afficher_Fichiers_Excel(ui->tNomFichier->currentRow(),ui->tNomFichier->currentColumn(),0);
}

void Principal::Tri_TNomFichier_Nom()
{
    Afficher_Fichiers_Excel(ui->tNomFichier->currentRow(),ui->tNomFichier->currentColumn(),1);
}

void Principal::Tri_TNomFichier_Ref()
{
    Afficher_Fichiers_Excel(ui->tNomFichier->currentRow(),ui->tNomFichier->currentColumn(),2);
}

void Principal::Remove_Row_DB()
{
    int l = ui->tNomFichier->currentRow();
    if(QMessageBox::question(this,"","Voulez-vous vraiment placer ce bon en terminé ?") == QMessageBox::Yes)
        m_DB.Requete("UPDATE En_Cours SET Ajout='Ok' WHERE Numero_Commande='" + ui->tNomFichier->item(l,5)->text() + "'");
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

void Principal::AddError(QString error)
{
    QDateTime t;

    if(error.contains("DB | E101")) error = tr("La requète sur la base de données à échouée");
    else if(error.contains("DB | E102")) error = tr("La préparation de la requète sur la base de données à échouée");
    else if(error.contains("DB | E001")) error = tr("Ouverture de la base de données échouée");
    else if(error.contains("DB | E002")) error = tr("Base de données indisponible");
    else if(error.contains("DB | E301")) error = tr("La sauvegarde de la base de données à échouée");
    else if(error.contains("Esabora | E201") || error.contains("Esabora | E301") || error.contains("Esabora | E502")
            || error.contains("Rexel | E103") || error.contains("Rexel | E104")) error = tr("Ouverture du fichier ") + error.split(" ").last() + tr(" échouée");
    else if(error.contains("Esabora | E202") || error.contains("Esabora | E510") || error.contains("Esabora | E511") || error.contains("Esabora | E505")) error = tr("Erreur dans la procédure d'ajout de bon");
    else if(error.contains("Esabora | E203")) error = tr("Démarrage d'Esabora échouée");
    else if(error.contains("Esabora | E401") || error.contains("Esabora | E501")) error = tr("Le fichier ") + error.split(" ").last() + tr(" n'existe pas");
    else if(error.contains("Esabora | E402")) error = tr("Validation du bon ") + error.split(" ").last() + tr(" échouée");
    else if(error.contains("Esabora | E514") || error.contains("Esabora | E512") || error.contains("Esabora | E513")) error = tr("La référence du matériel n'a pas été trouvée (Ligne incomplète)");
    else if(error.contains("Esabora | E510")) error = tr("La référence du matériel n'a pas été trouvée (Ligne incomplète)");
    else if(error.contains("Esabora | E509")) error = tr("La vérification de fenêtre Esabora à échouée");
    else if(error.contains("Esabora | E504")) error = tr("Le déplacement de la souris à échoué");
    else if(error.contains("Rexel | E001") || error.contains("Rexel | E002") || error.contains("Rexel | E101") || error.contains("Rexel | E102")) error = tr("Chargement de la page échouée");
    else if(error.contains("Rexel | E106") || error.contains("Rexel | E107")) error = "Une information n'a pas pu être récupérée";

    ui->eArgErreurs->addItem(t.currentDateTime().toString("dd/MM/yyyy hh:mm ") + error);
}

void Principal::PurgeError()
{
    ui->e_Erreur2->setText("0");
    ui->eArgErreurs->clear();
    ui->aff_Erreur->clear();
    ui->tabWidget->setTabText(1,"Information");
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














