#include "editortools.h"
#include "sourcecode/codeeditor.h"

//******************* FileRootItem
FileRootItem::FileRootItem(const QString& path,QTreeWidget* treewidget)
    : QTreeWidgetItem (treewidget, QTreeWidgetItem::Type)
{
    this->path = path;
    setIcon(0, ima::icon("python"));
    setToolTip(0, path);
    set_item_user_text(this, path);
}

void FileRootItem::set_path(const QString &path, bool fullpath)
{
    this->path = path;
    set_text(fullpath);
}

void FileRootItem::set_text(bool fullpath)
{
    if (fullpath)
        setText(0, this->path);
    else {
        QFileInfo info(this->path);
        setText(0, info.fileName());
    }
}

bool preceding_is_null(QTreeWidgetItem *parent,QTreeWidgetItem** preceding)
{
    if (*preceding == nullptr)
        return true;
    else {
        if (*preceding != parent) {
            while ((*preceding)->parent() != parent) {
                *preceding = (*preceding)->parent();
                if (*preceding == nullptr)
                    break;
            }
        }
        if (*preceding == nullptr)
            return true;
        else
            return false;
    }
}

//******************* TreeItem
TreeItem::TreeItem(const QString& name,int line,QTreeWidgetItem *parent)
    : QTreeWidgetItem(parent, QTreeWidgetItem::Type)
{
    setText(0, name);
    QString parent_text = parent->data(0, Qt::UserRole).toString();
    set_item_user_text(this, parent_text + "/" + name);
    this->line = line;
}

TreeItem::TreeItem(const QString& name,int line,QTreeWidgetItem *parent,
         QTreeWidgetItem *preceding)
    : QTreeWidgetItem(parent, preceding, QTreeWidgetItem::Type)
{
    setText(0, name);
    QString parent_text = parent->data(0, Qt::UserRole).toString();
    set_item_user_text(this, parent_text + "/" + name);
    this->line = line;
}

void TreeItem::set_icon(const QIcon &icon)
{
    setIcon(0, icon);
}

void TreeItem::setup()
{
    setToolTip(0, QString("Line %1").arg(this->line));
}


//***************** ClassItem
ClassItem::ClassItem(const QString& name,int line,QTreeWidgetItem *parent)
    : TreeItem (name,line,parent)
{}

ClassItem::ClassItem(const QString& name,int line,QTreeWidgetItem *parent,
          QTreeWidgetItem *preceding)
    : TreeItem (name,line,parent,preceding)
{}

void ClassItem::setup()
{
    set_icon(ima::icon("class"));
    setToolTip(0, QString("Class defined at line %1").arg(this->line));
}


FunctionItem::FunctionItem(const QString& name,int line,QTreeWidgetItem *parent)
    : TreeItem (name,line,parent)
{}

FunctionItem::FunctionItem(const QString& name,int line,QTreeWidgetItem *parent,
          QTreeWidgetItem *preceding)
    : TreeItem (name,line,parent,preceding)
{}

bool FunctionItem::is_method()
{
    QTreeWidgetItem* item = this->parent();
    if (item == nullptr)
        return false;
    ClassItem* classitem = dynamic_cast<ClassItem*>(item);
    if (!classitem)
        return false;
    return true;
}

void FunctionItem::setup()
{
    if (this->is_method()) {
        setToolTip(0, QString("Method defined at line %1").arg(this->line));
        QString name = this->text(0);
        if (name.startsWith("__"))
            set_icon(ima::icon("private2"));
        else if (name.startsWith("_"))
            set_icon(ima::icon("private1"));
        else
            set_icon(ima::icon("method"));
    }
    else {
        set_icon(ima::icon("function"));
        setToolTip(0, QString("Function defined at line %1").arg(this->line));
    }
}


CommentItem::CommentItem(const QString& name,int line,QTreeWidgetItem *parent)
    : TreeItem(name,line,parent)
{
    QString _name = lstrip(name,"# ");
    setText(0, _name);
    QString parent_text = parent->data(0, Qt::UserRole).toString();
    set_item_user_text(this, parent_text + "/" + _name);
}

