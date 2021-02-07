#include "collectionseditor.h"

const int LARGE_NROWS = 100;
const int ROWS_TO_LOAD = 50;



ReadOnlyCollectionsModel::ReadOnlyCollectionsModel(QObject* parent,
                         const QVariant& data,
                         const QString& title,
                         bool names,
                         bool minmax,
                         const QString& dataframe_format,
                         bool remote)
    : QAbstractTableModel (parent)
{
    this->names = names;
    this->minmax = minmax;
    this->dataframe_format = dataframe_format;
    this->remote = remote;

    this->title = title;
    if (!this->title.isEmpty())
        this->title = this->title + " - ";
    set_data(data);
}

QVariant ReadOnlyCollectionsModel::get_data() const
{
    return _data;
}

void ReadOnlyCollectionsModel::set_data(const QVariant &data)
{
    _data = data;
    QString data_type = data.typeName();

    //if coll_filter is not None
    showndata = data;

    header0 = "Index";
    if (names)
        header0 = "Name";
    //暂未实现元组
    if (data.type() == QVariant::List) {
        QList<QVariant> list = data.toList();
        keys.clear();
        for (int i=0;i<list.size();i++)
            keys.append(i);
        title += "List";
    }
    else if (data.type() == QVariant::Hash) {
        QHash<QString, QVariant> dict = data.toHash();
        keys.clear();
        for (auto it=dict.begin();it!=dict.end();it++) {
            QString key = it.key();
            keys.append(key);
        }
        title += "Dictionary";
        if (!names)
            header0 = "Key";
    }
    //实现不了对象，QVariant没有toMetaObject之类的方法
    title += " (" + QString::number(keys.size()) + " elements)";

    total_rows = keys.size();
    if (total_rows > LARGE_NROWS)
        rows_loaded = ROWS_TO_LOAD;
    else
        rows_loaded = total_rows;
    emit sig_setting_data();
    set_size_and_type();
    reset();
}

void ReadOnlyCollectionsModel::set_size_and_type(int start, int stop)
{
    QVariant data = _data;

    bool fetch_more;
    if (start != -1 && stop != -1) {
        start = 0;
        stop = rows_loaded;
        fetch_more = false;
    }
    else
        fetch_more = true;

    QList<int> sizes;
    QList<QString> types;
    if (remote) {
        ;//
    }
    else {
        if (data.type() == QVariant::List) {
            QList<QVariant> data_ = data.toList();
            for (int index = start; index < stop; ++index) {
                int idx = keys[index].toInt();
                sizes.append(get_size(data_[idx]));
                types.append(data_[idx].typeName());//get_human_readable_type
            }
        }
        else if (data.type() == QVariant::Hash) {
            QHash<QString, QVariant> data_ = data.toHash();
            for (int index = start; index < stop; ++index) {
                QString idx = keys[index].toString();
                sizes.append(get_size(data_[idx]));
                types.append(data_[idx].typeName());
                //get_human_readable_type直接用typeName()代替
            }
        }
    }

    if (fetch_more) {
        this->sizes = this->sizes + sizes;
        this->types = this->types + types;
    }
    else {
        this->sizes = sizes;
        this->types = types;
    }
}

template<class T1, class T2>
QList<T1> sort_against(QList<T1> list1, QList<T2> list2, bool reverse=false)
{
    Q_ASSERT(list1.size() == list2.size());
    QList<QPair<T2,T1>> tmp;
    for (int i=0;i<list1.size();i++)
        tmp.append(QPair<T2,T1>(list2[i], list1[i]));
    std::sort(tmp.begin(),tmp.end());
    if (reverse)
        std::reverse(tmp.begin(), tmp.end());
    QList<T1> res;
    for (int i=0;i<list1.size();i++)
        res.append(tmp[i].second);
    return res;
}

int ReadOnlyCollectionsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
}

int ReadOnlyCollectionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (total_rows <= rows_loaded)
        return total_rows;
    else
        return rows_loaded;
}

bool ReadOnlyCollectionsModel::canFetchMore(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (total_rows > rows_loaded)
        return true;
    else
        return false;
}

void ReadOnlyCollectionsModel::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent);
    int reminder = total_rows - rows_loaded;
    int items_to_fetch = qMin(reminder, ROWS_TO_LOAD);
    set_size_and_type(rows_loaded, rows_loaded + items_to_fetch);
    beginInsertRows(QModelIndex(), rows_loaded, rows_loaded + items_to_fetch-1);
    rows_loaded += items_to_fetch;
    endInsertRows();
}

QModelIndex ReadOnlyCollectionsModel::get_index_from_key(const QVariant &key) const
{
    int idx = keys.indexOf(key);
    if (idx != -1)
        return createIndex(idx, 0);
    else
        return QModelIndex();
}

