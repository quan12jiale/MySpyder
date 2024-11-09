#pragma once

#include "utils/encoding.h"
#include "utils/icon_manager.h"
#include "utils/misc.h"
#include "utils/programs.h"
#include "utils/qthelpers.h"
#include <QtWidgets>
#include <functional>


// 供editor.cpp调用
void show_in_external_file_explorer(const QStringList& fnames = QStringList());

class ExplorerWidget;

class IconProvider : public QFileIconProvider
{
public:
    static const QHash<QString,QString> APP_FILES;
    static const QHash<QString,QString> OFFICE_FILES;
public:
    QTreeView* treeview;
    QHash<QString,QString> application_icons;

    IconProvider(QTreeView* treeview);
    QIcon icon(const QFileInfo &qfileinfo) const override;
};


class DirView : public QTreeView
{
    Q_OBJECT
public:
    QStringList name_filters;
    QWidget* parent_widget;
    bool show_all;
    QMenu* menu;
    QList<QObject*> common_actions;
    QStringList __expanded_state;
    QStringList _to_be_loaded;
    QFileSystemModel* fsmodel;
    QPoint _scrollbar_positions;
public:
    DirView(QWidget* parent = nullptr);
    void setup_fs_model();
    virtual void install_model();
    void setup_view();
    void set_name_filters(const QStringList& name_filters);
    void set_show_all(bool state);
    virtual QString get_filename(const QModelIndex &index) const;
    virtual QModelIndex get_index(const QString& filename) const;
    QStringList get_selected_filenames() const;
    QString get_dirname(const QModelIndex &index) const;

    void setup(const QStringList& name_filters={"*.py", "*.pyw"}, bool show_all=false);
    virtual QList<QObject*> setup_common_actions();
    QList<QObject*> create_file_new_actions(const QStringList& fnames);
    QList<QObject*> create_file_import_actions(const QStringList& fnames);
    QList<QObject*> create_file_manage_actions(const QStringList& fnames);
    QList<QObject*> create_folder_manage_actions(const QStringList& fnames);
    QList<QObject*> create_context_menu_actions();
    void update_menu();

    bool viewportEvent(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void directory_clicked(const QString& dirname) {Q_UNUSED(dirname);}

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void startDrag(Qt::DropActions dropActions) override;

    void open(QStringList fnames = QStringList());
    void open_outside_spyder(QStringList fnames);
    void open_interpreter(QStringList fnames);
    void run(QStringList fnames = QStringList());
    virtual void remove_tree(const QString& dirname);
    // 源码yes_to_all为bool，返回值为bool
    int delete_file(const QString& fname,bool multiple,int yes_to_all);
    void _delete(QStringList fnames = QStringList());
    void convert_notebook(const QString& fname);

    QString rename_file(const QString& fname);
    void show_in_external_file_explorer(QStringList fnames = QStringList());
    void rename(QStringList fnames = QStringList());
    void move(QStringList fnames = QStringList(), const QString& directory=QString());
    QString create_new_folder(QString current_path,const QString& title,const QString& subtitle,bool is_package);
    void new_folder(const QString& basedir);
    void new_package(const QString& basedir);
    QString create_new_file(QString current_path,const QString& title,const QString& filters,std::function<void(QString)> create_func);
    void new_file(const QString& basedir);
    void new_module(QString basedir);

    QPoint get_scrollbar_position() const;
    void set_scrollbar_position(const QPoint& position);

    QStringList get_expanded_state();
    void set_expanded_state(const QStringList& state);
    void save_expanded_state();

    void restore_expanded_state();
    void filter_directories();
public slots:
    void reset_icon_provider();
    void edit_filter();
    void toggle_all(bool checked);
    void clicked();
    void convert_notebooks();
    virtual void go_to_parent_directory() {}
    void restore_scrollbar_positions();
    void restore_directory_state(const QString& fname);
    void follow_directories_loaded(const QString& fname);
private:
	QScopedPointer<QFileIconProvider> icon_provider_;
};


class ProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    QString root_path;
    QStringList path_list;

    ProxyModel(QObject* parent);
    void setup_filter(const QString& root_path,const QStringList& path_list);
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    bool filterAcceptsRow(int row, const QModelIndex &parent_index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    using QAbstractItemModel::persistentIndexList;
};


class FilteredDirView : public DirView
{
    Q_OBJECT
public:
    ProxyModel* proxymodel;
    QString root_path;

    FilteredDirView(QWidget* parent = nullptr);
    void setup_proxy_model();
    void install_model() override;
    void set_root_path(const QString& root_path);
    QModelIndex get_index(const QString& filename) const override;
    void set_folder_names(const QStringList& folder_names);
    QString get_filename(const QModelIndex &index) const override;
    void setup_project_view();
};


class ExplorerTreeWidget : public DirView
{
    Q_OBJECT
signals:
    void set_previous_enabled(bool);
    void set_next_enabled(bool);
private:
    int histindex;
public:
    QStringList history;
    int show_cd_only;//源码是bool
    QModelIndex __original_root_index;
    QString __last_folder;

    ExplorerTreeWidget(QWidget* parent = nullptr,int show_cd_only=-1);
    QList<QObject*> setup_common_actions() override;
    QModelIndex set_current_folder(const QString& folder);
    QString get_current_folder() const;
    void refresh(QString new_path=QString(),bool force_current=false);
    void directory_clicked(const QString& dirname) override;
    void update_history(QString directory);
    void chdir(QString directory=QString(),bool browsing_history=false);
public slots:
    void toggle_show_cd_only(bool checked);
    void go_to_parent_directory() override;
    void go_to_previous_directory();
    void go_to_next_directory();
};


class ExplorerWidget : public QWidget
{
    Q_OBJECT
signals:
    void sig_option_changed(const QString&, const QVariant&);
    void sig_open_file(const QString&);
    void sig_new_file(const QString&);
    void redirect_stdio(bool);
    void open_dir(const QString&);

public:
    ExplorerTreeWidget* treewidget;
    QToolButton* button_menu;
    QList<QToolButton*> action_widgets;
    ExplorerWidget(QWidget* parnet=nullptr,const QStringList& name_filters={"*.py", "*.pyw"},
                   bool show_all=false,int show_cd_only=-1,bool show_icontext=true);

public slots:
    void toggle_icontext(bool state);
};


/********** Tests **********/
class FileExplorerTest : public QWidget
{
    Q_OBJECT
public:
    ExplorerWidget* explorer;
    QLabel* label1;
    QLabel* label2;
    QLabel* label3;

    FileExplorerTest();
};

class ProjectExplorerTest1 : public QWidget
{
    Q_OBJECT
public:
    FilteredDirView* treewidget;
    ProjectExplorerTest1(QWidget* parent = nullptr);
};
