#include "player.h"
#include "music.h"

/********** PlayWidgets **********/
PlayWidgets::PlayWidgets(Window *parent)
    : QFrame ()
{
    this->setObjectName("playwidget");
    this->parent = parent;
    // 0模式是列表循环。1模式是单曲循环，2模式是随机播放
    this->module = 0;

    QFile file("QSS/playWidgets.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        this->setStyleSheet(in.readAll());
    }

    this->playList = new PlayList(this);
    this->currentMusic = new CurrentMusic(this);
    this->player = new Player(this);

    this->desktopLyric = new DesktopLyric(this);

    this->setButtons();
    this->setLabels();
    this->setSliders();

    this->setLayouts();

    this->loadCookies();
}

void PlayWidgets::setButtons()
{
    this->previousButton = new QPushButton(this);
    this->previousButton->setObjectName("previousButton");
    //this->previousButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    connect(previousButton, SIGNAL(clicked()), SLOT(previousSing()));

    this->playButton = new QPushButton(this);
    this->playButton->setObjectName("playButton");
    connect(playButton, &QAbstractButton::clicked,
            [this](){this->playEvent(this->player);});

    this->pauseButton = new QPushButton(this);
    this->pauseButton->setObjectName("pauseButton");
    connect(pauseButton, &QAbstractButton::clicked,
            [this](){this->pauseEvent(this->player);});
    this->pauseButton->hide();

    this->nextButton = new QPushButton(this);
    this->nextButton->setObjectName("nextButton");
    connect(nextButton, SIGNAL(clicked()), SLOT(nextSing()));

    this->volume = new QPushButton(this);
    this->volume->setObjectName("volume");
    connect(volume, SIGNAL(clicked()), SLOT(volumeEvent()));

    this->noVolume = new QPushButton(this);
    this->noVolume->setObjectName("no_volume");
    connect(noVolume, SIGNAL(clicked()), SLOT(noVolumeEvent()));
    this->noVolume->hide();

    this->single = new QPushButton(this);
    this->single->setObjectName("single");
    connect(single, SIGNAL(clicked()), SLOT(singleEvent()));
    this->single->hide();
    this->single->setToolTip(tr("单曲循环"));
    // 单曲循环、随机播放、顺序播放、列表循环

    this->repeat = new QPushButton(this);
    this->repeat->setObjectName("repeat");
    connect(repeat, SIGNAL(clicked()), SLOT(repeatEvent()));
    this->repeat->setToolTip(tr("列表循环"));

    this->shuffle = new QPushButton(this);
    this->shuffle->setObjectName("shuffle");
    connect(shuffle, SIGNAL(clicked()), SLOT(shuffleEvent()));
    this->shuffle->hide();
    this->shuffle->setToolTip(tr("随机循环"));

    this->lyricButton = new QPushButton(this);
    this->lyricButton->setObjectName("lyricButton");
    connect(lyricButton, SIGNAL(clicked()), SLOT(lyricEvent()));
    this->lyricButton->setToolTip(tr("打开歌词"));
    this->lyricButton->setText(tr("词"));

    this->playlist = new QPushButton(this);
    this->playlist->setObjectName("playlist");
    connect(playlist, SIGNAL(clicked()), SLOT(playlistEvent()));
    this->playlist->setToolTip(tr("打开播放列表"));
}

void PlayWidgets::setLabels()
{
    //当前播放时间
    this->currentTime = new QLabel(this);
    this->currentTime->setText("00:00");

    //歌曲总时间
    this->countTime = new QLabel(this);
    this->countTime->setText("00:00");
}

void PlayWidgets::setSliders()
{
    this->slider = new QSlider(this);
    this->slider->setMinimumHeight(5);
    this->slider->setMinimumWidth(440);
    // 将范围设置成1000滚动时更舒服。
    this->slider->setRange(0, 1000);
    this->slider->setObjectName("slider");
    this->slider->setOrientation(Qt::Horizontal);
    connect(slider, SIGNAL(sliderReleased()), SLOT(sliderEvent()));
    connect(slider, SIGNAL(sliderPressed()), SLOT(sliderPressEvent()));

    this->volumeSlider = new QSlider(this);
    this->volumeSlider->setValue(100);
    this->volumeSlider->setMinimumHeight(5);
    this->volumeSlider->setObjectName("volumeSlider");
    this->volumeSlider->setOrientation(Qt::Horizontal);
    connect(volumeSlider, SIGNAL(valueChanged(int)), SLOT(volumeChangedEvent()));
}

void PlayWidgets::setLayouts()
{
    this->mainLayout = new QHBoxLayout();

    this->mainLayout->addWidget(this->previousButton);
    this->mainLayout->addWidget(this->playButton);
    this->mainLayout->addWidget(this->pauseButton);
    this->mainLayout->addWidget(this->nextButton);

    this->mainLayout->addWidget(this->currentTime);
    this->mainLayout->addWidget(this->slider);
    this->mainLayout->addWidget(this->countTime);

    this->mainLayout->addWidget(this->volume);
    this->mainLayout->addWidget(this->noVolume);

    this->mainLayout->addWidget(this->volumeSlider);
    this->mainLayout->addSpacing(10);

    this->mainLayout->addWidget(this->single);
    this->mainLayout->addWidget(this->repeat);
    this->mainLayout->addWidget(this->shuffle);

    this->mainLayout->addSpacing(10);
    this->mainLayout->addWidget(this->lyricButton);
    this->mainLayout->addSpacing(10);

    this->mainLayout->addWidget(this->playlist);
    this->mainLayout->addStretch(1);
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(this->mainLayout);
}

void PlayWidgets::previousSing()
{
    this->player->playList->previous();
}

void PlayWidgets::nextSing()
{
    this->player->playList->next();
}

void PlayWidgets::setPlayerAndPlayList(const QHash<QString, QString> &data, int index)
{
    Q_UNUSED(index);
    QString authorAndName = data["author"] + data["name"];
    for (int i = 0; i < this->playList->musicList.size(); ++i) {
        QHash<QString,QString> mediaInfo = this->playList->musicList[i];
        QString checkAuthorAndName = mediaInfo["author"] + mediaInfo["name"];
        if (authorAndName == checkAuthorAndName) {
            if (this->player->playList->currentIndex == i && i != 0)
                return;

            this->player->setIndex(i);
            return;
        }
    }

    bool sureSetUp = this->player->setMusic(data["url"], data);
    if (!sureSetUp) {
        this->currentMusic->setShortInfo(tr("音乐不可播放"), data["author"], data["music_img"]);
        return;
    }

    this->playList->addMusic(data);
    this->player->playList->tabMusicEvent();
    this->playList->addPlayList(data["name"], data["author"], data["time"]);
}

void PlayWidgets::setPlayerAndPlaylists(QList<QHash<QString, QString> > datas)
{
    std::reverse(datas.begin(), datas.end());
    QHash<QString, QString> dict;
    for (int idx = 0; idx < datas.size(); ++idx) {
        dict = datas[idx];
        this->playList->addPlayList(dict["name"], dict["author"], dict["time"]);
    }

    this->playList->addMusics(datas);
    this->player->setAllMusics(datas);

    bool sureSetUp = this->player->setMusic(dict["url"], dict);
    if (!sureSetUp) {
        this->currentMusic->setShortInfo(tr("音乐不可播放"), dict["author"], dict["music_img"]);
        this->nextSing();
        return;
    }
    this->player->playList->tabMusicEvent();
}

