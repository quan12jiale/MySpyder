#include "ipythonconsole.h"

QString jupyter_runtime_dir()
{
    return QDir::homePath() + "/AppData/Roaming/jupyter/runtime";
}

KernelConnectionDialog::KernelConnectionDialog(QWidget* parent)
    : QDialog (parent)
{
    this->setWindowTitle("Connect to an existing kernel");

    QLabel* main_label = new QLabel("Please select the JSON connection file (<i>e.g.</i> "
                                    "<tt>kernel-3764.json</tt>) or enter the 4-digit ID (<i>e.g.</i> "
                                    "<tt>3764</tt>) of the existing kernel to connect to, "
                                    "and enter the SSH host name and credentials if a remote kernel.");
    main_label->setWordWrap(true);
    main_label->setAlignment(Qt::AlignJustify);

    QLabel* cf_label = new QLabel("Kernel ID/Connection file:");
    this->cf = new QLineEdit;
    this->cf->setPlaceholderText("ID number or path to connection file");
    this->cf->setMinimumWidth(250);
    QPushButton* cf_open_btn = new QPushButton("Browse");
    connect(cf_open_btn, SIGNAL(clicked()), SLOT(select_connection_file()));

    QHBoxLayout* cf_layout = new QHBoxLayout;
    cf_layout->addWidget(cf_label);
    cf_layout->addWidget(this->cf);
    cf_layout->addWidget(cf_open_btn);

    this->rm_cb = new QCheckBox("This is a remote kernel (via SSH)");

    credential_label = new QLabel("<b>Note:</b> If connecting to a remote kernel, only the "
                                          "SSH keyfile <i>or</i> the Password field need to be completed, "
                                          "unless the keyfile is protected with a passphrase.");
    credential_label->setWordWrap(true);

    this->hn = new QLineEdit;
    this->hn->setPlaceholderText("username@hostname:port");

    this->pw = new QLineEdit;
    this->pw->setPlaceholderText("Remote user password or "
                                 "SSH keyfile passphrase");
    this->pw->setEchoMode(QLineEdit::Password);

    this->kf = new QLineEdit;
    this->kf->setPlaceholderText("Path to SSH keyfile (optional)");
    kf_open_btn = new QPushButton("Browse");
    connect(kf_open_btn, SIGNAL(clicked()), SLOT(select_ssh_key()));

    QHBoxLayout* kf_layout = new QHBoxLayout;
    kf_layout->addWidget(this->kf);
    kf_layout->addWidget(kf_open_btn);

    ssh_form = new QFormLayout;
    ssh_form->addRow("Host name:", this->hn);
    ssh_form->addRow("Password:", this->pw);
    ssh_form->addRow("SSH keyfile:", kf_layout);

    accept_btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                       Qt::Horizontal, this);

    connect(accept_btns, SIGNAL(accepted()), SLOT(accept()));
    connect(accept_btns, SIGNAL(rejected()), SLOT(reject()));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(main_label);
    layout->addLayout(cf_layout);
    layout->addWidget(this->rm_cb);
    layout->addWidget(credential_label);
    layout->addLayout(ssh_form);
    layout->addWidget(this->accept_btns);

    ssh_set_enabled(this->rm_cb->checkState());
    connect(rm_cb, SIGNAL(stateChanged(int)), SLOT(ssh_set_enabled(int)));
}

void KernelConnectionDialog::ssh_set_enabled(int state)
{
    QList<QWidget*> widgets;
    widgets << credential_label << this->hn << this->pw
            << this->kf << kf_open_btn;
    foreach (QWidget* wid, widgets)
        wid->setEnabled(state);
    for (int i = 0; i < ssh_form->rowCount(); ++i)
        ssh_form->itemAt(2*i)->widget()->setEnabled(state);
}

void KernelConnectionDialog::select_connection_file()
{
    QString cf = QFileDialog::getOpenFileName(this, "Select kernel connection file",
                                              jupyter_runtime_dir(), "*.json;;*.*");
    this->cf->setText(cf);
}

void KernelConnectionDialog::select_ssh_key()
{
    QString kf = QFileDialog::getOpenFileName(this, "Select SSH keyfile",
                                              get_home_dir(), "*.pem;;*.*");
    this->kf->setText(kf);
}

Connect_Parameters KernelConnectionDialog::get_connection_parameters
    (QWidget* parent, KernelConnectionDialog* dialog)
{
    if (!dialog)
        dialog = new KernelConnectionDialog(parent);
    dialog->exec();
    bool is_remote = dialog->rm_cb->checkState();
    bool accepted = QDialog::Accepted;
    if (is_remote) {
        return Connect_Parameters(dialog->cf->text(),
                                  dialog->hn->text(),
                                  dialog->kf->text(),
                                  dialog->pw->text(),
                                  accepted);
    }
    else {
        QString path = dialog->cf->text();
        QFileInfo info(path);
        QString dir = info.absolutePath();
        QString filename = info.fileName();
        if (dir == "" && !filename.endsWith(".json"))
            path = jupyter_runtime_dir() + "/kernel-" + path+".json";
        return Connect_Parameters(path,
                                  QString(),
                                  QString(),
                                  QString(),
                                  accepted);
    }
}


/********** IPythonConsole **********/
IPythonConsole::IPythonConsole()
{

}


void IPythonConsole::run_script(QString filename, QString wdir, QString args, bool debug,
                                bool post_mortem, bool current_client, bool clear_variables)
{
    QFileInfo info(filename);
    bool is_cython = info.suffix() == "pyx";
    if (is_cython)
        current_client = false;

    QString line = QString("%1('%2'")
            .arg(debug ? "debugfile" : "runfile").arg(filename);
    if (!args.isEmpty())
        line += QString(", args='%1'").arg(args);
    if (!wdir.isEmpty())
        line += QString(", wdir='%1'").arg(wdir);
    if (post_mortem)
        line += ", post_mortem=True";
    line += ")";

    this->execute_code(line, current_client, clear_variables);
}

void IPythonConsole::execute_code(const QString &lines, bool current_client,
                                  bool clear_variables)
{

}

static void test()
{
    KernelConnectionDialog::get_connection_parameters(nullptr, nullptr);
}
