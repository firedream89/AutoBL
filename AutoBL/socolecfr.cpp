#include "socolecfr.h"

SocolecFr::SocolecFr(FctFournisseur *fct, const QString login, const QString mdp, const QString lien_Travail, const QString comp, DB *db):
    m_Login(login),m_MDP(mdp),m_UserName(comp),m_WorkLink(lien_Travail),m_Fct(fct),m_DB(db)
{
    DEBUG << "Init Class " << FRN;
}

bool SocolecFr::Start()
{
    DEBUG << "Start " << FRN;

    bool firstInit(false);
    bool error(false);

    QSqlQuery req = m_DB->Requete("SELECT * FROM En_Cours WHERE Fournisseur='" + QString(FRN) + "'");
    if(!req.next())
        firstInit = true;

    if(Connexion())
    {
        if(!Create_List_Invoice(firstInit))
            error = true;


    }
    else
        return false;

    if(error)
        return false;
    return true;
}

bool SocolecFr::Connexion()
{
    //Chargement de la page
    m_Fct->Info("Connexion...");
    if(!m_Fct->WebLoad("https://socolec.sonepar.fr/is-bin/INTERSHOP.enfinity/WFS/Sonepar-SOCOLEC-Site/fr_FR/-/EUR/ViewLogin-Start"))
    {
        m_Fct->FrnError(load,FRN,"Connexion");
        return false;
    }

    //Injection des scripts de connexion
    m_Fct->InsertJavaScript("document.getElementsByName('LoginForm_Login')[0].value = '" + m_Login + "';");
    m_Fct->InsertJavaScript("document.getElementsByName('LoginForm_Password')[0].value = '" + m_MDP + "';");
    m_Fct->InsertJavaScript("document.getElementsByName('LoginForm_RegistrationDomain')[0].value = '" + m_UserName + "';");
    m_Fct->InsertJavaScript("document.getElementsByName('LoginForm')[0].submit();");
    m_Fct->Loop(5000);

    //Contrôle de connexion
    if(m_Fct->FindTexte("Le N° de compte spécifié n'existe pas"))
    {
        m_Fct->FrnError(bad_Login,FRN,"Numéro de compte inconnu");
        return false;
    }
    else if(m_Fct->FindTexte("Email ou mot de passe incorrect"))
    {
        m_Fct->FrnError(bad_Login,FRN);
        return false;
    }
    else if(m_Fct->FindTexte("Historique des commandes"))
        return true;
}

bool SocolecFr::Create_List_Invoice(bool firstInit)
{
    //Chargement de la page
    if(!m_Fct->WebLoad("https://socolec.sonepar.fr/is-bin/INTERSHOP.enfinity/WFS/Sonepar-SOCOLEC-Site/fr_FR/-/EUR/"
                       "ViewPurchaseOrderList-Start?SortAttribute=CreationDate&AttributeType=DATE&SortDirection=DESC&ListAllOrders=1"))
    {
        m_Fct->FrnError(load,FRN,"Liste des commandes");
        return false;
    }

    //Enregistrement de la page
    m_Fct->SaveHtml();

    //Ouverture de la page
    QFile f(m_WorkLink + "/web_Temp.txt");
    if(!f.open(QIODevice::ReadOnly))
    {
        m_Fct->FrnError(open_File,FRN,f.fileName());
        return false;
    }
    QTextStream flux(&f);

    //Traitement des informations
    QString text,link,name,ref,date,etat,invoice_Number;
    while(!f.atEnd())
    {
        text = flux.readLine();
        if(text.contains("tr style=\"cursor: pointer;\""))//Point de départ d'une ligne du tableau
        {
            if(text.split("'").count() > 1)
                link = text.split("'").at(1);
            else
            {
                m_Fct->FrnError(variable,FRN,"link");
                return false;
            }
            text = flux.readLine();
            if(text.split(">").count() > 1)
                date = text.split(">").at(1).split("<").at(0).split("&").at(0);
            else
            {
                m_Fct->FrnError(variable,FRN,"date");
                return false;
            }
            flux.readLine();
            flux.readLine();
            flux.readLine();
            flux.readLine();
            flux.readLine();
            flux.readLine();
            flux.readLine();
            flux.readLine();
            flux.readLine();
            text = flux.readLine();
            text.replace(" ","");
            if(text.contains("0123456789",Qt::CaseInsensitive))
                invoice_Number = text;
            flux.readLine();
            flux.readLine();
            text = flux.readLine();
            if(text.split(">").count() > 1)
            {
                text = text.split(">").at(1);
                text.replace("&eacute;","é");
                if(text == "En attente" || text == "En traitement" || text == "En préparation" || text == "Livrée" || text == "Partiellement livrée" ||
                        text == "Partiellement facturée" || text == "Facturée" || text == "Terminée")
                    etat = text;
                else
                {
                    m_Fct->FrnError(variable,FRN,"Valeur Etat");
                    return false;
                }
            }
            else
            {
                m_Fct->FrnError(variable,FRN,"Etat");
                return false;
            }
            int ID(0);
            QSqlQuery req = m_DB->Requete("SELECT MAX(ID) FROM En_Cours");
            ID = req.value(0).toInt();
            ID++;
            m_DB->Requete("INSERT INTO En_Cours VALUES('" + QString::number(ID) + "','" + date + "','" + ref + "','" + invoice_Number + "','','" + link + "','" + etat + "','','" + name + "','0','','" + FRN + "')");
            if(firstInit)
                return true;
        }
    }
}

QStringList SocolecFr::Get_Invoice(const QString InvoiceNumber)
{
    return QStringList(0);
}

void SocolecFr::Set_Var(const QString login, const QString mdp, const QString comp)
{
    m_Login = login;
    m_MDP = mdp;
    m_UserName = comp;
}

bool SocolecFr::Test_Connexion()
{
    DEBUG << "TEST CONNEXION " << FRN;
    return Connexion();
}

QString SocolecFr::Get_Inf()
{
    return QString(INF);
}
