#include "neteaseapi.h"

NetEaseWebApi::NetEaseWebApi()
    : HttpRequest ()
{}

QByteArray NetEaseWebApi::httpRequest(const QString &url, const QString &method,
                                      QHash<QString, QString> data)
{
    if (!data.isEmpty())
        data = encrypted_request(data);

    QByteArray json = HttpRequest::httpRequest(url, method, data);
    return json;
}

QByteArray NetEaseWebApi::login(const QString &username, QString password)
{
    // 登录 无bug NetEaseWebApi netease;
    // netease->login("15733229885", "2b137945");
    QCryptographicHash md(QCryptographicHash::Md5);
    md.addData(password.toUtf8());
    password =  md.result().toHex();

    QHash<QString,QString> data;
    data["password"] = password;
    data["rememberLogin"] = "true";

    bool email;
    if (username.contains('@'))
        email = true;
    else
        email = false;
    if (email) {
        data["username"] = username;
        data["clientToken"] = "1_ZfeoJYpgEKaPdueAuHxAz56XkZ28JiOG_SrSyeuuSjQrobJdGvXFN2Jo4fzHb+oRQ";
    }
    else
        data["phone"] = username;

    QString urlEmail = "http://music.163.com/weapi/login?csrf_token=";
    QString urlPhone = "http://music.163.com/weapi/login/cellphone?csrf_token=";
    QString url = email ? urlEmail : urlPhone;

    QByteArray json;
#if defined (DEBUG_MACRO)
    data.clear();
    data["encSecKey"] = "a8b2d0f7df308d42273b2d61f770e2bfbabebbb81a68c80d89e64d7e43d9957f9817fd7ea66202017d6a5f0c8cb84c3937f53b745950f6b0722b31d5726006610acc679a91d2297a4ae506d80ea5f12f4581d283b7fa8eb957e85b83c36fdab4a348276eba86f89ecd5f871ad6b00e29239acc1c06bd309d68c6c82bc8ded2d6";
    data["params"] = "edNRAhkGwYd0v0Bb1k2sXVffnnDvA6nZNOS+HNztx6AG+gfC3KxZ+naLDk1s0q2zndwSbJApODiCeQSGWv9AWtT57XSnZj/d9sMsbHC1Nu+epUI3wPFxT+o7gL3egTaHGYAdKXp6meraPa3rTXy5Y81IJ5j4oALqNiqjETbLSQdkhOvGT7KxXdmdsQcz8bHliN/2RNp5jbWyS781OVO9nQ==";
    json = HttpRequest::httpRequest(url, "POST", data);
#else
    json = this->httpRequest(url, "POST", data);
#endif

    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return json;
    QJsonObject object = document.object();
    QJsonValue value = object["profile"];
    if (!value.isObject())
        return json;
    QJsonObject profile = value.toObject();

    QString avatarUrl = profile["avatarUrl"].toString();
    qDebug() << "avatarUrl =" << avatarUrl;

    long long userId = static_cast<long long>(profile["userId"].toDouble());
    qDebug() << "userId =" << userId;

    QString nickname = profile["nickname"].toString();
    qDebug() << "nickname =" << nickname;

    return json;
}