CommentItem::CommentItem(const QString& name,int line,QTreeWidgetItem *parent,
            QTreeWidgetItem *preceding)
    : TreeItem(name,line,parent,preceding)
{
    QString _name = lstrip(name,"# ");
    setText(0, _name);
    QString parent_text = parent->data(0, Qt::UserRole).toString();
    set_item_user_text(this, parent_text + "/" + _name);
}

void CommentItem::setup()
{
    this->set_icon(ima::icon("blockcomment"));
    QFont font = this->font(0);
    font.setItalic(true);
    setFont(0, font);
    setToolTip(0, QString("Line %1").arg(this->line));
}


CellItem::CellItem(const QString& name,int line,QTreeWidgetItem *parent)
    : TreeItem(name,line,parent)
{
    QString _name = lstrip(name, "#% ");
    if (_name.startsWith("<codecell>"))
        _name = lstrip(_name.mid(10));
    else if (_name.startsWith("In[")) {
        _name = _name.mid(2);
        if (_name.endsWith("]:"))
            _name.chop(1);
        _name = _name.trimmed();
    }
    setText(0, _name);
    QString parent_text = parent->data(0, Qt::UserRole).toString();
    set_item_user_text(this, parent_text + "/" + _name);
}

CellItem::CellItem(const QString& name,int line,QTreeWidgetItem *parent,
            QTreeWidgetItem *preceding)
    : TreeItem(name,line,parent,preceding)
{
    QString _name = lstrip(name, "#% ");
    if (_name.startsWith("<codecell>"))
        _name = lstrip(_name.mid(10));
    else if (_name.startsWith("In[")) {
        _name = _name.mid(2);
        if (_name.endsWith("]:"))
            _name.chop(1);
        _name = _name.trimmed();
    }
    setText(0, _name);
    QString parent_text = parent->data(0, Qt::UserRole).toString();
    set_item_user_text(this, parent_text + "/" + _name);
}

void CellItem::setup()
{
    set_icon(ima::icon("cell"));
    QFont font = this->font(0);
    font.setItalic(true);
    setFont(0, font);
    setToolTip(0, QString("Cell starts at line %1").arg(this->line));
}


//辅助函数
static QList<QTreeWidgetItem*> get_item_children(QTreeWidgetItem* item)
{
    QList<QTreeWidgetItem*> children;
    for (int index = 0; index < item->childCount(); ++index) {
        children.append(item->child(index));
    }
    QList<QTreeWidgetItem*> copy = children;
    foreach (QTreeWidgetItem* child, copy) {
        QList<QTreeWidgetItem*> others = get_item_children(child);
        if (!others.isEmpty())
            children.append(others);
    }

    QList<TreeItem*> tmp;
    foreach (QTreeWidgetItem* child, children) {
        tmp.append(dynamic_cast<TreeItem*>(child));
    }
    std::sort(tmp.begin(),tmp.end(),
          [](TreeItem* item1,TreeItem* item2) { return item1->line < item2->line; });
    QList<QTreeWidgetItem*> res;
    foreach (auto child, tmp) {
        res.append(dynamic_cast<QTreeWidgetItem*>(child));
    }
    return res;
}

static QTreeWidgetItem* item_at_line(QTreeWidgetItem* root_item,int line)
{
    QTreeWidgetItem* previous_item = root_item;
    foreach (QTreeWidgetItem* item, get_item_children(root_item)) {
        TreeItem* tmp = dynamic_cast<TreeItem*>(item);
        if (tmp && tmp->line > line)
            return previous_item;
        previous_item = item;
    }
    return root_item;
}


