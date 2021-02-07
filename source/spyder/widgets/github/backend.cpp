#include "backend.h"

BaseBackend::BaseBackend(const QString &formatter, const QString &button_text, const QString &button_tooltip,
                         const QIcon &button_icon, bool need_review)
{
    this->formatter = formatter;
    this->button_text = button_text;
    this->button_tooltip = button_tooltip;
    this->button_icon = button_icon;
    this->need_review = need_review;
    this->parent_widget = nullptr;
}

void BaseBackend::set_formatter(const QString &formatter)
{
    this->formatter = formatter;
}


GithubBackend::GithubBackend(const QString& gh_owner, const QString& gh_repo, const QString& formatter)
    : BaseBackend (formatter, "Submit on github", "Submit the issue on our issue tracker on github")
{
    this->gh_owner = gh_owner;
    this->gh_repo = gh_repo;
    this->_show_msgbox = true;
}



Credentials GithubBackend::_get_credentials_from_settings()
{
    bool remember_me = CONF_get("main", "report_error/remember_me").toBool();
    bool remember_token = CONF_get("main", "report_error/remember_token").toBool();
    QString username = CONF_get("main", "report_error/username", QString("")).toString();
    if (! remember_me)
        username = "";
    return Credentials(username, remember_me, remember_token);
}

void GithubBackend::_store_credentials(const QString &username, const QString &password, bool remember)
{
    if (!username.isEmpty() && !password.isEmpty() && remember) {
        CONF_set("main", "report_error/username", username);
        // keyring.set_password('github', username, password)
        if (this->_show_msgbox)
            QMessageBox::warning(this->parent_widget, "Failed to store password",
                                 "It was not possible to securely "
                                 "save your password. You will be "
                                 "prompted for your Github "
                                 "credentials next time you want "
                                 "to report an issue.");
        remember = false;

        CONF_set("main", "report_error/remember_me", remember);
    }
}

void GithubBackend::_store_token(const QString &token, bool remember)
{
    if (!token.isEmpty() && remember) {
        // keyring.set_password('github', 'token', token)
        if (this->_show_msgbox)
            QMessageBox::warning(this->parent_widget, "Failed to store token",
                                 "It was not possible to securely "
                                 "save your token. You will be "
                                 "prompted for your Github token "
                                 "next time you want to report "
                                 "an issue.");
        remember = false;

        CONF_set("main", "report_error/remember_token", remember);
    }
}

QHash<QString, QVariant> GithubBackend::get_user_credentials()
{
    QString password;
    QString token;
    Credentials tmp = this->_get_credentials_from_settings();
    QString username = tmp.username;
    bool remember_me = tmp.remember_me;
    bool remember_token = tmp.remember_token;
    bool valid_py_os = true;
    if (!username.isEmpty() && remember_me && valid_py_os) {
        if (this->_show_msgbox)
            QMessageBox::warning(this->parent_widget,
                                 "Failed to retrieve password",
                                 "It was not possible to retrieve "
                                 "your password. Please introduce "
                                 "it again.");
    }

    if (remember_token && valid_py_os) {
        if (this->_show_msgbox)
            QMessageBox::warning(this->parent_widget,
                                 "Failed to retrieve token",
                                 "It was not possible to retrieve "
                                 "your token. Please introduce it "
                                 "again.");
    }
    QHash<QString, QVariant> credentials;
    if (!running_under_pytest()) {
        credentials = DlgGitHubLogin::login(this->parent_widget,
                                            username,
                                            password,
                                            token,
                                            remember_me,
                                            remember_token);
        if (!credentials["username"].toString().isEmpty() &&
                !credentials["password"].toString().isEmpty() && valid_py_os) {
            this->_store_credentials(credentials["username"].toString(),
                    credentials["password"].toString(),
                    credentials["remember"].toBool());
            CONF_set("main", "report_error/remember_me", credentials["remember"].toBool());
        }
        if (!credentials["token"].toString().isEmpty() && valid_py_os) {
            this->_store_token(credentials["token"].toString(),
                    credentials["remember_token"].toBool());
            CONF_set("main", "report_error/remember_token",
                     credentials["remember_token"].toBool());
        }
    }
    else {
        credentials["username"] = username;
        credentials["password"] = password;
        credentials["token"] = "";
        credentials["remember"] = remember_me;
        credentials["remember_token"] = remember_token;
    }
    return credentials;
}
