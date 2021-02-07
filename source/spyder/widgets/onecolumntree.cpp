#include "onecolumntree.h"

OneColumnTree::OneColumnTree(QWidget* parent)
    : QTreeWidget (parent)
{
    setItemsExpandable(true);
    setColumnCount(1);
    connect(this,SIGNAL(itemActivated(QTreeWidgetItem *, int)),
            this,SLOT(activated(QTreeWidgetItem *)));
    connect(this,SIGNAL(itemClicked(QTreeWidgetItem *, int)),
            this,SLOT(clicked(QTreeWidgetItem *)));

    menu = new QMenu(this);
    common_actions = setup_common_actions();


    connect(this,SIGNAL(itemSelectionChanged()),
            this,SLOT(item_selection_changed()));
    item_selection_changed();
}

void OneColumnTree::set_title(const QString &title)
{
    setHeaderLabels(QStringList(title));
}

QList<QAction*> OneColumnTree::setup_common_actions()
{
    collapse_all_action = new QAction("Collapse all",this);
    connect(collapse_all_action,SIGNAL(triggered(bool)),this,SLOT(collapseAll()));
    collapse_all_action->setIcon(ima::icon("collapse"));
    collapse_all_action->setShortcutContext(Qt::WindowShortcut);

    expand_all_action = new QAction("Expand all",this);
    connect(expand_all_action,SIGNAL(triggered(bool)),this,SLOT(expandAll()));
    expand_all_action->setIcon(ima::icon("expand"));
    expand_all_action->setShortcutContext(Qt::WindowShortcut);

    restore_action = new QAction("Restore",this);
    connect(restore_action,SIGNAL(triggered(bool)),this,SLOT(restore()));
    restore_action->setIcon(ima::icon("restore"));
    restore_action->setToolTip("Restore original tree layout");
    restore_action->setShortcutContext(Qt::WindowShortcut);

    collapse_selection_action = new QAction("Collapse selection",this);
    connect(collapse_selection_action,SIGNAL(triggered(bool)),this,SLOT(collapse_selection()));
    collapse_selection_action->setIcon(ima::icon("collapse_selection"));
    collapse_selection_action->setShortcutContext(Qt::WindowShortcut);

    expand_selection_action = new QAction("Expand selection",this);
    connect(expand_selection_action,SIGNAL(triggered(bool)),this,SLOT(expand_selection()));
    expand_selection_action->setIcon(ima::icon("expand_selection"));
    expand_selection_action->setShortcutContext(Qt::WindowShortcut);

    QList<QAction*> actions;
    actions << collapse_all_action << expand_all_action
            <<restore_action << nullptr
           <<collapse_selection_action << expand_selection_action;
    return actions;
}

void OneColumnTree::update_menu()
{
    this->menu->clear();
    QList<QTreeWidgetItem*> items = selectedItems();
    QList<QAction*> actions = get_actions_from_items(items);
    if (!actions.isEmpty())
        actions.append(nullptr);
    actions.append(common_actions);
    add_actions(this->menu, actions);
}

QList<QAction*> OneColumnTree::get_actions_from_items(QList<QTreeWidgetItem*> items)
{
    Q_UNUSED(items);
    QList<QAction*> actions;
    return actions;
}

void OneColumnTree::restore()
{
    this->collapseAll();
    foreach (QTreeWidgetItem* item, this->get_top_level_items()) {
        this->expandItem(item);
    }
}
void OneColumnTree::__expand_item(QTreeWidgetItem *item)
{
    if (this->is_item_expandable(item)) {
        this->expandItem(item);
        for (int index = 0; index < item->childCount(); ++index) {
            QTreeWidgetItem* child = item->child(index);
            __expand_item(child);
        }
    }
}

void OneColumnTree::expand_selection()
{
    QList<QTreeWidgetItem*> items = this->selectedItems();
    if (items.isEmpty())
        items = this->get_top_level_items();
    foreach (QTreeWidgetItem* item, items) {
        this->__expand_item(item);
    }
    if (!items.isEmpty())
        this->scrollToItem(items[0]);
}

void OneColumnTree::__collapse_item(QTreeWidgetItem *item)
{
    this->collapseItem(item);
    for (int index = 0; index < item->childCount(); ++index) {
        QTreeWidgetItem* child = item->child(index);
        __collapse_item(child);
    }
}