void PlayWidgets::saveCookies()
{
    this->playList->saveCookies();
    this->player->saveCookies();
}

void PlayWidgets::loadCookies()
{
    this->playList->loadCookies();
    this->player->loadCookies();
}

// 事件
void PlayWidgets::playEvent(QMediaPlayer *media)
{
    this->playButton->hide();
    this->pauseButton->show();
    media->play();
}

void PlayWidgets::pauseEvent(QMediaPlayer *media)
{
    this->playButton->show();
    this->pauseButton->hide();
    media->pause();
}

void PlayWidgets::stopEvent(QMediaPlayer *media)
{
    this->playButton->show();
    this->pauseButton->hide();
    media->stop();
}

void PlayWidgets::volumeEvent()
{
    this->volume->hide();
    this->noVolume->show();
    this->volumeSlider->setValue(0);
}

void PlayWidgets::noVolumeEvent()
{
    this->volume->show();
    this->noVolume->hide();
    this->volumeSlider->setValue(100);
}

void PlayWidgets::volumeChangedEvent()
{
    this->player->setVolume(this->volumeSlider->value());
}

void PlayWidgets::singleEvent()
{
    this->single->hide();
    this->shuffle->show();
    this->player->playList->setPlaybackMode(_MediaPlaylist::Random);
    this->module = 2;// 0模式是列表循环。1模式是单曲循环，2模式是随机播放
}

void PlayWidgets::repeatEvent()
{
    this->repeat->hide();
    this->single->show();
    this->player->playList->setPlaybackMode(_MediaPlaylist::single);
    this->module = 1;
}

void PlayWidgets::shuffleEvent()
{
    this->shuffle->hide();
    this->repeat->show();
    this->player->playList->setPlaybackMode(_MediaPlaylist::Loop);
    this->module = 0;
}

void PlayWidgets::lyricEvent()
{
    QString lyricStyle = "QPushButton#lyricButton {{"
                         "width: 10px;"
                         "height: 10px;"
                         "margin: 0px;"
                         "padding: 3px;"
                         "background: %1;"
                         "color: %2;"
                         "border: 1px solid #919192;"
                         "}}"
                         "QPushButton#lyricButton:hover {{"
                         "background: #79797B;"
                         "color: #DCDCDC;"
                         "border-color: #DCDCDC;"
                         "}}";

    if (this->desktopLyric->isVisible()) {
        this->lyricButton->setToolTip(tr("打开歌词"));
        //this->lyricButton->setStyleSheet(lyricStyle.arg("none").arg("#79797B"));
        this->desktopLyric->hide();
        return;
    }

    this->lyricButton->setToolTip(tr("关闭歌词"));
    //this->lyricButton->setStyleSheet(lyricStyle.arg("#828282").arg("#FFFFFF"));
    this->desktopLyric->show();
    this->currentMusic->setDetailInfo();
}

void PlayWidgets::playlistEvent()
{
    if (this->playList->isVisible())
        this->playList->hide();
    else {
        this->playList->move(this->parent->width()-574,
                             this->parent->height()-477-this->height());
        this->playList->show();
        this->playList->raise();
    }
}

void PlayWidgets::sliderEvent()
{
    int value = this->slider->value();
    qint64 currentSliderTime = this->player->allTime()*value;
    this->player->setPosition(currentSliderTime);

    this->player->setConnects();
    disconnect(this->slider, SIGNAL(sliderMoved(int)), this, SLOT(sliderMovedEvent()));
}

void PlayWidgets::sliderPressEvent()
{
    this->player->setDisconnects();
    connect(this->slider, SIGNAL(sliderMoved(int)), this, SLOT(sliderMovedEvent()));
}

void PlayWidgets::sliderMovedEvent()
{
    int value = this->slider->value();
    qint64 currentSliderTime = this->player->allTime()*value/1000;
    QString text = itv2time(static_cast<int>(currentSliderTime));
    this->currentTime->setText(text);
    emit this->player->timeChanged(text);
}


/********** PlayList **********/
QString PlayList::musicListCookiesFolder = "cookies/playlist/musicList";

PlayList::PlayList(PlayWidgets *parent)
    : QFrame ()
{
    this->parent = parent;
    this->setParent(this->parent->parent);
    this->setObjectName("PlayList");

    this->musicList = QList<QHash<QString,QString>>();

    this->currentRow = -1;
    this->allRow = 0;
    this->itemColor = QBrush(QColor(95,99,99));

    QFile file("QSS/playList.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        //this->setStyleSheet(in.readAll());
    }

    this->resize(574, 477);

    this->hide();

    this->setButtons();
    this->setTables();
    this->setLayouts();
}

void PlayList::setButtons()
{
    this->closeButton = new QPushButton("x", this);
    this->closeButton->setObjectName("closeButton");
    connect(closeButton, SIGNAL(clicked()), SLOT(hide()));
}

void PlayList::setTables()
{
    this->playList = new _TableWidget(this);
    this->playList->setMinimumWidth(this->width());
    this->playList->setColumnCount(3);

    this->playList->setColumnWidth(0, this->width()/3*2);
    this->playList->horizontalHeader()->setVisible(false);
    this->playList->horizontalHeader()->setStretchLastSection(true);
    this->playList->verticalHeader()->setVisible(false);
    this->playList->setShowGrid(false);
    this->playList->setAlternatingRowColors(true);

    this->playList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->playList->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(playList, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), SLOT(play()));
}

void PlayList::setLayouts()
{
    this->mainLayout = new QVBoxLayout();

    this->headerLayout = new QHBoxLayout();
    this->headerLayout->addStretch(1);
    this->headerLayout->addWidget(this->closeButton);

    this->mainLayout->addLayout(this->headerLayout);
    this->mainLayout->addWidget(this->playList);

    this->setLayout(this->mainLayout);
}

void PlayList::play()
{
    if (this->currentRow == this->playList->currentRow())
        return;
    else {
        int currentRow = this->playList->currentRow();
        Q_ASSERT(0 <= currentRow && currentRow < this->musicList.size());
        QHash<QString,QString> currentMusic = this->musicList[currentRow];
        this->parent->player->setIndex(currentRow);
        this->parent->currentMusic->setShortInfo(currentMusic["name"], currentMusic["author"]);
        this->currentRow = currentRow;
    }
}

void PlayList::clear()
{
    this->playList->clears();
}

void PlayList::addMusic(const QHash<QString, QString> &data)
{
    this->musicList.append(data);
}

void PlayList::addMusics(const QList<QHash<QString, QString> > &mlist)
{
    this->musicList.append(mlist);
}

