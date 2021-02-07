#include "configmainfeatures.h"
#include "music.h"

ConfigWindow::ConfigWindow(Window *window)
    :QObject ()
{
    this->window = window;

    this->history = QList<int>();
    this->histindex = -1;

    this->bindConnect();
}

void ConfigWindow::addTab(QWidget *widget, const QString &name)
{
    this->window->mainContents->addTab(widget, name);
}

int ConfigWindow::allTab() const
{
    return this->window->mainContents->count();
}

void ConfigWindow::chdir(int index, bool browsing_history)
{
    if (browsing_history) {
        if (histindex >= 0 && histindex < history.size()) {
            index = this->history[histindex];
            this->window->mainContents->setCurrentIndex(index);
        }
    }
    else if (this->history.contains(index))
        this->histindex = this->history.indexOf(index);
    else {
        if (this->histindex == -1)
            this->history.clear();
        else
            this->history = this->history.mid(0, this->histindex+1);
        this->history.append(index);
        this->histindex = this->history.size()-1;
    }

    this->refresh();
}

void ConfigWindow::refresh()
{
    emit set_previous_enabled(this->histindex > 0);
    emit set_next_enabled(this->histindex >= 0 && this->histindex < this->history.size()-1);
}

void ConfigWindow::bindConnect()
{
    connect(window->mainContents, &QTabWidget::currentChanged,
            [this](int index){this->chdir(index);});
}

void ConfigWindow::nextTab()
{
    this->histindex += 1;
    this->chdir(-1, true);
}

void ConfigWindow::prevTab()
{
    if (this->histindex >= 0)
        this->histindex -= 1;
    this->chdir(-1, true);
}

void ConfigWindow::setTabIndex(int index)
{
    this->window->mainContents->setCurrentIndex(index);
}

QString ConfigWindow::getDownloadFolder() const
{
    // todo return this->window->downloadFrame->config->myDownloadFolder;
    return "";
}

void ConfigWindow::pullRecommendSong()
{
    QList<long long> topSongInHistory = getSongByPlayTimes();
    if (topSongInHistory.isEmpty())
        return;
    QList<int> randList;
    for (int i=0; i<topSongInHistory.size(); i++)
        randList.append(i);

    while (true) {
        if (randList.isEmpty())
            return;
        int randSong = randList.takeAt(randList.indexOf(random::choice(randList)));

        long long songId = topSongInHistory[randSong];
        QSet<QString> relativePlaylistIds = netease->getContainedPlaylists(songId);
        if (relativePlaylistIds.isEmpty())
            continue;

        for (auto it=relativePlaylistIds.begin();
             it!=relativePlaylistIds.end(); it++) {
            long long playlistId = (*it).toLongLong();
            QList<SongInfo> songInfo = netease->getRandomSongFromPlaylist(playlistId);
            if (songInfo.isEmpty())
                continue;

            this->window->recommendFrame->config->setSongs(songInfo);
            return;
        }
    }
}


/********** ConfigHeader **********/
QString ConfigHeader::loginCookiesFolder = "cookies/headers/loginInfor";

ConfigHeader::ConfigHeader(Header* header)
    : QObject ()
{
    this->header = header;

    connect(this, SIGNAL(finished()), this, SLOT(loginFinished()));
    connect(this, SIGNAL(breakSignal(const QString&)), this, SLOT(emitWarning(const QString&)));

    if (!this->header->loginBox->currentFrame) {
        connect(this->header->loginBox->phoneAndEMailFrame->enterLoginButton,
                SIGNAL(clicked()), this, SLOT(login()));
    }

    this->loginInfor = QJsonObject();
    this->result = QPair<int,QList<QHash<QString,QString>>>();
    this->songsDetail = QHash<long long,QString>();

    this->code = 200;
    this->isMax = false;

    this->bindConnect();
    this->loadCookies();
}