//tree_cache需要是指针或引用
static void remove_from_tree_cache(QHash<int,ItemLevelDebug>& tree_cache,int line=-1,
                                   QTreeWidgetItem* item=nullptr)
{
    if (line == -1) {
        for (auto it=tree_cache.begin();it!=tree_cache.end();it++) {
            line = it.key();
            QTreeWidgetItem* _it = it.value().item;
            if (_it == item)
                break;
        }
    }

    item = tree_cache[line].item;
    QString debug = tree_cache[line].debug;
    tree_cache.remove(line);

    try {
        QList<QTreeWidgetItem*> children;
        for (int index = 0; index < item->childCount(); ++index) {
            children.append(item->child(index));
        }
        foreach (QTreeWidgetItem* child, children) {
            remove_from_tree_cache(tree_cache,-1,child);
        }
        if (item->parent())
            item->parent()->removeChild(item);
    } catch (...) {
        qDebug()<<"unable to remove tree item: "<<debug;
    }
}


//****************** OutlineExplorerTreeWidget
OutlineExplorerTreeWidget::OutlineExplorerTreeWidget(QWidget* parent,bool show_fullpath,
                                                     bool show_all_files,bool show_comments)
    : OneColumnTree (parent)
{
    this->show_fullpath = show_fullpath;
    this->show_all_files = show_all_files;
    this->show_comments = show_comments;
    this->freeze = false;

    current_editor = nullptr;
    QString title = "Outline";
    set_title(title);
    setWindowTitle(title);
    setUniformRowHeights(true);
}

QList<QAction*> OutlineExplorerTreeWidget::get_actions_from_items(QList<QTreeWidgetItem *> items)
{
    Q_UNUSED(items);
    QAction* fromcursor_act = new QAction("Go to cursor position",this);
    connect(fromcursor_act,SIGNAL(triggered()),this,SLOT(go_to_cursor_position()));
    fromcursor_act->setIcon(ima::icon("fromcursor"));
    fromcursor_act->setShortcutContext(Qt::WindowShortcut);

    QAction* fullpath_act = new QAction("Show absolute path",this);
    connect(fullpath_act,SIGNAL(toggled(bool)),this,SLOT(toggle_fullpath_mode(bool)));
    fullpath_act->setCheckable(true);
    fullpath_act->setChecked(show_fullpath);
    fullpath_act->setShortcutContext(Qt::WindowShortcut);

    QAction* allfiles_act = new QAction("Show all files",this);
    connect(allfiles_act,SIGNAL(toggled(bool)),this,SLOT(toggle_show_all_files(bool)));
    allfiles_act->setCheckable(true);
    allfiles_act->setChecked(show_all_files);
    allfiles_act->setShortcutContext(Qt::WindowShortcut);

    QAction* comment_act = new QAction("Show special comments",this);
    connect(comment_act,SIGNAL(toggled(bool)),this,SLOT(toggle_show_comments(bool)));
    comment_act->setCheckable(true);
    comment_act->setChecked(show_comments);
    comment_act->setShortcutContext(Qt::WindowShortcut);

    QList<QAction*> actions;
    actions << fullpath_act << allfiles_act << comment_act << fromcursor_act;
    return actions;
}

//@Slot(bool)
void OutlineExplorerTreeWidget::toggle_fullpath_mode(bool state)
{
    show_fullpath = state;
    setTextElideMode(state ? Qt::ElideMiddle : Qt::ElideRight);
    for (int index = 0; index < topLevelItemCount(); ++index) {
        FileRootItem* item = dynamic_cast<FileRootItem*>(topLevelItem(index));
        if (item)
            item->set_text(show_fullpath);
    }
}

void OutlineExplorerTreeWidget::__hide_or_show_root_items(QTreeWidgetItem *item)
{
    foreach (QTreeWidgetItem* _it, get_top_level_items()) {
        _it->setHidden((_it != item) && (!show_all_files));
    }
}

//@Slot(bool)
void OutlineExplorerTreeWidget::toggle_show_all_files(bool state)
{
    show_all_files = state;
    if (current_editor) {
        size_t editor_id = editor_ids[current_editor];
        QTreeWidgetItem* item = editor_items[editor_id];
        __hide_or_show_root_items(item);
    }
}

