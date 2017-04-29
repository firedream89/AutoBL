#include "principal.h"
#include "ui_principal.h"

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFormLayout>
#include <QFile>
#include <QTextStream>
#include <QWidget>
#include <QDebug>
#include <QTableWidget>
#include <QLineEdit>
#include <QDir>

principal::principal(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::principal)
{
    ui->setupUi(this);

    connection();
    this->setWindowTitle("ENTREPRISE - SESSION : 1 - REPERTOIRE : DB");

    connect(ui->actionAjout_Bon,SIGNAL(triggered(bool)),this,SLOT(list_bon()));
    connect(ui->actionValidation_Bon,SIGNAL(triggered(bool)),this,SLOT(validation_Bon()));
}

principal::~principal()
{
    delete ui;
}

void principal::connection()
{
    QDialog *d = new QDialog(this);
    d->setWindowTitle("Connection");
    d->setObjectName("connection");
    connect(d,SIGNAL(rejected()),qApp,SLOT(quit()));
    connect(d,SIGNAL(finished(int)),d,SLOT(deleteLater()));
    QFormLayout *l = new QFormLayout(d);
    QLineEdit *login = new QLineEdit;
    login->setObjectName("login");
    QLineEdit *passwd = new QLineEdit;
    passwd->setObjectName("passwd");
    passwd->setEchoMode(QLineEdit::Password);
    QPushButton *enter = new QPushButton;
    enter->setText("Connection");
    connect(enter,SIGNAL(clicked(bool)),this,SLOT(login()));
    l->addRow("Login",login);
    l->addRow("Password",passwd);
    l->addWidget(enter);
    d->exec();
}

void principal::login()
{
    QDialog *f = this->findChild<QDialog*>("connection");
    if(f != NULL)
    {
        if(f->findChild<QLineEdit*>("login")->text() == "test" && f->findChild<QLineEdit*>("passwd")->text() == "pass")
        {
            f->close();
            this->showMaximized();
        }
        else
            QMessageBox::warning(f,"Erreur","Login ou mot de passe incorrecte !");
    }
}

void principal::list_bon()
{
    QDialog *f = new QDialog(this);
    f->setWindowTitle("Liste des commandes");
    f->setObjectName("listbon");

    QVBoxLayout *l = new QVBoxLayout(f);
    QTableWidget *t = new QTableWidget;
    t->insertColumn(0);
    l->addWidget(t);
    QPushButton *b = new QPushButton;
    b->setText("Nouveau");
    b->setShortcut(tr("Alt+C"));
    connect(b,SIGNAL(clicked(bool)),this,SLOT(create_Bon()));
    l->addWidget(b);

    QDir d;
    d.setPath(qApp->applicationDirPath());
    QFileInfoList list = d.entryInfoList();

    for(int i = 0;i<list.count();i++)
    {
        if(list.at(i).fileName().contains(".txt"))
        {
            t->insertRow(0);
            t->setItem(0,0,new QTableWidgetItem(list.at(i).fileName().split(".").at(0)));
        }
    }

    f->showMaximized();
}

void principal::create_Bon()
{
    QFile file("nBC.txt");
    if(!file.open(QIODevice::ReadWrite))
        QMessageBox::warning(this,"Erreur","Erreur dans le fichier f1");

    QDialog *f = new QDialog(this);
    connect(f,SIGNAL(rejected()),this,SLOT(show()));
    connect(f,SIGNAL(finished(int)),f,SLOT(deleteLater()));
    f->setObjectName("CBC");
    f->setWindowTitle("Création d'une commande en cours - session: 1 - DB");
    QFormLayout *l = new QFormLayout(f);
    QLineEdit *nBC = new QLineEdit;
    nBC->setText(QString::number(file.readLine().toInt() + 1));
    nBC->setObjectName("nbc");
    l->addRow("Numero BC",nBC);
    QLineEdit *frn = new QLineEdit();
    frn->setObjectName("frn");
    l->addRow("Fournisseur",frn);
    QLineEdit *intlct = new QLineEdit();
    intlct->setObjectName("int");
    l->addRow("Interlocuteur",intlct);
    QLineEdit *nch = new QLineEdit();
    nch->setObjectName("nch");
    l->addRow("Chantier",nch);
    QPushButton *b = new QPushButton;
    b->setText("Valider");
    b->setShortcut(tr("Alt+v"));
    connect(b,SIGNAL(clicked(bool)),this,SLOT(save_new_bl()));
    l->addRow(b);
    connect(nch,SIGNAL(editingFinished()),this,SLOT(find_chantier()));


    nBC->selectAll();
    qDebug() << "open window";
    f->exec();
}

