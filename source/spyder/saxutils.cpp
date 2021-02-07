#include "saxutils.h"

static QString __dict_replace(const QString& s, const QHash<QString,QString>& d)
{
    QString text = s;
    for (auto it = d.begin(); it != d.end(); ++it) {
        text.replace(it.key(), it.value());
    }
    return text;
}

QString escape(const QString& data, const QHash<QString,QString>& entities)
{
    QString _data = data;
    _data.replace("&", "&amp;");
    _data.replace(">", "&gt;");
    _data.replace("<", "&lt;");
    if (!entities.empty())
        _data = __dict_replace(_data, entities);
    return _data;
}