void OneColumnTree::collapse_selection()
{
    QList<QTreeWidgetItem*> items = this->selectedItems();
    if (items.isEmpty())
        items = this->get_top_level_items();
    foreach (QTreeWidgetItem* item, items) {
        this->__collapse_item(item);
    }
    if (!items.isEmpty())
        this->scrollToItem(items[0]);
}

void OneColumnTree::item_selection_changed()
{
    bool is_selection  = this->selectedItems().size() > 0;
    expand_selection_action->setEnabled(is_selection);
    collapse_selection_action->setEnabled(is_selection);
}

QList<QTreeWidgetItem*> OneColumnTree::get_top_level_items()
{
    QList<QTreeWidgetItem*> list;
    for (int i = 0; i < this->topLevelItemCount(); ++i) {
        list.append(this->topLevelItem(i));
    }
    return list;
}


QList<QTreeWidgetItem*> OneColumnTree::get_items()
{
    QList<QTreeWidgetItem*> itemlist;

    // 这里应该用& 任何被使用到的外部变量都隐式地以引用方式加以引用。
    std::function<void(QTreeWidgetItem*)> add_to_itemlist = [&](QTreeWidgetItem *item)
    {
        for (int index = 0; index < item->childCount(); ++index) {
            QTreeWidgetItem* citem = item->child(index);
            itemlist.append(citem);
            add_to_itemlist(citem);
        }
    };

    foreach (QTreeWidgetItem* tlitem, this->get_top_level_items()) {
        add_to_itemlist(tlitem);
    }
    return itemlist;
}

QPoint OneColumnTree::get_scrollbar_position() const
{
    return QPoint(horizontalScrollBar()->value(),
                  verticalScrollBar()->value());
}

void OneColumnTree::set_scrollbar_position(const QPoint &position)
{
    int hor = position.x(), ver = position.y();
    horizontalScrollBar()->setValue(hor);
    verticalScrollBar()->setValue(ver);
}

QHash<uint,bool> OneColumnTree::get_expanded_state()
{
    this->save_expanded_state();
    return __expanded_state;
}

void OneColumnTree::set_expanded_state(const QHash<uint,bool>& state)
{
    __expanded_state = state;
    this->restore_expanded_state();
}


void OneColumnTree::save_expanded_state()
{
    __expanded_state.clear();

    auto add_to_state = [this](QTreeWidgetItem *item)
    {
        QString user_text = get_item_user_text(item);
        this->__expanded_state[qHash(user_text)] = item->isExpanded();
    };

    std::function<void(QTreeWidgetItem*)> browse_children = [&](QTreeWidgetItem *item)
    {
        add_to_state(item);
        for (int index = 0; index < item->childCount(); ++index) {
            QTreeWidgetItem* citem = item->child(index);
            QString user_text = get_item_user_text(citem);
            __expanded_state[qHash(user_text)] = citem->isExpanded();
            browse_children(citem);
        }
    };

    foreach (QTreeWidgetItem* tlitem, this->get_top_level_items()) {
        browse_children(tlitem);
    }
}

void OneColumnTree::restore_expanded_state()
{
    if (__expanded_state.isEmpty())
        return;
    QList<QTreeWidgetItem*> itemlist = this->get_items();
    itemlist.append(this->get_top_level_items());
    foreach (QTreeWidgetItem* item, itemlist) {
        QString user_text = get_item_user_text(item);
        if (__expanded_state.contains(qHash(user_text))) {
            bool is_expanded = __expanded_state.value(qHash(user_text));
            item->setExpanded(is_expanded);
        }
    }
}

void OneColumnTree::sort_top_level_items(std::function<bool(QTreeWidgetItem*,QTreeWidgetItem*)> key)
{
    this->save_expanded_state();

    int count = this->topLevelItemCount();
    QList<QTreeWidgetItem*> items;
    for (int index = 0; index < count; ++index) {
        items.append(this->takeTopLevelItem(0));
    }
    std::sort(items.begin(),items.end(),key);

    for (int index = 0; index < items.size(); ++index) {
        this->insertTopLevelItem(index, items[index]);
    }
    this->restore_expanded_state();
}

void OneColumnTree::contextMenuEvent(QContextMenuEvent *event)
{
    update_menu();
    this->menu->popup(event->globalPos());
}