void PlayList::addPlayList(const QString &name, const QString &author, const QString &time)
{
    this->playList->setRowCount(this->allRow+1);

    QTableWidgetItem* musicName = new QTableWidgetItem(name);
    this->playList->setItem(this->allRow, 0, musicName);

    QTableWidgetItem* musicAuthor = new QTableWidgetItem(author);
    musicAuthor->setForeground(this->itemColor);
    this->playList->setItem(this->allRow, 1, musicAuthor);

    QTableWidgetItem* musicTime = new QTableWidgetItem(time);
    musicTime->setForeground(this->itemColor);
    this->playList->setItem(this->allRow, 2, musicTime);

    this->allRow += 1;
}

QList<QVariant> serialize_musicList(const QList<QHash<QString,QString>>& musicList)
{
    QList<QVariant> list;
    for (int i = 0; i < musicList.size(); ++i) {
        QHash<QString,QString> dict = musicList[i];
        QHash<QString,QVariant> tmp;
        for (auto it = dict.begin(); it != dict.end(); ++it) {
            QString key = it.key();
            QString val = it.value();
            tmp[key] = val;
        }
        list.append(tmp);
    }
    return list;
}

QList<QHash<QString,QString>> deserialize_musicList(const QList<QVariant>& list)
{
    QList<QHash<QString,QString>> musicList;
    for (int i = 0; i < list.size(); ++i) {
        QHash<QString,QVariant> tmp = list[i].toHash();
        QHash<QString,QString> dict;
        for (auto it = tmp.begin(); it != tmp.end(); ++it) {
            QString key = it.key();
            QString val = it.value().toString();
            dict[key] = val;
        }
        musicList.append(dict);
    }
    return musicList;
}

void PlayList::saveCookies()
{
    QSettings settings;
    settings.setValue(musicListCookiesFolder, serialize_musicList(musicList));
}

void PlayList::loadCookies()
{
    QSettings settings;
    QList<QVariant> list = settings.value(musicListCookiesFolder).toList();
    this->musicList = deserialize_musicList(list);

    foreach (auto dict, this->musicList) {
        this->addPlayList(dict["name"], dict["author"], dict["time"]);
    }
}


/********** CurrentMusic **********/
CurrentMusic::CurrentMusic(PlayWidgets* parent)
    : QFrame ()
{
    this->parent = parent;
    this->grandparent = this->parent->parent;
    this->setParent(this->grandparent);
    this->setObjectName("currentMusic");

    this->currentMusicId = 0;
    this->order = -2;
    this->count = 0;
    this->lyricCache = "";

    this->raise();
    this->activateWindow();

    this->shortInfo = new CurrentMusicShort(this);
    this->detailInfo = new CurrentMusicDetail(this);

    this->showDetail = nullptr;
    this->showShort = nullptr;
    this->mousePos = QPoint();

    this->mainLayout = new QHBoxLayout(this);
    this->mainLayout->addWidget(this->shortInfo);
    this->mainLayout->addWidget(this->detailInfo);

    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->mainLayout->setSpacing(0);

    this->setFeatures();

    QFile file("QSS/currentMusic.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        //this->setStyleSheet(in.readAll());
    }
}

void CurrentMusic::setShortInfo(const QString &name, const QString &author, QString pic)
{
    if (!pic.isEmpty()) {
        QStringList cacheList = os::listdir(cacheFolder);
        int idx = pic.lastIndexOf('/');
        QString names = pic;
        if (idx > -1)
            names = pic.mid(idx+1);
        if (cacheList.contains(names))
            pic = cacheFolder+'/'+names;

        this->shortInfo->musicPic->setSrc(pic);
    }
    this->shortInfo->musicName->setText(name);//在这里设置歌曲名字
    this->shortInfo->musicAuthor->setText(author);
}

void CurrentMusic::setDetailInfo()
{
    QString title = this->shortInfo->musicName->text();
    QString picSrc = this->shortInfo->musicPic->getSrc();

    this->detailInfo->titleLabel->setText(title);
    this->showLyric();//调用该函数显示歌词
}

void CurrentMusic::getDetailInfo()
{
    this->shortInfo->hide();
    this->detailInfo->show();

    this->showDetail = new QPropertyAnimation(this, "geometry");

    int x = this->pos().x();
    int y = this->pos().y();

    this->showDetail->setStartValue(QRect(x, y, this->width(), this->height()));
    this->showDetail->setEndValue(QRect(0, this->grandparent->header->height()+3,
                                        this->grandparent->width(),
                                        this->grandparent->mainContent->height()));
    this->showDetail->setDuration(300);
    this->showDetail->setEasingCurve(QEasingCurve::InBack);

    this->showDetail->start(QAbstractAnimation::DeleteWhenStopped);

    this->raise();
    this->setDetailInfo();
}

void CurrentMusic::showLyric()
{
    QString lyric = this->getLyric();
    this->lyricCallback(lyric);
}

void CurrentMusic::lyricCallback(const QString &future)
{
    this->detailInfo->removeAllLyricLabels();
    this->count = 0;
    this->order = -2;

    QStringList result = future.split('\n');
    this->parent->player->lrc_lst = ::getLyric(result);

    for (int x = 0; x < result.size(); ++x) {
        QString i = result[x];
        QRegularExpression re("\\[+(.*?)\\]+(.*)");
        QRegularExpressionMatch data = re.match(i);
        // [''] or [':)']
        if (!data.hasMatch()) {
            if (i.isEmpty())
                continue;
            else {
                _LyricLabel* label = new _LyricLabel("00:00", i, x, this);
                this->detailInfo->addLyricLabel(label);
                continue;
            }
        }

        // [00:17.460]
        if (data.captured(2).isEmpty()) {
            _LyricLabel* label = new _LyricLabel(data.captured(1), "\n", x, this);
            this->detailInfo->addLyricLabel(label);
            continue;
        }

        // [00:01.00] 作词 : buzz
        _LyricLabel* label = new _LyricLabel(data.captured(1), data.captured(2), x, this);
        this->detailInfo->addLyricLabel(label);
    }

    this->count = result.size();
}

void CurrentMusic::slide()
{
    if (count <= 0)
        return;
    double tmp = this->detailInfo->lyricFrames->maximumValue() / static_cast<double>(this->count);
    int maxValue = static_cast<int>(tmp) * this->order;
    this->detailInfo->lyricFrames->verticalScrollBar()->setValue(maxValue);
}

void CurrentMusic::unLightLyric()
{
    if (this->order < 0)
        return;
    Q_ASSERT(this->order < this->detailInfo->allLyrics.size());
    this->detailInfo->allLyrics[this->order]->unLightMe();
}

QString CurrentMusic::getLyric()
{
    QHash<QString,QString> musicInfo = this->parent->player->getCurrentMusicInfo();
    if (musicInfo.isEmpty())
        return tr("✧请慢慢欣赏~");

    long long musicId = musicInfo["music_id"].toLongLong();
    if (this->currentMusicId == musicId)
        return this->lyricCache;

    QString lyricUrl = musicInfo["lyric"];
    QString data;
    // 默认网易云，网易云没有返回歌词地址，所以lyricUrl.isEmpty.
    if (lyricUrl.isEmpty()) {
        data = netease->lyric(musicId); // 在这里获取歌词
    }

    if (data.isEmpty()) {
        this->currentMusicId = musicId;
        return tr("✧请慢慢欣赏~");
    }

    this->currentMusicId = musicId;
    this->lyricCache = data;
    return data;
}

