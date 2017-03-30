#include "fctfournisseur.h"

FctFournisseur::FctFournisseur(QString WorkLink):
    m_WorkLink(WorkLink)
{


    web = new QWebEngineView;
    timer = new QTimer;
    loop = new QEventLoop;
    m_Error = new Error;
    web_Timer = new QTimer;
    QObject::connect(timer,SIGNAL(timeout()),loop,SLOT(quit()));
    QObject::connect(web,SIGNAL(loadFinished(bool)),loop,SLOT(quit()));
    QObject::connect(web,SIGNAL(loadProgress(int)),this,SIGNAL(LoadProgress(int)));
    QObject::connect(web_Timer,SIGNAL(timeout()),this,SLOT(Delete_Web_Page()));
}

FctFournisseur::~FctFournisseur()
{
    delete web;
    delete timer;
    delete loop;
    delete web_Timer;
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
    for(int cpt=0;cpt<1;cpt++)
    {
        web->load(QUrl(lien));
        Loop(30000);
        if(timer->isActive())
        {
            Launch_Web_Timer();
            return true;
        }
    }
    return false;
}

bool FctFournisseur::FindTexte(QString texte)
{
    bool test(false),end(false);
    web->page()->findText(texte, QWebEnginePage::FindFlags(), [&test,&end](const bool found){test = found;end = true;});
    while(!end)
        Loop(500);
    DEBUG << "find " << test;
    return test;
}

bool FctFournisseur::SaveText()
{
    QFile fichier(m_WorkLink + "/web_Temp.txt");
    fichier.resize(0);
    if(!fichier.open(QIODevice::WriteOnly))
        return false;

    web->page()->triggerAction(QWebEnginePage::SelectAll);
    fichier.write(web->page()->selectedText().toUtf8());
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
}

void FctFournisseur::FrnError(int code,QString er)
{
    QString e;
    switch (code) {
    case 0:
        e = tr("Echec de connexion");
        break;
    case 1:
        e = tr("Echec de récupération des données");
        break;
    case 2:
        e = tr("Vérification état du bon de commande échouée");
        break;
    case 3:
        e = tr("Récupération du bon de livraison échouée");
        break;
    case 4:
        e = tr("Ouverture du fichier échouée");
        break;
    case 5:
        e = tr("Valeur non trouvée");
        break;
    case 6:
        e = tr("Requête vers la base de données échouée");
        break;
    case 7:
        e = tr("Echec chargement de la page");
        break;
    case 8:
        e = tr("nombre de bon de livraison supérieure à 5");
        break;
    case 9:
        e = tr("Liste de matériels incomplet");
        break;
    default:
        e = tr("Erreur inconnue");
        break;
    }

    QString frn;
    if(QString::number(code).at(0) == '9')
        frn = "Rexel.fr";

    emit error(frn + " | " + e + ": " + er);

    //New Error
    er = m_Error->Err(code,er);
    emit error(er);
   DEBUG << er;
}

void FctFournisseur::Info(QString i)
{
    emit info(i);
}

void FctFournisseur::Change_Load_Window(QString text)
{
    emit change_Load_Window(text);
}

void FctFournisseur::Launch_Web_Timer()
{
    web_Timer->start(300000);
}

void FctFournisseur::Delete_Web_Page()
{
    web->page()->deleteLater();
}