//@Slot(bool)
void OutlineExplorerTreeWidget::toggle_show_comments(bool state)
{
    show_comments = state;
    update_all();
}

//@Slot()
void OutlineExplorerTreeWidget::go_to_cursor_position()
{
    if (current_editor) {
        int line = current_editor->get_cursor_line_number();
        size_t editor_id = editor_ids[current_editor];
        QTreeWidgetItem* root_item = editor_items[editor_id];
        QTreeWidgetItem* item = item_at_line(root_item, line);
        setCurrentItem(item);
        scrollToItem(item);
    }
}

void OutlineExplorerTreeWidget::clear()
{
    set_title("");
    OneColumnTree::clear();
}

void OutlineExplorerTreeWidget::set_current_editor(CodeEditor *editor, const QString &fname, bool update)
{
    size_t editor_id = editor->get_document_id();
    if (editor_ids.values().contains(editor_id)) {
        QTreeWidgetItem* item = editor_items[editor_id];
        if (freeze == false) {
            scrollToItem(item);
            root_item_selected(item);
            __hide_or_show_root_items(item);
        }
        if (update) {
            save_expanded_state();
            QHash<int,ItemLevelDebug> tree_cache = editor_tree_cache[editor_id];
            populate_branch(editor,item,tree_cache);
            restore_expanded_state();
        }
    }
    else {
        FileRootItem* root_item = new FileRootItem(fname,this);
        root_item->set_text(show_fullpath);
        QHash<int,ItemLevelDebug> tree_cache = populate_branch(editor,root_item);

        __sort_toplevel_items();
        __hide_or_show_root_items(root_item);
        root_item_selected(root_item);
        editor_items[editor_id] = root_item;
        editor_tree_cache[editor_id] = tree_cache;
        resizeColumnToContents(0);
    }
    if (!editor_ids.contains(editor))
        editor_ids[editor] = editor_id;
    current_editor = editor;
}

void OutlineExplorerTreeWidget::file_renamed(CodeEditor *editor, const QString &new_filename)
{
    size_t editor_id = editor->get_document_id();
    if (editor_ids.values().contains(editor_id)) {
        QTreeWidgetItem* root_item = editor_items[editor_id];
        FileRootItem* item = dynamic_cast<FileRootItem*>(root_item);
        item->set_path(new_filename, show_fullpath);
        __sort_toplevel_items();
    }
}

void OutlineExplorerTreeWidget::update_all()
{
    save_expanded_state();
    for (auto it=editor_ids.begin();it!=editor_ids.end();it++) {
        CodeEditor* editor = it.key();
        size_t editor_id = it.value();
        QTreeWidgetItem* item = editor_items[editor_id];
        QHash<int,ItemLevelDebug> tree_cache = editor_tree_cache[editor_id];
        populate_branch(editor,item,tree_cache);     //暂时先禁用,再次调用该函数会出bug
    }
    restore_expanded_state();
}

void OutlineExplorerTreeWidget::remove_editor(CodeEditor *editor)
{
    if (editor_ids.contains(editor)) {
        if (current_editor == editor)
            current_editor = nullptr;
        size_t editor_id = editor_ids.take(editor);
        if (!editor_ids.values().contains(editor_id)) {
            QTreeWidgetItem* root_item = editor_items.take(editor_id);
            editor_tree_cache.remove(editor_id);
            try {
                takeTopLevelItem(indexOfTopLevelItem(root_item));
            } catch (...) {
            }
        }
    }
}

static bool sort_func(QTreeWidgetItem* item1,QTreeWidgetItem* item2)
{
    /*
     * os.path.basename(path) : 返回文件名
     * QFileInfo::fileName    : 返回文件名
    */
    FileRootItem* it1 = dynamic_cast<FileRootItem*>(item1);
    FileRootItem* it2 = dynamic_cast<FileRootItem*>(item2);
    if (!it1 || !it2)
        return true;
    QFileInfo info1(it1->path.toLower());
    QFileInfo info2(it2->path.toLower());
    int res = info1.fileName().compare(info2.fileName());
    if (res <= 0)
        return true;
    return false;
}

