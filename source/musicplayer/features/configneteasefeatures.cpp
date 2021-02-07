#include "configneteasefeatures.h"
#include "music.h"

ConfigNetEase::ConfigNetEase(NetEaseSingsArea *parent)
    : QObject ()
{
    this->netEase = parent;
    this->netEaseParent = this->netEase->parent;
    // window - > detailSings.
    this->detailFrame = this->netEaseParent->parent->detailSings;
    this->window = this->netEaseParent->parent;//源码命名为self.mainContents

    connect(this->netEase, SIGNAL(scrollDown()), SLOT(sliderDownEvent()));

    this->result = QList<QJsonObject>();

    this->reqResult = QJsonObject();
    // this->cache
    this->singsUrls = QStringList();
    this->picName = QString();

    this->singsFrames = QList<OneSing*>();
    this->singPicUrls = QStringList();
    this->singNames = QStringList();
    this->playlistIds = QList<long long>();
    this->singsIds = QList<long long>();

    this->sliderDown = false;

    // 布局用row。
    this->gridRow = 0;

    // 布局用column。
    this->gridColumn = 0;

    this->offset = 0;
    // 用于不足时的补足。
    this->offsetComplement = 30;

    this->myHeight = 0;
}

void ConfigNetEase::initThread()
{
    connect(this, SIGNAL(finished_net()), this, SLOT(threadSetSings()));
    this->getSings();

    connect(this, SIGNAL(finished_sings()), this, SLOT(setRequestsDetail()));
}

void ConfigNetEase::getSings()
{
    QByteArray json = netease->all_playlist(this->offset);
    if (json.isEmpty())
        return;
    QJsonDocument document = QJsonDocument::fromJson(json);
    QJsonObject object = document.object();
    QJsonValue value = object["playlists"];
    if (!value.isArray())
        return;
    QJsonArray playlist = value.toArray();
    if (playlist.size() == 0)
        return;
    for (int i = 0; i < playlist.size(); ++i) {
        QJsonValue tmp = playlist[i];
        QJsonObject dict = tmp.toObject();
        this->result.append(dict);

        QString name = dict["name"].toString();
        QString coverImgUrl = dict["coverImgUrl"].toString();
        long long id = static_cast<long long>(dict["id"].toDouble());

        this->singNames.append(name);
        this->singPicUrls.append(coverImgUrl);
        this->playlistIds.append(id);

        //qDebug() << "name =" << name;
        //qDebug() << "coverImgUrl =" << coverImgUrl;
        //qDebug() << "id =" << id;
    }

    emit finished_net();
}

void ConfigNetEase::threadSetSings()
{
    if (this->result.isEmpty())
        return;
    int length = this->singPicUrls.size();

    bool break_flag = true;
    for (int j = 0; j < 30; ++j) {
        int i = j + this->offset;
        if (i >= length) {
            this->offsetComplement = length % 30;
            break_flag = false;
            break;
        }

        Q_ASSERT(i>=0 && i<this->singPicUrls.size());
        QString picName = makeMd5(this->singPicUrls[i]);
        OneSing* frame = new OneSing(gridRow, gridColumn, playlistIds[i], picName);
        connect(frame, SIGNAL(clicked(long long, const QString&)),
                this, SLOT(startRequest(long long, const QString&)));
        frame->nameLabel->setText(singNames[i]);

        this->netEase->mainLayout->addWidget(frame, gridRow, gridColumn);
        this->singsFrames.append(frame);

        if (this->gridColumn == 3) {
            this->gridColumn = 0;
            this->gridRow += 1;
        }
        else
            this->gridColumn += 1;

        QStringList cacheList = os::listdir("cache");
        QString url = this->singPicUrls[i];
        QString names = makeMd5(url);
        if (cacheList.contains(names))
            frame->setStyleSheet(QString("QLabel#picLabel{border-image: url(cache/%1)}").arg(names));
        else {
            QByteArray json = network_Requests->httpRequest(url);
            if (json.isEmpty()) {
                QMessageBox::warning(nullptr, tr("网络错误"), tr("请检查网络连接后再试"));
                continue;
            }
            QPixmap pic;
            pic.loadFromData(json);
            pic = pic.scaled(180, 180);
            if (pic.save("cache/"+names, "jpg"))
                // 下面这一行就对应_setStyleCodesByThreadPool()函数
                frame->setStyleSheet(QString("QLabel#picLabel{border-image: url(cache/%1)}").arg(names));
        }
    }

    if (break_flag)
        this->offsetComplement = 30;
}

void ConfigNetEase::startRequest(long long ids,const QString& picName)
{
    this->picName = picName;
    this->requestsDetail(ids);
}

void ConfigNetEase::requestsDetail(long long ids)
{
    QByteArray json = netease->details_playlist(ids);
    if (json.isEmpty())
        return;
    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return;
    QJsonObject object = document.object();
    QJsonValue value = object["result"];
    if (!value.isObject())
        return;
    this->reqResult = value.toObject();

    QJsonValue val = this->reqResult["tracks"];
    QJsonArray tracks = val.toArray();
    this->singsIds.clear();
    this->singsUrls.clear();
    for (int i = 0; i < tracks.size(); ++i) {
        QJsonObject dict = tracks[i].toObject();
        long long music_id = static_cast<long long>(dict["id"].toDouble());
        this->singsIds.append(music_id);
        // 注意下一行代码 for i in enumerate
        this->singsUrls.append(QString("http(%1 %2)").arg(i).arg(music_id));
    }

    emit finished_sings();
}

void ConfigNetEase::setRequestsDetail()
{
    this->detailFrame->config->setupDetailFrames(this->reqResult,
                                                 this->singsUrls, this->singsIds);
    this->detailFrame->picLabel->setSrc(QString("cache/%1").arg(this->picName));
    this->detailFrame->picLabel->setStyleSheet("QLabel {padding: 10px;}");

    this->window->mainContents->setCurrentIndex(1);
}

void ConfigNetEase::sliderDownEvent()
{
    if (!this->netEase->isHidden()) {
        this->offset += this->offsetComplement;

        this->getSings();
    }
}
