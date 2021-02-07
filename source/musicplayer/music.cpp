#include "music.h"

Window::Window(QWidget* parent)
    : QWidget (parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setWindowIcon(QIcon("resource/format.ico"));
    this->setWindowTitle("Music");

    this->resize(1022, 670);

    this->header = new Header(this);

    skin_widget = new SkinWidget(this);
    connect(this->header, SIGNAL(showSkin()), this, SLOT(showSkinWidget()));
    connect(skin_widget, SIGNAL(changeSkin(QString)), this, SLOT(changeSkin(QString)));

    this->navigation = new Navigation(this);

    this->mainContent = new MainContent(this);
    this->detailSings = new DetailSings(this);
    this->nativeMusic = new NativeMusic(this);
    this->downloadFrame = new DownloadFrame(this);
    this->recommendFrame = new RecommendFrame(this);
    this->searchArea = new SearchArea(this);

    this->mainContents = new QTabWidget();
    this->mainContents->tabBar()->setObjectName("mainTab");

    this->playWidgets = new PlayWidgets(this);

    this->systemTray = new SystemTray("resource/logo.png", this);

    this->setContents();
    this->addAllPlaylist();
    this->setLines();
    this->setLayouts();
    this->configFeatures();

    QFile file("QSS/window.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        //this->setStyleSheet(in.readAll());
    }
    this->playWidgets->currentMusic->raise();

    this->skin_name = "resource/skin/red.png";//默认为红色
}

void Window::showSkinWidget()
{
    skin_widget->show();
}

void Window::changeSkin(QString skin_name)
{
    this->skin_name = skin_name;
    update();
}

void Window::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (!skin_name.isEmpty())
        painter.drawPixmap(rect(), QPixmap(skin_name));
}

void Window::addAllPlaylist()
{
    this->indexNetEaseSings = new NetEaseSingsArea(this->mainContent);
    //this->indexXiamiSings = new XiamiSingsArea(this->mainContent);
    //this->indexQQSings = new QQSingsArea(this->mainContent);
    this->mainContent->addTab(this->indexNetEaseSings, tr("网易云歌单"));
    //this->mainContent->addTab(this->indexXiamiSings, "虾米歌单");
    //this->mainContent->addTab(this->indexQQSings, "QQ歌单");
}

void Window::setContents()
{
    this->mainContents->addTab(this->mainContent, "MainContent");
    this->mainContents->addTab(this->detailSings, "DetailSings");
    this->mainContents->addTab(this->nativeMusic, "NativeMusic");
    this->mainContents->addTab(this->downloadFrame, "DownloadFrame");
    this->mainContents->addTab(this->recommendFrame, "RecommendFrame");
    this->mainContents->addTab(this->searchArea, "SearchArea");

    this->mainContents->setCurrentIndex(0);
}

void Window::setLines()
{
    this->line1 = new QFrame(this);
    this->line1->setObjectName("line1");
    this->line1->setFrameShape(QFrame::HLine);
    this->line1->setFrameShadow(QFrame::Plain);
    this->line1->setLineWidth(2);
}

void Window::setLayouts()
{
    this->mainLayout = new QVBoxLayout();
    this->mainLayout->addWidget(this->header);
    this->mainLayout->addWidget(this->line1);

    this->contentLayout = new QHBoxLayout();
    this->contentLayout->setStretch(0, 70);
    this->contentLayout->setStretch(1, 570);

    this->contentLayout->addWidget(this->navigation);
    this->contentLayout->addWidget(this->mainContents);

    this->contentLayout->setSpacing(0);
    this->contentLayout->setContentsMargins(0, 0, 0, 0)  ;

    this->mainLayout->addLayout(this->contentLayout);
    this->mainLayout->addWidget(this->playWidgets);

    this->mainLayout->setStretch(0, 43);
    this->mainLayout->setStretch(1, 0);
    this->mainLayout->setStretch(2, 576);
    this->mainLayout->setStretch(3, 50);

    this->mainLayout->setSpacing(0);
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(this->mainLayout);
}