void CurrentMusic::getShortInfo()
{
    this->detailInfo->hide();
    this->showShort = new QPropertyAnimation(this, "geometry");

    this->showShort->setStartValue(QRect(0, this->grandparent->header->height(),
                                          this->grandparent->width(),
                                          this->grandparent->mainContent->height()));
    this->showShort->setEndValue(QRect(0, this->grandparent->height()-64-this->parent->height(),
                                       this->grandparent->navigation->width(), 64));
    this->showShort->setDuration(300);
    this->showShort->setEasingCurve(QEasingCurve::InBack);

    this->showShort->start(QAbstractAnimation::DeleteWhenStopped);

    this->shortInfo->show();
    this->raise();
}

void CurrentMusic::setFeatures()
{
    connect(detailInfo->recoveryButton, SIGNAL(clicked()), SLOT(getShortInfo()));
}

void CurrentMusic::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->mousePos = QCursor::pos();
}

void CurrentMusic::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (QCursor::pos() != this->mousePos)
        return;
    else
        this->getDetailInfo();
}


/********** CurrentMusicShort **********/
CurrentMusicShort::CurrentMusicShort(CurrentMusic* parent)
    : QFrame ()
{
    this->parent = parent;
    this->setObjectName("short");

    this->setLabels();
    this->setLayouts();
    this->init();
}

void CurrentMusicShort::setLabels()
{
    this->musicName = new QLabel(this);
    this->musicName->adjustSize();
    this->musicAuthor = new QLabel(this);

    this->musicPic = new PicLabel("resource/no_music.png", 64, 64);
    this->musicPic->setObjectName("musicPic");

    this->musicMask = new PicLabel("resource/expand.png", 64, 64);
    this->musicMask->hide();
    this->musicMask->setStyleSheet("QLabel {background-color: rgba(0, 0, 0, 50%;)}");

    this->musicLayout = new QVBoxLayout(this->musicPic);
    this->musicLayout->setContentsMargins(0, 0, 0, 0);
    this->musicLayout->setSpacing(0);
    this->musicLayout->addWidget(this->musicMask);
}

void CurrentMusicShort::setLayouts()
{
    this->mainLayout = new QHBoxLayout();
    this->mainLayout->setContentsMargins(0, 0, 0, 0);

    this->musicInfo = new QVBoxLayout();
    this->musicInfo->setContentsMargins(0, 0, 0, 0);

    this->musicInfo->addWidget(this->musicName);
    this->musicInfo->addWidget(this->musicAuthor);

    this->mainLayout->addWidget(this->musicPic);
    this->mainLayout->addLayout(this->musicInfo);

    this->mainLayout->setStretch(0, 1);
    this->mainLayout->setStretch(1, 2);

    this->setLayout(this->mainLayout);
}

void CurrentMusicShort::init()
{
    this->musicName->setText("Enjoy it");
    this->musicAuthor->setText("Enjoy it");
}

void CurrentMusicShort::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (!this->musicMask->isVisible())
        this->musicMask->show();
}

void CurrentMusicShort::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (this->musicMask->isVisible())
        this->musicMask->hide();
}


/********** CurrentMusicDetail **********/
CurrentMusicDetail::CurrentMusicDetail(CurrentMusic* parent)
    : ScrollArea (parent)
{
    this->setObjectName("detail");
    this->hide();

    this->mainLayout = new QVBoxLayout(this->frame);
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->mainLayout->setSpacing(0);

    this->topLayout = new QHBoxLayout();
    this->topLayout->setContentsMargins(0, 0, 0, 0);
    this->topLayout->setSpacing(0);

    this->topMainLayout = new QVBoxLayout();
    this->topMainLayout->setContentsMargins(0, 0, 0, 0);
    this->topMainLayout->setSpacing(0);

    this->topHeaderLayout = new QHBoxLayout();
    this->topHeaderLayout->setContentsMargins(0, 0, 0, 0);
    this->topHeaderLayout->setSpacing(0);

    this->lyricFrames = new ScrollArea(nullptr);
    this->lyricFrames->setObjectName("lyricScroll");
    this->lyricFrames->frame->setObjectName("lyricFrame");
    this->lyricFramesLayout = new QVBoxLayout(this->lyricFrames->frame);
    this->lyricFramesLayout->setContentsMargins(0, 0, 0, 0);
    this->lyricFramesLayout->setSpacing(0);

    this->allLyrics = QList<_LyricLabel*>();

    this->titleLabel = new QLabel(tr("✧✧✧"));
    this->titleLabel->setObjectName("titleLabel");

    this->recoveryButton = new QPushButton();
    this->recoveryButton->setIcon(QIcon("resource/recovery.png"));
    this->recoveryButton->setObjectName("recoveryButton");
    this->recoveryButton->setMinimumSize(24, 24);
    this->recoveryButton->setMaximumSize(36, 36);

    this->setLayouts();
}

void CurrentMusicDetail::setLayouts()
{
    // 源码这个函数写得比较乱，这里调整一下顺序
    this->topHeaderLayout->addStretch(1);
    this->topHeaderLayout->addSpacing(100);
    this->topHeaderLayout->addWidget(this->titleLabel);
    this->topHeaderLayout->addStretch(1);
    this->topHeaderLayout->addSpacing(20);
    this->topHeaderLayout->addWidget(this->recoveryButton);
    this->topHeaderLayout->addSpacing(50);

    this->topMainLayout->addSpacing(25);
    this->topMainLayout->addLayout(this->topHeaderLayout);
    this->topMainLayout->addSpacing(30);
    this->topMainLayout->addWidget(this->lyricFrames);

    this->topLayout->addLayout(this->topMainLayout);

    this->mainLayout->addLayout(this->topLayout);
}

void CurrentMusicDetail::addLyricLabel(_LyricLabel *label)
{
    QList<QWidget*> widgets;
    widgets.append(label);
    HStretchBox* tmp = new HStretchBox(this->lyricFramesLayout, widgets);
    Q_UNUSED(tmp);

    this->allLyrics.append(label);
}

void CurrentMusicDetail::removeAllLyricLabels()
{
    foreach (_LyricLabel* label, this->allLyrics)
        label->deleteLater();
    this->allLyrics.clear();

    QLayoutItem *child;//源码是lyricFramesLayout.takeAt(i)
    while ((child = this->lyricFramesLayout->takeAt(0)) != nullptr) {
        delete child;
    }
}


/********** Player **********/
Player::Player(PlayWidgets* parent)
    : QMediaPlayer ()
{
    this->setObjectName("player");

    this->parent = parent;
    this->transTime = itv2time;
    this->musicTime = 0;
    this->playList = new _MediaPlaylist(this);

    this->playList->setPlaybackMode(_MediaPlaylist::Loop);

    this->lrc_lst = QList<QPair<QString,QString>>();

    this->setConnects();
}

