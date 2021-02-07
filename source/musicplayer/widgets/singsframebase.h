#ifndef SINGSFRAMEBASE_H
#define SINGSFRAMEBASE_H

#include "widgets/base.h"

class Window;
class MainContent;
class ConfigDetailSings;
class ConfigNavigation;

class SingsFrameBase : public ScrollArea
{
    Q_OBJECT
public:
    MainContent* parent;
    //只有这个ScrollArea的子类的mainLayout是QGridLayout*
    QGridLayout* mainLayout;

    SingsFrameBase(MainContent* parent);
};


class SingsSearchResultFrameBase : public QFrame
{
    Q_OBJECT
public:
    QWidget* parent;
    QVBoxLayout* singsFrameLayout;
    QLabel* noSingsContentsLabel;
    TableWidget* singsResultTable;
    QHBoxLayout* centerLabelLayout;

    SingsSearchResultFrameBase(QWidget* parent);
};


class DetailSings : public ScrollArea
{
    Q_OBJECT
public:
    ConfigDetailSings* config;

    Window* parent;

    PicLabel* picLabel;
    QLabel* titleLabel;
    QLabel* authorPic;
    QLabel* authorName;
    QTextEdit* descriptionText;

    QPushButton* showButton;
    QPushButton* descriptionButton;
    QPushButton* playAllButton;
    QTabWidget* contentsTab;
    TableWidget* singsTable;

    QVBoxLayout* mainLayout;
    QHBoxLayout* topLayout;
    QVBoxLayout* descriptionLayout;
    QHBoxLayout* titleLayout;
    QHBoxLayout* authorLayout;
    QHBoxLayout* descriptLayout;

    DetailSings(Window* parent);
    void setLabels();
    void setButtons();
    void setTabs();
    void setLayouts();
};


class OneSing : public QFrame
{
    Q_OBJECT
signals:
    void clicked(long long, const QString&);
public:
    int row;
    int column;
    long long ids;
    QString picName;
    QLabel* picLabel;
    QLabel* nameLabel;
    QVBoxLayout* mainLayout;
    QPoint mousePos;

    OneSing(int row,int column,long long ids,const QString& picName);
    void setStyleSheets(const QString& styleSheet=QString());
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
};


class PlaylistButton : public QPushButton
{
    Q_OBJECT
signals:
    void hasClicked(long long, const QString&);
public:
    ConfigNavigation* parent;
    long long ids;
    QString coverImgUrl;

    PlaylistButton(ConfigNavigation* parent,long long ids,const QString &coverImgUrl,
                   const QIcon &icon, const QString &text);
public slots:
    void clickedEvent();
};

#endif // SINGSFRAMEBASE_H
