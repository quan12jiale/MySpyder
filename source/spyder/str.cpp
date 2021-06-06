#include "str.h"

bool isdigit(const QString& text)
{
    for (int i = 0; i < text.size(); ++i) {
        if (!text[i].isDigit())
            return false;
    }
    return true;
}

QString lstrip(const QString& text)
{
    int len = text.size();
    int i = 0;
    while (i<len && text[i].isSpace())
        i++;

    return text.mid(i);
}
QString lstrip(const QString& text,const QString& before)
{
    int len = text.size();
    int i = 0;
    while (i<len && before.contains(text[i]))
        i++;

    return text.mid(i);
}

QString rstrip(const QString& text)
{
    int len = text.size();
    int j = len;
    do {
        j--;
    } while (j>=0 && text[j].isSpace());
    j++;

    return text.left(j);
}
QString rstrip(const QString& text,const QString& before)
{
    int len = text.size();
    int j = len;
    do {
        j--;
    } while (j>=0 && before.contains(text[j]));
    j++;

    return text.left(j);
}

// QString QString::leftJustified(int width, QChar fill = QLatin1Char(' '), bool truncate = false) const
QString ljust(const QString& text,int width,QChar fillchar)
{
    int len = text.size();
    if (width <= len)
        return text;
    QString right(len-width,fillchar);
    return text+right;
}

QString rjust(const QString& text,int width,QChar fillchar)
{
    int len = text.size();
    if (width <= len)
        return text;
    QString left(len-width,fillchar);
    return left + text;
}

QStringList splitlines(const QString& text, bool keepends)
{
    // keepends -- 在输出结果里是否保留换行符('\r', '\r\n', \n')，默认为 False，
    // 不包含换行符，如果为 True，则保留换行符
    /*  原字符串: "ab c\n\nde fg\rkl\r\n"
     *                          Qt                                  Python
     * ("ab c", "", "de fg", "kl", "", "")     splitlines() ["ab c", "", "de fg", "kl"]
     * SkipEmptyParts ("ab c", "de fg", "kl")  splitlines(True) ["ab c\n", "\n", "de fg\r", "kl\r\n"]
     *
    */
    QStringList list = text.split(QRegExp("[\r\n]"));
    //QStringList list = text.split(os::linesep);
    while (!list.isEmpty() && list.last().isEmpty())
        list.removeLast();
    if (keepends == false)
        return list;
    else {
        for (int i = 0; i < list.size(); ++i) {
            list[i] += "\n";
        }
        return list;
    }
}

QString repr(const QList<QList<QVariant>>& breakpoints)
{
    QString str;
    for (int i = 0; i < breakpoints.size(); ++i) {
        QList<QVariant> pair = breakpoints[i];
        str += QString(pair[0].toInt()) + "," + pair[1].toString();
        if (i != breakpoints.size()-1)
            str += os::linesep;
    }
    return str;
}

QList<QVariant> eval(const QString& breakpoints)
{
    QStringList list = breakpoints.split(os::linesep);
    QList<QVariant> res;
    foreach (QString tmp, list) {
        int idx = tmp.indexOf(',');
        Q_ASSERT(idx > 0);

        QList<QVariant> pair;
        pair.append(tmp.left(idx).toInt());
        pair.append(tmp.mid(idx+1));
        res.append(tmp);
    }
    return res;
}

bool startswith(const QString& text,const QStringList& iterable)
{
    for (int i = 0; i < iterable.size(); ++i) {
        if (text.startsWith(iterable[i]))
            return true;
    }
    return false;
}

bool endswith(const QString& text,const QStringList& iterable)
{
    for (int i = 0; i < iterable.size(); ++i) {
        if (text.endsWith(iterable[i]))
            return true;
    }
    return false;
}