void ConfigHeader::bindConnect()
{
    connect(this->header->closeButton, SIGNAL(clicked()),
            this->header->parent, SLOT(close()));
    connect(this->header->showminButton, SIGNAL(clicked()),
            this->header->parent, SLOT(showMinimized()));
    connect(this->header->showmaxButton, SIGNAL(clicked()),
            this, SLOT(showMaxiOrRevert()));

    connect(this->header->loginButton, SIGNAL(clicked()),
            this, SLOT(showLoginBox()));
    connect(this->header->prevButton, SIGNAL(clicked()),
            this->header->parent->config, SLOT(prevTab()));
    connect(this->header->nextButton, SIGNAL(clicked()),
            this->header->parent->config, SLOT(nextTab()));

    connect(this->header->parent->config, &ConfigWindow::set_previous_enabled,
            this->header->prevButton, &QPushButton::setEnabled);
    connect(this->header->parent->config, &ConfigWindow::set_next_enabled,
            this->header->nextButton, &QPushButton::setEnabled);

    connect(this->header->searchLine->button, SIGNAL(clicked()),
            this, SLOT(search()));
}

void ConfigHeader::showMaxiOrRevert()
{
    if (this->isMax) {
        this->header->parent->showNormal();
        this->isMax = false;
    }
    else {
        this->header->parent->showMaximized();
        this->isMax = true;
    }
}

void ConfigHeader::search()
{
    QString text = this->header->searchLine->text();
    QByteArray json = netease->search(text);
    if (json.isEmpty())
        return;
    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return;
    QJsonObject object = document.object();
    QJsonValue value = object["result"];
    if (!value.isObject())
        return;
    QJsonObject result = value.toObject();
    int songCount = result["songCount"].toInt();
    QJsonArray songs = result["songs"].toArray();

    if (songCount <= 0) {
        this->result.first = 0;
        this->result.second = QList<QHash<QString,QString>>();
    }
    else {
        QList<long long> songsIds;
        for (int i = 0; i < songs.size(); ++i) {
            QJsonObject dict = songs[i].toObject();
            songsIds.append(static_cast<long long>(dict["id"].toDouble()));
        }

        /********** 通过歌曲id获取歌曲url **********/
        this->songsDetail = musicid2url(songsIds);

        this->result.second.clear();
        for (int i = 0; i < songs.size(); ++i) {
            QJsonObject dict = songs[i].toObject();

            QHash<QString,QString> tmp;
            tmp["name"] = dict["name"].toString();

            QStringList artists;
            QJsonArray array = dict["ar"].toArray();
            for (int j = 0; j < array.size(); ++j) {
                QJsonObject item = array[j].toObject();
                artists.append(item["name"].toString());
            }
            QString authors = artists.join(',');
            tmp["authors"] = authors;

            tmp["picUrl"] = dict["al"].toObject()["picUrl"].toString();
            tmp["mp3Url"] = this->songsDetail[static_cast<long long>(dict["id"].toDouble())];
            tmp["duration"] = QString::number(dict["dt"].toInt());
            tmp["music_id"] = QString::number(static_cast<long long>(dict["id"].toDouble()));

            this->result.second.append(tmp);
        }
    }

    this->header->parent->searchArea->setText(text);
    this->header->parent->searchArea->config->setSingsData(this->result.second);
    this->header->parent->config->setTabIndex(5);
}

void ConfigHeader::showLoginBox()
{
    this->header->loginBox->open();
}

void ConfigHeader::login()
{
    // 用户名和密码
    QPair<QString,QString> informations =
            this->header->loginBox->checkAndGetLoginInformation();
    if (informations.first.isEmpty() || informations.second.isEmpty())
        return;
    this->loadLoginInformations(informations);
}

void ConfigHeader::loadLoginInformations(QPair<QString, QString> informations)
{
    QByteArray json = netease->login(informations.first, informations.second);
    if (json.isEmpty()) {
        emit breakSignal(tr("请检查网络后重试~."));
        this->code = 500;
        return;
    }

    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject()) {
        return;
    }
    QJsonObject object = document.object();
    QJsonValue code = object["code"];
    if (code.toInt() != 200) {
        emit breakSignal(object["msg"].toString());
        this->code = 500;
        return;
    }

    this->loginInfor = object;
    this->code = 200;

    this->username = informations.first;
    this->password = informations.second;
    emit finished();
}

void ConfigHeader::loginFinished()
{
    if (this->code == 200) {
        this->header->loginBox->accept();
        this->setUserData();
    }
}

