#pragma once

#include "utils/icon_manager.h"
#include "utils/qthelpers.h"
#include <QtWidgets>

class ArrayModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    static const int ROWS_TO_LOAD;
    static const int COLS_TO_LOAD;
    QWidget* dialog;
    QHash<QPair<int,int>,QVariant> changes;
    QStringList xlabels;
    QStringList ylabels;
    bool readonly;
    QVariant test_array; //类型存疑

    double sat;
    double val;
    double alp;
    QVector<QVector<QVariant>> _data;
    QString _format;
    int total_rows;
    int total_cols;

    double vmin;
    double vmax;
    double hue0;
    double dhue;
    bool bgcolor_enabled;
    int rows_loaded;
    int cols_loaded;
public:
    ArrayModel(const QVector<QVector<QVariant>>& data,
               const QString& format = "%.6g",
               const QStringList& xlabels=QStringList(),
               const QStringList& ylabels=QStringList(),
               bool readonly = false,
               QWidget* parent = nullptr);
    QString get_format() const;
    QVector<QVector<QVariant>> get_data() const;
    void set_format(const QString& format);
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool can_fetch_more(bool rows=false,bool columns=false) const;
    void fetch_more(bool rows=false,bool columns=false);

    QVariant get_value(const QModelIndex& index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void reset();
public slots:
    void bgcolor(int state);
};


class ArrayDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    QString dtype;

    ArrayDelegate(const QString& dtype,QWidget* parnet=nullptr);
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

public slots:
    void commitAndCloseEditor();
};


class ArrayView : public QTableView
{
    Q_OBJECT
public:
    QPair<int,int> shape;
    QMenu* menu;
    QAction* copy_action;

    ArrayView(QWidget* parent,QAbstractItemModel *model,
              const QString& dtype,const QPair<int,int>& shape);
    void load_more_data(int value,bool rows=false,bool columns=false);
    QMenu* setup_menu();
    QString _sel_to_text(const QModelIndexList& cell_range);
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

public slots:
    void resize_to_contents();
    void copy();
};


class ArrayEditorWidget : public QWidget
{
    Q_OBJECT
public:
    QVector<QVector<QVariant>> data;
    ArrayModel* model;
    ArrayView* view;
    ArrayEditorWidget(QWidget* parent,
                      const QVector<QVector<QVariant>>& data,
                      bool readonly = false,
                      const QStringList& xlabels=QStringList(),
                      const QStringList& ylabels=QStringList());
    void accept_changes();
    void reject_changes();
public slots:
    void change_format();
};


class ArrayEditor : public QDialog
{
    Q_OBJECT
public:
    QVector<QVector<QVariant>> data;
    ArrayEditorWidget* arraywidget;
    QStackedWidget* stack;
    QGridLayout* layout;
    QPushButton* btn_save_and_close;
    QPushButton* btn_close;
public:
    ArrayEditor(QWidget* parent = nullptr);
    bool setup_and_check(const QVector<QVector<QVariant>>& data,
                         QString title = "",
                         bool readonly = false,
                         const QStringList& xlabels=QStringList(),
                         const QStringList& ylabels=QStringList());
    QVector<QVector<QVariant>> get_value() const;
    void error(const QString& message);
public slots:
    void save_and_close_enable(const QModelIndex& left_top,
                               const QModelIndex& bottom_right);
    void current_widget_changed(int index);
    void accept();
    void reject();
};

//只有ArrayEditor::setup_and_check(data)中的data有可能是三维数组，其余三个都是二维数组
