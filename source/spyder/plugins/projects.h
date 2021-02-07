#ifndef PROJECTS_H
#define PROJECTS_H

#include "plugins/plugins.h"
#include "utils/misc.h"
#include "widgets/projects/projects_explorer.h"
#include "widgets/projects/projectdialog.h"
#include "widgets/projects/type/projects_type.h"

class Projects : public ProjectExplorerWidget, public SpyderPluginMixin
{
    Q_OBJECT
signals:
    void open_interpreter(const QString&);
    void sig_pythonpath_changed();

    void create_module(const QString&);
    void edit(const QString&);
    void removed(const QString&);
    void removed_tree(const QString&);
    void renamed(const QString&, const QString&);
    void renamed_tree(const QString&, const QString&);
    void run(const QString&);

    void sig_project_created(QString, QString, QStringList);
    void sig_project_loaded(const QString&);
    void sig_project_closed(const QString&);
    void sig_new_file(const QString&);
public:
    QStringList recent_projects;
    EmptyProject* current_active_project;
    EmptyProject* latest_project;

    QAction* new_project_action;
    QAction* open_project_action;
    QAction* close_project_action;
    QAction* delete_project_action;
    QAction* clear_recent_projects_action;
    QAction* edit_project_preferences_action;
    QMenu* recent_project_menu;

    QList<QAction*> recent_projects_actions;

    Projects(MainWindow* parent = nullptr);

    //------ SpyderPluginWidget API ----------
    virtual void initialize_plugin();
    virtual QString get_plugin_title() const;
    virtual QWidget* get_focus_widget() const;
    virtual QList<QAction*> get_plugin_actions();
    virtual void register_plugin();
    bool closing_plugin(bool cancelable=false);

    virtual void update_margins();

    virtual QPair<SpyderDockWidget*,Qt::DockWidgetArea> create_dockwidget();
    virtual QMainWindow* create_mainwindow();
    void create_toggle_view_action();
    virtual void refresh_plugin(){}

    // ------ Public API --------

    void update_project_actions();


    void open_project(QString path=QString(), bool restart_consoles=true,
                      bool save_previous_files=true);

    EmptyProject* get_active_project() const;
    void reopen_last_project();

    QStringList get_project_filenames();
    void set_project_filenames(const QStringList& recent_files);
    QString get_active_project_path();
    QStringList get_pythonpath(bool at_start=false);
    QString get_last_working_dir();
    void save_config();
    void load_config();


    void show_explorer();
    void restart_consoles();
    bool is_valid_project(const QString& path) const;
    void add_to_recent(const QString& project);

public slots:
    void visibility_changed(bool enable){SpyderPluginMixin::visibility_changed(enable);}
    void plugin_closed(){SpyderPluginMixin::plugin_closed();}
    void set_option(const QString& option,const QVariant& value){SpyderPluginMixin::set_option(option, value);}
    virtual void switch_to_plugin();//

    void setup_menu_actions();//
    void edit_project_preferences();//

    void create_new_project();//
    void _create_project(const QString& path);
    void close_project();//
    void _delete_project();//
    void clear_recent_projects();//

    void restore_scrollbar_position();//
    void update_explorer();//
};

#endif // PROJECTS_H
