#pragma once

#include "utils/misc.h"
#include "utils/icon_manager.h"
#include <QtWidgets>


class PathManager : public QDialog
{
    Q_OBJECT
signals:
    void redirect_stdio(bool);
public:
    QStringList pathlist;
    QStringList ro_pathlist;
    QStringList not_active_pathlist;
    QString last_path;
    QList<QToolButton*> selection_widgets;
    QList<QToolButton*> toolbar_widgets1;
    QListWidget* listwidget;
    QToolButton* sync_button;
    QList<QToolButton*> toolbar_widgets2;

    PathManager(QWidget* parent=nullptr, QStringList pathlist=QStringList(), QStringList ro_pathlist=QStringList(),
                QStringList not_active_pathlist=QStringList(), bool sync=true);
    QStringList active_pathlist() const;
    void _add_widgets_to_layout(QBoxLayout* layout, const QList<QToolButton*>& widgets);
    QList<QToolButton*> setup_top_toolbar(QBoxLayout* layout);
    QList<QToolButton*> setup_bottom_toolbar(QBoxLayout* layout, bool sync=true);
    QStringList get_path_list() const;
    void add_to_not_active_pathlist(const QString& path);
    void remove_from_not_active_pathlist(const QString& path);
    void update_list();
    void move_to(int absolute=-1, int relative=-1);
public slots:
    void synchronize();
    void update_not_active_pathlist(QListWidgetItem *item);
    void refresh(int row = -1);
    void remove_path();
    void add_path();
};
