#include "cged.h"


CGED::CGED(FctFournisseur *fct, const QString login, const QString mdp, const QString lien_Travail, const QString comp, DB *db):
    m_Login(login),m_MDP(mdp),m_UserName(comp),m_WorkLink(lien_Travail),m_Fct(fct),m_DB(db)
{
#define FRN "CGED"
#define INF "Nom utilisateur|Email|Mot de passe"

    DEBUG << "Init Class " << FRN;
}

bool CGED::Start()
{
    DEBUG << "Start " << FRN;

    bool error(false);
    QSqlQuery req;

    if(Connexion())
    {
        m_Fct->Info("Chargement des commandes...");
        DEBUG << "CGED | Connected";
        if(Create_List_Invoice() == false) { error = true; }

        //Update State
        DEBUG << "CGED | Update State";
        req = m_DB->Get_No_Closed_Invoice(FRN);

        while(req.next())
        {
            m_Fct->Info(tr("Mise à jour état commande %0").arg(req.value("Numero_Commande").toString()));
            Update_State(req.value("Numero_Commande").toString(),req.value("Lien_Commande").toString());
        }

        //Update Delivery
        DEBUG << "CGED | Update Delivery";
        req = m_DB->Get_Delivery_Invoice(FRN);
        while(req.next())
        {
            m_Fct->Info(tr("Récupération des bons de livraison commande %1").arg(req.value("Numero_Commande").toString()));
            Update_Delivery(req.value("Numero_Commande").toString(),req.value("Lien_Commande").toString());
        }
    }
    else
        return false;

    if(error) { return false; }
    return true;
}

bool CGED::Connexion()
{
    //Chargement de la page
    m_Fct->Info("Connexion...");
    if(m_Fct->WebLoad("https://www.cged.fr/INTERSHOP/web/WFS/Sonepar-CGED-Site/fr_FR/cged-private/EUR/ViewProfileSettings-ViewProfile") == false)
    {
        m_Fct->FrnError(load,FRN,"Connexion");
        return false;
    }
    //Injection des scripts de connexion
    m_Fct->InsertJavaScript("document.getElementById('ShopLoginForm_Login').value = '" + m_Login + "';");
    m_Fct->InsertJavaScript("document.getElementById('ShopLoginForm_Password').value = '" + m_MDP + "';");
    m_Fct->InsertJavaScript("document.getElementsByName('LoginUserForm')[0].submit();");
    m_Fct->Loop(5000);

    m_Fct->SaveHtml();

    QFile f(m_WorkLink + "/web_Temp.txt");
    if(f.open(QIODevice::ReadOnly) == false)
    {
        m_Fct->FrnError(open_File,FRN,f.fileName());
        return false;
    }
    QTextStream flux(&f);

    //Contrôle de connexion
    while(!flux.atEnd()) {
        QString v = flux.readLine();
        if(v.contains(m_UserName)) return true;
        else if(v.contains("Vous allez Ãªtre redirigÃ© vers votre compte.")) return true;
        else if(v.contains("Aucun compte trouvÃ© avec vos identifiants")) m_Fct->FrnError(bad_Login,FRN);
    }
    return false;
}