void ConfigHeader::setUserData()
{
    QJsonObject profile = this->loginInfor["profile"].toObject();
    QString avatarUrl = profile["avatarUrl"].toString();
    //qDebug() << "avatarUrl = " << avatarUrl;
    this->header->userPix->setSrc(avatarUrl);

    long long userId = static_cast<long long>(profile["userId"].toDouble());
    //qDebug() << "userId = " << userId;
    QByteArray json = netease->user_playlist(userId);
    if (json.isEmpty())
        return;
    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return;
    QJsonObject object = document.object();
    QJsonValue value = object["playlist"];
    if (!value.isArray())
        return;
    QJsonArray playlist = value.toArray();

    QString nickname = profile["nickname"].toString();
    //qDebug() << "nickname = " << nickname;
    this->header->loginButton->setText(nickname);

    disconnect(this->header->loginButton, SIGNAL(clicked()), nullptr, nullptr);
    connect(this->header->loginButton, SIGNAL(clicked()), this, SLOT(exitLogin()));

    this->header->parent->navigation->config->setPlaylists(playlist);
}

void ConfigHeader::emitWarning(const QString& warningStr)
{
    this->header->loginBox->setWarningAndShowIt(warningStr);
}

void ConfigHeader::exitLogin()
{
    this->loginInfor = QJsonObject();
    this->header->loginButton->setText(tr("未登录 ▼"));
    connect(this->header->loginButton, SIGNAL(clicked()),
            SLOT(showLoginBox()));
    this->header->userPix->setSrc("resource/nouser.png");
    this->header->parent->navigation->config->clearPlaylists();
}

void ConfigHeader::saveCookies()
{
    QSettings settings;
    settings.beginGroup(loginCookiesFolder);
    settings.setValue("username", this->username);
    settings.setValue("password", this->password);
    settings.endGroup();
}

void ConfigHeader::loadCookies()
{
    /*
    QSettings settings;
    settings.beginGroup(loginCookiesFolder);
    if (!settings.contains("username")) {
        settings.endGroup();
        return;
    }

    QString username = settings.value("username").toString();
    QString password = settings.value("password").toString();
    this->loadLoginInformations(QPair<QString,QString>(username, password));
    settings.endGroup();*/
}


/********** ConfigNavigation **********/
ConfigNavigation::ConfigNavigation(Navigation* navigation)
    : QObject ()
{
    this->navigation = navigation;
    this->detailFrame = this->navigation->parent->detailSings;
    this->window = this->navigation->parent;

    this->playlists = QList<PlaylistButton*>();

    this->result = QJsonObject();
    this->singsUrls = QStringList();
    this->coverImgUrl = QString();

    this->bindConnect();
}

void ConfigNavigation::bindConnect()
{
    connect(this->navigation->navigationList, SIGNAL(itemPressed(QListWidgetItem *)),
            SLOT(navigationListItemClickEvent()));
    connect(this->navigation->nativeList, SIGNAL(itemPressed(QListWidgetItem *)),
            SLOT(nativeListItemClickEvent(QListWidgetItem *)));
}

void ConfigNavigation::navigationListItemClickEvent()
{
    foreach (PlaylistButton* button, this->playlists) {
        if (button->isChecked()) {
            button->setCheckable(false);
            button->setCheckable(true);
            break;
        }
    }

    this->navigation->nativeList->setCurrentRow(-1);
    this->navigationListFunction();
}

void ConfigNavigation::nativeListItemClickEvent(QListWidgetItem *item)
{
    foreach (PlaylistButton* button, this->playlists) {
        if (button->isChecked()) {
            button->setCheckable(false);
            button->setCheckable(true);
            break;
        }
    }

    this->navigation->nativeList->setCurrentRow(-1);
    this->tabNativeFrame(item);
}

void ConfigNavigation::singsButtonClickEvent()
{
    this->navigation->navigationList->setCurrentRow(-1);
    this->navigation->nativeList->setCurrentRow(-1);
}

void ConfigNavigation::setPlaylists(const QJsonArray &datas)
{
    this->navigation->mainLayout->takeAt(this->navigation->mainLayout->count()-1);
    for (int i = 0; i < datas.size(); ++i) {
        QJsonObject dict = datas[i].toObject();
        long long id = static_cast<long long>(dict["id"].toDouble());
        QString coverImgUrl = dict["coverImgUrl"].toString();
        QString name = dict["name"].toString();

        PlaylistButton* button = new PlaylistButton(this, id, coverImgUrl,
                                                    QIcon("resource/notes2.png"), name);
        connect(button, SIGNAL(hasClicked(long long, const QString&)),
                this, SLOT(startRequest(long long,const QString&)));
        this->playlists.append(button);
        this->navigation->mainLayout->addWidget(button);
    }
    this->navigation->mainLayout->addStretch(1);
}

