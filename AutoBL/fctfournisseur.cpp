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
        tMax = 30000;
    timer->start(tMax);
    loop->exec();
}

bool FctFournisseur::WebLoad(QString lien)
{
    bool intCo = true;
    for(int cpt=0;cpt<1;cpt++)
    {
        Set_Load(false);
        web->load(QUrl(lien));
        Loop(30000);
        intCo = FindTexte("Aucune connexion Internet");
        if(timer->isActive())
            if(!FindTexte("Aucune connexion Internet"))
                return true;
            else
                m_Error->Err(noConnected,"",FCT);
        web->stop();
    }

    return false;
}

bool FctFournisseur::FindTexte(QString texte)
{
    bool test(false),end(false);
    QString t;

    web->page()->toPlainText([&t,&end](const QString result){t = result;end = true;});
    while(!end)
        Loop(500);
    if(t.contains(texte))
        test = true;

    DEBUG << "find " << test;
    return test;
}

bool FctFournisseur::SaveText()
{
    bool end = false;

    QFile fichier(m_WorkLink + "/web_Temp.txt");
    fichier.resize(0);
    if(!fichier.open(QIODevice::WriteOnly))
        return false;
    QTextStream flux(&fichier);
    web->page()->toPlainText([&flux,&end](const QString result){flux << result;end = true;});
    while(!end)
        Loop(500);
    fichier.close();
    return true;
}

bool FctFournisseur::SaveHtml()
{
    bool end(false);
    QFile fichier(m_WorkLink + "/web_Temp.txt");
    fichier.resize(0);
    if(!fichier.open(QIODevice::WriteOnly))
        return false;

    web->page()->toHtml([&fichier,&end](const QString &result){ fichier.write(result.toUtf8()); end = true; });
    while(!end)
        Loop(500);
    fichier.close();
    return true;
}

QVariant FctFournisseur::InsertJavaScript(QString script)
{
    bool end(false);
    QVariant r;
    web->page()->runJavaScript(script, [&r,&end](const QVariant &result){ r = result; end = true; });
    while(!end)
        Loop(500);
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
