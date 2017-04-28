#include "cookiejar.h"

QList<QNetworkCookie> CookieJar::cookiesForUrl ( const QUrl & url ) const
{
    qDebug() << url;
    return QNetworkCookieJar::cookiesForUrl(url);
}

bool CookieJar::setCookiesFromUrl ( const QList<QNetworkCookie> & cookieList, const QUrl & url )
{
    qDebug() << url;
    return QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
}

CookieJar::CookieJar(QObject *parent)
    :QNetworkCookieJar(parent)
{
    QFile cookieFile("Cookies.txt");
    if (cookieFile.exists() && cookieFile.open(QIODevice::ReadOnly) )
    {
        QList<QNetworkCookie> list;
        QByteArray line;
        while(!(line = cookieFile.readLine()).isNull())
        {
            list.append(QNetworkCookie::parseCookies(line));
        }
        setAllCookies(list);
    }
}

CookieJar::~CookieJar()
{
    QFile cookieFile("Cookies.txt");
    if (cookieFile.open(QIODevice::WriteOnly))
    {
        QTextStream out(&cookieFile);
        foreach(const QNetworkCookie & cookie, allCookies())
        {
            if (!cookie.isSessionCookie())
            {
               out << cookie.toRawForm() << endl;
            }
        }
    }
}