bool CGED::Create_List_Invoice()
{
    //Chargement de la page
    if(m_Fct->WebLoad("https://www.cged.fr/INTERSHOP/web/WFS/Sonepar-CGED-Site/fr_FR/cged-private/EUR/ViewOrders-List") == false)
    {
        m_Fct->FrnError(load,FRN,"Liste des commandes");
        return false;
    }

    //Enregistrement de la page
    m_Fct->SaveHtml();

    //Ouverture de la page
    QFile f(m_WorkLink + "/web_Temp.txt");
    if(f.open(QIODevice::ReadOnly) == false)
    {
        m_Fct->FrnError(open_File,FRN,f.fileName());
        return false;
    }
    QTextStream flux(&f);

    //Extraction lien data
    QString url;
    while (!flux.atEnd()) {
        QString v = flux.readLine();
        if(v.contains("data-datatables-url")) {
            qDebug() << v;
            url = v.split("\"").at(7);
            url = url.replace("&amp;","&");
        }
    }
    f.close();

    //Chargement de la page data
    qDebug() << "URL : " << url;
    if(m_Fct->WebLoad(url) == false)
    {
        m_Fct->FrnError(load,FRN,"Liste des commandes");
        return false;
    }

    //Enregistrement de la page
    m_Fct->SaveHtml();

    //Traitement des informations

    //Ouverture de la page
    if(f.open(QIODevice::ReadOnly) == false)
    {
        m_Fct->FrnError(open_File,FRN,f.fileName());
        return false;
    }
    f.seek(0);
    QStringList invoiceNumber;
    while(!flux.atEnd()) {
        QString v = flux.readLine();
        if(v.contains("ordernumber")) {
            invoiceNumber += v.split("\"").at(3);
        }
    }

    //DESC
    QStringList tmp;
    for(int i = 0;i < invoiceNumber.count();i++) {
        bool insert = false;
        for(int i2 = 0;i2 < tmp.count();i2++) {
            if(invoiceNumber.at(i) > tmp.at(i2)) {
                tmp.insert(i2, invoiceNumber.at(i));
                insert = true;
                break;
            }
        }
        if(tmp.isEmpty() || !insert) {
            tmp.append(invoiceNumber.at(i));
        }
    }
    invoiceNumber = tmp;

    flux.seek(0);

    bool end = false;
    for(int i = 0;i < invoiceNumber.count();i++) {
        while(!flux.atEnd()) {
            if(flux.readLine() == "{") {
                QString date = flux.readLine().split("\"").at(3);
                QString reference = flux.readLine().split("\"").at(3);
                QString name = flux.readLine().split("\"").at(3);
                flux.readLine();
                QString order = flux.readLine().split("\"").at(3);
                QString status = flux.readLine().split("\"").at(3);
                flux.readLine();
                flux.readLine();
                flux.readLine();
                QString link = flux.readLine().split("\"").at(4);

                //status
                status.replace("Ã©","é");
                if(status == "En attente" || status == "En traitement" || status == "En préparation" || status == "Enregistrée" || status == "Partiellement Annulée")
                {
                    status = "0";
                }
                else if(status == "Partiellement livrée" || status == "Partiellement facturée")
                {
                    status = "1";
                }
                else if(status == "Livrée" || status == "Facturée" || status == "Terminée")
                {
                    status = "2";
                }
                else if(status == "Annulée")
                {
                    status = -1;
                }
                else
                {
                    m_Fct->FrnError(variable,FRN,"Valeur Etat=" + status);
                    status = "0";
                }

                //date
                int day = date.split("\\/").at(0).toInt();
                int month = date.split("\\/").at(1).toInt();
                int year = date.split("\\/").at(2).toInt();
                date = QString("%0-%1-%2").arg(year).arg(month).arg(day);

                if(order == invoiceNumber.at(i)) {
                    if(status == -1)
                        break;

                    QStringList invoice;
                    invoice.append(order);
                    invoice.append(date);
                    invoice.append(link);
                    invoice.append(status);
                    invoice.append(reference);
                    invoice.append(name);
                    invoice.append(FRN);

                    if(!m_Fct->Add_Invoice(invoice))
                        end = true;
                    break;
                }
            }
        }
        flux.seek(0);
        if(end)
            break;
    }
    return true;
}

bool CGED::Update_Delivery(QString invoice,QString link)
{

    link.replace("Detail","Shipping");
    if(m_Fct->WebLoad(link) == false)
    {
        m_Fct->FrnError(load,FRN);
    }
    else
    {
        if(m_Fct->FindTexte(invoice) == false || m_Fct->FindTexte("Bon de livraison") == false)
        {
            m_Fct->FrnError(fail_check,FRN,"Page non chargée");
        }
        else
        {
            if(m_Fct->SaveHtml() == false)
            {
                m_Fct->FrnError(save_file,FRN,"Html");
            }
            else
            {
                QString bl;

                QFile f(m_WorkLink + "/web_Temp.txt");
                if(f.open(QIODevice::ReadOnly) == false)
                {
                    m_Fct->FrnError(open_File,FRN,"web_Temp");
                }
                else
                {
                    QTextStream flux(&f);
                    while(flux.atEnd() == false)
                    {
                        bl = flux.readLine();
                        if(bl.contains("Bon de livraison"))
                        {
                            bl = bl.split(" ").last().split("<").first();
                            break;
                        }
                    }
                    m_DB->Update_En_Cours(invoice,FRN,"Numero_Livraison",bl);
                    return true;
                }
            }
        }
    }

    return false;
}

bool CGED::Update_State(QString invoice,QString link)
{
    if(m_Fct->WebLoad(link) == false)
    {
        m_Fct->FrnError(load,FRN);
    }
    else
    {
        if(m_Fct->FindTexte(invoice) == false)
        {
            m_Fct->FrnError(fail_check,FRN,"Page non chargée");
        }
        else
        {
            int state(0);

            if(m_Fct->FindTexte("En attente")) { state = open; }
            else if(m_Fct->FindTexte("En traitement")) { state = open; }
            else if(m_Fct->FindTexte("En préparation")) { state = open; }
            else if(m_Fct->FindTexte("Enregistrée")) { state = open; }
            else if(m_Fct->FindTexte("Livrée")) { state = Close; }
            else if(m_Fct->FindTexte("Partiellement livrée")) { state = partial; }
            else if(m_Fct->FindTexte("Partiellement facturée")) { state = partial; }
            else if(m_Fct->FindTexte("Facturée")) { state = Close; }
            else if(m_Fct->FindTexte("Terminée")) { state = Close; }
            else if(m_Fct->FindTexte("Annulée"))
            {
                m_DB->Remove_En_Cours(invoice,FRN,"commande Annulée");
            }

            if(state != 0)
            {
                m_DB->Update_En_Cours(invoice,FRN,"Etat",QString::number(state));
            }
            return true;
        }
    }
    return false;
}

