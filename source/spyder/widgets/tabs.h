#pragma once

#include "utils/misc.h"
#include "utils/icon_manager.h"
#include "utils/qthelpers.h"

#include <QtWidgets>
#include <functional>

class TabBar;

class EditTabNamePopup : public QLineEdit
{
    Q_OBJECT
public:
    TabBar* main;
    QString split_char;
    int split_index;
    int tab_index;

    EditTabNamePopup(QWidget* parent,const QString& split_char,int split_index);
    bool eventFilter(QObject *widget, QEvent *event) override;
    void edit_tab(int index);
public slots:
    void edit_finished();
};


class TabBar : public QTabBar
{
    Q_OBJECT
signals:
    void sig_move_tab(int,int);
    void sig_move_tab(QString,int,int);
    void sig_change_name(const QString&);
public:
    QWidget* ancestor;
    QPoint __drag_start_pos;
    bool rename_tabs;
    EditTabNamePopup* tab_name_editor;

    TabBar(QWidget* parent,QWidget* ancestor,bool rename_tabs=false,
           const QString& split_char="",int split_index=0);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};


class BaseTabs : public QTabWidget
{
    Q_OBJECT
signals:
    void sig_close_tab(int);
public:
    QHash<Qt::Corner, QList<QPair<int, QToolButton*>>> corner_widgets;
    bool menu_use_tooltips;
    QMenu* menu;
    QMenu* browse_tabs_menu;
    QToolButton* browse_button;

public:
    BaseTabs(QWidget* parent,
             const QList<QAction*>& actions = QList<QAction*>(),
             QMenu* menu = nullptr,
             QHash<Qt::Corner, QList<QPair<int, QToolButton*>>> corner_widgets = QHash<Qt::Corner, QList<QPair<int, QToolButton*>>>(),
             bool menu_use_tooltips = false);
    void set_corner_widgets(const QHash<Qt::Corner, QList<QPair<int, QToolButton*>>>& corner_widgets);
    void add_corner_widgets(const QList<QPair<int, QToolButton*>>& widgets, Qt::Corner corner=Qt::TopRightCorner);
    void tab_navigate(int delta=1);
    void set_close_function(std::function<void(int)> func);
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
public slots:
    void update_browse_tabs_menu();
};


class Tabs : public BaseTabs
{
    Q_OBJECT
signals:
    void move_data(int,int);
    void move_tab_finished();
    void sig_move_tab(QString,QString,int,int);
public:
    Tabs(QWidget* parent,
         const QList<QAction*>& actions = QList<QAction*>(),
         QMenu* menu = nullptr,
         QHash<Qt::Corner, QList<QPair<int, QToolButton*>>> corner_widgets = QHash<Qt::Corner, QList<QPair<int, QToolButton*>>>(),
         bool menu_use_tooltips = false,
         bool rename_tabs = false,
         const QString& split_char = "",
         int split_index = 0);

public slots:
    void move_tab(int index_from,int index_to);
    void move_tab_from_another_tabwidget(QString tabwidget_from,
                                         int index_from,int index_to);
};
