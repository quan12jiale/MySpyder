#pragma once

#include "config/base.h"
#include "config/utils.h"
#include "utils/icon_manager.h"
#include "utils/misc.h"
#include "widgets/findreplace.h"
#include "widgets/editor.h"
#include "widgets/status.h"
#include "plugins/plugins.h"
#include "plugins/configdialog.h"
#include "plugins/outlineexplorer.h"
#include "plugins/runconfig.h"
#include <QPrintDialog>
#include <QPrintPreviewDialog>

class Projects;

class Editor : public SpyderPluginWidget //该类位于plugins.h
{
    Q_OBJECT
signals:
    void run_in_current_ipyclient(QString, QString, QString, bool, bool, bool, bool);
    void exec_in_extconsole(const QString&, bool);
    void redirect_stdio(bool);
    void open_dir(QString);
    void breakpoints_saved();
    void run_in_current_extconsole(QString, QString, QString, bool, bool);
    void open_file_update(const QString&);
public:
    QString TEMPFILE_PATH;
    QString TEMPLATE_PATH;

public:
    bool __set_eol_chars;

    Projects* projects;
    OutlineExplorer* outlineexplorer;
    //help

    QList<QAction*> file_dependent_actions;
    QList<QAction*> pythonfile_dependent_actions;
    QList<QAction*> dock_toolbar_actions;
    QList<QObject*> edit_menu_actions;
    QList<QAction*> stack_menu_actions;

    bool __first_open_files_setup;
    QList<EditorStack*> editorstacks;
    QHash<QWidget*, EditorStack*> last_focus_editorstack;//1214,1217行说明键可能是this
    QList<EditorMainWindow*> editorwindows;
    QList<LayoutSettings> editorwindows_to_be_created;
    QList<StrStrActions> toolbar_list;
    QList<QPair<QString, QList<QObject*>>> menu_list;

    QSize dialog_size;

    ReadWriteStatus* readwrite_status;
    EOLStatus* eol_status;
    EncodingStatus* encoding_status;
    CursorPositionStatus* cursorpos_status;

    QToolBar* dock_toolbar;

    QPair<QString, int> last_edit_cursor_pos;
    QList<QPair<QString, int>> cursor_pos_history;
    int cursor_pos_index;
    bool __ignore_cursor_position;

    //introspector

    FindReplace* find_widget;

    EditorSplitter* editorsplitter;

    QSplitter* splitter;

    QStringList recent_files;
    int untitled_num;

    //__last_ic_exec
    Last_Ec_Exec __last_ec_exec;

    QList<QPair<QString, QStringList>> edit_filetypes;
    QString edit_filters;


    QAction* new_action;
    QAction* open_last_closed_action;
    QAction* open_action;
    QAction* revert_action;
    QAction* save_action;
    QAction* save_all_action;

    QAction* print_action;
    QAction* close_action;
    QAction* close_all_action;

    QAction* winpdb_action;

    QAction* todo_list_action;
    QMenu* todo_menu;

    QAction* warning_list_action;
    QMenu* warning_menu;

    QAction* previous_warning_action;
    QAction* next_warning_action;
    QAction* previous_edit_cursor_action;
    QAction* previous_cursor_action;
    QAction* next_cursor_action;

    QAction* toggle_comment_action;
    QAction* indent_action;
    QAction* unindent_action;
    QAction* text_uppercase_action;
    QAction* text_lowercase_action;

    QAction* win_eol_action;
    QAction* linux_eol_action;
    QAction* mac_eol_action;
    QAction* showblanks_action;
    QAction* max_recent_action;
    QAction* clear_recent_action;

    QMenu* recent_file_menu;

    QList<QAction*> cythonfile_compatible_actions;


public:
    Editor(MainWindow* parent, bool ignore_last_opened_files=false);
    void set_projects(Projects* projects);
    void show_hide_projects();
    void set_outlineexplorer(OutlineExplorer* outlineexplorer);
    //set_help


    QString get_plugin_title() const;
    QIcon get_plugin_icon() const;
    QWidget* get_focus_widget() const;
    void visibility_changed(bool enable);
    void refresh_plugin();
    bool closing_plugin(bool cancelable=false);
    QList<QAction*> get_plugin_actions();
    void register_plugin();
    void update_font();

    EditorStack* __get_focus_editorstack() const;
    void set_last_focus_editorstack(QWidget* editorwindow, EditorStack* editorstack);
    EditorStack* get_last_focus_editorstack(EditorMainWindow* editorwindow=nullptr) const;
    void remove_last_focus_editorstack(EditorStack* editorstack);


    void register_editorstack(EditorStack* editorstack);
    bool unregister_editorstack(EditorStack* editorstack);
    void clone_editorstack(EditorStack* editorstack);

    void register_editorwindow(EditorMainWindow* window);
    void unregister_editorwindow(EditorMainWindow* window);

