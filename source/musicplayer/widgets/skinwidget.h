#ifndef SKINWIDGET_H
#define SKINWIDGET_H

#include "widgets/changeskinwidget.h"

class SkinWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SkinWidget(QWidget *parent = nullptr);
    void translateLanguage();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

signals:
    void changeSkin(QString skin_name);

private slots:
    void showSkin();
    void varyfySkin(QString skin_name);

private:
    void initTitle();
    void initCenter();

private:
    QString skin_name; //背景图片
    bool is_change; //是否改变背景

    QHBoxLayout *title_layout;
    QGridLayout *center_layout;

    QPoint move_point; //移动的距离
    bool mouse_press; //按下鼠标左键
    QLabel *title_label; //标题
    QLabel *title_icon_label; //标题图标
    QPushButton *close_button; //关闭按钮

    ChangeSkinWidget *change_skin_widget_0;
    ChangeSkinWidget *change_skin_widget_1;
    ChangeSkinWidget *change_skin_widget_2;
    ChangeSkinWidget *change_skin_widget_3;
    ChangeSkinWidget *change_skin_widget_4;
    ChangeSkinWidget *change_skin_widget_5;
};

#endif // SKINWIDGET_H
