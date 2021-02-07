#include "singsframebase.h"
#include "music.h"

SingsFrameBase::SingsFrameBase(MainContent* parent)
    : ScrollArea (parent)
{
    this->parent = parent;

    this->setObjectName("allSingsArea");
    QFile file("QSS/singsFrameBase.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        //this->setStyleSheet(in.readAll());
    }

    this->mainLayout = new QGridLayout(this->frame);
    this->mainLayout->setSpacing(0);
    this->mainLayout->setHorizontalSpacing(10);
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
}


/********** SingsSearchResultFrameBase **********/
SingsSearchResultFrameBase::SingsSearchResultFrameBase(QWidget* parent)
    : QFrame ()
{
    this->parent = parent;

    this->singsFrameLayout = new QVBoxLayout(this);
    this->singsFrameLayout->setContentsMargins(0, 0, 0, 0);
    this->singsFrameLayout->setSpacing(0);

    this->noSingsContentsLabel = new QLabel(this);
    this->noSingsContentsLabel->setMaximumHeight(60);

    this->noSingsContentsLabel->setObjectName("noSingsLable");
    this->noSingsContentsLabel->hide();

    this->singsResultTable = new TableWidget(3, QStringList({tr("音乐标题"), tr("歌手"), tr("时长")}));
    this->singsResultTable->setObjectName("singsTable");
    // 由于调用了this->parent->width()，所以不能向构造函数传入空指针
    this->singsResultTable->setMinimumWidth(this->parent->width());
    QHash<int,int> dict;
    dict[0] = this->width()/3*1.25;
    dict[1] = this->width()/3*1.25;
    dict[2] = this->width()/3*0.5;
    this->singsResultTable->setColumnWidths(dict);

    this->singsFrameLayout->addWidget(this->singsResultTable,
                                      Qt::AlignTop | Qt::AlignCenter);

    this->centerLabelLayout = new QHBoxLayout();
    this->centerLabelLayout->setContentsMargins(0, 0, 0, 0);
    this->centerLabelLayout->setSpacing(0);
    this->centerLabelLayout->addStretch(1);
    this->centerLabelLayout->addWidget(this->noSingsContentsLabel);
    this->centerLabelLayout->addStretch(1);

    this->singsFrameLayout->addLayout(this->centerLabelLayout);
}


/********** DetailSings **********/
DetailSings::DetailSings(Window *parent)
    : ScrollArea (parent)
{
    this->config = nullptr;

    this->parent = parent;
    this->setObjectName("detailSings");

    this->setLabels();
    this->setButtons();
    this->setTabs();
    this->setLayouts();

    QFile file("QSS/detailSings.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        //this->setStyleSheet(in.readAll());
    }
}

void DetailSings::setLabels()
{
    this->picLabel = new PicLabel(QString(), 200, 200);
    this->picLabel->setObjectName("picLabel");

    this->titleLabel = new QLabel(this->frame);
    this->titleLabel->setObjectName("titleLabel");
    this->titleLabel->setWordWrap(true);
    this->titleLabel->setMaximumHeight(40);

    this->authorPic = new QLabel(this->frame);
    this->authorName = new QLabel(this->frame);
    this->authorName->setObjectName("authorName");
    this->authorName->setMaximumHeight(28);

    this->descriptionText = new QTextEdit(this->frame);
    this->descriptionText->setReadOnly(true);
    this->descriptionText->setObjectName("descriptionText");
    this->descriptionText->setMaximumWidth(450);
    this->descriptionText->setMaximumHeight(100);
    this->descriptionText->setMinimumHeight(100);
}

void DetailSings::setButtons()
{
    this->showButton = new QPushButton(tr("歌单"));
    this->showButton->setObjectName("showButton");
    this->showButton->setMaximumSize(36, 20);

    this->descriptionButton = new QPushButton(tr("简介"));
    this->descriptionButton->setObjectName("descriptionButton");
    this->descriptionButton->setMaximumSize(36, 36);

    this->playAllButton = new QPushButton(tr("全部播放"));
    this->playAllButton->setIcon(QIcon("resource/playAll.png"));
    this->playAllButton->setObjectName("playAllButton");
    this->playAllButton->setMaximumSize(90, 24);
}

void DetailSings::setTabs()
{
    this->contentsTab = new QTabWidget(this->frame);

    this->singsTable = new TableWidget(3, QStringList({tr("音乐标题"), tr("歌手"), tr("时长")}));
    this->singsTable->setObjectName("singsTable");
    this->singsTable->setMinimumWidth(this->width());
    QHash<int,int> dict;
    dict[0] = this->width()/3*1.25;
    dict[1] = this->width()/3*1.25;
    dict[2] = this->width()/3*0.5;
    this->singsTable->setColumnWidths(dict);

    this->contentsTab->addTab(this->singsTable, tr("歌曲列表"));
}

