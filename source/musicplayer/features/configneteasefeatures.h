#ifndef CONFIGNETEASEFEATURES_H
#define CONFIGNETEASEFEATURES_H

#include "apis/neteaseapi.h"
#include "widgets/neteasesingsframes.h"

class ConfigNetEase : public QObject
{
    Q_OBJECT
signals:
    void finished_net();
    void finished_sings();
public:
    NetEaseSingsArea* netEase;
    MainContent* netEaseParent;
    DetailSings* detailFrame;
    Window* window;

    QList<QJsonObject> result;//67行

    QJsonObject reqResult;
    // cache
    QStringList singsUrls;
    QString picName;

    QList<OneSing*> singsFrames;
    QStringList singPicUrls;//79行
    QStringList singNames;
    QList<long long> playlistIds;
    QList<long long> singsIds;

    bool sliderDown;
    int gridRow;
    int gridColumn;
    int offset;
    int offsetComplement;
    int myHeight;

    ConfigNetEase(NetEaseSingsArea* parent);
    void initThread();
    void getSings();
    void requestsDetail(long long ids);

public slots:
    void threadSetSings();//
    //void _setStyleCodesByThreadPool();//
    void startRequest(long long ids,const QString& picName);//

    void setRequestsDetail();//
    void sliderDownEvent();//
};

#endif // CONFIGNETEASEFEATURES_H