void OutlineExplorerTreeWidget::__sort_toplevel_items()
{
    this->sort_top_level_items(sort_func);
}

QHash<int,ItemLevelDebug> OutlineExplorerTreeWidget::populate_branch(CodeEditor *editor,
                                                                     QTreeWidgetItem *root_item,
                                                                     QHash<int, ItemLevelDebug> tree_cache)
{
    foreach (int _l, tree_cache.keys()) {
        if (_l >= editor->get_line_count()) {
            if (tree_cache.contains(_l))
                remove_from_tree_cache(tree_cache, _l);
        }
    }
    QList<QPair<QTreeWidgetItem*,int>> ancestors;
    ancestors.append(qMakePair(root_item,0));
    QTreeWidgetItem* previous_item = nullptr;
    int previous_level = -1;

    QHash<int,sh::OutlineExplorerData> oe_data = editor->highlighter->get_outlineexplorer_data();
    editor->has_cell_separators = editor->highlighter->found_cell_separators;
    for (int block_nb = 0; block_nb < editor->get_line_count(); ++block_nb) {
        int line_nb = block_nb+1;
        // TODO源码是data = oe_data.get(block_nb);if data is None:
        sh::OutlineExplorerData data = oe_data.value(block_nb,sh::OutlineExplorerData());
        int level;
        if (data.fold_level == -1)
            level = -1;
        else
            level = data.fold_level;
        ItemLevelDebug pair = tree_cache.value(line_nb, ItemLevelDebug());
        QTreeWidgetItem* citem = pair.item;
        int clevel = pair.level;
        if (level == -1) {
            if (citem)
                remove_from_tree_cache(tree_cache, line_nb);
            continue;
        }

        bool not_class_nor_function = data.is_not_class_nor_function();
        QString class_name;
        QString func_name;
        if (!not_class_nor_function) {
            class_name = data.get_class_name();
            if (class_name.isEmpty()) {
                func_name = data.get_function_name();
                if (func_name.isEmpty()) {
                    if (citem != nullptr)
                        remove_from_tree_cache(tree_cache, line_nb);
                    continue;
                }
            }
        }

        if (previous_level != -1) {
            if (level == previous_level)
                ;
            else if (level > previous_level+4)
                continue;
            else if (level > previous_level)
                ancestors.append(qMakePair(previous_item, previous_level));
            else {
                while (ancestors.size() > 1 && level <= previous_level) {
                    ancestors.pop_back();
                    previous_level = ancestors.back().second;
                }
            }
        }
        QTreeWidgetItem* parent = ancestors.back().first;

        QString cname;
        if (citem != nullptr)
            cname = citem->text(0);

        QTreeWidgetItem* preceding;
        if (previous_item == nullptr)
            preceding = root_item;
        else
            preceding = previous_item;

        TreeItem* item = nullptr;
        if (not_class_nor_function) {
            if (data.is_comment() && !show_comments) {
                if (citem != nullptr)
                    remove_from_tree_cache(tree_cache, line_nb);
                continue;
            }
            if (citem != nullptr) {
                if (data.text == cname && level == clevel) {
                    previous_level = clevel;
                    previous_item = citem;
                    continue;
                }
                else
                    remove_from_tree_cache(tree_cache, line_nb);
            };
            if (data.is_comment()) {
                if (data.def_type == data.CELL) {
                    QTreeWidgetItem* _preceding = preceding;
                    if (preceding_is_null(parent, &_preceding))
                        item = new CellItem(data.text, line_nb, parent);
                    else
                        item = new CellItem(data.text, line_nb, parent, _preceding);
                }
                else {
                    QTreeWidgetItem* _preceding = preceding;
                    if (preceding_is_null(parent, &_preceding))
                        item = new CommentItem(data.text, line_nb, parent);
                    else
                        item = new CommentItem(data.text, line_nb, parent, _preceding);
                }
            }
            else {
                QTreeWidgetItem* _preceding = preceding;
                if (preceding_is_null(parent, &_preceding))
                    item = new TreeItem(data.text, line_nb, parent);
                else
                    item = new TreeItem(data.text, line_nb, parent, _preceding);
            }
        }

        else if (!class_name.isEmpty()) {
            if (citem != nullptr) {
                if (class_name == cname && level == clevel) {
                    previous_level = clevel;
                    previous_item = citem;
                    continue;
                }
                else
                    remove_from_tree_cache(tree_cache, line_nb);
            }
            QTreeWidgetItem* _preceding = preceding;
            if (preceding_is_null(parent, &_preceding))
                item = new ClassItem(data.text, line_nb, parent);
            else
                item = new ClassItem(data.text, line_nb, parent, _preceding);
        }

        else {
            if (citem != nullptr) {
                if (func_name == cname && level == clevel) {
                    previous_level = clevel;
                    previous_item = citem;
                    continue;
                }
                else
                    remove_from_tree_cache(tree_cache, line_nb);
            }
            QTreeWidgetItem* _preceding = preceding;
            if (preceding_is_null(parent, &_preceding))
                item = new FunctionItem(data.text, line_nb, parent);
            else
                item = new FunctionItem(data.text, line_nb, parent, _preceding);
        }

        item->setup();
        QString tmp = QString::number(item->line).rightJustified(6);
        QString debug = QString("%1 -- %2/%3").arg(tmp)
                        .arg(item->parent()->text(0))
                        .arg(item->text(0));
        tree_cache[line_nb] = ItemLevelDebug(item,level,debug);
        previous_level = level;
        previous_item = item;
    }
    return tree_cache;
}

