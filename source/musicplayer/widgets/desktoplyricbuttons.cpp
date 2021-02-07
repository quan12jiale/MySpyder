#include "desktoplyricbuttons.h"

Button::Button(QWidget *parent)
    : QPushButton (parent)
{
    this->status = 0;
}

void Button::loadPixmap(const QString &pic_name)
{
    this->pixmap = QPixmap(pic_name);
    this->btn_width = this->pixmap.width();
    this->btn_height = this->pixmap.height()/6;
    this->setFixedSize(this->btn_width, this->btn_height);
}

void Button::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->status = 2;
        this->update();
        emit clicked(true);
    }
}

void Button::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->status = 0;
        this->update();
        emit released();
    }
}

void Button::rst()
{
    this->status = 0;
    this->update();
}

void Button::rbReleased()
{
    this->status = 0;
    this->update();
}

void Button::rbPressed()
{
    this->status = 4;
    this->update();
}

void Button::lbReleased()
{
    this->status = 0;
    this->update();
}

void Button::lbPressed()
{
    this->status = 3;
    this->update();
}

void Button::bothReleased()
{
    this->status = 0;
    this->update();
}

void Button::bothPressed()
{
    this->status = 5;
    this->update();
}

void Button::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter;
    painter.begin(this);
    painter.drawPixmap(this->rect(), pixmap.copy(
                           0, btn_height * status, btn_width, btn_height));
}


/********** PlayButton **********/
PlayButton::PlayButton(QWidget *parent)
    : Button (parent)
{
    this->setCheckable(true);
}

void PlayButton::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    emit released();
}


/********** SRButton **********/
SRButton::SRButton(QWidget *parent)
    : PlayButton (parent)
{
    this->syn = 0;
}

void SRButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->status = 2;
        this->update();
        if (!this->isChecked()) {
            this->syn = 1;
            emit clicked(true);
        }
    }
}

void SRButton::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (this->isChecked() && this->syn != 1) {
        emit clicked(true);
        emit released();
    }
    else
        this->syn = 0;
}


/********** PushButton **********/
PushButton::PushButton(QWidget *parent)
    : QPushButton (parent)
{
    this->status = 0;
}

void PushButton::loadPixmap(const QString &pic_name)
{
    this->pixmap = QPixmap(pic_name);
    this->btn_width = this->pixmap.width()/4;
    this->btn_height = this->pixmap.height();
    this->setFixedSize(this->btn_width, this->btn_height);
}

void PushButton::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (!this->isChecked() && this->isEnabled()) {
        this->status = 1;
        this->update();
    }
}

void PushButton::setDisabled(bool disable)
{
    QPushButton::setDisabled(disable);
    if (!this->isEnabled()) {
        this->status = 2;
        this->update();
    }
    else {
        this->status = 0;
        this->update();
    }
}

void PushButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->status = 2;
        this->update();
    }
}

void PushButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked(true);
    if (!this->isChecked())
        this->status = 2;
    if (this->menu())
        this->menu()->exec(event->globalPos());
    this->update();
}

void PushButton::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (!this->isChecked() && this->isEnabled()) {
        this->status = 0;
        this->update();
    }
}

void PushButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter;
    painter.begin(this);
    painter.drawPixmap(this->rect(), this->pixmap.copy(
                           btn_width * status, 0, btn_width, btn_height));
    painter.end();
}


/********** PushButton2 **********/
PushButton2::PushButton2(QWidget *parent)
    : QPushButton (parent)
{}

void PushButton2::loadPixmap(const QPixmap &pic_name)
{
    this->pixmap = QPixmap(pic_name);
    this->btn_width = this->pixmap.width();
    this->btn_height = this->pixmap.height();
    this->setFixedSize(this->btn_width, this->btn_height);
}

void PushButton2::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter;
    painter.begin(this);
    painter.drawPixmap(this->rect(), this->pixmap.copy(
                           0, 0, btn_width, btn_height));
    painter.end();
}