void ConfigNavigation::clearPlaylists()
{
    foreach (PlaylistButton* btn, this->playlists)
        btn->deleteLater();
    this->playlists.clear();
    qDebug() << "该函数可能出错";
    qDebug() << __FILE__ << __func__;
    int count = this->navigation->mainLayout->count();
    for (int i = 11; i < count; ++i) {
        this->navigation->mainLayout->takeAt(i);
    }
    this->navigation->mainLayout->addStretch(1);
}

void ConfigNavigation::startRequest(long long ids,const QString& coverImgUrl)
{
    this->coverImgUrl = coverImgUrl;
    this->singsButtonClickEvent();

    /********** 通过歌单id获取歌单中歌曲id **********/
    QByteArray json = netease->details_playlist(ids);
    //qDebug() << __func__ << ids;
    if (json.isEmpty())
        return;
    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return;
    QJsonObject object = document.object();
    QJsonValue value = object["result"];
    if (!value.isObject())
        return;
    this->result = value.toObject();

    QJsonValue val = this->result["tracks"];
    QJsonArray tracks = val.toArray();
    QList<long long> singsIds;

    for (int i = 0; i < tracks.size(); ++i) {
        QJsonObject dict = tracks[i].toObject();
        long long music_id = static_cast<long long>(dict["id"].toDouble());
        singsIds.append(music_id);
    }

    /********** 通过歌曲id获取歌曲url **********/
    QHash<long long,QString> id_url_dict = musicid2url(singsIds);

    this->singsUrls.clear();
    foreach (long long music_id, singsIds) {
        this->singsUrls.append(id_url_dict[music_id]);
    }

    /**********  **********/
    this->detailFrame->config->setupDetailFrames(this->result,
                                                 this->singsUrls, singsIds);
    this->detailFrame->picLabel->setSrc(this->coverImgUrl);
    this->detailFrame->picLabel->setStyleSheet("QLabel {padding: 10px;}");

    this->window->mainContents->setCurrentIndex(1);
}

void ConfigNavigation::navigationListFunction()
{
    if (this->navigation->navigationList->currentRow() == 0)
        this->navigation->parent->mainContents->setCurrentIndex(0);
}

void ConfigNavigation::tabNativeFrame(QListWidgetItem *item)
{
    if (item->text() == " 本地音乐")
        this->window->mainContents->setCurrentIndex(2);
    else if (item->text() == " 我的下载")
        this->window->mainContents->setCurrentIndex(3);
    else if (item->text() == " 专属推荐")
        this->window->mainContents->setCurrentIndex(4);
}


/********** ConfigMainContent **********/
ConfigMainContent::ConfigMainContent(MainContent* mainContent)
    : QObject ()
{
    this->mainContent = mainContent;
}


/********** ConfigSearchArea **********/
ConfigSearchArea::ConfigSearchArea(SearchArea* searchArea)
    : QObject ()
{
    this->currentIndex = 0;
    this->currentName = tr("网易云");

    this->searchArea = searchArea;

    this->downloadFolder = this->searchArea->parent->config->getDownloadFolder();
    this->transTime = itv2time;

    this->searchResultTableIndexs = this->searchArea->neteaseSearchFrame->singsResultTable;

    this->musicList = QList<QHash<QString,QString>>();
    this->noContents = tr("很抱歉 未能找到关于<font style='text-align: center;' color='#23518F'>“%1”</font>的%2。");

    this->bindConnect();
    this->setContextMenu();
}

