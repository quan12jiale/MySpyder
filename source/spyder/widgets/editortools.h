#pragma once

#include "str.h"
#include "onecolumntree.h"

#include <QToolButton>
#include <QHBoxLayout>
#include <QWidget>
#include <QString>
#include <QRegularExpression>

class CodeEditor;

class FileRootItem : public QTreeWidgetItem
{
public:
    QString path;

    FileRootItem(const QString& path,QTreeWidget* treewidget);
    void set_path(const QString& path,bool fullpath);
    void set_text(bool fullpath);
};


class TreeItem :public QTreeWidgetItem
{
public:
    int line;
    TreeItem() = default;
    TreeItem(const QString& name,int line,QTreeWidgetItem *parent);
    TreeItem(const QString& name,int line,QTreeWidgetItem *parent,
             QTreeWidgetItem *preceding);
    void set_icon(const QIcon& icon);
    virtual void setup();
    virtual QString mytype() {return "TreeItem object";}
};


class ClassItem : public TreeItem
{
public:
    ClassItem(const QString& name,int line,QTreeWidgetItem *parent);
    ClassItem(const QString& name,int line,QTreeWidgetItem *parent,
              QTreeWidgetItem *preceding);
    void setup() override;
    QString mytype() override {return "ClassItem object";}
};

class FunctionItem : public TreeItem
{
public:
    FunctionItem(const QString& name,int line,QTreeWidgetItem *parent);
    FunctionItem(const QString& name,int line,QTreeWidgetItem *parent,
              QTreeWidgetItem *preceding);
    bool is_method();
    void setup() override;
    QString mytype() override {return "FunctionItem object";}
};

class CommentItem : public TreeItem
{
public:
    CommentItem(const QString& name,int line,QTreeWidgetItem *parent);
    CommentItem(const QString& name,int line,QTreeWidgetItem *parent,
                QTreeWidgetItem *preceding);
    void setup() override;
    QString mytype() override {return "CommentItem object";}
};

class CellItem : public TreeItem
{
public:
    CellItem(const QString& name,int line,QTreeWidgetItem *parent);
    CellItem(const QString& name,int line,QTreeWidgetItem *parent,
                QTreeWidgetItem *preceding);
    void setup() override;
    QString mytype() override {return "CellItem object";}
};


struct ItemLevelDebug
{
    QTreeWidgetItem* item;
    int level;
    QString debug;
    ItemLevelDebug() : item(nullptr),level(-1),debug(QString()){}
    ItemLevelDebug(QTreeWidgetItem* _item,int _level,const QString& _debug)
        : item(_item),level(_level),debug(_debug){}
};


class OutlineExplorerTreeWidget : public OneColumnTree
{
    Q_OBJECT
public:
    bool show_fullpath;
    bool show_all_files;
    bool show_comments;
    bool freeze;
    QHash<CodeEditor*,size_t> editor_ids;
    QHash<size_t,QTreeWidgetItem*> editor_items;
    QHash<size_t,QHash<int,ItemLevelDebug>> editor_tree_cache;
        //外层键是editor_id, 内层键是line行号
    CodeEditor* current_editor;
public:
    OutlineExplorerTreeWidget(QWidget* parent,bool show_fullpath=false,
                              bool show_all_files=true,bool show_comments=true);
    QList<QAction*> get_actions_from_items(QList<QTreeWidgetItem*> items) override;
    void __hide_or_show_root_items(QTreeWidgetItem* item);
    void clear();
    void set_current_editor(CodeEditor* editor,const QString& fname,bool update);
    void file_renamed(CodeEditor* editor,const QString& new_filename);
    void update_all();
    void remove_editor(CodeEditor* editor);
    void __sort_toplevel_items();
    QHash<int,ItemLevelDebug> populate_branch(CodeEditor* editor,QTreeWidgetItem* root_item,
                                              QHash<int,ItemLevelDebug>* tree_cache=nullptr);
    void root_item_selected(QTreeWidgetItem* item);
    void restore();
    QTreeWidgetItem* get_root_item(QTreeWidgetItem* item);
    void activated(QTreeWidgetItem* item) override;
    void clicked(QTreeWidgetItem* item) override;
public slots:
    void toggle_fullpath_mode(bool state);
    void toggle_show_all_files(bool state);
    void toggle_show_comments(bool state);
    void go_to_cursor_position();
};

class OutlineExplorerWidget : public QWidget //为了在plugins/outlineexplorer.h中使用多重继承
{
    Q_OBJECT
signals:
    void edit_goto(const QString&,int,const QString&);
    void edit(const QString&);
    void outlineexplorer_is_visible();

public:
    OutlineExplorerWidget(QWidget* parent=nullptr,bool show_fullpath=false,
                          bool show_all_files=true,bool show_comments=true);
    QList<QToolButton*> setup_buttons();
    void set_current_editor(CodeEditor* editor,const QString& fname,bool update,bool clear);
    void remove_editor(CodeEditor* editor);
    QHash<QString, QVariant> get_options() const;
    void update();
    void file_renamed(CodeEditor* editor,const QString& new_filename);
public slots:
    void toggle_visibility(bool state);
public:
    OutlineExplorerTreeWidget* treewidget;
    QAction* visibility_action;
};
