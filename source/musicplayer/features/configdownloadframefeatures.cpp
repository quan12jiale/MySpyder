#include "configdownloadframefeatures.h"
#include "music.h"

static HttpRequest* myRequests = new HttpRequest;

QString replace_forbidden_sym(QString string)
{
    QRegularExpression re("[\\\\/:*?\"<>|]{1}");
    string.replace(re, "");
    return string;
}

/********** ConfigNative **********/
QString ConfigDownloadFrame::myDownloadFrameCookiesFolder = "cookies/downloadInfo/downloadFolder";

ConfigDownloadFrame::ConfigDownloadFrame(DownloadFrame* downloadFrame)
    : QObject ()
{
    this->downloadFrame = downloadFrame;
    this->showTable = this->downloadFrame->singsTable;

    this->musicList = QList<QHash<QString,QString>>();
    this->folder = QStringList();
    this->myDownloadFolder = QStandardPaths::standardLocations(QStandardPaths::MusicLocation).value(0, QDir::homePath());

    this->_setDownloadFolder(this->myDownloadFolder);
    this->bindConnect();
    //this->loadCookies();
}

void ConfigDownloadFrame::bindConnect()
{
    connect(downloadFrame->selectButton, SIGNAL(clicked()), SLOT(selectFolder()));
    connect(downloadFrame->singsTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)),
            SLOT(itemDoubleClickedEvent()));
}

void ConfigDownloadFrame::getDownloadSignal()
{
    Window* window = this->downloadFrame->parent;
    connect(window->searchArea->config, SIGNAL(download(const QHash<QString,QString>&)),
            SLOT(downloadSong(const QHash<QString,QString>&)));
    connect(window->detailSings->config, SIGNAL(download(const QHash<QString,QString>&)),
            SLOT(downloadSong(const QHash<QString,QString>&)));
}

void ConfigDownloadFrame::_setDownloadFolder(const QString &folderName)
{
    this->fromPathLoadSong(folderName);
    this->myDownloadFolder = folderName;
    this->downloadFrame->currentStorageFolder->setText(folderName);
}

void ConfigDownloadFrame::downloadSong(QHash<QString, QString> musicInfo)
{
    qDebug() << "正在下载的音乐的信息:" << musicInfo;

    QString url = musicInfo["url"];
    if (url.isEmpty())
        return;
    //url = "http://m10.music.126.net/20200305100723/1bc32ee05e5c65d0da42d2f020d74a25/ymusic/0df5/35ff/179d/aeba26ef2af3d41d4724a25442816725.mp3";
    int idx = url.lastIndexOf('/');
    if (idx == -1)
        return;
    QRegularExpression re(".*\\.[a-zA-Z0-9]+");
    int idx2 = url.indexOf(re, idx+1);
    QString musicName;
    if (idx2 != -1) {
        QString allMusicName = url.mid(idx2);
        int idx3 = allMusicName.lastIndexOf('.');
        QString musicSuffix = allMusicName.mid(idx3+1);
        musicName = QString("%1.%2").arg(musicInfo["name"]+" - "+musicInfo["author"]).arg(musicSuffix);
    }
    else
        musicName = "random_name.mp3";
    musicName = replace_forbidden_sym(musicName);

    // 从托盘栏给出提示。
    this->downloadFrame->parent->systemTray->showMessage("~~~", musicName+ tr(" 加入下载队列"));
    QByteArray data = myRequests->httpRequest(url);

    QString localPath = this->myDownloadFolder + '/' + musicName;
    QFile file(localPath);
    QFileInfo info(localPath);
    if (file.open(QIODevice::WriteOnly)) {
        qint64 bytes = file.write(data);
        // 从托盘栏给出提示。
        if (bytes == -1)
            this->downloadFrame->parent->systemTray->showMessage("~~~", info.absoluteFilePath()+ tr(" 保存失败"));
        else
            this->downloadFrame->parent->systemTray->showMessage("~~~", info.absoluteFilePath()+ tr(" 下载完成"));
    }

    musicInfo["url"] = localPath;
    this->musicList.append(musicInfo);
    this->updateDownloadShowTable(musicInfo);
}

void ConfigDownloadFrame::updateDownloadShowTable(const QHash<QString, QString> &musicInfo)
{
    QStringList showInfo;
    showInfo << musicInfo["name"] << musicInfo["author"] << musicInfo["time"];

    //在这里写入音乐标题，歌手，时长
    int rowCount = this->showTable->rowCount();
    this->showTable->setRowCount(rowCount+1);
    for (int i = 0; i < 3; ++i) {
        this->showTable->setItem(rowCount, i, new QTableWidgetItem(showInfo[i]));
    }
}

void ConfigDownloadFrame::fromPathLoadSong(const QString &selectFolder)
{
    // 需要eyed3库
}

void ConfigDownloadFrame::selectFolder()
{
    QString selectFolder = QFileDialog::getExistingDirectory();
    if (!selectFolder.isEmpty()) {
        this->folder.append(selectFolder);
        this->_setDownloadFolder(selectFolder);
        this->fromPathLoadSong(selectFolder);
    }
}

void ConfigDownloadFrame::saveCookies()
{
    QSettings settings;
    settings.setValue(myDownloadFrameCookiesFolder, myDownloadFolder);
}

void ConfigDownloadFrame::loadCookies()
{
    QSettings settings;
    myDownloadFolder = settings.value(myDownloadFrameCookiesFolder).toString();
}

void ConfigDownloadFrame::itemDoubleClickedEvent()
{
    int currentRow = this->downloadFrame->singsTable->currentRow();
    Q_ASSERT(0 <= currentRow && currentRow < this->musicList.size());
    QHash<QString,QString> data = this->musicList[currentRow];

    this->downloadFrame->parent->playWidgets->setPlayerAndPlayList(data);
}