void principal::find_chantier()
{
    QString text;
    if(this->findChild<QDialog*>("CBC") != NULL)
    {
        this->findChild<QDialog*>("CBC")->findChild<QLineEdit*>("nch")->blockSignals(true);
        text = this->findChild<QDialog*>("CBC")->findChild<QLineEdit*>("nch")->text();
    }

    QFile file("lBC.txt");
    if(!file.open(QIODevice::ReadOnly))
        QMessageBox::warning(this,"Erreur","Erreur dans le fichier f1");
    QTextStream flux(&file);
    bool test(false);
    while(!flux.atEnd())
        if(flux.readLine().contains(text))
            test = true;
    if(!test)
    {
        QMessageBox::question(0,"Erreur","Le chantier n'existe pas, voulez vous le créer ?");
        this->findChild<QDialog*>("CBC")->findChild<QLineEdit*>("nch")->blockSignals(false);
    }
}

void principal::save_new_bl()
{
    if(this->findChild<QDialog*>("CBC") != NULL)
    {
        QDialog *f = this->findChild<QDialog*>("CBC");
        QString nbc = f->findChild<QLineEdit*>("nbc")->text();
        QString frn = f->findChild<QLineEdit*>("frn")->text();
        QString intlct = f->findChild<QLineEdit*>("int")->text();
        QString nch = f->findChild<QLineEdit*>("nch")->text();

        if(frn.isEmpty() || intlct.isEmpty() || nch.isEmpty())
        {
            QMessageBox::warning(f,"Erreur","Des informations sont manquantes");
            return;
        }
        QFile file(nbc + ".txt");
        QFile file2("lBC.txt");
        QFile file3("nBC.txt");
        if(!file.open(QIODevice::WriteOnly) || !file2.open(QIODevice::ReadOnly) || !file3.open(QIODevice::WriteOnly))
        {
            QMessageBox::warning(f,"Erreur","Erreur dans le fichier f1");
            return;
        }
        QTextStream flux(&file);
        QTextStream flux2(&file2);
        QString nameCh;
        while(!flux2.atEnd())
        {
            nameCh = flux2.readLine();
            if(nameCh.contains(nch))
            {
                nameCh = nameCh.split(";").at(1);
                break;
            }
            nameCh.clear();
        }
        flux << "Numero Esabora : " << nbc << "\r\n" << "Fournisseur : " << frn << "\r\n" << "Interlocuteur : " << intlct << "\r\n" << "Numéro Chantier : " << nch << "\r\n" << "Nom Chantier : " << nameCh;
        file3.resize(0);
        file3.write(nbc.toLatin1());

        this->showMaximized();
        show_Bon(nbc);
        f->accept();
    }
    else
        QMessageBox::warning(0,"Erreur","Erreur dans le fichier f1");
}

