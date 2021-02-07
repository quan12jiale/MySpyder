#pragma once

#include <QtCore>
#include <QtWidgets>

namespace random {

QList<QChar> sample(const QString& population, int k);

template <typename T>
T choice(const QList<T>& seq)
{
    Q_ASSERT(!seq.isEmpty());
    int i = std::rand() % seq.size();
    return seq[i];
}

}

namespace os {

extern QString curdir; // 定义在D:\Anaconda3\lib\ntpath.py第11行
extern QString pardir;
extern QString extsep;
extern QString sep;
extern QString pathsep;
extern QString altsep;
extern QString defpath;
extern QString devnull;

QPair<QString, QString> splitdrive(const QString& p);

QStringList listdir(const QString& folder);

QStringList glob_glob(const QString& directory,const QStringList &nameFilters);

void makedirs(const QString& name);

} // namespace os