QVariant ReadOnlyCollectionsModel::get_key(const QModelIndex &index) const
{
    return keys[index.row()];
}

QVariant ReadOnlyCollectionsModel::get_value(const QModelIndex &index) const
{
    if (index.column() == 0)
        return keys[index.row()];
    else if (index.column() == 1)
        return types[index.row()];
    else if (index.column() == 2)
        return sizes[index.column()];
    else if (_data.type() == QVariant::List) {
        QList<QVariant> data = _data.toList();
        return data[keys[index.row()].toInt()];
    }
    else if (_data.type() == QVariant::Hash) {
        QHash<QString, QVariant> data = _data.toHash();
        return data[keys[index.row()].toString()];
    }
    return QVariant();
}

QColor ReadOnlyCollectionsModel::get_bgcolor(const QModelIndex &index) const
{
    QColor color;
    if (index.column() == 0) {
        color = QColor(Qt::lightGray);
        color.setAlphaF(0.05);
    }
    else if (index.column() < 3) {
        color = QColor(Qt::lightGray);
        color.setAlphaF(0.2);
    }
    else {
        color = QColor(Qt::lightGray);
        color.setAlphaF(0.3);
    }
    return color;
}

QVariant ReadOnlyCollectionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    QVariant value = get_value(index);
    if (index.column() == 3 && remote)
        value = value.toHash()["view"];
    QString display;
    if (index.column() == 3)
        display = value_to_display(value);
    else
        display = value.toString();

    if (role == Qt::DisplayRole)
        return display;
    else if (role == Qt::EditRole)
        return value_to_display(value);
    else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 3) {
            if (display.split(QRegExp("[\r\n]"),QString::SkipEmptyParts).size() < 3)
                return int(Qt::AlignLeft|Qt::AlignVCenter);
            else
                return int(Qt::AlignLeft|Qt::AlignTop);
        }
        else
            return int(Qt::AlignLeft|Qt::AlignVCenter);
    }
    else if (role == Qt::BackgroundColorRole)
        return get_bgcolor(index);
    else if (role == Qt::FontRole) {
        QFont font("Consolas", 9, 50);
        font.setItalic(false);
        return font;
    }
    return QVariant();
}

QVariant ReadOnlyCollectionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Horizontal) {
        QStringList headers;
        headers << header0 << "Type" << "Size" << "Value";
        return headers[section];
    }
    else
        return QVariant();
}

Qt::ItemFlags ReadOnlyCollectionsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemFlags(QAbstractTableModel::flags(index) |
                         Qt::ItemIsEditable);
}

void ReadOnlyCollectionsModel::reset()
{
    beginResetModel();
    endResetModel();
}


/********** ReadOnlyCollectionsModel **********/
CollectionsModel::CollectionsModel(QObject* parent,
                         const QVariant& data,
                         const QString& title,
                         bool names,
                         bool minmax,
                         const QString& dataframe_format,
                         bool remote)
    : ReadOnlyCollectionsModel (parent, data, title, names, minmax, dataframe_format, remote)
{}

void CollectionsModel::set_value(const QModelIndex &index, const QVariant &value)
{
    if (_data.type() == QVariant::List) {
        QList<QVariant> list = _data.toList();
        list[keys[index.row()].toInt()] = value;
        _data = list;
        list = showndata.toList();
        list[keys[index.row()].toInt()] = value;
        showndata = list;
    }
    else if (_data.type() == QVariant::Hash) {
        QHash<QString, QVariant> dict = _data.toHash();
        dict[keys[index.row()].toString()] = value;
        _data = dict;
        dict = showndata.toHash();
        dict[keys[index.row()].toString()] = value;
        showndata = dict;
    }
    sizes[index.row()] = get_size(value);
    types[index.row()] = value.typeName();
    emit sig_setting_data();
}

QColor CollectionsModel::get_bgcolor(const QModelIndex &index) const
{
    QVariant value = get_value(index);
    QColor color;
    if (index.column() < 3)
        color = ReadOnlyCollectionsModel::get_bgcolor(index);
    else {
        QString color_name;
        if (remote)
            color_name = value.toHash()["color"].toString();
        else
            color_name = get_color_name(value);
        color = QColor(color_name);
        color.setAlphaF(0.2);
    }
    return color;
}

bool CollectionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role);
    if (!index.isValid())
        return false;
    if (index.column() < 3)
        return false;
    QVariant res = display_to_value(value, get_value(index));
    set_value(index, value);
    emit dataChanged(index, index);
    return true;
}


/********** CollectionsDelegate **********/


