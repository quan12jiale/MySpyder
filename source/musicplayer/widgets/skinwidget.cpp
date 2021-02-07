#include "skinwidget.h"

SkinWidget::SkinWidget(QWidget *parent)
    : QWidget(parent)
{
    this->resize(620, 445);

    //初始化为未按下鼠标左键
    mouse_press = false;
    skin_name = QString("");
    is_change = false;

    //设置标题栏隐藏
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);

    this->initTitle();
    this->initCenter();

    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addLayout(title_layout);
    main_layout->addLayout(center_layout);
    main_layout->addStretch();
    main_layout->setSpacing(0);
    main_layout->setContentsMargins(0, 0, 0, 0);

    setLayout(main_layout);

    this->translateLanguage();

    this->showSkin();
}

void SkinWidget::initTitle()
{
    title_label = new QLabel();
    title_icon_label = new QLabel();
    close_button = new QPushButton();

    QPixmap title_pixmap("resource/format.ico");
    title_icon_label->setPixmap(title_pixmap);
    title_icon_label->setFixedSize(16, 16);
    title_icon_label->setScaledContents(true);

    close_button->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    title_label->setFixedHeight(30);

    title_layout = new QHBoxLayout();
    title_layout->addWidget(title_icon_label, 0, Qt::AlignVCenter);
    title_layout->addWidget(title_label, 0, Qt::AlignVCenter);
    title_layout->addStretch();
    title_layout->addWidget(close_button, 0, Qt::AlignTop);
    title_layout->setSpacing(5);
    title_layout->setContentsMargins(10, 0, 5, 0);

    title_label->setStyleSheet("color:white;");
    connect(close_button, SIGNAL(clicked()), this, SLOT(hide()));
}

void SkinWidget::initCenter()
{
    center_layout = new QGridLayout();
    center_layout->setSpacing(5);
    center_layout->setContentsMargins(5, 35, 5, 0);

    change_skin_widget_0 = new ChangeSkinWidget();
    change_skin_widget_1 = new ChangeSkinWidget();
    change_skin_widget_2 = new ChangeSkinWidget();
    change_skin_widget_3 = new ChangeSkinWidget();
    change_skin_widget_4 = new ChangeSkinWidget();
    change_skin_widget_5 = new ChangeSkinWidget();

    connect(change_skin_widget_0, SIGNAL(changeSkin(QString)), this, SLOT(varyfySkin(QString)));
    connect(change_skin_widget_1, SIGNAL(changeSkin(QString)), this, SLOT(varyfySkin(QString)));
    connect(change_skin_widget_2, SIGNAL(changeSkin(QString)), this, SLOT(varyfySkin(QString)));
    connect(change_skin_widget_3, SIGNAL(changeSkin(QString)), this, SLOT(varyfySkin(QString)));
    connect(change_skin_widget_4, SIGNAL(changeSkin(QString)), this, SLOT(varyfySkin(QString)));
    connect(change_skin_widget_5, SIGNAL(changeSkin(QString)), this, SLOT(varyfySkin(QString)));

    center_layout->addWidget(change_skin_widget_0, 0, 0);
    center_layout->addWidget(change_skin_widget_1, 0, 1);
    center_layout->addWidget(change_skin_widget_2, 0, 2);
    center_layout->addWidget(change_skin_widget_3, 1, 0);
    center_layout->addWidget(change_skin_widget_4, 1, 1);
    center_layout->addWidget(change_skin_widget_5, 1, 2);
}

void SkinWidget::showSkin()
{
    change_skin_widget_0->changeSkin("resource/skin/black.png", tr("酷炫黑"));
    change_skin_widget_1->changeSkin("resource/skin/red.png", tr("官方红"));
    change_skin_widget_2->changeSkin("resource/skin/pink.png", tr("可爱粉"));
    change_skin_widget_3->changeSkin("resource/skin/blue.png", tr("天际蓝"));
    change_skin_widget_4->changeSkin("resource/skin/green.png", tr("清新绿"));
    change_skin_widget_5->changeSkin("resource/skin/golden.png", tr("土豪金"));
}

void SkinWidget::translateLanguage()
{
    title_label->setText("title");
    close_button->setToolTip("close");
}

void SkinWidget::varyfySkin(QString skin_name)
{
    this->skin_name = skin_name;
    is_change = true;
    update();
    emit changeSkin(skin_name);
}

void SkinWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), QPixmap(skin_name));

    QPainter painter2(this);
    QLinearGradient linear2(rect().topLeft(), rect().bottomLeft());
    linear2.setColorAt(0, Qt::white);
    linear2.setColorAt(0.5, Qt::white);
    linear2.setColorAt(1, Qt::white);
    painter2.setPen(Qt::white); //设定画笔颜色，到时侯就是边框颜色
    painter2.setBrush(linear2);
    painter2.drawRect(QRect(0, 30, this->width(), this->height()-30));

    QPainter painter3(this);
    painter3.setPen(Qt::gray);
    static const QPointF points[4] = {QPointF(0, 30), QPointF(0, this->height()-1), QPointF(this->width()-1, this->height()-1), QPointF(this->width()-1, 30)};
    painter3.drawPolyline(points, 4);
}

void SkinWidget::mousePressEvent( QMouseEvent * event )
{
    //只能是鼠标左键移动和改变大小
    if(event->button() == Qt::LeftButton)
    {
        mouse_press = true;
    }

    //窗口移动距离
    move_point = event->globalPos() - pos();
}

void SkinWidget::mouseReleaseEvent(QMouseEvent *)
{
    mouse_press = false;
}

void SkinWidget::mouseMoveEvent(QMouseEvent *event)
{
    //移动窗口
    if(mouse_press)
    {
        QPoint move_pos = event->globalPos();
        move(move_pos - move_point);
    }
}

