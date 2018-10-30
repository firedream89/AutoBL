#include "fournisseur.h"


////////////
///FRN CLASS UPDATE
/// -Start
/// -Get_Invoice_List
/// -Test_Connexion
/// -Get_Frn_Inf
///////////

Fournisseur::Fournisseur(QString lien_Travail, DB *db, Error *err):
    m_Lien_Travail(lien_Travail),m_DB(db),m_Error(err)
{
    DEBUG << "Fournisseur | Init de la classe";
    m_fct = new FctFournisseur(m_Lien_Travail,m_Error,m_DB);
    connect(m_fct,SIGNAL(info(QString)),this,SIGNAL(Info(QString)));
    connect(m_fct,SIGNAL(LoadProgress(int)),this,SIGNAL(LoadProgress(int)));
    connect(m_fct,SIGNAL(change_Load_Window(QString)),this,SIGNAL(Change_Load_Window(QString)));
    connect(m_fct,SIGNAL(Find_Fab(QString)),this,SIGNAL(Find_Fab(QString)));
}

Fournisseur::~Fournisseur()
{
    delete m_fct;
}

bool Fournisseur::Add(const QString nom,const QString complement,const QString login,const QString mdp)
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
    if(l.isEmpty()) { return false; }
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

            if(nom == FRN1)
            {
                RexelFr *frn = new RexelFr(m_fct,login,mdp,m_Lien_Travail,comp,m_DB);
                frn->Start();
                m_fct->Add_Invoices_To_DB();
            }
            else if(nom == FRN2)
            {
                SocolecFr *frn = new SocolecFr(m_fct,login,mdp,m_Lien_Travail,comp,m_DB);
                frn->Start();
                m_fct->Add_Invoices_To_DB();
            }
            else
                m_Error->Err(failFrn,nom,FRN);
        }
        else
            m_Error->Err(failData,"",FRN);
    }
    return true;
}

QStringList Fournisseur::Get_Invoice_List(const QString& frn,const QString& invoiceNumber)
{
    QStringList f = Find_Fournisseur(frn);
    DEBUG << frn << invoiceNumber;
    if(f.isEmpty())
    {
        m_Error->Err(findFrn,frn,FRN);
        return QStringList(0);
    }
    ///Init variables fonction
    QString login = f.at(0);
    QString mdp = f.at(1);
    QString comp = f.at(2);

    QSqlQuery req = m_DB->Requete("SELECT Lien_Commande FROM En_Cours WHERE Numero_Commande='" + invoiceNumber + "' AND Fournisseur='" + frn + "'");
    if(!req.next())
    {
        m_Error->Err(requete,"invoice not found",FRN);
        return QStringList(0);
    }
    QString link = req.value(0).toString();

    QStringList final;
    if(frn == FRN1)
    {
        RexelFr *frn = new RexelFr(m_fct,login,mdp,m_Lien_Travail,comp,m_DB);
        final = frn->Get_Invoice(invoiceNumber);
    }
    else if(frn == FRN2)
    {
        SocolecFr *frn = new SocolecFr(m_fct,login,mdp,m_Lien_Travail,comp,m_DB);
        final = frn->Get_Invoice(invoiceNumber,link);
    }
    return final;
}

bool Fournisseur::Update_Var(const QString& frn,const QString& login,const QString& mdp,const QString& complement)
{
    DEBUG << "UPDATE " << frn;
    for(int cpt = 0;cpt<fournisseurs.count();cpt++)
    {
        if(fournisseurs.at(cpt).contains(frn))
        {
            fournisseurs.replaceInStrings(fournisseurs.at(cpt),"nom=" + frn + "&login=" + login + "&mdp=" + mdp + "&comp=" + complement);
            return true;
        }
    }
    return false;
}

bool Fournisseur::Del(const QString nom)
{
    DEBUG << "DEL " << nom;
    for(int cpt = 0;cpt<fournisseurs.count();cpt++)
    {
        if(fournisseurs.at(cpt).contains(nom))
        {
            fournisseurs.removeAt(cpt);
            return true;
        }
    }
    return false;
}

bool Fournisseur::Test_Connexion(const QString &nom)
{
    DEBUG << "TEST " << nom;

    QStringList f = Find_Fournisseur(nom);
    if(f.isEmpty())
    {
        m_Error->Err(findFrn,nom,FRN);
        return false;
    }
    if(nom == FRN1)
    {
        RexelFr *frn = new RexelFr(m_fct,f.at(0),f.at(1),m_Lien_Travail,f.at(2),m_DB);
        if(frn->Test_Connexion() == false)
        {
            if(m_fct->FindTexte("Informations invalides, veuillez réessayer"))
            {
                emit Info(tr("Identifiants incorrect"));
            }
            else
            {
                emit Info(tr("Une erreur inconnue s'est produite"));
            }
        }
        else
        {
            emit Info(tr("Connexion à %0 réussie").arg(nom));
            return true;
        }
    }
    else if(nom == FRN2)
    {
        SocolecFr * frn = new SocolecFr(m_fct,f.at(0),f.at(1),m_Lien_Travail,f.at(2),m_DB);
        if(frn->Test_Connexion() == false)
        {
            if(m_fct->FindTexte("Le N° de compte spécifié n'existe pas"))
            {
                emit Info(tr("Le numéro de compte est incorrecte"));
            }
            else if(m_fct->FindTexte("Email ou mot de passe incorrect"))
            {
                emit Info(tr("Email ou mot de passe incorrect"));
            }
            else
            {
                emit Info(tr("Une erreur unconnue s'est produite"));
            }
        }
        else
        {
            emit Info(tr("Connexion à %0 réussie").arg(nom));
            return true;
        }
    }
    else
        m_Error->Err(findFrn,"",FRN);

    return false;
}

QStringList Fournisseur::Find_Fournisseur(QString nom)
{
    for(int cpt = 0;cpt<fournisseurs.count();cpt++)
    {
        if(fournisseurs.at(cpt).contains(nom))
        {
            QStringList l;
            l.append(fournisseurs.at(cpt).split("&").at(1).split("=").at(1));
            l.append(fournisseurs.at(cpt).split("&").at(2).split("=").at(1));
            l.append(fournisseurs.at(cpt).split("&").at(3).split("=").at(1));
            return l;
        }
    }
    DEBUG << "Fournisseur non trouvé !";
    return QStringList();
}

QStringList Fournisseur::Find_Fournisseur_From_DB(const QString nom)
{
    QSqlQuery req = m_DB->Requete("SELECT Valeur FROM Options WHERE Nom='" + nom + "'");
    if(req.next())
    {
        QStringList var = m_DB->Decrypt(req.value(0).toString()).split("|");
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

QString Fournisseur::List_Frn() const
{
    return QString(FRNLIST);
}

QString Fournisseur::Get_Frn_Inf(QString frn) const
{
    if(frn == FRN1)
    {
        return RexelFr::Get_Info();
    }
    else if(frn == FRN2)
    {
        return SocolecFr::Get_Inf();
    }
    else
    {
        m_Error->Err(variable,"Get_Frn_Inf",FRN);
    }
    return QString();

}

void Fournisseur::Set_Fab(QString fab)
{
    m_fct->Return_Fab(fab);
}

