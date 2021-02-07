#ifndef MUSIC_H
#define MUSIC_H

#include "widgets/downloadframe.h"
#include "widgets/native.h"
#include "widgets/neteasesingsframes.h"
#include "widgets/player.h"
#include "widgets/recommendframe.h"
#include "widgets/systemtray.h"
#include "widgets/loginframes.h"
#include "widgets/skinwidget.h"

#include "features/configmainfeatures.h"
#include "features/configneteasefeatures.h"
#include "features/configdownloadframefeatures.h"
#include "features/configrecommendframefeatures.h"

class Header;
class Navigation;
class MainContent;
class SearchArea;

class Window : public QWidget
{
    Q_OBJECT
public:
    Header* header;
    Navigation* navigation;
    PlayWidgets* playWidgets;
    DetailSings* detailSings;
    MainContent* mainContent;
    NativeMusic* nativeMusic;
    DownloadFrame* downloadFrame;
    RecommendFrame* recommendFrame;
    SearchArea* searchArea;

    QTabWidget* mainContents;
    SystemTray* systemTray;

    NetEaseSingsArea* indexNetEaseSings;
    //XiamiSingsArea* indexXiamiSings;
    //QQSingsArea* indexQQSings;

    QFrame* line1;
    SkinWidget *skin_widget; //显示皮肤界面
    QString skin_name;//主窗口背景图片的名称

    QVBoxLayout* mainLayout;
    QHBoxLayout* contentLayout;

    ConfigWindow* config;//178行

    bool m_drag;//332行
    QPoint m_DragPosition;

    Window(QWidget* parent);
    void addAllPlaylist();
    void setContents();
    void setLines();
    void setLayouts();
    void configFeatures();
protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);

    void paintEvent(QPaintEvent *event);
public slots:
    void showSkinWidget();
    void changeSkin(QString skin_name);
};


class Header : public QFrame
{
    Q_OBJECT
signals:
    void showSkin();
public:
    ConfigHeader* config;
    Window* parent;
    LoginBox* loginBox;
    QPushButton* closeButton;
    QPushButton* showminButton;
    QPushButton* showmaxButton;
    QPushButton* loginButton;
    QPushButton* prevButton;
    QPushButton* nextButton;

    QPushButton *skin_button;

    PicLabel* logoLabel;
    QLabel* descriptionLabel;
    PicLabel* userPix;
    SearchLineEdit* searchLine;
    QFrame* line1;
    QHBoxLayout* mainLayout;

    Header(Window* parent);
    void setButtons();
    void setLabels();
    void setLineEdits();
    void setLines();
    void setLayouts();
protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
};


class Navigation : public QScrollArea
{
    Q_OBJECT
public:
    ConfigNavigation* config;

    Window* parent;
    QFrame* frame;

    QLabel* recommendLabel;
    QLabel* myMusic;
    QLabel* singsListLabel;

    QListWidget* navigationList;
    QListWidget* nativeList;
    QVBoxLayout* mainLayout;

    Navigation(Window* parent);
    void setLabels();
    void setListViews();
    void setLayouts();
    void setSingsList(){}
};


class MainContent : public ScrollArea
{
    Q_OBJECT
public:
    ConfigMainContent* config;

    QVBoxLayout* mainLayout;
    Window* parent;
    QTabWidget* tab;
    MainContent(Window* parent);
    void addTab(QWidget* widget,const QString& name="");
};


class SearchArea : public ScrollArea
{
    Q_OBJECT
public:
    ConfigSearchArea* config;

    QVBoxLayout* mainLayout;
    Window* parent;
    QLabel* titleLabel;
    QTabWidget* contentsTab;

    NetEaseSearchResultFrame* neteaseSearchFrame;

    QString text;

    SearchArea(Window* parent);
    void setSingsFrame();
    void setText(const QString& text);
};

void start();

void test2();

#endif // MUSIC_H
