#ifndef PLAYER_H
#define PLAYER_H

#include "widgets/addition.h"
#include "widgets/base.h"
#include "widgets/desktoplyricbuttons.h"
#include "apis/neteaseapi.h"
#include <QMediaPlayer>
#include <functional>

class Window;
class PlayList;
class CurrentMusic;
class CurrentMusicShort;
class CurrentMusicDetail;
class Player;
class _TableWidget;
class _MediaPlaylist;
class _LyricLabel;
class DesktopLyric;
class Lyric_Handle;

class PlayWidgets : public QFrame
{
    Q_OBJECT
public:
    Window* parent;
    int module;

    PlayList* playList;
    CurrentMusic* currentMusic;
    Player* player;
    DesktopLyric* desktopLyric;

    QPushButton* previousButton;
    QPushButton* playButton;
    QPushButton* pauseButton;
    QPushButton* nextButton;
    QPushButton* volume;
    QPushButton* noVolume;

    QPushButton* single;
    QPushButton* repeat;
    QPushButton* shuffle;
    QPushButton* lyricButton;
    QPushButton* playlist;

    QLabel* currentTime;
    QLabel* countTime;
    QSlider* slider;
    QSlider* volumeSlider;
    QHBoxLayout* mainLayout;

    PlayWidgets(Window* parent);
    void setButtons();
    void setLabels();
    void setSliders();
    void setLayouts();

    void setPlayerAndPlayList(const QHash<QString,QString>& data,int index=0);
    void setPlayerAndPlaylists(QList<QHash<QString,QString>> datas);
    void saveCookies();
    void loadCookies();

    void playEvent(QMediaPlayer* media);
    void pauseEvent(QMediaPlayer* media);
    void stopEvent(QMediaPlayer* media);

public slots:
    void previousSing();//
    void nextSing();//

    void volumeEvent();//
    void noVolumeEvent();//
    void volumeChangedEvent();//

    void singleEvent();//
    void repeatEvent();//
    void shuffleEvent();//
    void lyricEvent();//
    void playlistEvent();//
    void sliderEvent();//
    void sliderPressEvent();//
    void sliderMovedEvent();
};


class PlayList : public QFrame
{
    Q_OBJECT
public:
    static QString musicListCookiesFolder;

    PlayWidgets* parent;

    QList<QHash<QString,QString>> musicList;
    int currentRow;
    int allRow;
    QBrush itemColor;

    QPushButton* closeButton;
    _TableWidget* playList;

    QVBoxLayout* mainLayout;
    QHBoxLayout* headerLayout;

    PlayList(PlayWidgets* parent);
    void setButtons();
    void setTables();
    void setLayouts();
    void clear();

    void addMusic(const QHash<QString,QString>& data);
    void addMusics(const QList<QHash<QString,QString>>& mlist);
    void addPlayList(const QString& name,const QString& author,const QString& time);
    void saveCookies();
    void loadCookies();
public slots:
    void play();//
};


class CurrentMusic : public QFrame
{
    Q_OBJECT
public:
    PlayWidgets* parent;
    Window* grandparent;

    long long currentMusicId;
    int order;
    int count;

    // 歌词缓存
    QString lyricCache;
    CurrentMusicShort* shortInfo;
    CurrentMusicDetail* detailInfo;

    QPropertyAnimation* showDetail;
    QPropertyAnimation* showShort;
    QPoint mousePos;

    QHBoxLayout* mainLayout;

    CurrentMusic(PlayWidgets* parent);
    void setShortInfo(const QString& name,const QString& author,
                      QString pic=QString());
    void setDetailInfo();
    void getDetailInfo();
    void showLyric();
    void lyricCallback(const QString& future);
    void slide();
    void unLightLyric();

    void setFeatures();
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
public slots:
    QString getLyric();//@toTask
    void getShortInfo();//
};


class CurrentMusicShort : public QFrame
{
    Q_OBJECT
public:
    CurrentMusic* parent;
    QLabel* musicName;
    QLabel* musicAuthor;
    PicLabel* musicPic;
    PicLabel* musicMask;
    QVBoxLayout* musicLayout;

    QHBoxLayout* mainLayout;
    QVBoxLayout* musicInfo;

    CurrentMusicShort(CurrentMusic* parent);
    void setLabels();
    void setLayouts();
    void init();
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent *event);
};


class CurrentMusicDetail : public ScrollArea
{
    Q_OBJECT
public:
    QVBoxLayout* mainLayout;
    QHBoxLayout* topLayout;
    QVBoxLayout* topMainLayout;
    QHBoxLayout* topHeaderLayout;

    ScrollArea* lyricFrames;
    QVBoxLayout* lyricFramesLayout;
    QList<_LyricLabel*> allLyrics;
    QLabel* titleLabel;
    QPushButton* recoveryButton;

    CurrentMusicDetail(CurrentMusic* parent);
    void setLayouts();
    void addLyricLabel(_LyricLabel* label);
    void removeAllLyricLabels();
};


class Player : public QMediaPlayer
{
    Q_OBJECT
signals:
    void timeChanged(const QString&);
public:
    PlayWidgets* parent;
    std::function<QString(int)> transTime;
    qint64 musicTime;
    _MediaPlaylist* playList;

