#ifndef IPYTHONCONSOLE_H
#define IPYTHONCONSOLE_H

#include "config/base.h"
#include "plugins/plugins.h"

struct Connect_Parameters
{
    QString connection_file;
    QString hostname;
    QString sshkey;
    QString password;
    bool ok;
    Connect_Parameters(QString _connection_file, QString _hostname,
                       QString _sshkey, QString _password, bool _ok)
    {
        connection_file = _connection_file;
        hostname = _hostname;
        sshkey = _sshkey;
        password = _password;
        ok = _ok;
    }
};

class KernelConnectionDialog : public QDialog
{
    Q_OBJECT
public:
    QLineEdit* cf;
    QCheckBox* rm_cb;
    QLineEdit* hn;
    QLineEdit* pw;
    QLineEdit* kf;
    QDialogButtonBox* accept_btns;

    QLabel* credential_label;
    QPushButton* kf_open_btn;
    QFormLayout* ssh_form;

    KernelConnectionDialog(QWidget* parent=nullptr);

    static Connect_Parameters get_connection_parameters
    (QWidget* parent=nullptr, KernelConnectionDialog* dialog=nullptr);

public slots:
    void ssh_set_enabled(int state);
    void select_connection_file();//
    void select_ssh_key();//
};

class IPythonConsole : QWidget
{
    Q_OBJECT
public:
    IPythonConsole();
    void execute_code(const QString& lines, bool current_client=true, bool clear_variables=false);

public slots:
    void run_script(QString filename, QString wdir, QString args, bool debug,
                    bool post_mortem, bool current_client, bool clear_variables);

};

#endif // IPYTHONCONSOLE_H
