#ifndef INFOWINDOW_H
#define INFOWINDOW_H

#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QDebug>

#define DEBUG qDebug()

class InfoWindow: public QObject
{
    Q_OBJECT
public:
    InfoWindow(QWidget *parent, QString windowTitle, int type = 0);
    ~InfoWindow();
    void Add_Label(QString name,QString text = 0);
    void Update_Label(QString label, QString text);
    void Close();

private:
    QDialog *w;
};

#endif // INFOWINDOW_H
