#pragma once

#include <QHash>
#include <QStringList>

// from urllib.request import build_opener, HTTPSHandler, HTTPError, Request
// 因此该文件很难实现

class GitHub
{
public:
    int x_ratelimit_remaining;
    int x_ratelimit_limit;
    int x_ratelimit_reset;
    QString _authorization;
    QString _client_id;
    QString _client_secret;//这些变量的类型都不确定
    QString _redirect_uri;
    QString _scope;

    GitHub(QString username=QString(), QString password=QString(), QString access_token=QString(),
           QString client_id=QString(), QString client_secret=QString(),
           QString redirect_uri=QString(), QString scope=QString());
    //void authorize_url(const QString& state=QString());
    //QString get_access_token(const QString& code, const QString& state=QString());
    //void __getattr__(self, attr);
    //void _http(const QString& _method, const QString& _path);//, **kw
    bool _process_resp(const QHash<QString, QString>& headers);
};