void ConfigSearchArea::bindConnect()
{
    connect(searchArea->contentsTab, SIGNAL(tabBarClicked(int)),
            SLOT(searchBy(int)));

    connect(searchArea->neteaseSearchFrame->singsResultTable,
            SIGNAL(itemDoubleClicked(QTableWidgetItem*)), SLOT(itemDoubleClickedEvent()));
    //connect(searchArea->xiamiSearchFrame->singsResultTable,
    //        SIGNAL(itemDoubleClicked(QTableWidgetItem*)), SLOT(itemDoubleClickedEvent()));
    //connect(searchArea->qqSearchFrame->singsResultTable,
     //       SIGNAL(itemDoubleClicked(QTableWidgetItem*)), SLOT(itemDoubleClickedEvent()));

    searchArea->neteaseSearchFrame->singsResultTable->contextMenuFun =
            [this](QContextMenuEvent *event){this->contextEvent(event);};
    //connect(searchArea->xiamiSearchFrame->singsResultTable,
    //        SIGNAL(sigRightButtonPress()), SLOT(contextEvent()));
    //connect(searchArea->qqSearchFrame->singsResultTable,
    //        SIGNAL(sigRightButtonPress()), SLOT(contextEvent()));
}

void ConfigSearchArea::setContextMenu()
{
    actionDownloadSong = new QAction(tr("下载"), this);
    connect(actionDownloadSong, SIGNAL(triggered()), SLOT(downloadSong()));
}

void ConfigSearchArea::downloadSong()
{
    Q_ASSERT(0 <= currentIndex && currentIndex < this->musicList.size());
    QHash<QString,QString> musicInfo = musicList[currentIndex];
    QString url = musicInfo["url"];
    if (!url.contains("http:") && !url.contains("https")) {
        long long songId = musicInfo["music_id"].toLongLong();
        QList<long long> ids;
        ids.append(songId);

        /********** 通过歌曲id获取歌曲url **********/
        QHash<long long,QString> id_url_dict = musicid2url(ids);
        musicInfo["url"] = id_url_dict[songId];
    }
    emit download(musicInfo);
}

void ConfigSearchArea::searchBy(int index)
{
    QString currentWidgetName = this->searchArea->contentsTab->tabText(index);
    this->currentName = currentWidgetName;
    this->search(currentWidgetName);
}

void ConfigSearchArea::search(const QString &name)
{
    QByteArray json = netease->search(this->searchArea->text);
    if (json.isEmpty())
        return;
    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return;
    QJsonObject object = document.object();
    QJsonValue value = object["result"];
    if (!value.isObject())
        return;
    QJsonObject result = value.toObject();
    int songCount = result["songCount"].toInt();
    QJsonArray songs = result["songs"].toArray();
    QList<QHash<QString,QString>> songs_tmp;

    if (songCount > 0) {
        QList<long long> songsIds;
        for (int i = 0; i < songs.size(); ++i) {
            QJsonObject dict = songs[i].toObject();
            songsIds.append(static_cast<long long>(dict["id"].toDouble()));
        }

        /********** 通过歌曲id获取歌曲url **********/
        QHash<long long,QString> songsDetail = musicid2url(songsIds);

        /**********  **********/
        for (int i = 0; i < songs.size(); ++i) {
            QJsonObject dict = songs[i].toObject();

            QHash<QString,QString> tmp;
            tmp["name"] = dict["name"].toString();

            QStringList artists;
            QJsonArray array = dict["ar"].toArray();
            for (int j = 0; j < array.size(); ++j) {
                QJsonObject item = array[j].toObject();
                artists.append(item["name"].toString());
            }
            QString authors = artists.join(',');
            tmp["authors"] = authors;

            tmp["picUrl"] = dict["al"].toObject()["picUrl"].toString();
            tmp["mp3Url"] = songsDetail[static_cast<long long>(dict["id"].toDouble())];
            tmp["duration"] = QString::number(dict["dt"].toInt());
            tmp["music_id"] = QString::number(static_cast<long long>(dict["id"].toDouble()));
            tmp["lyric"] = QString();//dict["lyric"].toString();网易提供的API没有lyric这一项

            songs_tmp.append(tmp);
        }
    }

    this->setSingsData(songs_tmp);
}