void Window::configFeatures()
{
    this->config = new ConfigWindow(this);
    this->config->chdir(0);//因为this->mainContents->setCurrentIndex(0);
    this->header->config = new ConfigHeader(this->header);
    this->navigation->config = new ConfigNavigation(this->navigation);

    this->mainContent->config = new ConfigMainContent(this->mainContent);
    this->detailSings->config = new ConfigDetailSings(this->detailSings);
    //this->nativeMusic->config = new ConfigNative(this->nativeMusic);
    this->downloadFrame->config = new ConfigDownloadFrame(this->downloadFrame);
    this->recommendFrame->config = new ConfigRecommendFrame(this->recommendFrame);
    this->searchArea->config = new ConfigSearchArea(this->searchArea);

    this->indexNetEaseSings->config = new ConfigNetEase(this->indexNetEaseSings);
    //this->indexXiamiSings->config = new ConfigXiami(this->indexXiamiSings);
    //this->indexQQSings->config = new ConfigQQ(this->indexQQSings);
    this->systemTray->config = new ConfigSystemTray(this->systemTray);

    this->indexNetEaseSings->config->initThread();
    //this->indexXiamiSings->config->initThread();
    //this->indexQQSings->config->initThread();

    // 当前耦合度过高。
    this->downloadFrame->config->getDownloadSignal();

    this->config->pullRecommendSong();

    QRect screen = QApplication::desktop()->availableGeometry();
    this->playWidgets->desktopLyric->resize(screen.width(), 50);
    this->playWidgets->desktopLyric->move(0, screen.height() - 100);
}

void Window::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    this->header->config->saveCookies();
    //this->nativeMusic->config->saveCookies();
    //this->downloadFrame->config->saveCookies();
    this->playWidgets->saveCookies();

    this->systemTray->hide();
}

void Window::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    //this->playWidgets->currentMusic->raise();
    this->playWidgets->currentMusic->move(0, this->height()-64-this->playWidgets->height());
}


/********** Header **********/
Header::Header(Window* parent)
    : QFrame ()
{
    this->config = nullptr;

    this->setObjectName("Header");
    this->parent = parent;
    this->loginBox = new LoginBox(this);

    this->setButtons();
    this->setLabels();
    this->setLineEdits();
    this->setLines();
    this->setLayouts();

    QFile file("QSS/header.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        //this->setStyleSheet(in.readAll());// 源码中的header.qss文件字体部分有问题
    }
}

void Header::setButtons()
{
    this->closeButton = new QPushButton(tr("×"), this);
    this->closeButton->setObjectName("closeButton");
    this->closeButton->setMinimumSize(16, 16);

    this->showminButton = new QPushButton(tr("_"), this);
    this->showminButton->setObjectName("minButton");
    this->showminButton->setMinimumSize(16, 16);

    this->showmaxButton = new QPushButton(tr("□"));
    this->showmaxButton->setObjectName("maxButton");
    this->showmaxButton->setMinimumSize(16, 16);

    this->loginButton = new QPushButton(tr("未登录 ▼"), this);
    this->loginButton->setObjectName("loginButton");

    this->prevButton = new QPushButton(tr("<"));
    this->prevButton->setObjectName("prevButton");
    this->prevButton->setMaximumSize(28, 22);
    this->prevButton->setMinimumSize(28, 22);
    this->prevButton->setEnabled(false);

    this->nextButton = new QPushButton(tr(">"));
    this->nextButton->setObjectName("nextButton");
    this->nextButton->setMaximumSize(28, 22);
    this->nextButton->setMinimumSize(28, 22);
    this->nextButton->setEnabled(false);

    this->skin_button = new QPushButton(this);
    this->skin_button->setIcon(QIcon("resource/skin_button.png"));
    this->skin_button->setToolTip("change skin");
    this->skin_button->setMinimumSize(16, 16);
    connect(skin_button, SIGNAL(clicked()), this, SIGNAL(showSkin()));
}

void Header::setLabels()
{
    this->logoLabel = new PicLabel("resource/format.png", 32, 32);

    this->descriptionLabel = new QLabel(this);
    this->descriptionLabel->setText("<b>Music</b>");

    this->userPix = new PicLabel("resource/no_music.png", 32, 32, "resource/user_pic_mask.png");
    this->userPix->setMinimumSize(22, 22);
    this->userPix->setObjectName("userPix");
}

void Header::setLineEdits()
{
    this->searchLine = new SearchLineEdit(this);
    this->searchLine->setPlaceholderText(tr("搜索音乐, 歌手, 歌词, 用户"));
}

void Header::setLines()
{
    this->line1 = new QFrame(this);
    this->line1->setObjectName("line1");
    this->line1->setFrameShape(QFrame::VLine);
    this->line1->setFrameShadow(QFrame::Plain);
    this->line1->setMaximumSize(1, 25);
}

