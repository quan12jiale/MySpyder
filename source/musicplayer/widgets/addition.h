#ifndef ADDITION_H
#define ADDITION_H

#include <QtWidgets>

QString itv2time(int seconds);

class SearchLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    QWidget* parent;
    QPushButton* button;
    QSpacerItem* spaceItem;
    QHBoxLayout* mainLayout;

    SearchLineEdit(QWidget* parent);
};


struct PlayTimesStruct {
    long long id;
    QString name;
    QString author;
    int playTimes;
    PlayTimesStruct() = default;
    PlayTimesStruct(long long id,const QString& name,const QString& author,int playTimes)
        : id(id), name(name), author(author), playTimes(playTimes) {}

    friend QDataStream &operator<<(QDataStream &out, const PlayTimesStruct &obj)
    {
        out << obj.id << obj.name << obj.author << obj.playTimes;
        return out;
    }
    friend QDataStream &operator>>(QDataStream &in, PlayTimesStruct &obj)
    {
        in >> obj.id >> obj.name >> obj.author >> obj.playTimes;
        return in;
    }
};
Q_DECLARE_METATYPE(PlayTimesStruct);


QList<long long> getSongByPlayTimes(int maxes = 5);
void addPlayTimesById(long long songId, const QString &name, const QString &author);

#endif // ADDITION_H