void ConfigSearchArea::setSingsData(const QList<QHash<QString, QString> > &data)
{
    SingsSearchResultFrameBase* searchArea =
            dynamic_cast<SingsSearchResultFrameBase*>(this->searchArea->contentsTab->currentWidget());
    if (!searchArea)
        return;
    if (data.size() == 0) {
        searchArea->noSingsContentsLabel->setText(
                    this->noContents.arg(this->searchArea->text).arg(tr("单曲")));
        searchArea->singsResultTable->hide();
        searchArea->noSingsContentsLabel->show();
    }
    else {
        searchArea->singsResultTable->setRowCount(data.size());

        QList<QHash<QString,QString>> musicList;
        for (int count = 0; count <data.size(); ++count) {
            QHash<QString,QString> datas = data[count];

            QString picUrl = datas["picUrl"];
            QString url = datas["'mp3Url"];
            QString name = datas["name"];
            QString authors = datas["authors"];
            QString duration = transTime(datas["duration"].toInt() / 1000);
            QString musicId = datas["music_id"];

            searchArea->singsResultTable->setItem(count, 0, new QTableWidgetItem(name));
            searchArea->singsResultTable->setItem(count, 1, new QTableWidgetItem(authors));
            searchArea->singsResultTable->setItem(count, 2, new QTableWidgetItem(duration));

            QHash<QString,QString> tmp;
            tmp["name"] = name;
            tmp["time"] = duration;
            tmp["author"] = authors;
            tmp["music_img"] = picUrl;
            tmp["music_id"] = musicId;
            musicList.append(tmp);
        }

        searchArea->noSingsContentsLabel->hide();
        searchArea->singsResultTable->show();

        this->musicList = musicList;
    }
}

void ConfigSearchArea::itemDoubleClickedEvent()
{
    SingsSearchResultFrameBase* searchArea =
            dynamic_cast<SingsSearchResultFrameBase*>(this->searchArea->contentsTab->currentWidget());
    if (!searchArea)
        return;
    int currentRow = searchArea->singsResultTable->currentRow();
    Q_ASSERT(0 <= currentRow && currentRow < this->musicList.size());
    QHash<QString,QString> data = this->musicList[currentRow];
    this->searchArea->parent->playWidgets->setPlayerAndPlayList(data);
}

void ConfigSearchArea::contextEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event);
    TableWidget* currentWidget = searchResultTableIndexs;
    if (!currentWidget)
        return;

    QTableWidgetItem* item = currentWidget->itemAt(currentWidget->mapFromGlobal(QCursor::pos()));
    this->menu = new QMenu(currentWidget);
    this->menu->addAction(this->actionDownloadSong);

    this->currentIndex = item->row() - 1;
    if (this->currentIndex < -1)
        this->currentIndex = -1;

    this->menu->exec(QCursor::pos());
}


/********** ConfigDetailSings **********/
ConfigDetailSings::ConfigDetailSings(DetailSings* parent)
    : QObject ()
{
    this->detailSings = parent;
    this->musicList = QList<QHash<QString,QString>>();

    this->currentIndex = 0;

    this->grandparent = this->detailSings->parent;
    this->player = this->grandparent->playWidgets->player;
    this->playList = this->grandparent->playWidgets;
    this->currentMusic = this->grandparent->playWidgets->currentMusic;
    this->transTime = itv2time;
    this->detailSings->singsTable->contextMenuFun =
            [this](QContextMenuEvent *event){this->singsFrameContextMenuEvent(event);};

    this->bindConnect();
    this->setContextMenu();
}

void ConfigDetailSings::bindConnect()
{
    connect(this->detailSings->playAllButton, SIGNAL(clicked()),
            SLOT(addAllMusicToPlayer()));
    connect(this->detailSings->singsTable, SIGNAL(itemDoubleClicked(QTableWidgetItem *)),
            SLOT(itemDoubleClickedEvent()));
}

void ConfigDetailSings::setContextMenu()
{
    this->actionNextPlay = new QAction(tr("下一首播放"), this);
    connect(actionNextPlay, SIGNAL(triggered()), SLOT(addToNextPlay()));

    this->actionDownloadSong = new QAction(tr("下载"), this);
    connect(actionDownloadSong, SIGNAL(triggered()), SLOT(downloadSong()));
}

void ConfigDetailSings::addToNextPlay()
{
    Q_ASSERT(0<=this->currentIndex && this->currentIndex<this->musicList.size());
    QHash<QString,QString> data = this->musicList[this->currentIndex];
    QList<QHash<QString,QString>> list;
    list.append(data);
    this->player->setAllMusics(list);
    this->playList->playList->addMusic(data);
    this->playList->playList->addPlayList(data["name"], data["author"], data["time"]);
}