void principal::show_Bon(QString bon)
{
    QFile file(bon + ".txt");
    if(!file.open(QIODevice::ReadOnly))
        QMessageBox::warning(this,"Erreur","Erreur dans le fichier f1");

    QDialog *f = new QDialog(this);
    f->setObjectName("showbon");
    f->setWindowTitle("Commande " + bon + "- session : 1 - DB");
    connect(f,SIGNAL(rejected()),this,SLOT(show()));
    connect(f,SIGNAL(rejected()),this,SLOT(save_bon()));
    QVBoxLayout *l = new QVBoxLayout(f);
    QTableWidget *t = new QTableWidget;
    t->setObjectName("table");
    t->insertColumn(0);
    t->insertColumn(1);
    t->insertColumn(2);
    t->insertColumn(3);
    t->insertColumn(4);
    t->insertColumn(5);
    t->insertRow(0);
    t->insertRow(1);
    t->setItem(0,0,new QTableWidgetItem());
    t->setItem(0,1,new QTableWidgetItem());
    t->setItem(0,2,new QTableWidgetItem());
    t->setItem(0,3,new QTableWidgetItem());
    t->setItem(0,4,new QTableWidgetItem());
    t->setItem(0,5,new QTableWidgetItem());
    t->setItem(1,0,new QTableWidgetItem());
    t->setItem(1,1,new QTableWidgetItem());
    t->setItem(1,2,new QTableWidgetItem());
    t->setItem(1,3,new QTableWidgetItem());
    t->setItem(1,4,new QTableWidgetItem());
    t->setItem(1,5,new QTableWidgetItem());
    t->setCurrentCell(0,0);
    connect(t,SIGNAL(itemSelectionChanged()),this,SLOT(add_row()));
    l->addWidget(t);

    f->showMaximized();
}

void principal::add_row()
{
    if(this->findChild<QDialog*>("showbon") != NULL)
        if(this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->rowCount() ==
                this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->currentRow()+1)
        {
            this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->insertRow(
                    this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->currentRow()+1);
            int i = this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->currentRow()+1;
            this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->setItem(i,0,new QTableWidgetItem());
            this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->setItem(i,1,new QTableWidgetItem());
            this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->setItem(i,2,new QTableWidgetItem());
            this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->setItem(i,3,new QTableWidgetItem());
            this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->setItem(i,4,new QTableWidgetItem());
            this->findChild<QDialog*>("showbon")->findChild<QTableWidget*>("table")->setItem(i,5,new QTableWidgetItem());
        }
}

void principal::save_bon()
{
    if(QMessageBox::question(0,"Sauvegarder","Voulez-vous enregistrer les informations ?") == QMessageBox::Yes)
    {
        QDialog *f = this->findChild<QDialog*>("showbon");
        if(f != NULL)
        {
            QTableWidget *t = f->findChild<QTableWidget*>("table");
            QFile file(f->windowTitle().split(" ").at(1) + ".txt");
            if(!file.open(QIODevice::WriteOnly | QIODevice::Append))
                QMessageBox::warning(0,"Erreur","Erreur dans le fichier f1");
            QTextStream flux(&file);
            for(int i=0;i<t->rowCount()-2;i++)
                flux << "\r\n" << t->item(i,0)->text() << ";" << t->item(i,1)->text() << ";" << t->item(i,2)->text() << ";" << t->item(i,3)->text();
        }
        else
            QMessageBox::warning(0,"Erreur","Erreur dans le fichier f1");
    }
    this->findChild<QDialog*>("showbon")->deleteLater();
}

