#ifndef CONFIGRECOMMENDFRAMEFEATURES_H
#define CONFIGRECOMMENDFRAMEFEATURES_H

#include "widgets/addition.h"
#include "widgets/recommendframe.h"
#include "apis/neteaseapi.h"

QHash<QString,QString> songInfo_asdict(const SongInfo& songInfo);

class ConfigRecommendFrame : public QObject
{
    Q_OBJECT
public:
    RecommendFrame* recommendFrame;
    QList<QHash<QString,QString>> musicList;

    ConfigRecommendFrame(RecommendFrame* parent);
    void setSongs(const QList<SongInfo>& musicInfo);
    void bindConnect();

public slots:
    void itemDoubleClickedEvent();
};

#endif // CONFIGRECOMMENDFRAMEFEATURES_H