QByteArray NetEaseWebApi::user_playlist(long long uid, int offset, int limit)
{
    // 个人歌单   536802748是我的网易云账号的实际id
    // netease->user_playlist(536802748);
    // uid需要为long long类型
    QHash<QString,QString> data;
    data["offset"] = QString::number(offset);
    data["uid"] = QString::number(uid);
    data["limit"] = QString::number(limit);
    data["csrf_token"] = "";

    QString url = "http://music.163.com/weapi/user/playlist";

    QByteArray json;
#if defined (DEBUG_MACRO)
    data.clear();
    data["encSecKey"] = "a8b2d0f7df308d42273b2d61f770e2bfbabebbb81a68c80d89e64d7e43d9957f9817fd7ea66202017d6a5f0c8cb84c3937f53b745950f6b0722b31d5726006610acc679a91d2297a4ae506d80ea5f12f4581d283b7fa8eb957e85b83c36fdab4a348276eba86f89ecd5f871ad6b00e29239acc1c06bd309d68c6c82bc8ded2d6";
    data["params"] = "8JwpEDuUk0syKC4ydNqKOhc3h5s9IOgKF53OiRfcJld0qGJ4qZ2SmyvER6x+cBjhP/NRQv+P96xY0gQyCxu1L2hAkkVgCT6MDKO/ig8xR6p/u1ZnJ3y9OUz7FvpsTOHbGAReKu8pdHVsflPDOwdJyg==";
    json = HttpRequest::httpRequest(url, "POST", data);
#else
    json = this->httpRequest(url, "POST", data);
#endif

    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return json;
    QJsonObject object = document.object();
    QJsonValue value = object["playlist"];
    if (!value.isArray())
        return json;
    QJsonArray playlist = value.toArray();
    for (int i = 0; i < playlist.size(); ++i) {
        QJsonValue tmp = playlist[i];
        QJsonObject dict = tmp.toObject();
        //qDebug() << "id =" << static_cast<long long>(dict["id"].toDouble());
        // 有的id超过int类型的范围
        //qDebug() << "coverImgUrl =" << dict["coverImgUrl"].toString();
        //qDebug() << "name =" << dict["name"].toString();
    }
    return json;
}

QByteArray NetEaseWebApi::all_playlist(int offset, const QString &cat,
                                       const QString &types, int index)
{
    // 全部歌单
    // netease->all_playlist();
    QString url = QString("http://music.163.com/api/playlist/list?cat=%1&type=%2&order=%3&offset=%4&total=true&limit=30&index=%5")
            .arg(cat).arg(types).arg(types).arg(offset).arg(index);

    QByteArray json = this->httpRequest(url, "GET");

    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return json;
    QJsonObject object = document.object();
    QJsonValue value = object["playlists"];
    if (!value.isArray())
        return json;
    QJsonArray playlist = value.toArray();
    for (int i = 0; i < playlist.size(); ++i) {
        QJsonValue tmp = playlist[i];
        QJsonObject dict = tmp.toObject();
        //qDebug() << "id =" << static_cast<long long>(dict["id"].toDouble());
        // 有的id超过int类型的范围
        //qDebug() << "coverImgUrl =" << dict["coverImgUrl"].toString();
        //qDebug() << "name =" << dict["name"].toString();
    }
    return json;
}

QByteArray NetEaseWebApi::details_playlist(long long ids)
{
    // 某个歌单的详情 无bug
    // netease->details_playlist(3081025492);
    QString url = QString("http://music.163.com/api/playlist/detail?id=%1").arg(ids);
    QByteArray json = this->httpRequest(url, "GET");

    return json;
}

