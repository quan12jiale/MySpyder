#include "fnmatch.h"

namespace fnmatch {

QString translate(const QString& pat)
{
    int i=0, n=pat.size();
    QString res;
    while (i < n) {
        QChar c = pat[i];
        i++;
        if (c == '*')
            res = res + ".*";
        else if (c == '?')
            res = res + '.';
        else if (c == '[') {
            int j = i;
            if (j < n && pat[j] == '!')
                j++;
            if (j < n && pat[j] == ']')
                j++;
            while (j < n && pat[j] != ']')
                j++;
            if (j >= n)
                res = res + "\\[";
            else {
                QString stuff = pat.mid(i, j-i);
                if (!stuff.contains("--"))
                    stuff.replace("\\", "\\\\");
                else {
                    QStringList chunks;
                    int k;
                    if (pat[i] == '!')
                        k = i+2;
                    else
                        k = i+1;
                    while (true) {
                        //源码是k = pat.find('-', k, j)
                        k = pat.indexOf('-', k);
                        if (k < 0 || k >= j)
                            break;
                        chunks.append(pat.mid(i, k-i));
                        i = k+1;
                        k = k+3;
                    }
                    chunks.append(pat.mid(i, j-i));
                    QStringList list;
                    foreach (QString s, chunks)
                        list.append(s.replace("\\", "\\\\").replace("-", "\\-"));
                    stuff = list.join('-');
                }
                stuff.replace("([&~|])", "\\\\\\1");
                i = j+1;
                if (stuff[0] == '!')
                    stuff = '^' + stuff.mid(1);
                else if (stuff[0] == '^' || stuff[0] == '[')
                    stuff = "\\" + stuff;
                res = QString("%1[%2]").arg(res).arg(stuff);
            }
        }
        else
            res = res + QRegularExpression::escape(c);
    }
    return QString("(?s:%1)\\Z").arg(res);
}

} // namespace fnmatch
