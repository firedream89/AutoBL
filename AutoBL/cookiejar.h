#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QtGui>
#include <QApplication>

#include <QNetworkCookieJar>
#include <QNetworkCookie>

class CookieJar : public QNetworkCookieJar
{
    public:
        CookieJar(QObject *parent);
        ~CookieJar();
        QList<QNetworkCookie> cookiesForUrl ( const QUrl & url ) const;
        bool setCookiesFromUrl ( const QList<QNetworkCookie> & cookieList, const QUrl & url );
};

#endif