void Header::setLayouts()
{
    this->mainLayout = new QHBoxLayout();
    this->mainLayout->setSpacing(0);
    this->mainLayout->addWidget(this->logoLabel);
    this->mainLayout->addWidget(this->descriptionLabel);
    this->mainLayout->addSpacing(70);
    this->mainLayout->addWidget(this->prevButton);
    this->mainLayout->addWidget(this->nextButton);
    this->mainLayout->addSpacing(10);
    this->mainLayout->addWidget(this->searchLine);
    this->mainLayout->addStretch(1);
    this->mainLayout->addWidget(this->userPix);
    this->mainLayout->addSpacing(7);
    this->mainLayout->addWidget(this->loginButton);
    this->mainLayout->addSpacing(7);
    this->mainLayout->addWidget(this->skin_button);//换肤按钮
    this->mainLayout->addSpacing(7);
    this->mainLayout->addWidget(this->line1);
    this->mainLayout->addSpacing(30);
    this->mainLayout->addWidget(this->showminButton);
    this->mainLayout->addWidget(this->showmaxButton);
    this->mainLayout->addSpacing(3);
    this->mainLayout->addWidget(this->closeButton);

    this->setLayout(this->mainLayout);
}

void Header::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        this->parent->m_drag = true;
        this->parent->m_DragPosition = event->globalPos() - this->parent->pos();
        event->accept();
    }
}

void Header::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        this->parent->move(event->globalPos() - this->parent->m_DragPosition);
        event->accept();
    }
}

void Header::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton)
        this->parent->m_drag = false;
    //源码是self.m_drag = False
}


/********** Navigation **********/
Navigation::Navigation(Window* parent)
    : QScrollArea() // parent
{
    this->config = nullptr;

    this->parent = parent;
    this->frame = new QFrame();
    this->setMaximumWidth(200);

    this->setWidget(this->frame);
    this->setWidgetResizable(true);
    this->frame->setMinimumWidth(200);

    // 定义3个事件函数，方便扩展。
    //self.navigationListFunction = self.none
    //self.nativeListFunction = self.none
    //self.singsFunction = self.none

    QFile file("QSS/navigation.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        QString style = in.readAll();
        //this->setStyleSheet(style);
        //this->frame->setStyleSheet(style);
    }

    // 包括显示信息： 推荐 我的音乐 歌单。
    this->setLabels();
    // 包括详细的内容：发现音乐，FM，MV等。
    this->setListViews();

    this->setLayouts();
}

void Navigation::setLabels()
{
    this->recommendLabel = new QLabel(tr(" 推荐"));
    this->recommendLabel->setObjectName("recommendLabel");
    this->recommendLabel->setMaximumHeight(27);

    this->myMusic = new QLabel(tr(" 我的音乐"));
    this->myMusic->setObjectName("myMusic");
    this->myMusic->setMaximumHeight(27);

    this->singsListLabel = new QLabel(tr(" 收藏与创建的歌单"));
    this->singsListLabel->setObjectName("singsListLabel");
    this->singsListLabel->setMaximumHeight(27);
}

void Navigation::setListViews()
{
    this->navigationList = new QListWidget();
    this->navigationList->setMaximumHeight(110);
    this->navigationList->setObjectName("navigationList");
    this->navigationList->addItem(new QListWidgetItem(QIcon("resource/music.png"), tr(" 发现音乐")));
    this->navigationList->addItem(new QListWidgetItem(QIcon("resource/signal.png"), tr(" 私人FM")));
    this->navigationList->addItem(new QListWidgetItem(QIcon("resource/movie.png"), tr(" MV")));
    this->navigationList->setCurrentRow(0);

    this->nativeList = new QListWidget();
    this->nativeList->setObjectName("nativeList");
    this->nativeList->setMaximumHeight(100);
    this->nativeList->addItem(new QListWidgetItem(QIcon("resource/notes.png"), tr(" 本地音乐")));
    this->nativeList->addItem(new QListWidgetItem(QIcon("resource/download_icon.png"), tr(" 我的下载")));
    this->nativeList->addItem(new QListWidgetItem(QIcon("resource/recommend_icon.png"), tr(" 专属推荐")));
}

