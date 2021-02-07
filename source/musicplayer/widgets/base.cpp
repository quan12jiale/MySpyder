#include "base.h"

QString makeMd5(const QString& raw)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(raw.toUtf8());
    QByteArray resultArray = hash.result();

    return resultArray.toHex();
}

/********** QueueObject **********/
QueueObject::QueueObject()
    : QObject ()
{}

void QueueObject::put(const QueueData& data)
{
    this->queue.enqueue(data);
    emit add();
}

QueueData QueueObject::get()
{
    if (this->queue.isEmpty())
        return QueueData();
    return this->queue.dequeue();
}


/********** ScrollArea **********/
ScrollArea::ScrollArea(QWidget *parent)
    : QScrollArea ()
{
    Q_UNUSED(parent);
    this->frame = new QFrame;
    this->frame->setObjectName("frame");
    connect(this->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(sliderPostionEvent()));

    this->setWidgetResizable(true);
    this->setWidget(this->frame);
}

void ScrollArea::sliderPostionEvent()
{
    if (this->verticalScrollBar()->value() == this->verticalScrollBar()->maximum())
        emit scrollDown();
}

int ScrollArea::maximumValue() const
{
    return this->verticalScrollBar()->maximum();
}


/********** TableWidget **********/
void contextMenuTemp(QContextMenuEvent *)
{}

TableWidget::TableWidget(int count,const QStringList& headerLables)
    : QTableWidget ()
{
    this->contextMenuFun = contextMenuTemp;

    this->setColumnCount(count);
    this->setHorizontalHeaderLabels(headerLables);

    this->horizontalHeader()->setStretchLastSection(true);
    this->verticalHeader()->setVisible(false);
    this->setShowGrid(false);
    this->setAlternatingRowColors(true);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void TableWidget::setColumnWidths(const QHash<int, int> &widths)
{
    for (auto it = widths.begin(); it != widths.end(); ++it) {
        this->setColumnWidth(it.key(), it.value());
    }
}

void TableWidget::contextMenuEvent(QContextMenuEvent *event)
{
    this->contextMenuFun(event);
}


/********** HStretchBox **********/
HStretchBox::HStretchBox(QBoxLayout* parentLayout,const QList<QWidget*>& widgets,
            int frontStretch,int behindStretch)
    : QHBoxLayout ()
{
    this->setContentsMargins(0, 0, 0, 0);
    this->setSpacing(0);

    this->addStretch(frontStretch);
    foreach (QWidget* widg, widgets)
        this->addWidget(widg);
    this->addStretch(behindStretch);

    parentLayout->addLayout(this);
}

QueueObject* picsQueue = new QueueObject;
QString cacheFolder = "cache";

/********** PicLabel **********/
PicLabel::PicLabel(const QString& src,int width,int height,const QString& pixMask)
    : QLabel ()
{
    this->src = src;
    this->width = width;
    this->height = height;
    this->pixMask = pixMask;

    if (!src.isEmpty())
        this->setSrc(src);

    if (this->width) {
        this->setMaximumSize(width, height);
        this->setMinimumSize(width, height);
    }
}

void PicLabel::setSrc(const QString &src)
{
    if (src.contains("http") || src.contains("https")) {
        QStringList cacheList = os::listdir(cacheFolder);
        QString names = makeMd5(src);
        QString localSrc = cacheFolder+'/'+names;
        if (cacheList.contains(names)) {
            this->setSrc(localSrc);
            return;
        }

        GetPicture* task = new GetPicture(this, src);
        QThreadPool::globalInstance()->start(task);
    }

    else {
        this->src = src;
        QPixmap pic(src);
        pic = pic.scaled(this->width, this->height);

        if (!this->pixMask.isEmpty()) {
            QPixmap mask(this->pixMask);
            mask = mask.scaled(this->width, this->height);
            pic.setMask(mask.createHeuristicMask());
        }
        this->setPixmap(pic);
    }
}

QString PicLabel::getSrc() const
{
    return  this->src;
}


/********** GetPicture **********/
GetPicture::GetPicture(PicLabel* widget, const QString& src)
    : QRunnable ()
{
    this->widget = widget;
    this->src = src;
}

void GetPicture::run()
{
    QString names = makeMd5(this->src);
    HttpRequest requests;
    QByteArray json = requests.httpRequest(this->src);
    if (json.isEmpty()) {
        QMessageBox::warning(nullptr, "网络错误", "请检查网络连接");
        return;
    }
    picsQueue->put(QueueData(this->widget, json, names));
}


//辅助函数
void __addPic()
{
    QueueData data = picsQueue->get();
    if (data.widget == nullptr)
        return;

    PicLabel* widget = data.widget;

    QPixmap pic;
    pic.loadFromData(data.content);
    QString localSrc = cacheFolder+'/'+data.names;
    pic = pic.scaled(widget->width, widget->height);
    if (pic.save(localSrc, "jpg"))
        widget->src = localSrc;

    if (!widget->pixMask.isEmpty()) {
        QPixmap mask(widget->pixMask);
        mask = mask.scaled(widget->width, widget->height);
        pic.setMask(mask.createHeuristicMask());
    }
    widget->setPixmap(pic);
}