void Player::setConnects()
{
    connect(this, SIGNAL(durationChanged(qint64)), SLOT(countTimeEvent(qint64)));
    connect(this, SIGNAL(positionChanged(qint64)), SLOT(positionChangedEvent(qint64)));
    connect(this, SIGNAL(stateChanged(QMediaPlayer::State)), SLOT(stateChangedEvent()));
    connect(this, SIGNAL(error(QMediaPlayer::Error)), SLOT(dealError(QMediaPlayer::Error)));

    this->playList->setInitConnection();
}

void Player::setDisconnects()
{
    disconnect(this, SIGNAL(durationChanged(qint64)), this, SLOT(countTimeEvent(qint64)));
    disconnect(this, SIGNAL(positionChanged(qint64)), this, SLOT(positionChangedEvent(qint64)));
    disconnect(this, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(stateChangedEvent()));
    disconnect(this, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(dealError(QMediaPlayer::Error)));

    disconnect(this, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), nullptr, nullptr);
}

bool Player::setMusic(const QString &url, const QHash<QString, QString> &data)
{
    if (!url.isEmpty()) {
        if (url.contains("http") || url.contains("file"))
            //bug DirectShowPlayerService::doSetUrlSource: Unresolved error code 0x80040216 ()
            // 是因为下面这行，第一次，传入的url是"http0",不是合法url
            this->playList->addMedias(QMediaContent(QUrl(url)), data);
        else
            this->playList->addMedias(QMediaContent(QUrl::fromLocalFile(url)), data);
        return true;
    }
    return false;
}

void Player::setAllMusics(const QList<QHash<QString, QString> > &datas)
{
    this->playList->addAllMedias(datas);
}

void Player::setIndex(int index)
{
    this->playList->setCurrentIndex(index);
}

qint64 Player::allTime() const
{
    return this->duration() / 1000;
}

QHash<QString,QString> Player::getCurrentMusicInfo() const
{
    return this->playList->mediaList.value(
                this->currentMedia().canonicalUrl().toString());
}

void Player::playMusic()
{
    //源码是playMusic(self, url=None, data=None)
    //不过全部三处调用都是playMusic()
    this->parent->playEvent(this);
}

void Player::dealError(QMediaPlayer::Error error)
{
    if (error != QMediaPlayer::NoError) {
        QString url = this->currentMedia().canonicalUrl().toString();
        if (this->playList->mediaList.contains(url)) {
            QHash<QString,QString> musicInfo = this->playList->mediaList.take(url);
            this->loadRealMusicUrl(musicInfo);
        }
        else {
            qDebug() << "尝试重新获取音乐地址出错，请清空或删除无效的音乐信息后重试。";
        }
    }
}

void Player::loadRealMusicUrl(QHash<QString, QString> musicInfo)
{
    long long musicId = musicInfo["music_id"].toLongLong();
    qDebug() << "歌曲地址失效，尝试重新获取，id号:" << musicId;

    if (!musicId) {
        this->parent->nextSing();
        return;
    }

    QList<long long> ids;
    ids.append(musicId);
    QByteArray json = netease->singsUrl(ids);
    if (json.isEmpty()) {
        this->parent->nextSing();
        return;
    }
    QJsonDocument document = QJsonDocument::fromJson(json);
    QJsonObject object = document.object();
    QJsonValue value = object["data"];
    if (!value.isArray())
        return;
    QJsonArray playlist = value.toArray();

    Q_ASSERT(playlist.size() == 1);
    QString url = playlist[0].toObject()["url"].toString();
    musicInfo["url"] = url;
    this->playList->mediaList[url] = musicInfo;

    Q_ASSERT(0 <= playList->currentIndex &&
             playList->currentIndex < playList->musics.size());
    this->playList->musics[playList->currentIndex] = Music(url);
    this->playList->play();
}

void Player::saveCookies()
{
    this->playList->saveCookies();
}

void Player::loadCookies()
{
    this->playList->loadCookies();
}

void Player::countTimeEvent(qint64 duration)
{
    this->musicTime = duration / 1000;
    this->parent->countTime->setText(transTime(static_cast<int>(this->musicTime)));
    this->playList->duration = duration;
}

void Player::positionChangedEvent(qint64 position)
{
    qint64 currentTime = this->position() / 1000;
    QString transedTime = transTime(static_cast<int>(currentTime));
    this->parent->currentTime->setText(transedTime);
    emit timeChanged(transedTime);

    if (this->musicTime == 0)
        return;

    // 先乘再除，防止溢出
    this->parent->slider->setValue(static_cast<int>(currentTime * 1000/this->musicTime));
    this->setLyricEvent(position);
}

void Player::stateChangedEvent()
{
    if (this->state() == QMediaPlayer::StoppedState && this->playList->mediaCount() == 0
            && this->parent->pauseButton->isVisible())
        this->parent->stopEvent(this);
}

void Player::setLyricEvent(qint64 position)
{
    QTime t(0, 0, 0);
    t = t.addMSecs(static_cast<int>(position));
    QString lycF, lycL, lycM;
    if (!this->lrc_lst.isEmpty()) {
        int lenOfLrc = this->lrc_lst.size();
        for (int i = 0; i < lenOfLrc; ++i) {
            if (this->lrc_lst[i].first.contains(t.toString("mm:ss"))) {
                QTime t1 = t;
                int intervel;
                if (i < lenOfLrc - 1) {
                    QString x = this->lrc_lst[i+1].first;
                    x.replace("[", "");
                    x.replace("]", "");
                    t1 = QTime::fromString(x, "mm:ss.z");
                    intervel = t.msecsTo(t1);
                }
                else {
                    t1 = QTime::fromString("00:10.99");
                    intervel = 3000;
                }

                this->parent->desktopLyric->stopMask();
                this->parent->desktopLyric->setText(this->lrc_lst[i].second, intervel);
                this->parent->desktopLyric->startMask();
                if (i > 0)
                    lycM = this->lrc_lst[i-1].second;
                int j = 0;
                while (j < i-1) {
                    lycF += this->lrc_lst[j].second + '\n';
                    j += 1;
                }
                j = i;
                while (j < lenOfLrc-1) {
                    lycL += this->lrc_lst[j+1].second + '\n';
                    j += 1;
                }
                break;
            }
        }
    }
}


/********** _TableWidget **********/
_TableWidget::_TableWidget(PlayList *parent)
    : QTableWidget ()
{
    this->parent = parent;
    this->setMouseTracking(true);
    connect(this, SIGNAL(entered(QModelIndex)), SLOT(itemHover(QModelIndex)));
    connect(this, SIGNAL(viewportEntered()), SLOT(viewportHover()));

    this->itemDualBackground = QColor(45,47,51);
    this->itemBackground = QColor(48,50,54);
    this->itemHoverBackground = QColor(50,52,56);
    this->itemHoverColor = QColor(226,226,226);
    this->itemColor = QColor(220,221,228);
    this->itemColor2 = QColor(95,95,99);

    // 当前记录的行数，用于判断鼠标是否离开了当前行。
    this->itemHoverRow = -1;
    this->items = QList<QTableWidgetItem*>();

    this->setActions();
}

