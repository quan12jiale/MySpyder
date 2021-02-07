#pragma once

#include <QtWidgets>


class DlgGitHubLogin : public QDialog
{
    Q_OBJECT
public:
    QTabWidget* tabs;
    QLineEdit* le_user;
    QLineEdit* le_password;
    QCheckBox* cb_remember;
    QLineEdit* le_token;
    QCheckBox* cb_remember_token;
    QPushButton* bt_sign_in;

    DlgGitHubLogin(QWidget* parent, QString username, QString password,
                   QString token, bool remember=false, bool remember_token=false);
    bool eventFilter(QObject* obj, QEvent* event);
    bool is_keyring_available() const;

    static QHash<QString, QVariant> login(QWidget* parent, QString username, QString password,
                      QString token, bool remember, bool remember_token);
public slots:
    void update_btn_state();
};
