#ifndef ERROR_H
#define ERROR_H

#include <QString>
#include <QObject>

typedef enum {
    no_Error,//0
    open_File,//1
    bad_Username,//2
    bad_Login,//3
    load,//4
    fail_check,//5
    variable,//6
    requete,//7
    too_many,//8
    Not_BC,//9
    BC,//10
    BL,//11
    Run_Esabora,//12
    Traitement,//13
    Window,//14
    Focus,//15
    Mouse//16
}ERR;

class Error
{
public:
    Error();
    QString Err(int code, QString e = 0, QString fromClass = 0);

};

#endif // ERROR_H