    QList<QPair<QString,QString>> lrc_lst;

    Player(PlayWidgets* parent);
    void setConnects();
    void setDisconnects();
    bool setMusic(const QString& url, const QHash<QString,QString>& data);
    void setAllMusics(const QList<QHash<QString,QString>>& datas);
    void setIndex(int index);
    qint64 allTime() const;
    QHash<QString,QString> getCurrentMusicInfo() const;

    void playMusic();

    void loadRealMusicUrl(QHash<QString,QString> musicInfo);//@toTask
    void saveCookies();
    void loadCookies();

    void setLyricEvent(qint64 position);
public slots:
    void dealError(QMediaPlayer::Error error);//
    void countTimeEvent(qint64 duration);//
    void positionChangedEvent(qint64 position);//
    void stateChangedEvent();//
};


class _TableWidget : public QTableWidget
{
    Q_OBJECT
public:
    PlayList* parent;
    QColor itemDualBackground;
    QColor itemBackground;
    QColor itemHoverBackground;
    QColor itemHoverColor;
    QColor itemColor;
    QColor itemColor2;

    int itemHoverRow;
    QList<QTableWidgetItem*> items;
    QAction* actionClear;
    QAction* actionRemove;
    QMenu* menu;

    _TableWidget(PlayList* parent);
    void setActions();
    void _setItems(int row);

    void contextMenuEvent(QContextMenuEvent *event);
public slots:
    void itemHover(const QModelIndex &index);//
    void viewportHover();//
    void clears();//
    void remove();//
};


struct Music
{
    QMediaContent content;
    QString url;
    bool is_str;
    Music(const QMediaContent& content) {
        this->content = content;
        this->is_str = false;
    }
    Music(const QString& url = QString()) {
        this->url = url;
        this->is_str = true;
    }
    Music& operator=(const Music& other) {
        this->content = other.content;
        this->url = other.url;
        this->is_str = other.is_str;
        return *this;
    }
};


class _MediaPlaylist : public QObject
{
    Q_OBJECT
public:
    static QString musicsCookiesFolder;
    static QString mediaListCookiesFolder;
    static int single;
    static int Loop;
    static int Random;

    Player* parent;
    PlayWidgets* playWidgets;
    qint64 duration;
    QList<Music> musics;
    int currentIndex;
    QHash<QString,QHash<QString,QString>> mediaList;
    int myModel;


    _MediaPlaylist(Player* parent);
    void setInitConnection();
    void addMedias(const QMediaContent & url,const QHash<QString,QString>& data);
    void addAllMedias(const QList<QHash<QString,QString>>& datas);
    void clear();
    int mediaCount() const;
    void removeMedias(int row);

    void next();
    void previous();
    void play();
    void setCurrentIndex(int index);
    void setPlaybackMode(int model);

    void setShortInfo(const QHash<QString,QString>& musicInfo);
    void setLyric(const QHash<QString,QString>& musicInfo);
    void setSystemTrayTip(const QString& tipMessage);
    void updatePlayTimes(const QHash<QString,QString>& musicInfo);
    void saveCookies();
    void loadCookies();

    void tabMusicEvent();
public slots:
    void mediaStatusChangedEvent(QMediaPlayer::MediaStatus status);//
};


class _LyricLabel : public QLabel
{
    Q_OBJECT
public:
    CurrentMusic* parent;
    QString myTime;
    QString myLyric;
    int myOrder;

    _LyricLabel(const QString& myTime,const QString& lyric,
                int myOrder,CurrentMusic* parent);
    void unLightMe();
public slots:
    void lightMe(const QString& currentTime);//
};


class DesktopLyric : public QDialog
{
    Q_OBJECT
public:
    QString lyric;
    int intervel;
    QRectF maskRect;
    double maskWidth;
    double widthBlock;

    QTimer* t;
    QRect screen;
    Lyric_Handle* handle;
    QVBoxLayout* verticalLayout;
    QFont font;

    QRect textRect;//1626行
    bool m_drag;//1641行
    QPoint m_DragPosition;

    DesktopLyric(PlayWidgets* parent);
    void setText(const QString& s, int intervel=0);
    void hideLyric();
    void leaveEvent(QEvent *event);
    //void enterEvent(QEvent *event);/需不需要实现为空

    void startMask();
    void stopMask();
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
public slots:
    void changeMask();//
    void hide();
    void newPos(const QPoint& p);//
};


class Lyric_Handle : public QDialog
{
    Q_OBJECT
signals:
    void lyricmoved(const QPoint&);
public:
    DesktopLyric* parent;

    QTimer* timer;
    QHBoxLayout* horiLayout;
    QLabel* handler;
    PushButton2* lockBt;
    PushButton2* hideBt;

    Lyric_Handle(DesktopLyric* parent);
    bool isInTitle(int xPos, int yPos) const;
    void moveEvent(QMoveEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
public slots:
    void lockLyric();//
    void hideLyric();//
};

QList<QPair<QString,QString>> getLyric(const QStringList& rawLyric);

#endif // PLAYER_H
