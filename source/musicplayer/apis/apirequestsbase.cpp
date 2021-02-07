#include "apirequestsbase.h"

HttpRequest::HttpRequest()
    : QObject ()
{
    this->manager = new QNetworkAccessManager;
}

HttpRequest::~HttpRequest()
{
    delete this->manager;
}

QByteArray HttpRequest::httpRequest(const QString &url, const QString &method,
                              QHash<QString, QString> data)
{
    QEventLoop loop;
    connect(this->manager, SIGNAL(finished(QNetworkReply *)),
            &loop, SLOT(quit()));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.101 Safari/537.36");
    request.setRawHeader("Cookie", "os=pc");//加上这一行fnewsong()函数才能正常运行

    QNetworkReply* reply = nullptr;
    if (method.toUpper() == "GET") {
        reply = this->manager->get(request);
    }
    else if (method.toUpper() == "POST") {
        if (!data.isEmpty()) {
            QUrlQuery post_data;
            foreach (const QString& key, data.keys()) {
                QString val = data[key];
                // 必须得toPercentEncoding
                post_data.addQueryItem(key, val.toUtf8().toPercentEncoding());
            }
            reply = manager->post(request, post_data.toString().toUtf8());
        }
    }
    loop.exec();
    disconnect(this->manager, SIGNAL(finished(QNetworkReply *)),
            &loop, SLOT(quit()));

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray json;

    switch (statusCode) {
    case 200:
    {
        json = reply->readAll();
    }
        break;
    case 0:
    {
        //qDebug() << "网络错误";
    }
        break;
    default:
    {
        qDebug() << "statusCode = " << statusCode;
    }
        break;
    }

    reply->deleteLater();

    return json;
}

QByteArray HttpRequest::search(const QString &, int, int, int)
{
    throw "HttpRequest search method shouldn't be call";
}

HttpRequest* network_Requests = new HttpRequest;


/********** API **********/
API::API(QObject *parent) : QObject(parent)
{
    QNetworkRequest r;
    r.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    r.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
    r.setRawHeader("Connection", "keep-alive");
    r.setRawHeader("Pragma", "no-cache");
    r.setRawHeader("Cache-Control", "no-cache");
    r.setRawHeader("Accept-Language", "zh-CN,zh;q=0.8");
    r.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.101 Safari/537.36");

    QUrlQuery post_data;
    post_data.addQueryItem("params",QByteArray("jvU/YndD9WTVqDHXLIGEzH6cdiNibBz3nWZuI1gEdUIHzekECSLyBZ3mstCkPuva/mzg7FZyjq9YXIkj/MBnNf4+nTUARYpBfP0eB3gVvZtZ52T5OSs5eXvJwgBLz3mPNewrSyND/lIFQf6ZH9lxZ2DTifzyCXoArgVJBkZ8ezF8DRu40044+ROqsTbj+PchgH/ed1Xqmk6v2GngTb1iRw==").toPercentEncoding());
    post_data.addQueryItem("encSecKey",QByteArray("0bf2af769d66c155cc84e4ef800d97d69c3717cca72787b4957c4a66a90e1580a65c518060b65b0ed2887a2117533dd74da2ad91abb5269b408250febb6830893326b914de4f5223b356c0883eee08796f0b831254dddb2407a0995c02f6ee2d8d2907fab295a986a37e24d3475690334806f2b052aaabb59f14e866cd331b46").toPercentEncoding());

    r.setUrl(QUrl("http://music.163.com/weapi/login/cellphone?csrf_token="));

    QNetworkAccessManager *m = new QNetworkAccessManager(this);

    connect(m,&QNetworkAccessManager::finished,this,[=](QNetworkReply *reply){
        qDebug() << "rec:";
        QByteArray res = reply->readAll();

        qDebug() << "read:" <<QTextCodec::codecForName("UTF-8")->toUnicode(res);
    });

    qDebug() << "post";
    m->post(r,post_data.toString().toUtf8());
}
