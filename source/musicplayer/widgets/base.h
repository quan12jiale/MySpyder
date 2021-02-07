#ifndef BASE_H
#define BASE_H

#include <QtWidgets>
#include "os.h"
#include "apis/apirequestsbase.h"
#include <functional>

class PicLabel;

struct QueueData
{
    PicLabel* widget;
    QByteArray content;
    QString names;
    QueueData() { widget = nullptr; }
    QueueData(PicLabel* widget, const QByteArray &content, const QString &names)
        : widget(widget), content(content), names(names) {}
};

QString makeMd5(const QString& raw);

class QueueObject : public QObject
{
    Q_OBJECT
signals:
    void add();
public:
    QueueObject();
    void put(const QueueData& data);
    QueueData get();
private:
    QQueue<QueueData> queue;
};


class ScrollArea : public QScrollArea
{
    Q_OBJECT
signals:
    void scrollDown();
public:
    QFrame* frame;

    ScrollArea(QWidget* parent);
    int maximumValue() const;
public slots:
    void sliderPostionEvent();
};


class TableWidget : public QTableWidget
{
    Q_OBJECT
public:
    std::function<void(QContextMenuEvent*)> contextMenuFun;
    TableWidget(int count,const QStringList& headerLables);
    void setColumnWidths(const QHash<int,int>& widths);

    void contextMenuEvent(QContextMenuEvent *event);//见configMainFeatures.py592行
};


class HStretchBox : public QHBoxLayout
{
public:
    HStretchBox(QBoxLayout* parentLayout,const QList<QWidget*>& widgets,
                int frontStretch=1,int behindStretch=1);
};

extern QueueObject* picsQueue;
extern QString cacheFolder;

class PicLabel : public QLabel
{
    Q_OBJECT
public:
    QString src;
    int width;
    int height;
    QString pixMask;

    PicLabel(const QString& src,int width=200,int height=200,const QString& pixMask=QString());
    void setSrc(const QString& src);
    QString getSrc() const;
};


class GetPicture : public QRunnable
{
public:
    GetPicture(PicLabel* widget, const QString& src);
    void run();
private:
    PicLabel* widget;
    QString src;
};

void __addPic();

#endif // BASE_H
