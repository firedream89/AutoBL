#include "fournisseur.h"

Fournisseur::Fournisseur(QString lien_Travail):
    m_Lien_Travail(lien_Travail)
{
    DEBUG << "Fournisseur | Init de la classe";
    m_fct = new FctFournisseur(m_Lien_Travail);
    connect(m_fct,SIGNAL(info(QString)),this,SIGNAL(Info(QString)));
    connect(m_fct,SIGNAL(error(QString)),this,SIGNAL(Erreur(QString)));
    connect(m_fct,SIGNAL(LoadProgress(int)),this,SIGNAL(LoadProgress(int)));
    connect(m_fct,SIGNAL(change_Load_Window(QString)),this,SIGNAL(Change_Load_Window(QString)));
}

Fournisseur::~Fournisseur()
{
    delete m_fct;
}
bool Fournisseur::Add(const QString nom,const QString login,const QString mdp,const QString complement)
{
    ///Ajout d'un fournisseur
    DEBUG << "Add " << nom;
    fournisseurs.append("nom=" + nom + "&login=" + login + "&mdp=" + mdp + "&comp=" + complement);
    return true;
}

bool Fournisseur::Add(const QString nom)
{
    ///Ajout d'un fournisseur
    DEBUG << "Add " << nom;
    QStringList l = Find_Fournisseur_From_DB(nom);
    if(l.isEmpty())
        return false;
    fournisseurs.append("nom=" + nom + "&login=" + l.at(1) + "&mdp=" + l.at(2) + "&comp=" + l.at(0));
    return true;
}

bool Fournisseur::Start()
{
    ///Démarrage du traitement
    for(int f=0;f<fournisseurs.count();f++)
    {
        if(fournisseurs.at(f).split("&").count() == 4)
        {
            ///Init variables fonction
            QString nom = fournisseurs.at(f).split("&").at(0).split("=").at(1);
            QString login = fournisseurs.at(f).split("&").at(1).split("=").at(1);
            QString mdp = fournisseurs.at(f).split("&").at(2).split("=").at(1);
            QString comp = fournisseurs.at(f).split("&").at(3).split("=").at(1);

            emit En_Cours_Fournisseur(nom);

            if(nom == "Rexel.fr")
            {
                RexelFr *frn = new RexelFr(m_fct,login,mdp,m_Lien_Travail,comp);

                frn->Start();
            }
            else
                emit Erreur(tr("Erreur critique interne : le nom de fournisseur n'existe pas(%1)").arg(nom));
        }
        else
            emit Erreur(tr("Erreur Critique interne sur les données d'un fournisseur"));
    }
    return true;
}

QStringList Fournisseur::Get_Invoice_List(const QString& frn,const QString& invoiceNumber)
{
    QStringList f = Find_Fournisseur(frn);
    DEBUG << frn << invoiceNumber;
    if(f.isEmpty())
    {
        emit Erreur(tr("Fournisseur non trouvé !(%0)").arg(frn));
        return QStringList(0);
    }
    ///Init variables fonction
    QString login = f.at(0);
    QString mdp = f.at(1);
    QString comp = f.at(2);

    if(frn == "Rexel.fr")
    {
        RexelFr *frn = new RexelFr(m_fct,login,mdp,m_Lien_Travail,comp);
        return frn->Get_Invoice(invoiceNumber);
    }
}

bool Fournisseur::Update_Var(const QString& frn,const QString& login,const QString& mdp,const QString& complement)
{
    DEBUG << "UPDATE " << frn;
    for(int cpt = 0;cpt<fournisseurs.count();cpt++)
        if(fournisseurs.at(cpt).contains(frn))
        {
            fournisseurs.replaceInStrings(fournisseurs.at(cpt),"nom=" + frn + "&login=" + login + "&mdp=" + mdp + "&comp=" + complement);
            return true;
        }
    return false;
}

bool Fournisseur::Del(const QString nom)
{
    DEBUG << "DEL " << nom;
    for(int cpt = 0;cpt<fournisseurs.count();cpt++)
        if(fournisseurs.at(cpt).contains(nom))
        {
            fournisseurs.removeAt(cpt);
            return true;
        }
    return false;
}

bool Fournisseur::Test_Connexion(const QString &nom)
{
    DEBUG << "TEST " << nom;

    QStringList f = Find_Fournisseur(nom);
    if(f.isEmpty())
    {
        emit Erreur(tr("Fournisseur non trouvé !(%0)").arg(nom));
        return false;
    }
    if(nom == "Rexel.fr")
    {
        RexelFr *frn = new RexelFr(m_fct,f.at(0),f.at(1),m_Lien_Travail,f.at(2));
        if(!frn->Test_Connexion())
        {
            if(m_fct->FindTexte("Informations invalides, veuillez réessayer"))
                emit Info("Identifiants incorrect");
        }
        else
        {
            emit Info(tr("Connexion à %0 réussie").arg(nom));
            return true;
        }
    }
    else
        emit Erreur("Fournisseur non trouvé");
    return false;
}

QStringList Fournisseur::Find_Fournisseur(QString nom)
{
    for(int cpt = 0;cpt<fournisseurs.count();cpt++)
        if(fournisseurs.at(cpt).contains(nom))
        {
            QStringList l;
            l.append(fournisseurs.at(cpt).split("&").at(1).split("=").at(1));
            l.append(fournisseurs.at(cpt).split("&").at(2).split("=").at(1));
            l.append(fournisseurs.at(cpt).split("&").at(3).split("=").at(1));
            return l;
        }
    DEBUG << "Fournisseur non trouvé !";
    return QStringList();
}

QStringList Fournisseur::Find_Fournisseur_From_DB(const QString nom)
{
    QSqlQuery req = m_DB.Requete("SELECT Valeur FROM Options WHERE Nom='" + nom + "'");
    if(req.next())
    {
        QStringList var = m_DB.Decrypt(req.value(0).toString()).split("|");
        QStringList l;
        if(var.count() != 3)
        {
            DEBUG << "la variable du fournisseur " << nom << " est incomplète";
            DEBUG << var;
            return QStringList();
        }
        l.append(var.at(0));
        l.append(var.at(1));
        l.append(var.at(2));
        return l;
    }
    DEBUG << "Fournisseur non trouvé !";
    return QStringList();
}

void Fournisseur::Show_Web()
{
    m_fct->WebOpen();
}
