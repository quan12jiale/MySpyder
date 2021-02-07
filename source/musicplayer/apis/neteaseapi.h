#ifndef NETEASEAPI_H
#define NETEASEAPI_H

//#define DEBUG_MACRO

#include "apis/apirequestsbase.h"
#include "apis/neteaseencode.h"

struct SongInfo
{
    long long music_id;
    QString music_img;
    QString url;
    QString lyric;
    int time;
    QString name;
    QString author;
    SongInfo(long long music_id,QString music_img,QString url,
             const QString& lyric,int time,
             QString name,QString author)
        : music_id(music_id), music_img(music_img), url(url),
        lyric(lyric), time(time), name(name), author(author){}
};

class NetEaseWebApi : public HttpRequest
{
    Q_OBJECT
public:
    NetEaseWebApi();
    QByteArray httpRequest(const QString& url,const QString& method="GET",
               QHash<QString,QString> data = QHash<QString,QString>());
    QByteArray login(const QString &username, QString password);
    QByteArray user_playlist(long long uid, int offset=0, int limit=1000);
    QByteArray all_playlist(int offset=0,const QString& cat="%E5%85%A8%E9%83%A8%E6%AD%8C%E5%8D%95",
               const QString& types="all",int index=1);
    QByteArray details_playlist(long long ids);

    QByteArray search(const QString& s,int offset=0,int limit=100,int stype=1);
    QByteArray singsUrl(const QList<long long>& ids);
    QByteArray newsong(int areaID=0,int offset=0,const QString& total="true",int limit=100);
    QByteArray fnewsong(int year=2015,int month=4,const QString& area="ALL");
    QString lyric(long long ids);
    QSet<QString> getContainedPlaylists(long long songId);
    QList<SongInfo> getRandomSongFromPlaylist(long long playlistId);
};

extern NetEaseWebApi* netease;

QHash<long long,QString> musicid2url(const QList<long long>& songsIds);

#endif // NETEASEAPI_H
