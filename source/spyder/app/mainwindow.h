#pragma once

#include <QtWidgets>
#include "config/utils.h"
#include "plugins/history.h"
#include "plugins/projects.h"
#include "plugins/outlineexplorer.h"
#include "plugins/plugins_editor.h"
#include "plugins/plugins_explorer.h"
#include "plugins/plugins_findinfiles.h"
#include "plugins/workingdirectory.h"
#include "widgets/status.h"
#include "widgets/reporterror.h"
#include "widgets/pathmanager.h"
#include "widgets/ipythonconsole/control.h"

#include "plugins/maininterpreter.h"
#include "app/tour.h"

void set_opengl_implementation(const QString& option);
void initialize();

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    static const QMainWindow::DockOptions DOCKOPTIONS;
    static const int CURSORBLINK_OSDEFAULT;
    QString SPYDER_PATH;
    QString SPYDER_NOT_ACTIVE_PATH;
    static const int DEFAULT_LAYOUTS;
signals:
    void restore_scrollbar_position();
    void all_actions_defined();
    void sig_pythonpath_changed();
    void sig_open_external_file(const QString&);
    void sig_resized(QResizeEvent*);
    void sig_moved(QMoveEvent*);
public:
    QString default_style;

    DialogManager* dialog_manager;

    QString init_workdir;
    bool profile;
    bool multithreaded;
    bool new_instance;
    QString open_project;
    QString window_title;

    QList<ShortCutStrStrBool> shortcut_data;

    QStringList path;
    QStringList not_active_path;
    QStringList project_path;

    WorkingDirectory* workingdirectory;
    Editor* editor;

    Projects* projects;
    OutlineExplorer* outlineexplorer;
    Explorer* explorer;
    HistoryLog* historylog;
    FindInFiles* findinfiles;
    QList<SpyderPluginMixin*> thirdparty_plugins;

    QAction* check_updates_action;
    // thread_updates
    // worker_updates
    bool give_updates_feedback;

    int prefs_index;
    QSize prefs_dialog_size;

    // Actions
    QAction* lock_dockwidgets_action;
    QAction* show_toolbars_action;
    QAction* close_dockwidget_action;
    QAction* undo_action;
    QAction* redo_action;
    QAction* copy_action;
    QAction* cut_action;
    QAction* paste_action;
    QAction* selectall_action;
    QAction* maximize_action;
    QAction* fullscreen_action;

    // Menu bars
    QMenu* file_menu;
    QList<QObject*> file_menu_actions;

    QMenu* edit_menu;
    QList<QObject*> edit_menu_actions;

    QMenu* search_menu;
    QList<QObject*> search_menu_actions;

    QMenu* source_menu;
    QList<QObject*> source_menu_actions;

    QMenu* run_menu;
    QList<QObject*> run_menu_actions;

    QMenu* debug_menu;
    QList<QObject*> debug_menu_actions;

    QMenu* consoles_menu ;
    QList<QObject*> consoles_menu_actions;

    QMenu* projects_menu;
    QList<QObject*> projects_menu_actions;

    QMenu* tools_menu;
    QList<QObject*> tools_menu_actions;

    QMenu* external_tools_menu;
    QList<QObject*> external_tools_menu_actions;

    QMenu* view_menu;

    QMenu* plugins_menu;
    QList<QAction*> plugins_menu_actions;

    QMenu* toolbars_menu;

    QMenu* help_menu;
    QList<QObject*> help_menu_actions;

    // Status bar widgets
    MemoryStatus* mem_status;

    // Toolbars
    QList<QToolBar*> visible_toolbars;
    QList<QToolBar*> toolbarslist;

    QToolBar* main_toolbar;
    QList<QAction*> main_toolbar_actions;

    QToolBar* file_toolbar;
    QList<QAction*> file_toolbar_actions;

    QToolBar* edit_toolbar;
    QList<QAction*> edit_toolbar_actions;

    QToolBar* search_toolbar;
    QList<QAction*> search_toolbar_actions;

    QToolBar* source_toolbar;
    QList<QAction*> source_toolbar_actions;

    QToolBar* run_toolbar;
    QList<QAction*> run_toolbar_actions;

    QToolBar* debug_toolbar;
    QList<QAction*> debug_toolbar_actions;

    QToolBar* layout_toolbar;
    QList<QAction*> layout_toolbar_actions;

    QTimer* timer_shutdown;//474行

    QSplashScreen* splash;

    QList<SpyderPluginMixin*> widgetlist;//487行

    bool already_closed;
    bool is_starting_up;
    bool is_setting_up;

    bool dockwidgets_locked;
    QList<QDockWidget*> floating_dockwidgets;
    QSize window_size;
    QPoint window_position;
    QByteArray state_before_maximizing;
    int current_quick_layout;//default=4
    // previous_layout_settings项目中没有用到
    SpyderPluginMixin* last_plugin;//501行
    bool fullscreen_flag;

    bool maximized_flag;

    QRect saved_normal_geometry;

    QWidget* last_focused_widget;
    QWidget* previous_focused_widget;

    QAction* toggle_next_layout_action;//568行
    QAction* toggle_previous_layout_action;
    QAction* file_switcher_action;
    QAction* symbol_finder_action;

    QAction* wp_action;//723行

	tour::AnimatedTour* tour;
    QMenu* tours_menu;//959行
    QList<QAction*> tour_menu_actions;
	QList<QMap<QString, QVariant >> tours_available;

    QMenu* quick_layout_menu;

    bool toolbars_visible;//1223行

    QString base_title;//1291行

    bool first_spyder_run;//1428行

    QList<QHash<QString, size_t>> _layout_widget_info;
    QTimer* _custom_layout_timer;//1687行

    QAction* ql_save;//1774行
    QAction* ql_preferences;
    QAction* ql_reset;

    MainWindow(const QHash<QString,QVariant>& options =QHash<QString,QVariant>());
    QToolBar* create_toolbar(const QString& title, const QString& object_name,
                             int iconsize=24);
    void setup();
    void post_visible_setup();
    void set_window_title();
    void report_missing_dependencies();

    WindowSettings load_window_settings(const QString& prefix, bool _default=false,
                                        const QString& section="main");
    WindowSettings get_window_settings();
    void set_window_settings(QString hexstate, QSize window_size, QSize prefs_dialog_size,
                             QPoint pos, bool is_maximized, bool is_fullscreen);
    void save_current_window_settings(const QString& prefix, const QString& section="main",
                                      bool none_state=false);
    void tabify_plugins(SpyderPluginMixin* first, SpyderPluginMixin* second);

    void setup_layout(bool _default=false);
    void setup_default_layouts(int index, const WindowSettings& settings);//default=4
    void toggle_layout(const QString& direction="next");
    void quick_layout_set_menu();

    void quick_layout_switch(int index);

    // --- Show/Hide toolbars
    void _update_show_toolbars_action();
    void save_visible_toolbars();
    void get_visible_toolbars();
    void load_last_visible_toolbars();

    void free_memory();
    void show_shortcuts(const QString& menu);
    void hide_shortcuts(const QString& menu);
    FocusWidgetProperties get_focus_widget_properties();

    void create_plugins_menu();
    void create_toolbars_menu();
    QMenu* createPopupMenu();
    void set_splash(const QString& message);
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void moveEvent(QMoveEvent *event);
    void hideEvent(QHideEvent *event);
    void add_dockwidget(SpyderPluginMixin* child);

    void add_to_toolbar();
    QString render_issue(QString description="", const QString& traceback="");

    void open_file(QString fname, bool external=false);
    QStringList get_spyder_pythonpath() const;
    void add_path_to_sys_path();
    void remove_path_from_sys_path();


    void open_external_console(QString fname, QString wdir, QString args, bool interact, bool debug,
                               bool python, QString python_args, bool systerm, bool post_mortem=false);

    void apply_settings();
    void apply_panes_settings();
    void apply_statusbar_settings();

    void register_shortcut(QObject* qaction_or_qshortcut, const QString& context,
                           const QString& name, bool add_sc_to_tip=false);
    void apply_shortcuts();
    void start_open_files_server();

    void open_fileswitcher(bool symbol=false);
    void add_to_fileswitcher(Editor* plugin, BaseTabs* tabs, QList<FileInfo*> data, QIcon icon);
    void  _check_updates_ready();
    bool _test_setting_opengl(const QString& option);

    void __update_maximize_action();
    void __update_fullscreen_action();
    void report_issue(const QString& body=QString(), const QString& title=QString(),
                      bool open_webpage=false);
public slots:
	void show_tour();// int index
    void toggle_previous_layout();
    void toggle_next_layout();
    void close_current_dockwidget();
    void toggle_lock_dockwidgets(bool value);
    void open_symbolfinder();

    void show_dependencies();
    void trouble_guide();
    void google_group();
    void global_callback();

    void update_edit_menu();
    void update_search_menu();
    void reset_window_layout();
    void quick_layout_save();
    void quick_layout_settings();

    void show_toolbars();
    void valid_project();
    void edit_preferences();
    void path_manager_callback();
    void win_env();
    void maximize_dockwidget(bool restore=false);
    void toggle_fullscreen();

    void change_last_focused_widget(QWidget *old, QWidget *now);
    bool closing(bool cancelable=false);

    void about();
    void pythonpath_changed();

    void show_shortcuts_dialog();
    void check_updates(bool startup=false);

    void open_external_file(const QString& fname);

    void reset_spyder();
    void restart(bool reset=false);

    void layout_fix_timer();
    void redirect_internalshell_stdio(bool state);

    void __preference_page_changed(int index);
    void set_prefs_size(const QSize& size);

    void plugin_focus_changed();
    void execute_in_external_console(const QString& lines, bool focus_to_editor);
};