void _TableWidget::setActions()
{
    this->actionClear = new QAction(tr("清空"), this);
    connect(actionClear, SIGNAL(triggered()), SLOT(clears()));

    this->actionRemove = new QAction(tr("从列表中删除"), this);
    connect(actionRemove, SIGNAL(triggered()), SLOT(remove()));
}

void _TableWidget::_setItems(int row)
{
    this->items.append(this->item(row, 0));
    this->items.append(this->item(row, 1));
    this->items.append(this->item(row, 2));
    foreach (QTableWidgetItem* item, this->items) {
        if (item) {
            item->setForeground(this->itemHoverColor);
            item->setBackground(this->itemHoverBackground);
        }
    }
    this->itemHoverRow = row;
}

void _TableWidget::itemHover(const QModelIndex &index)
{
    int row = index.row();
    if (row != this->itemHoverRow) {
        if (this->items.isEmpty())
            this->_setItems(row);
        else {
            if (row % 2 == 0) {
                foreach (QTableWidgetItem* item, this->items) {
                    if (item)
                        item->setBackground(this->itemDualBackground);
                }
            }
            else {
                foreach (QTableWidgetItem* item, this->items) {
                    if (item)
                        item->setBackground(this->itemBackground);
                }
            }

            this->items[0]->setForeground(this->itemColor);
            for (int i = 1; i < this->items.size(); ++i) {
                QTableWidgetItem* item = this->items[i];
                item->setForeground(this->itemColor2);
            }

            this->items.clear();
            this->_setItems(row);
        }
    }
}

void _TableWidget::viewportHover()
{
    if (this->items.isEmpty())
        return;

    this->items[0]->setForeground(this->itemColor);
    for (int i = 1; i < this->items.size(); ++i) {
        QTableWidgetItem* item = this->items[i];
        item->setForeground(this->itemColor2);
    }

    this->items.clear();
    this->itemHoverRow = -1;
}

void _TableWidget::clears()
{
    this->clearContents();
    this->setRowCount(0);
    _MediaPlaylist* playlist = this->parent->parent->player->playList;
    playlist->clear();
    //playlist.removeList = []
    //没有removeList这个属性
    playlist->mediaList.clear();

    this->parent->musicList.clear();
    this->parent->currentRow = -1;
    this->parent->allRow = 0;
    this->items.clear();
    this->itemHoverRow = -1;

    this->parent->parent->player->stop();
}

void _TableWidget::remove()
{
    int row = this->currentRow();
    Player* player = this->parent->parent->player;

    this->removeRow(row);

    Q_ASSERT(row >= 0 && row < this->parent->musicList.size());
    this->parent->musicList.removeAt(row);
    player->playList->removeMedias(row);

    this->parent->allRow -= 1;
    if (this->itemHoverRow == row) {
        this->itemHoverRow = -1;
        this->items.clear();
    }
}

void _TableWidget::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event);
    QTableWidgetItem* item = this->itemAt(this->mapFromGlobal(QCursor::pos()));
    this->menu = new QMenu(this);

    if (!item)
        this->menu->addAction(this->actionClear);
    else {
        this->menu->addAction(this->actionRemove);
        this->menu->addAction(this->actionClear);
    }
    this->menu->exec(QCursor::pos());
}


/********** _MediaPlaylist **********/
QString _MediaPlaylist::musicsCookiesFolder = "cookies/mediaPlaylist/musics";
QString _MediaPlaylist::mediaListCookiesFolder = "cookies/mediaPlaylist/mediaList";
int _MediaPlaylist::single = 1;
int _MediaPlaylist::Loop = 3;
int _MediaPlaylist::Random = 4;
// 之所以定义为1、3、4，是参考的QMediaPlaylist::PlaybackMode枚举

_MediaPlaylist::_MediaPlaylist(Player* parent)
    : QObject ()
{
    this->parent = parent;
    this->playWidgets = this->parent->parent;

    this->duration = 0;

    this->musics = QList<Music>();

    this->currentIndex = 0;

    // 用于记录歌曲的信息。
    this->mediaList.clear();

    // 默认是3.循环。
    this->myModel = 3;
}

void _MediaPlaylist::setInitConnection()
{
    connect(this->parent, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            SLOT(mediaStatusChangedEvent(QMediaPlayer::MediaStatus)));
}

void _MediaPlaylist::addMedias(const QMediaContent &url, const QHash<QString, QString> &data)
{
    this->parent->setMedia(url);
    this->musics.append(Music(url));
    this->currentIndex = this->musics.size() - 1;
    this->mediaList[url.canonicalUrl().toString()] = data;
    this->parent->playMusic();

    this->playWidgets->parent->systemTray->setToolTip(data["name"] + '-' + data["author"]);
}

void _MediaPlaylist::addAllMedias(const QList<QHash<QString, QString> > &datas)
{
    foreach (auto dict, datas) {
        this->musics.append(Music(dict["url"]));
        this->mediaList[dict["url"]] = dict;
    }
}

void _MediaPlaylist::clear()
{
    this->musics.clear();
    this->mediaList.clear();
    this->currentIndex = 0;
    this->duration = 0;
}

int _MediaPlaylist::mediaCount() const
{
    return this->musics.size();
}

void _MediaPlaylist::removeMedias(int row)
{
    if (this->musics.size() == 0)
        return;
    Q_ASSERT(0 <= row && row < this->musics.size());
    this->musics.removeAt(row);
}

void _MediaPlaylist::next()
{
    if (this->myModel == 4) {
        //random.randint(a,b)
        //Return random integer in range [a, b], including both end points.
        int index = std::rand() % this->musics.size();
        this->setCurrentIndex(index);
        return;
    }
    if (this->currentIndex + 1 >= this->musics.size())
        this->currentIndex = 0;
    else
        this->currentIndex += 1;
    this->play();
}

void _MediaPlaylist::previous()
{
    if (this->myModel == 4) {
        int index = std::rand() % this->musics.size();
        this->setCurrentIndex(index);
        return;
    }
    if (this->currentIndex - 1 <= -1)
        this->currentIndex = 0;
    else
        this->currentIndex -= 1;
    this->play();
}

void _MediaPlaylist::play()
{
    Music media;
    try {
        media = this->musics[this->currentIndex];
    } catch (...) {
        qDebug() << __FILE__ << __func__;
        return;
    }

    if (media.is_str) {
        Music content;
        if (media.url.contains("http") || media.url.contains("file"))
            content = Music(QMediaContent(QUrl(media.url)));
        else
            content = Music(QMediaContent(QUrl::fromLocalFile(media.url)));

        this->musics.insert(currentIndex, content);
        media = content;
    }

    Q_ASSERT(!media.is_str);
    this->parent->setMedia(media.content);
    this->parent->playMusic();
    this->tabMusicEvent();
}

void _MediaPlaylist::setCurrentIndex(int index)
{
    if (index < 0 || index >= this->musics.size())
        return;

    Music media = this->musics[index];
    if (media.is_str) {
        Music content;
        if (media.url.contains("http") || media.url.contains("file"))
            content = Music(QMediaContent(QUrl(media.url)));
        else
            content = Music(QMediaContent(QUrl::fromLocalFile(media.url)));

        this->musics.insert(currentIndex, content);
        this->parent->setMedia(content.content);
    }
    else
        this->parent->setMedia(media.content);

    this->parent->playMusic();
    this->tabMusicEvent();
    this->currentIndex = index;
}

