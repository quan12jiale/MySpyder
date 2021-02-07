#include "loginframes.h"

InputLine::InputLine(QWidget *parent, int width, int height,
                     const QString &placeholderText)
    : QLineEdit ()
{
    this->parent = parent;
    if (width) {
        this->setMaximumWidth(width);
        this->setMinimumWidth(width);
    }
    if (height) {
        this->setMaximumHeight(height);
        this->setMinimumHeight(height);
    }
    if (!placeholderText.isEmpty())
        this->setPlaceholderText(placeholderText);
}

namespace loginFrames {

QString Header::myStyle = "QLabel {"
                          "margin-left: 8px; "
                          "font-weight: bold;"
                          "font-size: 15px;"
                          "} "
                          "QPushButton {"
                          "border: none;"
                          "font: bold;"
                          "font-size: 13px;"
                          "margin-right: 8px;"
                          "} "
                          "QPushButton:hover{"
                          "color: #DCDDE4;"
                          "}";

Header::Header(const QString& title, QWidget* parent)
    : QFrame ()
{
    this->parent = parent;
    this->setStyleSheet(this->myStyle);

    this->mainLayout = new QHBoxLayout(this);
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->mainLayout->setSpacing(0);

    this->title = new QLabel(title);
    this->mainLayout->addWidget(this->title);
    this->mainLayout->addStretch(1);

    this->closeButton = new QPushButton(tr("x"));
    this->mainLayout->addWidget(this->closeButton);
}

} // namespace loginFrames


/********** LoginBox **********/
LoginBox::LoginBox(Header *parent)
    : QDialog ()
{
    this->parent = parent;

    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setWindowTitle(tr("登陆"));
    this->setObjectName("LoginBox");

    this->resize(520, 400);

    // 可能会有多个渠道登陆的后续扩展。
    this->currentFrame = 0;

    this->mainLayout = new QVBoxLayout(this);
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->mainLayout->setSpacing(0);

    this->phoneAndEMailFrame = new PhoneAndEMailFrame(this);

    this->mainLayout->addWidget(this->phoneAndEMailFrame);
}

void LoginBox::setWarningAndShowIt(const QString &warningStr)
{
    if (!this->currentFrame)
        this->phoneAndEMailFrame->setWarningAndShowIt(warningStr);
}

QPair<QString,QString> LoginBox::checkAndGetLoginInformation() const
{
    if (!this->currentFrame)
        return this->phoneAndEMailFrame->checkAndGetLoginInformation();
    return QPair<QString,QString>();
}


/********** PhoneAndEMailFrame **********/
PhoneAndEMailFrame::PhoneAndEMailFrame(LoginBox* parent)
    : QFrame ()
{
    this->parent = parent;
    this->resize(520, 400);

    this->mainLayout = new QVBoxLayout(this);
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->mainLayout->setSpacing(0);

    this->header = new loginFrames::Header(tr("扫码登录更安全"), this);
    this->header->setMinimumHeight(45);
    connect(this->header->closeButton, SIGNAL(clicked()),
            this->parent, SLOT(accept()));
    this->mainLayout->addWidget(this->header);

    this->mainLayout->addStretch(1);

    this->usernameLine = new InputLine(this, 320, 50, tr("请输入用户名"));
    this->usernameLine->setObjectName("usernameLine");
    QList<QWidget*> widgets;
    widgets.append(this->usernameLine);
    this->usernameCenterBox = new HStretchBox(this->mainLayout, widgets);

    this->mainLayout->addSpacing(10);

    this->passwordLine = new InputLine(this, 320, 50, tr("请输入密码"));
    this->passwordLine->setObjectName("passwordLine");
    widgets.clear();
    widgets.append(this->passwordLine);
    this->passwordCenterBox = new HStretchBox(this->mainLayout, widgets);
    this->passwordLine->setEchoMode(QLineEdit::Password);

    this->warningIconLabel = new QLabel();
    this->warningIconLabel->setObjectName("warningIconLabel");
    this->warningIconLabel->setMaximumSize(14, 14);
    this->warningIconLabel->setMinimumSize(14, 14);
    this->warningIconLabel->hide();
    this->warningLabel = new QLabel(tr("请输入用户名"));
    this->warningLabel->hide();
    this->warningLabel->setObjectName("warningLabel");

    widgets.clear();
    widgets << this->warningIconLabel << this->warningLabel;
    this->warningCenterBox = new HStretchBox(this->mainLayout,
                                             widgets, 1, 2);

    this->mainLayout->addSpacing(30);

    this->enterLoginButton = new QPushButton(tr("登 录"));
    this->enterLoginButton->setObjectName("enterButton");
    this->enterLoginButton->setMaximumSize(320, 50);
    this->enterLoginButton->setMinimumSize(320, 50);
    widgets.clear();
    widgets.append(this->enterLoginButton);
    this->enterLoginCenterBox = new HStretchBox(this->mainLayout, widgets);

    this->mainLayout->addSpacing(30);
    this->mainLayout->addStretch(1);

    QFile file("QSS/phoneAndEMailFrame.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        this->setStyleSheet(in.readAll());
    }
}

QPair<QString,QString> PhoneAndEMailFrame::checkAndGetLoginInformation() const
{
    QPair<QString,QString> pair;
    QString username = this->usernameLine->text();
    QString password = this->passwordLine->text();

    if (username.isEmpty() || password.isEmpty())
        this->warningIconLabel->show();

    if (username.isEmpty()) {
        this->warningLabel->setText(tr("请输入用户名"));
        this->warningLabel->show();
        return pair;
    }

    if (password.isEmpty()) {
        this->warningLabel->setText(tr("请输入密码"));
        this->warningLabel->show();
        return pair;
    }

    this->warningIconLabel->hide();
    this->warningLabel->hide();
    pair.first = username;
    pair.second = password;
    return pair;
}

void PhoneAndEMailFrame::setWarningAndShowIt(const QString &warningStr)
{
    this->warningLabel->setText(warningStr);

    this->warningLabel->show();
    this->warningIconLabel->show();
}

static void test()
{
    LoginBox* box = new LoginBox(nullptr);
    box->show();
}
