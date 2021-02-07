#include "encoding.h"
#include <QDebug>

namespace encoding {



bool write(const QString& text,const QString& filename,QIODevice::OpenMode mode)
{
    QFile file(filename);
    bool ok = file.open(mode);
    if (ok) {
        file.write(text.toUtf8());
        file.close();
    }
    return ok;
}

bool writelines(const QStringList& lines,const QString& filename,QIODevice::OpenMode mode)
{
    return write(lines.join(os::linesep), filename, mode);
}

QString read(const QString& filename)
{
    QFile file(filename);
    bool ok = file.open(QIODevice::ReadOnly);
    if (ok) {
        QString text = file.readAll();
        file.close();
        return text;
    }
    return QString();
}

QStringList readlines(const QString& filename)
{
    QString text = read(filename);
    return text.split(os::linesep);
}

bool is_text_file(const QString& filename)
{
    try {
        return !is_binary(filename);
    } catch (...) {
        return false;
    }
}

} // namespace encoding