void principal::validation_Bon()
{
    QDialog *f = new QDialog(this);
    f->setObjectName("valbon");
    f->setWindowTitle("Réception des commandes magasinier");
    QFormLayout *l = new QFormLayout(f);
    QLineEdit *bc = new QLineEdit;
    bc->setObjectName("bc");
    l->addRow("Bon de commande",bc);
    connect(bc,SIGNAL(editingFinished()),this,SLOT(aff_bon()));
    QLineEdit *c = new QLineEdit;
    c->setObjectName("c");
    l->addRow("Commentaire",c);
    QLineEdit *bl = new QLineEdit;
    bl->setObjectName("bl");
    l->addRow("Bon de livraison",bl);
    QPushButton *b1 = new QPushButton;
    b1->setText("Total");
    b1->setShortcut(tr("Alt+L"));
    connect(b1,SIGNAL(clicked(bool)),this,SLOT(total_bon()));
    l->addWidget(b1);
    QPushButton *b2 = new QPushButton;
    b2->setText("Valider");
    b2->setShortcut(tr("Alt+T"));
    connect(b2,SIGNAL(clicked(bool)),this,SLOT(valid_bon()));
    l->addWidget(b2);
    QTableWidget *t = new QTableWidget;
    t->setObjectName("table");
    t->insertColumn(0);
    t->insertColumn(0);
    t->insertColumn(0);
    t->insertColumn(0);
    l->addRow(t);

    f->showMaximized();

    connect(f,SIGNAL(finished(int)),bc,SLOT(deco_bon()));
    connect(f,SIGNAL(finished(int)),f,SLOT(deleteLater()));
}

void principal::aff_bon()
{
    QDialog *f = this->findChild<QDialog*>("valbon");
    if(f != NULL)
    {
        if(f->findChild<QLineEdit*>("bc")->text().isEmpty())
            return;
        f->findChild<QLineEdit*>("bc")->blockSignals(true);
        QFile file(f->findChild<QLineEdit*>("bc")->text() + ".txt");
        file.open(QIODevice::ReadOnly);
        if(file.atEnd())
        {
            QMessageBox::warning(0,"Erreur","Ce bon de commande n'existe pas");
            return;
        }
        QTableWidget *t = f->findChild<QTableWidget*>("table");
        while(t->rowCount() > 0)
            t->removeRow(0);
        QTextStream flux(&file);
        while(!flux.atEnd())
        {
            QString text = flux.readLine();
            if(text.contains(";"))
            {
                t->insertRow(0);
                t->setItem(0,0,new QTableWidgetItem(text.split(";").at(0)));
                t->setItem(0,1,new QTableWidgetItem(text.split(";").at(1)));
                t->setItem(0,2,new QTableWidgetItem(text.split(";").at(2)));
                t->setItem(0,3,new QTableWidgetItem(text.split(";").at(3)));
            }
        }
    }
}

void principal::total_bon()
{
    QDialog *f = this->findChild<QDialog*>("valbon");
    if(f != NULL && f->findChild<QTableWidget*>("table")->rowCount() != 0)
    {
        if(QMessageBox::question(0,"","Total = quantité livré ?") == QMessageBox::Yes)
        {
            QFile file(f->findChild<QLineEdit*>("bc")->text() + ".txt");
            if(!file.open(QIODevice::WriteOnly | QIODevice::Append))
                QMessageBox::warning(0,"Erreur","Erreur dans le fichier f1");
            QTextStream flux(&file);
            flux << "\r\nLivrée en totalité";
        }

    }

}

void principal::valid_bon()
{
    QDialog *f = this->findChild<QDialog*>("valbon");
    if(f != NULL && f->findChild<QTableWidget*>("table")->rowCount() != 0)
    {
        if(QMessageBox::question(0,"","Ajouter le bon au chantier ?") == QMessageBox::Yes)
        {
            QFile file(f->findChild<QLineEdit*>("bc")->text() + ".txt");
            if(!file.open(QIODevice::WriteOnly | QIODevice::Append))
                QMessageBox::warning(0,"Erreur","Erreur dans le fichier f1");
            QTextStream flux(&file);
            flux << "\r\nAjouté au chantier";

            while(f->findChild<QTableWidget*>("table")->rowCount() > 0)
                f->findChild<QTableWidget*>("table")->removeRow(0);
            for(int i=0;i<f->findChildren<QLineEdit*>().count();i++)
                f->findChildren<QLineEdit*>().at(i)->clear();
            f->findChild<QLineEdit*>("bc")->blockSignals(false);
        }
    }
}

void principal::deco_bon()
{
    this->findChild<QDialog*>("valbon")->findChild<QLineEdit*>("bc")->blockSignals(true);
}