void OutlineExplorerTreeWidget::root_item_selected(QTreeWidgetItem *item)
{
    for (int index = 0; index < topLevelItemCount(); ++index) {
        QTreeWidgetItem* root_item = topLevelItem(index);
        if (root_item == item)
            expandItem(root_item);
        else
            collapseItem(root_item);
    }
}

void OutlineExplorerTreeWidget::restore()
{
    if (current_editor) {
        collapseAll();
        size_t editor_id = editor_ids[current_editor];
        root_item_selected(editor_items[editor_id]);
    }
}

QTreeWidgetItem* OutlineExplorerTreeWidget::get_root_item(QTreeWidgetItem *item)
{
    QTreeWidgetItem* root_item = item;
    while (root_item->parent() != nullptr)
        root_item = root_item->parent();
    return root_item;
}

void OutlineExplorerTreeWidget::activated(QTreeWidgetItem *item)
{
    int line = 0;
    TreeItem* treeitem = dynamic_cast<TreeItem*>(item);
    if (treeitem)
        line = treeitem->line;
    FileRootItem* root_item = dynamic_cast<FileRootItem*>(get_root_item(item));
    freeze = true;
    OutlineExplorerWidget* outlineparent = dynamic_cast<OutlineExplorerWidget*>(parent());
    if (line)
        emit outlineparent->edit_goto(root_item->path, line, item->text(0));
    else
        emit outlineparent->edit(root_item->path);
    freeze = false;
    QObject* parent = current_editor->parent();
    for (auto it=editor_items.begin();it!=editor_items.end();it++) {
        size_t editor_id = it.key();
        QTreeWidgetItem* i_item = it.value();
        FileRootItem* filerootItem = dynamic_cast<FileRootItem*>(i_item);
        if (filerootItem && filerootItem == root_item) {
            for (auto it2=editor_ids.begin();it2!=editor_ids.end();it2++) {
                CodeEditor* editor = it2.key();
                size_t _id = it2.value();
                if (_id == editor_id && editor->parent() == parent) {
                    current_editor = editor;
                    break;
                }
            }
            break;
        }
    }
}

void OutlineExplorerTreeWidget::clicked(QTreeWidgetItem *item)
{
    FileRootItem* root_item = dynamic_cast<FileRootItem*>(item);
    if (root_item)
        root_item_selected(item);
    activated(item);
}