void _MediaPlaylist::setPlaybackMode(int model)
{
    this->myModel = model;
}

void _MediaPlaylist::setShortInfo(const QHash<QString, QString> &musicInfo)
{
    QString name = musicInfo["name"];
    QString author = musicInfo["author"];
    QString pic = musicInfo["music_img"];
    this->playWidgets->currentMusic->setShortInfo(name, author, pic);
}

void _MediaPlaylist::setLyric(const QHash<QString, QString> &musicInfo)
{
    QString name = musicInfo["name"];
    QString author = musicInfo["author"];
    if (this->playWidgets->currentMusic->detailInfo->isVisible()
            || this->playWidgets->desktopLyric->isVisible()) {
        this->playWidgets->desktopLyric->setText(
                    QString("%1-%2").arg(name).arg(author), 0);
        this->playWidgets->currentMusic->setDetailInfo();
    }
}

void _MediaPlaylist::setSystemTrayTip(const QString &tipMessage)
{
    this->playWidgets->parent->systemTray->setToolTip(tipMessage);
}

void _MediaPlaylist::updatePlayTimes(const QHash<QString, QString> &musicInfo)
{
    if (musicInfo["url"].contains("http("))
        return;
    addPlayTimesById(musicInfo["music_id"].toLongLong(),
            musicInfo["name"], musicInfo["author"]);
}

QHash<QString,QVariant> serialize_mediaList(const QHash<QString,QHash<QString,QString>>& mediaList)
{
    QHash<QString,QVariant> res;
    for (auto it = mediaList.begin(); it != mediaList.end(); ++it) {
        QString key = it.key();
        QHash<QString,QString> dict = it.value();
        QHash<QString,QVariant> tmp;
        for (auto it2 = dict.begin(); it2 != dict.end(); ++it2)
            tmp[it2.key()] = it2.value();

        res[key] = tmp;
    }
    return res;
}

QHash<QString,QHash<QString,QString>> deserialize_mediaList(const QHash<QString,QVariant>& res)
{
    QHash<QString,QHash<QString,QString>> mediaList;
    for (auto it = res.begin(); it != res.end(); ++it) {
        QString key = it.key();
        QHash<QString,QVariant> tmp = it.value().toHash();
        QHash<QString,QString> dict;
        for (auto it2 = tmp.begin(); it2 != tmp.end(); ++it2)
            dict[it2.key()] = it2.value().toString();

        mediaList[key] = dict;
    }
    return mediaList;
}

void _MediaPlaylist::saveCookies()
{
    QStringList strList;
    for (int row = 0; row < this->musics.size(); ++row) {
        Music data = this->musics[row];
        if (!data.is_str)
            this->musics[row] = Music(this->musics[row].content.canonicalUrl().toString());
        strList.append(this->musics[row].url);
    }
    QSettings settings;
    settings.setValue(this->musicsCookiesFolder, strList);

    QHash<QString,QVariant> res = serialize_mediaList(this->mediaList);
    settings.setValue(this->mediaListCookiesFolder, res);
}

void _MediaPlaylist::loadCookies()
{
    QSettings settings;
    QHash<QString,QVariant> res = settings.value(mediaListCookiesFolder).toHash();
    this->mediaList = deserialize_mediaList(res);

    //
}

void _MediaPlaylist::mediaStatusChangedEvent(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        // 循环
        if (this->myModel == Loop)
            this->next();
        // 随机
        else if (this->myModel == Random) {
            int index = std::rand() % this->musics.size();
            this->setCurrentIndex(index);
        }
        // 单曲
        else
            this->parent->play();
    }
}

void _MediaPlaylist::tabMusicEvent()
{
    QString indexUrl;
    try {
        indexUrl = this->parent->currentMedia().canonicalUrl().toString();
        QHash<QString,QString> musicInfo = this->mediaList[indexUrl];
        this->setShortInfo(musicInfo);

        this->setLyric(musicInfo);

        this->setSystemTrayTip(musicInfo["name"]+'-'+musicInfo["author"]);
        this->updatePlayTimes(musicInfo);
    } catch (...) {
        qDebug() << QString("tabbing music error has ignored. index%1").arg(indexUrl);
    }
}


/********** _LyricLabel **********/
_LyricLabel::_LyricLabel(const QString& myTime,const QString& lyric,int myOrder,CurrentMusic* parent)
    : QLabel (lyric)
{
    this->setObjectName("lyric");
    this->parent = parent;
    this->myTime = myTime;
    int idx = this->myTime.lastIndexOf('.');
    if (idx > -1) // xx:yy.zzz时间只精确到xx:yy
        this->myTime = this->myTime.left(idx);
    this->myLyric = lyric;
    this->myOrder = myOrder;

    connect(parent->parent->player, SIGNAL(timeChanged(const QString&)),
            this, SLOT(lightMe(const QString&)));

    this->setMinimumHeight(40);

    this->setTextInteractionFlags(Qt::TextSelectableByMouse);
}

void _LyricLabel::lightMe(const QString &currentTime)
{
    if (currentTime == this->myTime) {
        if (this->parent->order != this->myOrder) {
            this->parent->unLightLyric();
            this->parent->order = this->myOrder;
            this->parent->slide();
            //如果是当前时间，则歌词颜色显示为白色
            this->setText(QString("<font color=\"white\">%1</font>").arg(this->myLyric));
        }
    }
}

void _LyricLabel::unLightMe()
{
    this->setText(this->myLyric);
}


/********** DesktopLyric **********/
DesktopLyric::DesktopLyric(PlayWidgets *parent)
    : QDialog ()
{
    Q_UNUSED(parent);
    this->lyric = QString("Lyric Show.");
    this->intervel = 0;
    this->maskRect = QRectF(0, 0, 0, 0);
    this->maskWidth = 0;
    this->widthBlock = 0;
    this->t = new QTimer();
    this->screen = QApplication::desktop()->availableGeometry();
    this->setObjectName("Dialog");
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint
                         | Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::Tool);
    this->setMinimumHeight(65);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->handle = new Lyric_Handle(this);
    this->verticalLayout = new QVBoxLayout(this);
    this->verticalLayout->setSpacing(0);
    this->verticalLayout->setContentsMargins(0, 0, 0, 0);
    this->verticalLayout->setObjectName("verticalLayout");
    this->font = QFont("微软雅黑", 50);
    this->font.setPixelSize(50);
    connect(handle, SIGNAL(lyricmoved(const QPoint&)), SLOT(newPos(const QPoint&)));
    connect(this->t, SIGNAL(timeout()), SLOT(changeMask()));
}

void DesktopLyric::changeMask()
{
    this->maskWidth += this->widthBlock;
    this->update();
}

void DesktopLyric::setText(const QString &s, int intervel)
{
    this->lyric = s;
    this->intervel = intervel;
    this->maskWidth = 0;
    this->update();
}

void DesktopLyric::hideLyric()
{
    this->hide();
}

void DesktopLyric::leaveEvent(QEvent *event)
{
    this->handle->leaveEvent(event);
}

