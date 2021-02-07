#pragma once

#include "nsview.h"
#include "arrayeditor.h"

class ReadOnlyCollectionsModel : public QAbstractTableModel
{
    Q_OBJECT
signals:
    void sig_setting_data();
public:
    bool names;
    bool minmax;
    QString dataframe_format;//暂定为qstring
    bool remote;
    QString header0;
    QVariant _data;
    int total_rows;
    int rows_loaded; //源码179行
    QVariant showndata;
    QList<QVariant> keys;
    QString title;
    QList<int> sizes;
    QList<QString> types;//暂定

public:
    ReadOnlyCollectionsModel(QObject* parent,
                             const QVariant& data,
                             const QString& title="",
                             bool names=false,
                             bool minmax=false,
                             const QString& dataframe_format=QString(),
                             bool remote=false);
    QVariant get_data() const;
    void set_data(const QVariant& data);//暂时没有实现coll_filter
    void set_size_and_type(int start=-1,int stop=-1);
    //void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    QModelIndex get_index_from_key(const QVariant& key) const;
    QVariant get_key(const QModelIndex &index) const;
    QVariant get_value(const QModelIndex &index) const;
    virtual QColor get_bgcolor(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void reset();
};


class CollectionsModel : public ReadOnlyCollectionsModel
{
    Q_OBJECT
public:
    CollectionsModel(QObject* parent,
                     const QVariant& data,
                     const QString& title="",
                     bool names=false,
                     bool minmax=false,
                     const QString& dataframe_format=QString(),
                     bool remote=false);
    void set_value(const QModelIndex &index, const QVariant& value);
    QColor get_bgcolor(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
};



