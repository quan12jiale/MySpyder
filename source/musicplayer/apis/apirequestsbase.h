#ifndef APIREQUESTSBASE_H
#define APIREQUESTSBASE_H

#include <QtNetwork>
#include <QJsonDocument>
#include <QMessageBox>


struct PlaylistButtonArg
{
    int id;
    QString coverImgUrl;
    QString name;
    PlaylistButtonArg(int id,const QString& coverImgUrl,const QString& name)
        : id(id), coverImgUrl(coverImgUrl), name(name){}
};


class HttpRequest : public QObject
{
    Q_OBJECT
public:
    QNetworkAccessManager* manager;

    HttpRequest();
    ~HttpRequest();
    QByteArray httpRequest(const QString& url,const QString& method="GET",
               QHash<QString,QString> data = QHash<QString,QString>());
    virtual QByteArray search(const QString& s,int offset=0,int limit=100,int stype=1);
};

extern HttpRequest* network_Requests;


class API : public QObject
{
    Q_OBJECT
public:
    API(QObject *parent = nullptr);
};


#endif // APIREQUESTSBASE_H