QByteArray NetEaseWebApi::search(const QString &s, int offset, int limit, int stype)
{
    // netease->search("清明雨上");
    QString url = "http://music.163.com/weapi/cloudsearch/get/web";
    QHash<QString,QString> data;
    data["s"] = s;
    data["offset"] = QString::number(offset);
    data["limit"] = QString::number(limit);
    data["type"] = QString::number(stype);

    QByteArray json;
#if defined (DEBUG_MACRO)
    data.clear();
    data["encSecKey"] = "a8b2d0f7df308d42273b2d61f770e2bfbabebbb81a68c80d89e64d7e43d9957f9817fd7ea66202017d6a5f0c8cb84c3937f53b745950f6b0722b31d5726006610acc679a91d2297a4ae506d80ea5f12f4581d283b7fa8eb957e85b83c36fdab4a348276eba86f89ecd5f871ad6b00e29239acc1c06bd309d68c6c82bc8ded2d6";
    data["params"] = "vWQ8lUyLijCmf/1sih55VMRw+nfWNzxQNDYDZ81HnHvxyEH9gu19motYNk/jcuoAMcJO3+XIh2VcP42CyOJNEN0csx+/CChmfj6lZHKP7YD7BmGFiUSBgWeUEb31WZd8nbEwCk//pDgH6xq5XOatCw==";
    json = HttpRequest::httpRequest(url, "POST", data);
#else
    data = encrypted_request2(data);
    json = HttpRequest::httpRequest(url, "POST", data);
#endif

    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return json;
    QJsonObject object = document.object();
    QJsonValue value = object["result"];
    if (!value.isObject())
        return json;
    QJsonObject result = value.toObject();
    int songCount = result["songCount"].toInt();
    //qDebug() << "songCount =" << songCount;
    QJsonArray songs = result["songs"].toArray();
    for (int i = 0; i < songs.size(); ++i) {
        QJsonObject dict = songs[i].toObject();
        QString name = dict["name"].toString();
        //qDebug() << "name =" << name;

        QString picUrl = dict["al"].toObject()["picUrl"].toString();
        //qDebug() << "picUrl =" << picUrl;

        long long music_id = static_cast<long long>(dict["id"].toDouble());
        //qDebug() << "music_id =" << music_id;

        int duration = dict["dt"].toInt();
        //qDebug() << "duration =" << duration;
    }

    return json;
}

QByteArray NetEaseWebApi::singsUrl(const QList<long long> &ids)
{
    // QList<long long> ids;ids << 108795 << 108796;netease->singsUrl(ids);
    QString url = "http://music.163.com/weapi/song/enhance/player/url";

    QByteArray json;
#if defined (DEBUG_MACRO)
    QHash<QString,QString> data;
    data.clear();
    data["encSecKey"] = "a8b2d0f7df308d42273b2d61f770e2bfbabebbb81a68c80d89e64d7e43d9957f9817fd7ea66202017d6a5f0c8cb84c3937f53b745950f6b0722b31d5726006610acc679a91d2297a4ae506d80ea5f12f4581d283b7fa8eb957e85b83c36fdab4a348276eba86f89ecd5f871ad6b00e29239acc1c06bd309d68c6c82bc8ded2d6";
    data["params"] = "8JwpEDuUk0syKC4ydNqKOi5okbIv4oi4gtb/kZfdY79OHavTG3n7LC9yBzT9GxAhi2LnMhYE9Gv8ZezwpmnoLsaPSdqgeFHCbC0ACFqw58febcQz8tdRBllcUt1U4l/e";
    json = HttpRequest::httpRequest(url, "POST", data);
#else
    QHash<QString,QString> data = encrypted_request(ids);
    json = HttpRequest::httpRequest(url, "POST", data);
#endif

    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return json;
    QJsonObject object = document.object();
    QJsonValue value = object["data"];
    if (!value.isArray())
        return json;
    QJsonArray playlist = value.toArray();
    for (int i = 0; i < playlist.size(); ++i) {
        QJsonValue tmp = playlist[i];
        QJsonObject dict = tmp.toObject();
        //qDebug() << "url =" << dict["url"].toString();
    }
    return json;
}

QByteArray NetEaseWebApi::newsong(int areaID, int offset, const QString &total, int limit)
{
    // netease->newsong();
    QString url = QString("http://music.163.com/api/discovery/new/songs?areaId=%1&offset=%2&total=%3&limit=%4")
            .arg(areaID).arg(offset).arg(total).arg(limit);
    return this->httpRequest(url, "GET");
}

QByteArray NetEaseWebApi::fnewsong(int year, int month, const QString &area)
{
    // netease->fnewsong();
    QString url = QString("http://music.163.com/api/discovery/new/albums/area?year=%1&month=%2&area=%3&type=hot&offset=0&total=true&limit=20&rcmd=true")
            .arg(year).arg(month).arg(area);
    return this->httpRequest(url, "GET");
}

