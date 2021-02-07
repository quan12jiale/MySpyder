#pragma once

#include "utils/misc.h"
#include "widgets/explorer.h"


class ProjectsExplorerTreeWidget : public FilteredDirView
{
    Q_OBJECT
signals:
    void sig_delete_project();
public:
    QString last_folder;//没用到
    bool show_hscrollbar;

    ProjectsExplorerTreeWidget(QWidget* parent, bool show_hscrollbar=true);
    QList<QObject*> setup_common_actions();
protected:
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

public slots:
    void toggle_hscrollbar(bool checked);
    void _delete(QStringList fnames = QStringList());
};


class ProjectExplorerWidget : public QWidget
{
    Q_OBJECT
signals:
    void redirect_stdio(bool);
    void sig_option_changed(const QString&, const QVariant&);
    void sig_open_file(const QString&);
public:
    ProjectsExplorerTreeWidget* treewidget;
    ProjectsExplorerTreeWidget* emptywidget;
    QStringList name_filters;
    bool show_all;
    bool show_hscrollbar;
    //current_active_project

    ProjectExplorerWidget(QWidget* parent, const QStringList& name_filters=QStringList(),
                          bool show_all=true, bool show_hscrollbar=true);
    void setup_layout();
    void closing_widget(){}
    void set_project_dir(const QString& directory);
    void clear();
    void setup_project(const QString& directory);

public slots:
    void delete_project();
};


class ProjectExplorerTest : public QWidget
{
    Q_OBJECT
public:
    ProjectExplorerWidget* explorer;
    QString directory;
    ProjectExplorerTest(const QString& directory=QString());
};
