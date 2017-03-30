#ifndef ERROR_H
#define ERROR_H

#include <QString>
#include <QObject>

typedef enum {
    no_Error,
    open_File,
    bad_Username,
    bad_Login,
    load,
    fail_check,
    variable,
    requete,
    too_many
}ERR;

class Error
{
public:
    Error();
    QString Err(int code, QString e = 0);
};

#endif // ERROR_H