QString NetEaseWebApi::lyric(long long ids)
{
    // netease->lyric(108795);
    QString url = QString("http://music.163.com/api/song/lyric?os=osx&id=%1&lv=-1&kv=-1&tv=-1").arg(ids);
    QByteArray json = this->httpRequest(url, "GET");

    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return QString();
    QJsonObject object = document.object();
    QJsonObject lrc = object["lrc"].toObject();
    QJsonValue tmp = lrc["lyric"];
    QString lyric = tmp.toString();
    //qDebug() << "lyric =" << lyric;
    return lyric;
}

QSet<QString> NetEaseWebApi::getContainedPlaylists(long long songId)
{
    // 无bug  http://music.163.com/song?id=29953681重定向到
    // https://music.163.com/song?id=29953681
    // netease->getContainedPlaylists(29953681);
    QString url = QString("https://music.163.com/song?id=%1").arg(songId);
    QByteArray json = HttpRequest::httpRequest(url, "GET");

    QRegularExpression re("<ul class=\"m-rctlist f-cb\">[.\\s\\S]+?</ul>");
    QString containedUl;
    QRegularExpressionMatch match = re.match(json);
    if (match.hasMatch())
        containedUl = match.captured();

    QRegularExpression re2("data-res-id=\"(.+)\"");
    QSet<QString> playlists;
    QRegularExpressionMatchIterator it = re2.globalMatch(containedUl);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        playlists.insert(match.captured(1));
    }
    //qDebug() <<"playlist ="<< playlists;
    return playlists;
}

QList<SongInfo> NetEaseWebApi::getRandomSongFromPlaylist(long long playlistId)
{
    QList<SongInfo> SongInfoList;

    QByteArray json = this->details_playlist(playlistId);
    QJsonDocument document = QJsonDocument::fromJson(json);
    if (!document.isObject())
        return SongInfoList;
    QJsonObject object = document.object();
    QJsonValue value = object["result"];
    if (!value.isObject())
        return SongInfoList;
    QJsonObject result = value.toObject();
    QJsonValue val = result["tracks"];
    if (!val.isArray())
        return SongInfoList;
    QJsonArray tracks = val.toArray();

    for (int i = 0; i < tracks.size(); ++i) {
        QJsonObject dict = tracks[i].toObject();
        long long music_id = static_cast<long long>(dict["id"].toDouble());
        //qDebug() << "music_id =" << music_id;

        int time = dict["duration"].toInt();
        //qDebug() << "time =" << time;

        QString name = dict["name"].toString();
        //qDebug() << "name =" << name;

        QString music_img = dict["album"].toObject()["blurPicUrl"].toString();

        QStringList list;
        QJsonArray artists = dict["artists"].toArray();
        for (int j = 0; j < artists.size(); ++j) {
            QJsonObject tmp = artists[j].toObject();
            list.append(tmp["name"].toString());
        }
        QString author = list.join('-');
        //qDebug() << "author =" << author;

        SongInfo songInfo(music_id, music_img, "http(0)",
                          QString(), time, name, author);
        SongInfoList.append(songInfo);
    }
    return SongInfoList;
}

NetEaseWebApi* netease = new NetEaseWebApi;

QHash<long long,QString> musicid2url(const QList<long long>& songsIds)
{
    /********** 通过歌曲id获取歌曲url **********/
    QHash<long long,QString> id_url_dict;
    QByteArray json = netease->singsUrl(songsIds);
    if (json.isEmpty())
        return id_url_dict;
    QJsonDocument document = QJsonDocument::fromJson(json);
    QJsonObject object = document.object();
    QJsonValue value = object["data"];
    QJsonArray playlist = value.toArray();
    for (int i = 0; i < playlist.size(); ++i) {
        QJsonValue tmp = playlist[i];
        QJsonObject dict = tmp.toObject();
        long long music_id = static_cast<long long>(dict["id"].toDouble());
        QString url = dict["url"].toString();
        id_url_dict[music_id] = url;
    }
    return id_url_dict;
}