    QStringList get_filenames() const;
    int get_filename_index(const QString& filename) const;
    EditorStack* get_current_editorstack(EditorMainWindow* editorwindow=nullptr) const;
    CodeEditor* get_current_editor() const;
    FileInfo* get_current_finfo() const;
    QString get_current_filename() const;
    bool is_file_opened() const;
    int is_file_opened(const QString& filename) const;
    CodeEditor* set_current_filename(const QString& filename, EditorMainWindow* editorwindow=nullptr);

    void __add_recent_file(const QString& fname);
    void _clone_file_everywhere(FileInfo* finfo);

    void _new(QString fname=QString(), EditorStack* editorstack=nullptr, QString text=QString());
    void edit_template();

    void load(QString filename, int _goto=-1, QString word="",
              QWidget* editorwin=nullptr, bool processevents=true);
    void load(QStringList filenames, int _goto=-1, QString word="",
              QWidget* editorwin=nullptr, bool processevents=true);



    void close_file_from_name(QString filename);




    void toggle_eol_chars(const QString& os_name, bool checked);



    void update_cursorpos_actions();
    void add_cursor_position_to_history(const QString& filename, int position, bool fc=false);
    void cursor_moved(const QString& filename0, int position0, const QString& filename1, int position1);

    void clear_breakpoint(const QString& filename, int lineno);
    void debug_command(const QString& command);



    void zoom(int factor);
    void apply_plugin_settings(const QStringList& options);
    QStringList get_open_filenames() const;
    void set_open_filenames();
    void setup_open_files();
    void save_open_files();
    void set_create_new_file_if_empty(bool value);
public slots:
    void restore_scrollbar_position();//
    void save_focus_editorstack();//

    void removed(const QString& filename);
    void removed_tree(QString dirname);
    void renamed(const QString& source, const QString& dest);
    void renamed_tree(const QString& source, const QString& dest);

    void close_file_in_all_editorstacks(const QString& editorstack_id_str,const QString& filename);//
    void file_saved_in_editorstack(const QString& editorstack_id_str,
                                   const QString& original_filename, const QString& filename);//
    void file_renamed_in_data_in_editorstack(const QString& editorstack_id_str,
                                             const QString& original_filename, const QString& filename);//
    void set_editorstack_for_introspection();//

    void setup_other_windows();//
    EditorMainWindow* create_new_window();//

    void set_path();//
    EditorStack* get_current_tab_manager();

    void refresh_file_dependent_actions();//
    void refresh_save_all_action();//
    void update_warning_menu();
    void analysis_results_changed();//
    void update_todo_menu();
    void todo_results_changed();//
    void refresh_eol_chars(const QString& os_name);//

    void opened_files_list_changed();//
    void update_code_analysis_actions();
    void update_todo_actions();
    void rehighlight_cells();//

    // todo
    void save_breakpoints(QString filename, const QString& breakpoint_str);//
    void __load_temp_file();
    void __set_workdir();//

    void update_recent_file_menu();//
    void clear_recent_files();//
    void change_max_recent_files();//

    void print_file();//
    void print_preview();//
    void close_file();//
    void close_all_files();//
    bool save(int index=-1, bool force=false);
    void save_as();//
    void save_copy_as();//
    void save_all();//
    void revert();//

    void find();//
    void find_next();//
    void find_previous();//
    void replace();//
    void open_last_closed();//

    void indent();//
    void unindent();
    void text_uppercase();
    void text_lowercase();
    void toggle_comment();
    void blockcomment();
    void unblockcomment();//

    void go_to_next_todo();//
    void go_to_next_warning();//
    void go_to_previous_warning();//
    void run_winpdb();//

    void remove_trailing_spaces();//
    void fix_indentation();//

    void text_changed_at(const QString& filename, int position);//
    void current_file_changed(const QString& filename, int position);//
    void go_to_last_edit_location();//
    void __move_cursor_position(int index_move);
    void go_to_previous_cursor_position();//
    void go_to_next_cursor_position();//
    void go_to_line(int line = -1);

    void set_or_clear_breakpoint();//
    void set_or_edit_conditional_breakpoint();//
    void clear_all_breakpoints();//

    void edit_run_configurations();//
    void run_file(bool debug=false);
    void set_dialog_size(const QSize& size);
    void debug_file();//
    void re_run_file();//
    void run_selection();//
    void run_cell();//
    void run_cell_and_advance();//
    void re_run_last_cell();//

    void toggle_show_blanks(bool checked);
    void load();

    void switch_to_plugin(){SpyderPluginMixin::switch_to_plugin();}
    virtual void starting_long_process(const QString& message){SpyderPluginMixin::starting_long_process(message);}
    virtual void ending_long_process(const QString& message=""){SpyderPluginMixin::ending_long_process(message);}
};

