#include "gh_login.h"

static QString TOKEN_URL = "https://github.com/settings/tokens/new?scopes=public_repo";

DlgGitHubLogin::DlgGitHubLogin(QWidget* parent, QString username, QString password,
               QString token, bool remember, bool remember_token)
    : QDialog (parent)
{
    QString title = "Sign in to Github";
    this->resize(415, 375);
    this->setWindowTitle(title);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QString html = "<html><head/><body><p align=\"center\">"
                   "%1</p></body></html>";
    QLabel* lbl_html = new QLabel(html.arg(title));
    lbl_html->setStyleSheet("font-size: 16px;");

    this->tabs = new QTabWidget;

    QFormLayout* basic_form_layout = new QFormLayout;
    basic_form_layout->setContentsMargins(-1, 0, -1, -1);

    QLabel* basic_lbl_msg = new QLabel("For regular users, i.e. users <b>without</b>"
                                       " two-factor authentication enabled");
    basic_lbl_msg->setWordWrap(true);
    basic_lbl_msg->setAlignment(Qt::AlignJustify);

    QLabel* lbl_user = new QLabel("Username:");
    basic_form_layout->setWidget(0, QFormLayout::LabelRole, lbl_user);
    this->le_user = new QLineEdit;
    connect(le_user, SIGNAL(textChanged(QString)), this, SLOT(update_btn_state()));
    basic_form_layout->setWidget(0, QFormLayout::FieldRole, this->le_user);
    basic_form_layout->addRow("", new QWidget);

    QLabel* lbl_password = new QLabel("Password: ");
    basic_form_layout->setWidget(2, QFormLayout::LabelRole, lbl_password);
    this->le_password = new QLineEdit;
    this->le_password->setEchoMode(QLineEdit::Password);
    connect(le_password, SIGNAL(textChanged(QString)), this, SLOT(update_btn_state()));
    basic_form_layout->setWidget(2, QFormLayout::FieldRole, this->le_password);

    this->cb_remember = nullptr;

    bool valid_py_os = true;
    if (this->is_keyring_available() && valid_py_os) {
        this->cb_remember = new QCheckBox("Remember me");
        this->cb_remember->setToolTip("Spyder will save your credentials safely");
        this->cb_remember->setChecked(remember);
        basic_form_layout->setWidget(4, QFormLayout::FieldRole, this->cb_remember);
    }

    QWidget* basic_auth = new QWidget;
    QVBoxLayout* basic_layout = new QVBoxLayout;
    basic_layout->addSpacerItem(new QSpacerItem(QSpacerItem(0, 8)));
    basic_layout->addWidget(basic_lbl_msg);
    basic_layout->addSpacerItem(new QSpacerItem(QSpacerItem(0, 1000, QSizePolicy::Minimum, QSizePolicy::Expanding)));
    basic_layout->addLayout(basic_form_layout);
    basic_layout->addSpacerItem(new QSpacerItem(QSpacerItem(0, 1000, QSizePolicy::Minimum, QSizePolicy::Expanding)));
    basic_auth->setLayout(basic_layout);
    this->tabs->addTab(basic_auth, "Password Only");

    QFormLayout* token_form_layout = new QFormLayout;
    token_form_layout->setContentsMargins(-1, 0, -1, -1);

    QLabel* token_lbl_msg = new QLabel(QString("For users <b>with</b> two-factor "
                                               "authentication enabled, or who prefer a "
                                               "per-app token authentication.<br><br>"
                                               "You can go <b><a href=\"%1\">here</a></b> "
                                               "and click \"Generate token\" at the bottom "
                                               "to create a new token to use for this, with "
                                               "the appropriate permissions.").arg(TOKEN_URL));
    token_lbl_msg->setOpenExternalLinks(true);
    token_lbl_msg->setWordWrap(true);
    token_lbl_msg->setAlignment(Qt::AlignJustify);

    QLabel* lbl_token = new QLabel("Token: ");
    token_form_layout->setWidget(1, QFormLayout::LabelRole, lbl_token);
    this->le_token = new QLineEdit;
    this->le_token->setEchoMode(QLineEdit::Password);
    connect(le_token, SIGNAL(textChanged()), this, SLOT(update_btn_state()));
    token_form_layout->setWidget(1, QFormLayout::FieldRole, this->le_token);

    this->cb_remember_token = nullptr;
    if (this->is_keyring_available() && valid_py_os) {
        this->cb_remember_token = new QCheckBox("Remember token");
        this->cb_remember_token->setToolTip("Spyder will save your token safely");
        this->cb_remember_token->setChecked(remember_token);
        basic_form_layout->setWidget(3, QFormLayout::FieldRole, this->cb_remember_token);
    }

    QWidget* token_auth = new QWidget;
    QVBoxLayout* token_layout = new QVBoxLayout;
    token_layout->addSpacerItem(new QSpacerItem(QSpacerItem(0, 8)));
    token_layout->addWidget(token_lbl_msg);
    token_layout->addSpacerItem(new QSpacerItem(QSpacerItem(0, 1000, QSizePolicy::Minimum, QSizePolicy::Expanding)));
    token_layout->addLayout(token_form_layout);
    token_layout->addSpacerItem(new QSpacerItem(QSpacerItem(0, 1000, QSizePolicy::Minimum, QSizePolicy::Expanding)));
    token_auth->setLayout(token_layout);
    this->tabs->addTab(token_auth, "Access Token");

    this->bt_sign_in = new QPushButton("Sign in");
    connect(bt_sign_in, SIGNAL(clicked(bool)), this, SLOT(accept()));
    this->bt_sign_in->setDisabled(true);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(lbl_html);
    layout->addWidget(this->tabs);
    layout->addWidget(this->bt_sign_in);
    this->setLayout(layout);

    if (!username.isEmpty() && !password.isEmpty()) {
        this->le_user->setText(username);
        this->le_password->setText(password);
        this->bt_sign_in->setFocus();
    }
    else if (!username.isEmpty()) {
        this->le_user->setText(username);
        this->le_password->setFocus();
    }
    else if (!token.isEmpty())
        this->le_token->setText(token);
    else
        this->le_user->setFocus();

    this->setFixedSize(this->width(), this->height());
    this->le_password->installEventFilter(this);
    this->le_user->installEventFilter(this);
    connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(update_btn_state()));
}

