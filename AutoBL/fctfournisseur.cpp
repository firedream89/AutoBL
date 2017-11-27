#include "fctfournisseur.h"

FctFournisseur::FctFournisseur(QString WorkLink, Error *err):
    m_WorkLink(WorkLink)
{
    web = new QWebEngineView;
    timer = new QTimer;
    loop = new QEventLoop;
    m_Error = err;
    QObject::connect(timer,SIGNAL(timeout()),loop,SLOT(quit()));
    QObject::connect(web,SIGNAL(loadFinished(bool)),loop,SLOT(quit()));
    QObject::connect(web,SIGNAL(loadProgress(int)),this,SIGNAL(LoadProgress(int)));
    QObject::connect(web,SIGNAL(loadFinished(bool)),this,SLOT(Set_Load(bool)));

    web->setEnabled(false);
}

FctFournisseur::~FctFournisseur()
{
    delete web;
    delete timer;
    delete loop;
    delete m_Error;
}

void FctFournisseur::Loop(int tMax)
{
    if(tMax == 0)
    {
        tMax = 30000;
    }
    timer->start(tMax);
    loop->exec();
}

bool FctFournisseur::WebLoad(QString lien)
{
    for(int cpt=0;cpt<1;cpt++)
    {
        Set_Load(false);
        web->load(QUrl(lien));
        Loop(30000);
        if(timer->isActive())
        {
            if(FindTexte("Aucune connexion Internet") == false) { return true; }
        }
        else
        {
            m_Error->Err(noConnected,"",FCT);
        }
        web->stop();
    }
    return false;
}

bool FctFournisseur::FindTexte(QString texte)
{
    bool test(false),end(false);
    QString t;

    web->page()->toPlainText([&t,&end](const QString result){t = result;end = true;});
    while(end == false)
    {
        Loop(500);
    }
    if(t.contains(texte))
    {
        test = true;
    }

    DEBUG << "find " << test;
    return test;
}

bool FctFournisseur::SaveText()
{
    bool end = false;

    QFile fichier(m_WorkLink + "/web_Temp.txt");
    fichier.resize(0);
    if(fichier.open(QIODevice::WriteOnly) == false) { return false; }
    QTextStream flux(&fichier);
    web->page()->toPlainText([&flux,&end](const QString result){flux << result;end = true;});
    while(end == false)
    {
        Loop(500);
    }
    fichier.close();
    return true;
}

bool FctFournisseur::SaveHtml()
{
    bool end(false);
    QFile fichier(m_WorkLink + "/web_Temp.txt");
    fichier.resize(0);
    if(fichier.open(QIODevice::WriteOnly) == false) { return false; }

    web->page()->toHtml([&fichier,&end](const QString &result){ fichier.write(result.toUtf8()); end = true; });
    while(end == false)
    {
        Loop(500);
    }
    fichier.close();
    return true;
}

QVariant FctFournisseur::InsertJavaScript(QString script)
{
    bool end(false);
    QVariant r;
    web->page()->runJavaScript(script, [&r,&end](const QVariant &result){ r = result; end = true; });
    while(end == false)
    {
        Loop(500);
    }
    return r;
}

void FctFournisseur::WebOpen()
{
    web->show();
    web->setEnabled(true);
}

void FctFournisseur::FrnError(int code,QString frn,QString er)
{
    DEBUG << m_Error->Err(code,er,frn);
}

void FctFournisseur::Info(QString i)
{
    emit info(i);
}

void FctFournisseur::Change_Load_Window(QString text)
{
    emit change_Load_Window(text);
}

void FctFournisseur::Stop_Load()
{
    web->stop();
}

void FctFournisseur::Set_Load(bool state)
{
    m_load = state;
}

bool FctFournisseur::Get_Load_Finished()
{
    return m_load;
}