void Navigation::setLayouts()
{
    this->mainLayout = new QVBoxLayout(this->frame);
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->mainLayout->setSpacing(0);
    this->mainLayout->addSpacing(10);
    this->mainLayout->addWidget(this->recommendLabel);
    this->mainLayout->addSpacing(3);
    this->mainLayout->addWidget(this->navigationList);
    this->mainLayout->addSpacing(1);

    this->mainLayout->addWidget(this->myMusic);
    this->mainLayout->addSpacing(3);
    this->mainLayout->addWidget(this->nativeList);
    this->mainLayout->addSpacing(1);

    this->mainLayout->addWidget(this->singsListLabel);
    this->mainLayout->addSpacing(1);

    this->mainLayout->addStretch(1);

    this->setContentsMargins(0, 0, 0, 0);
}


/********** MainContent **********/
MainContent::MainContent(Window* parent)
    : ScrollArea (parent)
{
    this->config = nullptr;

    this->parent = parent;
    this->setObjectName("MainContent");

    // 连接导航栏的按钮。
    // self.parent.navigation.navigationListFunction = self.navigationListFunction
    QFile file("QSS/mainContent.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        //this->setStyleSheet(in.readAll());
    }

    this->tab = new QTabWidget();
    this->tab->setObjectName("contentsTab");

    this->mainLayout = new QVBoxLayout();
    this->mainLayout->setSpacing(0);
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->mainLayout->addWidget(this->tab);

    this->frame->setLayout(this->mainLayout);
}

void MainContent::addTab(QWidget *widget, const QString &name)
{
    this->tab->addTab(widget, name);
}


/********** SearchArea **********/
SearchArea::SearchArea(Window* parent)
    : ScrollArea (parent)
{
    this->config = nullptr;

    this->parent = parent;
    this->setObjectName("searchArea");
    QFile file("QSS/searchArea.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        //this->setStyleSheet(in.readAll());
    }

    this->mainLayout = new QVBoxLayout(this->frame);

    this->titleLabel = new QLabel(this->frame);

    this->contentsTab = new QTabWidget(this->frame);

    this->mainLayout->addWidget(this->titleLabel);
    this->mainLayout->addWidget(this->contentsTab);

    this->setSingsFrame();
}

void SearchArea::setSingsFrame()
{
    this->neteaseSearchFrame = new NetEaseSearchResultFrame(this);
    this->contentsTab->addTab(this->neteaseSearchFrame, tr("网易云"));
}

void SearchArea::setText(const QString &text)
{
    this->text = text;//ConfigSearchArea::search函数中用到
    this->titleLabel->setText(QString("搜索<font color='#23518F'>“%1”</font><br>").arg(text));
}

void start()
{
    Window* win = new Window(nullptr);//从函数返回，得用new
    win->show();
    win->playWidgets->currentMusic->resize(win->navigation->width(), 64);
}

void test2()
{
    QWidget* widget = new QWidget;

    Header* header = new Header(nullptr);

    QFrame* line1 = new QFrame(nullptr);
    line1->setObjectName("line1");
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Plain);
    line1->setLineWidth(2);

    Navigation* navigation = new Navigation(nullptr);

    PlayWidgets* playWidgets = new PlayWidgets(nullptr);

    QTabWidget* mainContents = new QTabWidget();

    // 中间部分
    MainContent* mainContent = new MainContent(nullptr);
    DetailSings* detailSings = new DetailSings(nullptr);
    NativeMusic* nativeMusic = new NativeMusic(nullptr);
    DownloadFrame* downloadFrame = new DownloadFrame(nullptr);
    RecommendFrame* recommendFrame = new RecommendFrame(nullptr);
    SearchArea* searchArea = new SearchArea(nullptr);

    mainContents->addTab(mainContent, "");
    mainContents->addTab(detailSings, "");
    mainContents->addTab(nativeMusic, "");
    mainContents->addTab(downloadFrame, "");
    mainContents->addTab(recommendFrame, "");
    mainContents->addTab(searchArea, "");

    mainContents->setCurrentIndex(0);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(header);
    mainLayout->addWidget(line1);

    QHBoxLayout* contentLayout = new QHBoxLayout();

    contentLayout->addWidget(navigation);
    contentLayout->addWidget(mainContents);

    mainLayout->addLayout(contentLayout);
    mainLayout->addWidget(playWidgets);
    widget->setLayout(mainLayout);

    CurrentMusicShort* musicshort = new CurrentMusicShort(nullptr);
    musicshort->show();

    widget->show();
}