OutlineExplorerWidget::OutlineExplorerWidget(QWidget* parent,bool show_fullpath,
                                            bool show_all_files,bool show_comments)
    : QWidget (parent)
{
    treewidget = new OutlineExplorerTreeWidget(this,
                                               show_fullpath,
                                               show_all_files,
                                               show_comments);

    visibility_action = new QAction("Show/hide outline explorer",this);
    connect(visibility_action,SIGNAL(toggled(bool)),this,SLOT(toggle_visibility(bool)));
    visibility_action->setCheckable(true);
    visibility_action->setChecked(true);
    QIcon icon = ima::get_icon("outline_explorer_vis.png");
    visibility_action->setIcon(icon);
    visibility_action->setShortcutContext(Qt::WindowShortcut);

    QHBoxLayout* btn_layout = new QHBoxLayout;
    btn_layout->setAlignment(Qt::AlignLeft);
    foreach (auto btn, setup_buttons()) {
        btn_layout->addWidget(btn);
    }
    QVBoxLayout* layout = create_plugin_layout(btn_layout, treewidget);
    setLayout(layout);
}

//@Slot(bool)
void OutlineExplorerWidget::toggle_visibility(bool state)
{
    setVisible(state);
    CodeEditor* current_editor = treewidget->current_editor;
    if (current_editor) {
        current_editor->clearFocus();
        current_editor->setFocus();
        if (state)
            emit outlineexplorer_is_visible();
    }
}

QList<QToolButton*> OutlineExplorerWidget::setup_buttons()
{
    QToolButton* fromcursor_btn = new QToolButton(this);
    fromcursor_btn->setIcon(ima::icon("fromcursor"));
    fromcursor_btn->setToolTip("Go to cursor position");
    fromcursor_btn->setAutoRaise(true);
    connect(fromcursor_btn,SIGNAL(clicked(bool)),
            treewidget,SLOT(go_to_cursor_position()));

    QList<QToolButton*> buttons;
    buttons.append(fromcursor_btn);
    QList<QAction*> actions;
    actions << treewidget->collapse_all_action
            << treewidget->expand_all_action
            << treewidget->restore_action
            << treewidget->collapse_selection_action
            << treewidget->expand_selection_action;
    foreach (QAction* action, actions) {
        QToolButton* button = new QToolButton(this);
        button->setAutoRaise(true);
        buttons.append(button);
        buttons.last()->setDefaultAction(action);
    }
    return buttons;
}

void OutlineExplorerWidget::set_current_editor(CodeEditor *editor, const QString &fname,
                                               bool update, bool clear)
{
    if (clear)
        remove_editor(editor);
    if (editor->highlighter)
        treewidget->set_current_editor(editor,fname,update);
}

void OutlineExplorerWidget::remove_editor(CodeEditor *editor)
{
    treewidget->remove_editor(editor);
}

QHash<QString, QVariant> OutlineExplorerWidget::get_options() const
{
    QHash<QString, QVariant> dict;
    dict["show_fullpath"] = this->treewidget->show_fullpath;
    dict["show_all_files"] = this->treewidget->show_all_files;
    dict["show_comments"] = this->treewidget->show_comments;
    QHash<uint,bool> tmp = this->treewidget->get_expanded_state();
    QHash<QString, QVariant> val;//把QHash<uint,bool>转为QHash<QString, QVariant>
    foreach (uint key, tmp.keys()) {
        val[QString::number(key)] = tmp[key];
    }
    dict["expanded_state"] = val;

    dict["scrollbar_position"] = this->treewidget->get_scrollbar_position();
    dict["visibility"] = this->isVisible();
    return dict;
}

void OutlineExplorerWidget::update()
{
    treewidget->update_all();
}

void OutlineExplorerWidget::file_renamed(CodeEditor *editor, const QString &new_filename)
{
    treewidget->file_renamed(editor, new_filename);
}
