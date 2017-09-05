#include "infowindow.h"

InfoWindow::InfoWindow(QWidget *parent,QString windowTitle, int type)
{
    if(parent->findChild<QDialog*>("InfoWindow") != NULL)
    {
        w = parent->findChild<QDialog*>("InfoWindow");
    }
    else
    {
        if(type == 1)
        {
            w = new QDialog(parent,Qt::Tool | Qt::CustomizeWindowHint);
        }
        else
        {
            w = new QDialog(parent);
        }
        w->setObjectName("InfoWindow");
        w->setWindowTitle(windowTitle);
        QFormLayout *l = new QFormLayout(w);
        lw = 1000;
    }
}

InfoWindow::~InfoWindow()
{
    w->close();
    w->deleteLater();
}

void InfoWindow::Add_Label(QString name, bool row)
{
    QFormLayout *l = w->findChild<QFormLayout*>();
    if(l == NULL)
    {
        DEBUG << "InfoWindow | layout non trouvé !";
        return;
    }
    QLabel *lb = new QLabel;
    lb->setObjectName(name);
    if(row)
    {
        l->addRow(name,lb);
    }
    else
    {
        l->addWidget(lb);
    }
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


    if(w->findChild<QFormLayout*>()->rowCount() == 1)
    {
        qDebug() << l->width() << w->width() << lw << w->findChild<QFormLayout*>()->rowCount();
        if(lw < l->width())
        {
            qDebug() << "Update size";
            w->setMinimumWidth(l->width()+50);
        }
        lw = l->width();
    }
}

QString InfoWindow::Get_Label_Text(QString label) const
{
    if(w->findChild<QLabel*>(label) != NULL)
    {
        return w->findChild<QLabel*>(label)->text();
    }
    else
    {
        return QString();
    }
}

void InfoWindow::Show() const
{
    w->show();
}

void InfoWindow::Close() const
{
    w->close();
    w->deleteLater();
}
