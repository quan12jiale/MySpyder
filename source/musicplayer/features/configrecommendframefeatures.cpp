#include "configrecommendframefeatures.h"
#include "music.h"

QHash<QString,QString> songInfo_asdict(const SongInfo& songInfo)
{
    QHash<QString,QString> data;
    data["music_id"] = QString::number(songInfo.music_id);
    data["music_img"] = songInfo.music_img;
    data["url"] = songInfo.url;
    data["lyric"] = songInfo.lyric;
    data["time"] = QString::number(songInfo.time);
    data["name"] = songInfo.name;
    data["author"] = songInfo.author;

    return data;
}

ConfigRecommendFrame::ConfigRecommendFrame(RecommendFrame *parent)
    :QObject ()
{
    this->recommendFrame = parent;
    this->musicList = QList<QHash<QString,QString>>();
    this->bindConnect();
}

void ConfigRecommendFrame::setSongs(const QList<SongInfo> &musicInfo)
{
    this->recommendFrame->singsTable->setRowCount(musicInfo.size());

    for (int index = 0; index < musicInfo.size(); ++index) {
        SongInfo data = musicInfo[index];

        this->musicList.append(songInfo_asdict(data));
        recommendFrame->singsTable->setItem(
                    index, 0, new QTableWidgetItem(data.name));
        recommendFrame->singsTable->setItem(
                    index, 1, new QTableWidgetItem(data.author));
        recommendFrame->singsTable->setItem(
                    index, 2, new QTableWidgetItem(itv2time(data.time / 1000)));
    }
}

void ConfigRecommendFrame::bindConnect()
{
    connect(recommendFrame->singsTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)),
            SLOT(itemDoubleClickedEvent()));
}

void ConfigRecommendFrame::itemDoubleClickedEvent()
{
    int currentRow = recommendFrame->singsTable->currentRow();
    Q_ASSERT(0 <= currentRow && currentRow < this->musicList.size());
    QHash<QString,QString> data = this->musicList[currentRow];
    this->recommendFrame->parent->playWidgets->setPlayerAndPlayList(data);
}