QStringList FctFournisseur::Control_Fab(QStringList list)
{
    DEBUG << "Esabora | Verif_List";
    connect(this,SIGNAL(Set_Fab(QString)),loop,SLOT(quit()));

    if(list.isEmpty() || list.count() % 7 != 0)
    {
        DEBUG << "Liste matériels Vide !";
        return QStringList(0);
    }

    QFile file(m_WorkLink + "/Config/Fab.esab");
    if(file.open(QIODevice::ReadOnly) == false)
    {
        m_Error->Err(open_File,"Fab.esab",FCT);
    }
    QTextStream flux(&file);

    QStringList final;
    for(int i=0;i<list.count()-1;i+=7)
    {
        final.append(list.at(i));
        final.append(list.at(i+1));
        final.append(list.at(i+2));
        if(list.at(i+3).isEmpty() && list.at(i+2).isEmpty() == false)//si Fabricant connu mais pas Fab
        {
            flux.seek(0);
            DEBUG << "If";
            while(flux.atEnd() == false)//vérif si Fab déjà connu
            {
                QString var = flux.readLine();
                if(var.split(";").count() == 2 && var.split(";").at(0) == list.at(i+2))
                {
                    final.append(var.split(";").at(1));
                    file.seek(0);
                    break;
                }
            }
            if(flux.atEnd())//Sinon rechercher Fab sur esabora
            {
                emit Find_Fab(list.at(i+2));
                Loop(120000);
                if(m_Fab.isEmpty() == false && m_Fab.count() == 3)
                {
                    file.seek(SEEK_END);
                    flux << list.at(i+2) + ";" + m_Fab + "\r\n";
                    file.seek(0);
                    final.append(m_Fab);
                }
                else
                    final.append("");
            }
        }
        else
        {
            file.seek(0);
            DEBUG << "Else";
            QString var = flux.readAll();
            DEBUG << "Control existing row";
            if(list.at(i+2).isEmpty() == false)
            {
                if(var.contains(list.at(i+2) + ";" + list.at(i+3)) == false)
                {
                    DEBUG << "Add New entry";
                    file.close();
                    if(file.open(QIODevice::WriteOnly | QIODevice::Append) == false)
                        m_Error->Err(open_File,"Fab.esab",FCT);
                    flux << list.at(i+2) + ";" + list.at(i+3) + "\r\n";
                    file.waitForBytesWritten(10000);
                    file.close();
                    if(file.open(QIODevice::ReadOnly) == false)
                        m_Error->Err(open_File,"Fab.esab",FCT);
                }
            }
            else
            {
                DEBUG << "Erreur, fabricant non trouvé dans la commande";
            }
            final.append(list.at(i+3));
        }
        final.append(list.at(i+4));
        final.append(list.at(i+5));
        final.append(list.at(i+6));
    }
    DEBUG << "Esabora | Fin Verif_List";
    disconnect(this,SIGNAL(Set_Fab(QString)),loop,SLOT(quit()));
    m_Fab.clear();

    return final;
    /*if(list.isEmpty() || list.count() < 7)
    {
        DEBUG << "Liste matériels Vide !";
        return;
    }
    connect(this,SIGNAL(Set_Fab(QString)),loop,SLOT(quit()));

    QFile file(m_WorkLink + "/Config/Fab.esab");
    if(file.open(QIODevice::ReadWrite) == false)
    {
        m_Error->Err(open_File,FCT,"Fab.esab");
    }
    QTextStream flux(&file);

    for(int i=0;i<list.count();i++)
    {
        if(list.at(i+3).isEmpty() && list.at(i+2).isEmpty() == false)//si Fabricant connu mais pas Fab
        {
            file.seek(0);
            while(flux.atEnd() == false)//vérif si Fab déjà connu
            {
                QString var = flux.readLine();
                if(var.contains(list.at(i+2)))
                {
                    file.seek(0);
                    break;
                }
            }
            if(file.atEnd())//Sinon rechercher Fab sur esabora
            {
                emit Find_Fab(list.at(i+2));
                Loop(120000);
                if(m_Fab.isEmpty() == false && m_Fab.count() == 3)
                {
                    file.seek(SEEK_END);
                    flux << list.at(i+2) + ";" + m_Fab + "\r\n";
                    file.seek(0);
                }
                else
                {
                    DEBUG << "Constructeur non trouvé : " << m_Fab;
                    flux << list.at(i+2) << ";\r\n";
                }
            }
        }
        else
        {
            bool find(false);
            while(flux.atEnd() == false)
            {
                if(flux.readLine().contains(list.at(2)))
                {
                    find = true;
                }
            }
            if(find == false)
            {
                flux << list.at(i+2) + ";" + list.at(i+3) + "\r\n";
            }
        }
        i += 6;
    }
    disconnect(this,SIGNAL(Set_Fab(QString)),loop,SLOT(quit()));
    m_Fab.clear();*/
}

void FctFournisseur::Return_Fab(QString fab)
{
    m_Fab = fab;
    emit Set_Fab();
}
