#include "os.h"

namespace random {

QList<QChar> sample(const QString& population, int k)
{
    int n = population.size();
    Q_ASSERT(0 <= k && k <= n);
    QList<QChar> result;
    for (int i=0; i<k; i++)
        result.append(0);
    int setsize = 21;
    if (k > 5)
        setsize += static_cast<int>(std::pow(4, std::ceil(std::log(k*3) / std::log(4))));
    if (n <= setsize) {
        QList<QChar> pool;
        foreach (const QChar& ch, population)
            pool.append(ch);

        for (int i = 0; i < k; ++i) {
            int j = std::rand() % (n-i);
            result[i] = pool[j];
            pool[j] = pool[n-i-1];
        }
    }
    return result;
}

}

namespace os {

QString curdir = ".";
QString pardir = "..";
QString extsep = ".";
QString sep = "\\";
QString pathsep = ";";
QString altsep = "/";
QString defpath = ".;C:\\bin";
QString devnull = "nul";

QPair<QString, QString> splitdrive(const QString& p)
{
    if (p.size() >= 2) {
        QString colon = ":";
        if (p.mid(1,1) == colon)
            return QPair<QString, QString>(p.left(2), p.mid(2));
    }
    return QPair<QString, QString>(QString(""), p);
}

QStringList listdir(const QString& folder)
{
    // os.listdir返回相对路径
    // dir.entryList也是返回相对路径
    QDir dir(folder);
    QStringList list = dir.entryList();
    list.removeAll(".");
    list.removeAll("..");
    return list;
}

QStringList glob_glob(const QString& directory,const QStringList &nameFilters)
{
    // dir.entryList返回相对路径，glob.glob返回绝对路径
    Q_ASSERT(!nameFilters.isEmpty());
    QDir dir(directory);
    dir.setNameFilters(nameFilters);

    QStringList list;
    foreach (const QString& path, dir.entryList()) {
        list.append(directory+'/'+path);
    }
    return list;
}

void makedirs(const QString& name)
{
    int idx = name.lastIndexOf('/');
    QString head, tail;
    if (idx != -1) {
        head = name.left(idx);
        tail = name.mid(idx+1);
        if (tail.isEmpty()) {
            int idx2 = head.lastIndexOf('/');
            if (idx2 != -1) {
                tail = head.mid(idx2+1);
                head = head.left(idx2);
            }
        }
    }

    if (!head.isEmpty() && !tail.isEmpty() && !QFileInfo::exists(head)) {
        makedirs(head);

        if (tail == curdir)
            return;
    }

    if (!head.isEmpty() && !tail.isEmpty()) {
        QDir dir(head);
        if (dir.mkdir(tail))
            qDebug() << __func__ << head << tail;
        else {
            QMessageBox::critical(nullptr,"mkdir error",
                                  QString("Unable to create a %1 directory"
                                          " under the %2 directory!")
                                  .arg(tail).arg(head));
        }
    }
}


} // namespace os
