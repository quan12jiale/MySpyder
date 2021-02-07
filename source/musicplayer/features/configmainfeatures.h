#ifndef CONFIGMAINFEATURES_H
#define CONFIGMAINFEATURES_H

#include "widgets/base.h"
#include "widgets/player.h"
#include "widgets/systemtray.h"
#include "widgets/singsframebase.h"
#include "features/configrecommendframefeatures.h"
#include <functional>

class Header;
class Navigation;
class MainContent;
class SearchArea;

class ConfigWindow : public QObject
{
    Q_OBJECT
signals:
    void set_previous_enabled(bool);
    void set_next_enabled(bool);
private:
    int histindex;
public:
    Window* window;
    QList<int> history;

    ConfigWindow(Window* window);
    void addTab(QWidget* widget, const QString& name="");
    int allTab() const;

    void bindConnect();

    void setTabIndex(int index);
    QString getDownloadFolder() const;
    void pullRecommendSong();

    void refresh();
public slots:
    void chdir(int index=-1,bool browsing_history=false);//
    void nextTab();//
    void prevTab();//
};


class ConfigHeader : public QObject
{
    Q_OBJECT
signals:
    void finished();
    void breakSignal(const QString&);
public:
    static QString loginCookiesFolder;

    Header* header;

    QJsonObject loginInfor;
    QPair<int,QList<QHash<QString,QString>>> result;// songCount, songs
    QHash<long long,QString> songsDetail;

    QString username;
    QString password;

    int code;
    bool isMax;

    ConfigHeader(Header* header);
    void bindConnect();

    void loadLoginInformations(QPair<QString,QString> informations);

    void saveCookies();
    void loadCookies();
public slots:
    void showMaxiOrRevert();//
    void search();//@toTask
    void showLoginBox();//
    void login();//
    void loginFinished();//

    void setUserData();//@toTask
    void emitWarning(const QString& warningStr);//
    void exitLogin();//
};


class ConfigNavigation : public QObject
{
    Q_OBJECT
public:
    Navigation* navigation;
    DetailSings* detailFrame;
    Window* window;//源码命名为mainContents
    QList<PlaylistButton*> playlists;

    QJsonObject result;
    QStringList singsUrls;
    QString coverImgUrl;

    ConfigNavigation(Navigation* navigation);
    void bindConnect();

    void singsButtonClickEvent();
    void setPlaylists(const QJsonArray& datas);
    void clearPlaylists();

    void navigationListFunction();
    void tabNativeFrame(QListWidgetItem *item);
public slots:
    void navigationListItemClickEvent();//
    void nativeListItemClickEvent(QListWidgetItem *item);//
    void startRequest(long long ids,const QString& coverImgUrl);//@toTask
};


class ConfigMainContent : public QObject
{
    Q_OBJECT
public:
    MainContent* mainContent;
    ConfigMainContent(MainContent* mainContent);
};


class ConfigSearchArea : public QObject
{
    Q_OBJECT
signals:
    void download(const QHash<QString,QString>&);
public:
    SearchArea* searchArea;
    int currentIndex;
    QString currentName;
    QString downloadFolder;
    std::function<QString(int)> transTime;

    TableWidget* searchResultTableIndexs;
    QList<QHash<QString,QString>> musicList;
    QString noContents;

    QAction* actionDownloadSong;
    QMenu* menu;

    ConfigSearchArea(SearchArea* searchArea);
    void bindConnect();
    void setContextMenu();

    void search(const QString& name);
    void setSingsData(const QList<QHash<QString,QString>>& data);

public slots:
    void downloadSong();//@toTask
    void searchBy(int index);//
    void itemDoubleClickedEvent();//
    void contextEvent(QContextMenuEvent *event);//自定义的函数
};


class ConfigDetailSings : public QObject
{
    Q_OBJECT
signals:
    void download(const QHash<QString,QString>&);
public:
    DetailSings* detailSings;
    QList<QHash<QString,QString>> musicList;

    int currentIndex;
    Window* grandparent;
    Player* player;
    PlayWidgets* playList;
    CurrentMusic* currentMusic;
    std::function<QString(int)> transTime;

    QAction* actionNextPlay;
    QAction* actionDownloadSong;

    QMenu* menu;//681行

    ConfigDetailSings(DetailSings* parent);
    void bindConnect();
    void setContextMenu();

    void setupDetailFrames(const QJsonObject& datas,const QStringList& singsUrls,
                           const QList<long long>& singsIds);

public slots:
    void addToNextPlay();//
    void downloadSong();//
    void addAllMusicToPlayer();//

    void itemDoubleClickedEvent();//
    void singsFrameContextMenuEvent(QContextMenuEvent *event);//
};


class ConfigSystemTray : public QObject
{
    Q_OBJECT
public:
    SystemTray* systemTray;
    ConfigSystemTray(SystemTray* systemTray);
    void addActions();
};

#endif // CONFIGMAINFEATURES_H
