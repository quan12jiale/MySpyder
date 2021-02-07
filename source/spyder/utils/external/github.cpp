#include "github.h"

GitHub::GitHub(QString username, QString password, QString access_token,
               QString client_id, QString client_secret,
               QString redirect_uri, QString scope)
{
    this->x_ratelimit_remaining = -1;
    this->x_ratelimit_limit = -1;
    this->x_ratelimit_reset = -1;
    if (!username.isEmpty() && !password.isEmpty()) {
        QByteArray byte_array = QString("%1:%2").arg(username).arg(password).toUtf8();
        QString userandpass = byte_array.toBase64();
        this->_authorization = QString("Basic %1").arg(userandpass);
    }
    else
        this->_authorization = QString("token %1").arg(access_token);
    this->_client_id = client_id;
    this->_client_secret = client_secret;
    this->_redirect_uri = redirect_uri;
    this->_scope = scope;
}

bool GitHub::_process_resp(const QHash<QString, QString> &headers)
{
    bool is_json = false;
    if (!headers.empty()) {
        foreach (QString k, headers.keys()) {
            QString h = k.toLower();
            if (h == "x-ratelimit-remaining")
                this->x_ratelimit_remaining = headers[k].toInt();
            else if (h == "x-ratelimit-limit")
                this->x_ratelimit_limit = headers[k].toInt();
            else if (h == "x-ratelimit-reset")
                this->x_ratelimit_reset = headers[k].toInt();
            else if (h == "content-type")
                is_json = headers[k].startsWith("application/json");
        }
    }
    return is_json;
}
