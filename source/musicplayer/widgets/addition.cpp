#include "addition.h"

QString itv2time(int seconds)
{
    QTime time(0, seconds / 60, seconds % 60);
    QString format = "mm:ss";
    return time.toString(format);
}


SearchLineEdit::SearchLineEdit(QWidget *parent)
    : QLineEdit ()
{
    this->setObjectName("SearchLine");
    this->parent = parent;
    this->setMinimumSize(218, 20);
    QFile file("QSS/searchLine.qss");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        //this->setStyleSheet(in.readAll());
    }

    this->button = new QPushButton(this);
    this->button->setMaximumSize(13, 13);
    this->button->setCursor(QCursor(Qt::PointingHandCursor));

    this->setTextMargins(3, 0, 19, 0);

    this->spaceItem = new QSpacerItem(150, 10, QSizePolicy::Expanding);

    this->mainLayout = new QHBoxLayout();
    this->mainLayout->addSpacerItem(this->spaceItem);

    this->mainLayout->addWidget(this->button);
    this->mainLayout->addSpacing(10);
    this->mainLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(this->mainLayout);
}


bool comp(const PlayTimesStruct& t1,  const PlayTimesStruct& t2)
{
    return t1.playTimes > t2.playTimes;
}

// 获取前k大个元素，可以用快排的变形

QList<long long> getSongByPlayTimes(int maxes)
{
    QList<long long> res;
    QSettings settings;
    settings.beginGroup("db");
    QStringList keys = settings.allKeys();

    std::vector<PlayTimesStruct> list(static_cast<size_t>(keys.size()));
    for (int i = 0; i < keys.size(); ++i) {
        PlayTimesStruct val = settings.value(keys[i]).value<PlayTimesStruct>();
        list[static_cast<size_t>(i)] = val;
    }

    std::sort(list.begin(), list.end(), comp);
    if (keys.size() <= maxes) {
        for (int i = 0; i < keys.size(); ++i) {
            PlayTimesStruct val = list[static_cast<size_t>(i)];
            res.append(val.id);
        }
    }
    else {
        for (int i = 0; i < maxes; ++i) {
            PlayTimesStruct val = list[static_cast<size_t>(i)];
            res.append(val.id);
        }
    }

    settings.endGroup();
    return res;
}

void addPlayTimesById(long long songId, const QString &name, const QString &author)
{
    QSettings settings;
    settings.beginGroup("db");
    QString idStr = QString::number(songId);
    if (settings.contains(idStr)) {
        PlayTimesStruct val = settings.value(idStr).value<PlayTimesStruct>();
        val.playTimes++;
        settings.setValue(idStr, QVariant::fromValue(val));
    }
    else {
        PlayTimesStruct val(songId, name, author, 1);
        settings.setValue(idStr, QVariant::fromValue(val));
    }
    settings.endGroup();
}

static void test()
{
    qDebug() << itv2time(static_cast<int>(12.34));
    SearchLineEdit* lineedit = new SearchLineEdit(nullptr);
    lineedit->show();
}