void DetailSings::setLayouts()
{
    this->mainLayout = new QVBoxLayout();
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->mainLayout->setSpacing(0);

    this->topLayout = new QHBoxLayout();
    this->topLayout->setContentsMargins(0, 0, 0, 0);
    this->topLayout->setSpacing(0);

    this->descriptionLayout = new QVBoxLayout();
    this->descriptionLayout->setContentsMargins(0, 0, 0, 0);
    this->descriptionLayout->setSpacing(0);

    this->titleLayout = new QHBoxLayout();
    this->titleLayout->setContentsMargins(0, 0, 0, 0);
    this->titleLayout->setSpacing(0);
    this->titleLayout->addWidget(this->showButton);
    this->titleLayout->addSpacing(5);
    this->titleLayout->addWidget(this->titleLabel);

    this->authorLayout = new QHBoxLayout();
    this->authorLayout->setContentsMargins(0, 0, 0, 0);
    this->authorLayout->setSpacing(0);
    this->authorLayout->addWidget(this->authorPic);
    this->authorLayout->addWidget(this->authorName);
    this->authorLayout->addStretch(1);

    this->descriptLayout = new QHBoxLayout();
    this->descriptLayout->setContentsMargins(0, 0, 0, 0);
    this->descriptLayout->setSpacing(0);
    this->descriptLayout->addWidget(this->descriptionButton);
    this->descriptLayout->addWidget(this->descriptionText);

    this->descriptionLayout->addSpacing(5);
    this->descriptionLayout->addLayout(this->titleLayout);
    this->descriptionLayout->addLayout(this->authorLayout);
    this->descriptionLayout->addSpacing(5);
    this->descriptionLayout->addWidget(this->playAllButton);
    this->descriptionLayout->addSpacing(10);
    this->descriptionLayout->addLayout(this->descriptLayout);

    this->topLayout->addWidget(this->picLabel);
    this->topLayout->addSpacing(18);
    this->topLayout->addLayout(this->descriptionLayout);

    this->mainLayout->addLayout(this->topLayout);
    this->mainLayout->addWidget(this->contentsTab);

    this->frame->setLayout(this->mainLayout);
}


/********** OneSing **********/
OneSing::OneSing(int row,int column,long long ids,const QString& picName)
    : QFrame ()
{
    this->setObjectName("oneSing");

    this->row = row;
    this->column = column;
    // 歌单号。
    this->ids = ids;
    // 大图的缓存名。
    this->picName = picName;

    this->setMinimumSize(180, 235);

    this->picLabel = new QLabel();
    this->picLabel->setObjectName("picLabel");
    this->picLabel->setMinimumSize(180, 180);
    this->picLabel->setMaximumSize(180, 180);

    this->nameLabel = new QLabel();
    this->nameLabel->setMaximumWidth(180);
    this->nameLabel->setWordWrap(true);

    this->mainLayout = new QVBoxLayout(this);

    this->mainLayout->addWidget(this->picLabel);
    this->mainLayout->addWidget(this->nameLabel);
}

void OneSing::setStyleSheets(const QString &styleSheet)
{
    if (!styleSheet.isEmpty()) {
        //this->setStyleSheet(styleSheet);
    }
}

void OneSing::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->mousePos = QCursor::pos();
}

void OneSing::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (QCursor::pos() != this->mousePos)
        return;
    else
        emit clicked(this->ids, this->picName);
}


/********** PlaylistButton **********/
PlaylistButton::PlaylistButton(ConfigNavigation *parent, long long ids, const QString &coverImgUrl,
                               const QIcon &icon, const QString &text)
    : QPushButton (icon, text)
{

    this->parent = parent;

    this->setCheckable(true);
    this->setAutoExclusive(true);

    this->ids = ids;
    this->coverImgUrl = coverImgUrl;

    //self.catch = None

    connect(this, SIGNAL(clicked()), SLOT(clickedEvent()));
}

void PlaylistButton::clickedEvent()
{
    emit hasClicked(this->ids, this->coverImgUrl);
}

static void test()
{
    DetailSings* sings = new DetailSings(nullptr);
    sings->show();

    PlaylistButton* button = new PlaylistButton(nullptr, 3081025492,
                                                "http://p4.music.126.net/USRrIEfSNxJ4JnfKp4f6pA==/109951164470147956.jpg",
                                                QIcon("resource/notes2.png"), "hello");
    button->show();
}
