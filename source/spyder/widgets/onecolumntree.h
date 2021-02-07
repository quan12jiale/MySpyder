#pragma once

#include "utils/icon_manager.h"
#include "utils/qthelpers.h"

#include <QDebug>
#include <QTreeWidget>
#include <QScrollBar>
#include <QContextMenuEvent>
#include <functional>

class OneColumnTree : public QTreeWidget
{
    Q_OBJECT
public:
    QMenu* menu;
    QAction* collapse_all_action;
    QAction* collapse_selection_action;
    QAction* restore_action;
    QAction* expand_all_action;
    QAction* expand_selection_action;
    QList<QAction*> common_actions;
    QHash<uint,bool> __expanded_state;

    OneColumnTree(QWidget* parent);

    void set_title(const QString& title);
    QList<QAction*> setup_common_actions();
    void update_menu();
    virtual QList<QAction*> get_actions_from_items(QList<QTreeWidgetItem*> items);

public slots:
    virtual void activated(QTreeWidgetItem *item) = 0;//Double-click event
    virtual void clicked(QTreeWidgetItem *){}

    void restore();
    void expand_selection();
    void collapse_selection();

    void item_selection_changed();
public:
    virtual bool is_item_expandable(QTreeWidgetItem *) { return true; }
    void __expand_item(QTreeWidgetItem *item);
    void __collapse_item(QTreeWidgetItem *item);

    QList<QTreeWidgetItem*> get_top_level_items();
    QList<QTreeWidgetItem*> get_items();

    QPoint get_scrollbar_position() const;
    void set_scrollbar_position(const QPoint& position);
    QHash<uint,bool> get_expanded_state();
    void set_expanded_state(const QHash<uint,bool>& state);
    void save_expanded_state();

    void restore_expanded_state();
    void sort_top_level_items(std::function<bool(QTreeWidgetItem*,QTreeWidgetItem*)> key);
    void contextMenuEvent(QContextMenuEvent *event) override;
};
