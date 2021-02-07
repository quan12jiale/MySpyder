#include "nsview.h"

static QString SCALAR_COLOR = "#0000ff";
static QString CUSTOM_TYPE_COLOR = "#7755aa";

int get_size(const QVariant& item)
{
    if (item.type() == QVariant::List) {
        QList<QVariant> list = item.toList();
        return list.size();
    }
    else if (item.type() == QVariant::Hash) {
        QHash<QString, QVariant> dict = item.toHash();
        return dict.size();
    }
    return 1;
}

QString get_color_name(const QVariant& value)
{
    switch (value.type()) {
    case QVariant::List:
        return "#ffff00";
    case QVariant::Hash:
        return "#00ffff";
    case QVariant::Bool:
        return "#ff00ff";
    case QVariant::Int:
    case QVariant::Double:
        return SCALAR_COLOR;
    case QVariant::String:
        return "#800000";
    default:
        break;
    }
    return CUSTOM_TYPE_COLOR;
}

QString collections_display(const QVariant& value,int level)
{
    bool is_dict = (value.type() == QVariant::Hash);
    QHash<QString,QVariant> elements_dict;
    QList<QVariant> elements_list;
    if (is_dict)
        elements_dict = value.toHash();
    else
        elements_list = value.toList();

    QString display;
    if (level <= 2) {
        QStringList dispalys;
        if (is_dict) {
            for (auto it =elements_dict.begin() ; it != elements_dict.end(); ++it) {
                QString k = it.key();
                QVariant v = it.value();
                dispalys.append(value_to_display(k,level) + ':' +
                                value_to_display(v,level));
            }
        }
        else {
            for (int i = 0; i < elements_list.size(); ++i) {
                QVariant e = elements_list[i];
                dispalys.append(value_to_display(e));
            }
        }
        display = dispalys.join(", ");
    }
    else
        display = "...";

    if (is_dict)
        display = '{' + display + '}';
    else
        display = '[' + display + ']';
    return display;
}

QString value_to_display(const QVariant& value,int level)
{
    QString display;
    if (value.type() == QVariant::List || value.type() == QVariant::Hash)
        display = collections_display(value, level+1);
    else if (value.type() == QVariant::String) { //如果是字符串
        display = value.toString();
        if (level > 0)
            display = "'" + display + "'";
    }
    else //如果是数字
        display = value.toString();

    if (display.size() > 70) {
        QString ellipses = " ...";
        display = rstrip(display.left(70)) + ellipses;
    }
    return display;
}

QVariant display_to_value(QVariant value,const QVariant&  default_value)
{
    switch (default_value.type()) {
    case QVariant::Bool:
        value = value.toBool();
        break;
    case QVariant::String:
        value = value.toString();
        break;
    case QVariant::Double:
        value = value.toDouble();
        break;
    case QVariant::Int:
        value = value.toInt();
        break;
    default:
        return default_value;
    }
    return value;
}
