#ifndef DESKTOPLYRICBUTTONS_H
#define DESKTOPLYRICBUTTONS_H

#include <QtWidgets>

class Button : public QPushButton
{
    Q_OBJECT
public:
    int status;
    QPixmap pixmap;
    int btn_width;
    int btn_height;

    Button(QWidget *parent);
    void loadPixmap(const QString& pic_name);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void rst();

    void rbReleased();
    void rbPressed();
    void lbPressed();
    void lbReleased();
    void bothPressed();
    void bothReleased();
    void paintEvent(QPaintEvent *event);
};


class PlayButton : public Button
{
    Q_OBJECT
public:
    PlayButton(QWidget *parent);
    void mouseReleaseEvent(QMouseEvent *event);
};


class SRButton : public PlayButton
{
    Q_OBJECT
public:
    int syn;
    SRButton(QWidget *parent);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
};


class PushButton : public QPushButton
{
    Q_OBJECT
public:
    int status;
    QPixmap pixmap;
    int btn_width;
    int btn_height;

    PushButton(QWidget *parent);
    void loadPixmap(const QString& pic_name);
    void enterEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);
    void paintEvent(QPaintEvent *event);
public slots:
    void setDisabled(bool disable);
};


class PushButton2 : public QPushButton
{
    Q_OBJECT
public:
    QPixmap pixmap;
    int btn_width;
    int btn_height;

    PushButton2(QWidget *parent);
    void loadPixmap(const QPixmap& pic_name);
    void paintEvent(QPaintEvent *event);
};

#endif // DESKTOPLYRICBUTTONS_H