QStringList CGED::Get_Invoice(const QString InvoiceNumber,QString link)
{
    //Retourne une liste d'un tableau de commande
    //0 = nb commande
    //boucle de 7 strings designation,reference,fabricant,fab,prix unitaire,quantité livré,quantité restante

    DEBUG << "CGED | Connexion";
    m_Fct->Change_Load_Window(tr("Connexion..."));
    if(Connexion() == false) { return QStringList(nullptr); }

    m_Fct->Change_Load_Window(tr("Chargement de la commande..."));

    DEBUG << "CGED | Chargement de la page";
    if(m_Fct->WebLoad(link) == false)
    {
        m_Fct->FrnError(load,FRN,link);
    }
    else
    {
        DEBUG << "CGED | Vérification de la page";
        if(m_Fct->FindTexte(InvoiceNumber) == false)
        {
            m_Fct->FrnError(fail_check,FRN,InvoiceNumber);
        }
        else
        {
            DEBUG << "CGED | Traitement des informations de la page";
            if(m_Fct->SaveHtml() == false)
            {
                m_Fct->FrnError(save_file,FRN,"Html");
            }
            else
            {
                ///Traitement des données de la page
                QFile file(m_WorkLink + "/web_Temp.txt");
                if(file.open(QIODevice::ReadOnly) == false)
                {
                    m_Fct->FrnError(open_File,FRN,"web_Temp.txt");
                }
                else
                {
                    QTextStream flux(&file);    
                    bool skip = false;
                    QStringList list;
                    QString fab, ref, desc, prix,qt;

                    while(flux.atEnd() == false)
                    {
                        QString var = flux.readLine();
                        if(var.contains("Fabricant :")) {//Fabriquant
                            fab = var.split(">").at(1).split("<").first();
                        }
                        else if(var.contains("Ref fabricant")) {//référence
                            ref = var.split(" ").last().split("<").first();
                        }
                        else if(var.contains("i class=\"desc\"")) {//description
                            desc = var.split(">").at(2).split("<").first();
                        }
                        else if(var.contains("alcenter")) {//quantité
                            DEBUG << "quantity";
                            qt = var.split(">").at(1).split("<").first();
                            var = flux.readLine();
                            prix = QString::number(var.split(" ").first().split(">").last().replace(",",".").toDouble());
                            DEBUG << "prix" << prix << var;
                            if(qt > 1)
                                prix = QString::number(prix.toDouble() / qt.toDouble());
                        }
                        else if(var.contains("Annul&eacute;e")) {//si annulée
                            skip = true;
                        }
                        else if(var.contains("infos pointer")) {
                            DEBUG << fab << ref << desc << qt << prix;
                            if(!skip && !fab.isEmpty() && ! ref.isEmpty() && !prix.isEmpty() && !qt.isEmpty()) {
                                list.append(desc);
                                list.append(ref);
                                list.append(fab);
                                list.append("");
                                list.append(prix);
                                list.append(qt);
                                list.append("NC");

                                desc.clear();
                                ref.clear();
                                fab.clear();
                                prix.clear();
                                qt.clear();
                            }
                            desc.clear();
                            ref.clear();
                            fab.clear();
                            prix.clear();
                            qt.clear();
                            skip = false;
                        }
                    }
                    DEBUG << fab << ref << desc << qt << prix;
                    if(!skip && !fab.isEmpty() && ! ref.isEmpty() && !prix.isEmpty() && !qt.isEmpty()) {
                        list.append(desc);
                        list.append(ref);
                        list.append(fab);
                        list.append("");
                        list.append(prix);
                        list.append(qt);
                        list.append("NC");
                    }
                    DEBUG << list.count() % 7;
                    if(!list.isEmpty() && list.count() % 7 == 0) {
                        return list;
                    }
                    else {
                        m_Fct->FrnError(variable,FRN,"Création tableau(liste incomplète)");
                    }
                }
            }
        }
    }
    return QStringList(nullptr);
}

void CGED::Set_Var(const QString login, const QString mdp, const QString comp)
{
    m_Login = login;
    m_MDP = mdp;
    m_UserName = comp;
}

bool CGED::Test_Connexion()
{
    DEBUG << "TEST CONNEXION " << FRN;
    return Connexion();
}

QString CGED::Get_Inf()
{
    return QString(INF);
}