void DesktopLyric::hide()
{
    QDialog::hide();
    this->handle->hide();
}

void DesktopLyric::newPos(const QPoint &p)
{
    this->move(this->pos().x() + p.x(), this->pos().y() + p.y());
}

void DesktopLyric::startMask()
{
    this->t->start(100);
}

void DesktopLyric::stopMask()
{
    this->t->stop();
    this->update();
}

void DesktopLyric::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setFont(this->font);

    QLinearGradient linear(QPoint(this->rect().topLeft()),
                           QPoint(this->rect().bottomLeft()));
    linear.setStart(0, 10);
    linear.setFinalStop(0, 50);
    linear.setColorAt(0.1, QColor(14, 179, 255));
    linear.setColorAt(0.5, QColor(154, 232, 255));
    linear.setColorAt(0.9, QColor(14, 179, 255));

    QLinearGradient linear2(QPoint(this->rect().topLeft()),
                            QPoint(this->rect().bottomLeft()));
    linear2.setStart(0, 10);
    linear2.setFinalStop(0, 50);
    linear2.setColorAt(0.1, QColor(222, 54, 4));
    linear2.setColorAt(0.5, QColor(255, 172, 116));
    linear2.setColorAt(0.9, QColor(222, 54, 4));

    painter.setPen(QColor(0, 0, 0, 200));
    painter.drawText(QRect(1, 1, this->screen.width(), 60),
                     Qt::AlignHCenter | Qt::AlignVCenter, this->lyric);

    painter.setPen(QColor(Qt::transparent));
    painter.drawText(QRect(0, 0, this->screen.width(), 60),
                     Qt::AlignHCenter | Qt::AlignVCenter, this->lyric, &this->textRect);

    painter.setPen(QPen(linear, 0));
    painter.drawText(this->textRect, Qt::AlignLeft | Qt::AlignVCenter, this->lyric);
    if (this->intervel != 0)
        this->widthBlock = this->textRect.width()/(this->intervel/150.0);
    else
        this->widthBlock = 0;
    this->maskRect = QRectF(this->textRect.x(), this->textRect.y(),
                            this->textRect.width(), this->textRect.height());
    this->maskRect.setWidth(this->maskWidth);
    painter.setPen(QPen(linear2, 0));
    painter.drawText(this->maskRect, Qt::AlignLeft | Qt::AlignVCenter, this->lyric);
}

void DesktopLyric::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        this->m_drag = true;
        this->m_DragPosition = event->globalPos() - this->pos();
        event->accept();
    }
}

void DesktopLyric::mouseMoveEvent(QMouseEvent *event)
{
    try {
        if (event->buttons() & Qt::LeftButton) {
            this->move(event->globalPos() - this->m_DragPosition);
            event->accept();
        }
    } catch (...) {
    }
}

void DesktopLyric::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton)
        this->m_drag = false;
}


/********** Lyric_Handle **********/
Lyric_Handle::Lyric_Handle(DesktopLyric* parent)
    : QDialog (parent)
{
    this->parent = parent;

    this->timer = new QTimer();
    this->setObjectName("Dialog");
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint
                         | Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::Tool);
    //this->setStyleSheet("QDialog { background: #2c7ec8; border: 0px solid black;}");
    this->horiLayout = new QHBoxLayout(this);
    this->horiLayout->setSpacing(5);
    this->horiLayout->setContentsMargins(0, 0, 0, 0);
    this->horiLayout->setObjectName("horiLayout");
    this->handler = new QLabel(this);
    this->handler->setToolTip("Move Lyric");
    // TODO 注意这几张图片并不存在
    this->handler->setPixmap(QPixmap(":/icons/handler.png"));
    this->handler->setMouseTracking(true);
    this->lockBt = new PushButton2(this);
    this->lockBt->setToolTip("Unlocked");
    this->lockBt->loadPixmap(QPixmap(":/icons/unlock.png"));
    this->hideBt = new PushButton2(this);
    this->hideBt->setToolTip("Hide Lyric");
    this->hideBt->loadPixmap(QPixmap(":/icons/close.png").copy(48, 0, 16, 16));
    this->lockBt->setCheckable(true);

    this->horiLayout->addWidget(this->handler);
    this->horiLayout->addWidget(this->lockBt);
    this->horiLayout->addWidget(this->hideBt);

    connect(lockBt, SIGNAL(clicked()), SLOT(lockLyric()));
    connect(hideBt, SIGNAL(clicked()), SLOT(hideLyric()));
    connect(timer, SIGNAL(timeout()), SLOT(hide()));
}

void Lyric_Handle::lockLyric()
{
    if (this->lockBt->isChecked()) {
        this->lockBt->loadPixmap(QPixmap(":/icons/lock.png"));
        this->lockBt->setToolTip("Locked");
        this->lockBt->update();
    }
    else {
        this->lockBt->loadPixmap(QPixmap(":/icons/unlock.png"));
        this->lockBt->setToolTip("Unlocked");
        this->lockBt->update();
    }
}

void Lyric_Handle::hideLyric()
{
    this->parent->hide();
    this->hide();
}

bool Lyric_Handle::isInTitle(int xPos, int yPos) const
{
    if (this->lockBt->isChecked())
        return false;
    else
        return yPos <= this->height() && xPos >= 0
                && xPos <= this->handler->width();
}

void Lyric_Handle::moveEvent(QMoveEvent *event)
{
    emit lyricmoved(QPoint(event->pos() - event->oldPos()));
}

void Lyric_Handle::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    this->setFocus();
    this->timer->stop();
}

void Lyric_Handle::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    this->timer->stop();
    this->timer->start(3000);
}


// 辅助函数
QList<QPair<QString,QString>> getLyric(const QStringList& rawLyric)
{
    QRegularExpression r1("\\[(\\d{2}:\\d{2}(.\\d+)?)\\]");
    QRegularExpression r2("\\[\\d+:+.+\\](.*)");

    QList<QPair<QString,QString>> lrc_lst;
    foreach (const QString& line, rawLyric) {
        QRegularExpressionMatch times = r1.match(line);
        QRegularExpressionMatch lrc_words = r2.match(line);
        if (!(times.hasMatch() && lrc_words.hasMatch()))
            continue;
        if (lrc_words.captured(1).trimmed().isEmpty())
            // 如果歌词部分为空；或者是类似这样的形式  || lrc_words.captured(1)[0].isSpace()
            // [00:00.000] 作曲 : 林俊杰  时间和后面之间有空格
            // 该接口返回的歌词发生了一定变化，现在歌词部分和时间之间也有了一个空格
            continue;

        QPair<QString,QString> item(times.captured(1), lrc_words.captured(1));
        lrc_lst.append(item);
    }
    std::sort(lrc_lst.begin(), lrc_lst.end());
    return lrc_lst;
}

static void test()
{
    PlayWidgets* widget = new PlayWidgets(nullptr);
    widget->show();

    CurrentMusic* current = new CurrentMusic(nullptr);
    current->show();

    CurrentMusicShort* musicshort = new CurrentMusicShort(nullptr);
    musicshort->show();
}
