#include "infowindow.h"

InfoWindow::InfoWindow(QWidget *parent,QString windowTitle, int type)
{
    if(parent->findChild<QDialog*>("InfoWindow") != NULL)
        w = parent->findChild<QDialog*>("InfoWindow");
    else
    {
        if(type == 1)
            w = new QDialog(parent,Qt::Tool | Qt::CustomizeWindowHint);
        else
            w = new QDialog(parent);
        w->setObjectName("InfoWindow");
        w->setWindowTitle(windowTitle);
        QFormLayout *l = new QFormLayout(w);
    }
}

InfoWindow::~InfoWindow()
{

}

void InfoWindow::Add_Label(QString name, QString text)
{
    QFormLayout *l = w->findChild<QFormLayout*>();
    if(l == NULL)
    {
        DEBUG << "InfoWindow | layout non trouvé !";
        return;
    }
    QLabel *lb = new QLabel;
    lb->setObjectName(name);
    lb->setText(text);
    l->addRow(name,lb);
}

void InfoWindow::Update_Label(QString label, QString text)
{
    QLabel *l = w->findChild<QLabel*>(label);
    if(l == NULL)
    {
        DEBUG << "InfoWindow | label non trouvé !";
        return;
    }
    l->setText(text);
}

void InfoWindow::Close()
{
    w->close();
    w->deleteLater();
}