bool DlgGitHubLogin::eventFilter(QObject *obj, QEvent *event)
{
    QList<QObject*> interesting_objects;
    interesting_objects << this->le_password << this->le_user;
    if (interesting_objects.contains(obj) && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyevent = dynamic_cast<QKeyEvent*>(event);
        if (keyevent->key() == Qt::Key_Return &&
                (keyevent->modifiers() & Qt::ControlModifier) &&
                this->bt_sign_in->isEnabled()) {
            this->accept();
            return true;
        }
    }
    return false;
}

void DlgGitHubLogin::update_btn_state()
{
    bool user = this->le_user->text().trimmed() != "";
    bool password = this->le_password->text().trimmed() != "";
    bool token = this->le_token->text().trimmed() != "";
    bool enable = ((user && password &&
                    this->tabs->currentIndex() == 0) ||
                   (token && this->tabs->currentIndex() == 1));
    this->bt_sign_in->setEnabled(enable);
}

bool DlgGitHubLogin::is_keyring_available() const
{
    /* Check if keyring is available for password storage.
     * try:
           import keyring  # analysis:ignore
           return True
    */
    return false;
}

QHash<QString, QVariant> DlgGitHubLogin::login(QWidget *parent, QString username, QString password, QString token,
                           bool remember, bool remember_token)
{
    DlgGitHubLogin* dlg = new DlgGitHubLogin(parent, username, password, token, remember, remember_token);
    QHash<QString, QVariant> credentials;
    if (dlg->exec() == dlg->Accepted) {
        QString user = dlg->le_user->text();
        QString password = dlg->le_password->text();
        QString token = dlg->le_token->text();
        bool remember, remember_token;
        if (dlg->cb_remember)
            remember = dlg->cb_remember->isChecked();
        else
            remember = false;
        if (dlg->cb_remember_token)
            remember_token = dlg->cb_remember_token->isChecked();
        else
            remember_token = false;

        credentials["username"] = user;
        credentials["password"] = password;
        credentials["token"] = token;
        credentials["remember"] = remember;
        credentials["remember_token"] = remember_token;
        return credentials;
    }
    credentials["username"] = QString();
    credentials["password"] = QString();
    credentials["token"] = QString();
    credentials["remember"] = false;
    credentials["remember_token"] = false;
    return credentials;
}


static void test()
{
    DlgGitHubLogin* dlg = new DlgGitHubLogin(nullptr, QString(), QString(), QString());
    dlg->show();
    dlg->exec();
}