void ConfigDetailSings::downloadSong()
{
    Q_ASSERT(0 <= currentIndex && currentIndex < this->musicList.size());
    QHash<QString,QString> musicInfo = musicList[currentIndex];
    QString url = musicInfo["url"];
    if (!url.contains("http:") && !url.contains("https")) {
        long long songId = musicInfo["music_id"].toLongLong();
        QList<long long> ids;
        ids.append(songId);

        /********** 通过歌曲id获取歌曲url **********/
        QHash<long long,QString> id_url_dict = musicid2url(ids);
        musicInfo["url"] = id_url_dict[songId];
    }
    emit download(musicInfo);
}

void ConfigDetailSings::addAllMusicToPlayer()
{
    this->playList->setPlayerAndPlaylists(this->musicList);
}

void ConfigDetailSings::setupDetailFrames(const QJsonObject &datas, const QStringList &singsUrls,
                                          const QList<long long> &singsIds)
{
    this->musicList.clear();

    this->detailSings->singsTable->clearContents();
    this->detailSings->titleLabel->setText(datas["name"].toString());
    QJsonObject creator = datas["creator"].toObject();
    this->detailSings->authorName->setText(creator["nickname"].toString());
    // 有些没有简介
    QString description;
    if (datas.contains("description"))
        description = datas["description"].toString();

    this->detailSings->descriptionText->setText(description);
    int trackCount = datas["trackCount"].toInt();
    this->detailSings->singsTable->setRowCount(trackCount);
    int len = std::min(trackCount, std::min(singsIds.size(),
                                            singsUrls.size()));
    QJsonArray tracks = datas["tracks"].toArray();
    for (int j = 0; j < len; ++j) {
        QJsonObject i = tracks[j].toObject();
        QString names = i["name"].toString();
        QTableWidgetItem* musicName = new QTableWidgetItem(names);
        this->detailSings->singsTable->setItem(j, 0, musicName);

        QJsonArray artists = i["artists"].toArray();
        Q_ASSERT(artists.size() >= 1);
        QJsonObject art = artists[0].toObject();
        QString author = art["name"].toString();
        QTableWidgetItem* musicAuthor = new QTableWidgetItem(author);
        this->detailSings->singsTable->setItem(j, 1, musicAuthor);

        int duration = i["duration"].toInt();
        QString times = transTime(duration / 1000);
        QTableWidgetItem* musicTime = new QTableWidgetItem(times);
        this->detailSings->singsTable->setItem(j, 2, musicTime);

        QJsonObject album = i["album"].toObject();
        QString music_img = album["blurPicUrl"].toString();

        QString lyric = i["lyric"].toString();

        QHash<QString,QString> dict;
        dict["url"] = singsUrls[j];
        dict["name"] = names;
        dict["time"] = times;
        dict["author"] = author;
        dict["music_img"] = music_img;
        long long music_id = singsIds[j];
        dict["music_id"] = QString::number(music_id);
        dict["lyric"] = lyric;

        this->musicList.append(dict);
    }
}

void ConfigDetailSings::itemDoubleClickedEvent()
{
    int currentRow = this->detailSings->singsTable->currentRow();
    Q_ASSERT(0 <= currentRow && currentRow < this->musicList.size());
    QHash<QString,QString> data = this->musicList[currentRow];

    this->playList->setPlayerAndPlayList(data);
}

void ConfigDetailSings::singsFrameContextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event);
    QTableWidgetItem* item = this->detailSings->singsTable->itemAt(
                detailSings->singsTable->mapFromGlobal(QCursor::pos()));
    this->menu = new QMenu(this->detailSings->singsTable);

    this->menu->addAction(this->actionNextPlay);
    this->menu->addAction(this->actionDownloadSong);

    this->currentIndex = item->row();//源码这里-1导致获取上一首歌曲
    //if (this->currentIndex < -1)
        //this->currentIndex = -1;

    this->menu->exec(QCursor::pos());
}


/********** ConfigSystemTray **********/
ConfigSystemTray::ConfigSystemTray(SystemTray* systemTray)
    : QObject ()
{
    this->systemTray = systemTray;
    this->addActions();
}

void ConfigSystemTray::addActions()
{
    QAction* closeAction = new QAction(tr("退出"), this->systemTray);
    connect(closeAction, SIGNAL(triggered()),
            this->systemTray->parent, SLOT(close()));
    this->systemTray->addAction(closeAction);
}
