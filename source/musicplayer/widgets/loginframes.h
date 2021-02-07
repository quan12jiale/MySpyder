#ifndef LOGINFRAMES_H
#define LOGINFRAMES_H

#include "widgets/base.h"

class Header;

class InputLine : public QLineEdit
{
    Q_OBJECT
public:
    QWidget* parent;
    InputLine(QWidget* parent,int width=0,int height=0,
              const QString& placeholderText=QString());
};

namespace loginFrames {

class Header : public QFrame
{
    Q_OBJECT
public:
    static QString myStyle;
    QWidget* parent;
    QHBoxLayout* mainLayout;
    QLabel* title;
    QPushButton* closeButton;
    Header(const QString& title, QWidget* parent);
};

} // namespace loginFrames

class PhoneAndEMailFrame;

class LoginBox : public QDialog
{
    Q_OBJECT
public:
    Header* parent;
    int currentFrame;
    QVBoxLayout* mainLayout;
    PhoneAndEMailFrame* phoneAndEMailFrame;

    LoginBox(Header* parent);
    void setWarningAndShowIt(const QString& warningStr);
    QPair<QString,QString> checkAndGetLoginInformation() const;
};


class PhoneAndEMailFrame : public QFrame
{
    Q_OBJECT
public:
    LoginBox* parent;
    QVBoxLayout* mainLayout;
    loginFrames::Header* header;

    InputLine* usernameLine;
    HStretchBox* usernameCenterBox ;
    InputLine* passwordLine;
    HStretchBox* passwordCenterBox;

    QLabel* warningIconLabel;
    QLabel* warningLabel;
    HStretchBox* warningCenterBox;
    QPushButton* enterLoginButton;
    HStretchBox* enterLoginCenterBox;

    PhoneAndEMailFrame(LoginBox* parent);
    QPair<QString,QString> checkAndGetLoginInformation() const;
    void setWarningAndShowIt(const QString& warningStr);
};

#endif // LOGINFRAMES_H
