#pragma once

#include "config/config_main.h"
#include "config/base.h"
#include "utils/external/github.h"
#include "widgets/github/gh_login.h"

class BaseBackend
{
public:
    QString formatter;
    QString button_text;
    QString button_tooltip;
    QIcon button_icon;
    bool need_review;
    QWidget* parent_widget;

    BaseBackend(const QString& formatter, const QString& button_text, const QString& button_tooltip,
                const QIcon& button_icon=QIcon(), bool need_review=true);
    void set_formatter(const QString& formatter);
    virtual bool send_report(const QString& title, const QString& body, const QString& application_log=QString()) = 0;
};

struct Credentials
{
    QString username;
    bool remember_me;
    bool remember_token;
    Credentials(const QString& _username, bool _remember_me, bool _remember_token)
    {
        username = _username;
        remember_me = _remember_me;
        remember_token = _remember_token;
    }
};

class GithubBackend : public BaseBackend
{
public:
    QString gh_owner;
    QString gh_repo;
    bool _show_msgbox;

    GithubBackend(const QString& gh_owner, const QString& gh_repo, const QString& formatter=QString());
    //bool send_report(const QString& title, const QString& body, const QString& application_log=QString());
    Credentials _get_credentials_from_settings();
    void _store_credentials(const QString& username, const QString& password, bool remember=false);
    void _store_token(const QString& token, bool remember=false);
    QHash<QString, QVariant> get_user_credentials();
};
