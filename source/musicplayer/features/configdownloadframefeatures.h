#ifndef CONFIGDOWNLOADFRAMEFEATURES_H
#define CONFIGDOWNLOADFRAMEFEATURES_H

#include "widgets/downloadframe.h"

class ConfigDownloadFrame : public QObject
{
    Q_OBJECT
public:
    static QString myDownloadFrameCookiesFolder;

    DownloadFrame* downloadFrame;
    QTableWidget* showTable;
    QList<QHash<QString,QString>> musicList;
    QStringList folder;
    QString myDownloadFolder;

    ConfigDownloadFrame(DownloadFrame* downloadFrame);
    void bindConnect();
    void getDownloadSignal();
    void _setDownloadFolder(const QString& folderName);

    void updateDownloadShowTable(const QHash<QString,QString>& musicInfo);
    void fromPathLoadSong(const QString& selectFolder);

    void saveCookies();
    void loadCookies();
public slots:
    // 该类应该moveToThread
    void downloadSong(QHash<QString,QString> musicInfo);
    void selectFolder();//
    void itemDoubleClickedEvent();//
};

#endif // CONFIGDOWNLOADFRAMEFEATURES_H
