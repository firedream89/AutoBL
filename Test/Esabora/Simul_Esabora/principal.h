#ifndef PRINCIPAL_H
#define PRINCIPAL_H

#include <QMainWindow>

namespace Ui {
class principal;
}

class principal : public QMainWindow
{
    Q_OBJECT

public:
    explicit principal(QWidget *parent = 0);
    ~principal();

private slots:
    void login();
    void create_Bon();
    void find_chantier();
    void save_new_bl();
    void show_Bon(QString text);
    void validation_Bon();
    void add_row();
    void save_bon();
    void total_bon();
    void valid_bon();
    void aff_bon();
    void deco_bon();
    void list_bon();

private:
    void connection();

    Ui::principal *ui;
};

#endif // PRINCIPAL_H
