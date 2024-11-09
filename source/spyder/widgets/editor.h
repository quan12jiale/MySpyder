#pragma once

#include "config/base.h"
#include "config/gui.h"
#include "config/utils.h"
#include "utils/encoding.h"
#include "utils/sourcecode.h"
#include "widgets/findreplace.h"
#include "widgets/tabs.h"
#include "widgets/status.h"
#include "widgets/sourcecode/codeeditor.h"
#include "widgets/explorer.h"

class Editor;
class FileInfo;
class EditorStack;
class EditorPluginExample;

// 实际是QList<QPair<QString, int>>
typedef std::function<QList<QList<QVariant>>(const QString&)> FUNC_CHECKER;
typedef std::function<void(const QList<QList<QVariant>>&)> FUNC_END_CALLBACK;


class AnalysisThread : public QThread
{
    Q_OBJECT
public:
    FUNC_CHECKER checker;
    QList<QList<QVariant>> results;
    QString source_code;

    AnalysisThread(QObject* parent, FUNC_CHECKER checker,
                   const QString& source_code);
protected:
    void run();
};


class ThreadManager : public QObject
{
    Q_OBJECT
public:
    int max_simultaneous_threads;
    QHash<FileInfo*, QList<AnalysisThread*>> started_threads;
    QList<QPair<AnalysisThread*, FileInfo*>> pending_threads;
    QHash<AnalysisThread*, FUNC_END_CALLBACK> end_callbacks;

    ThreadManager(QObject* parent, int max_simultaneous_threads=2);
    void close_threads(FileInfo* parent);
    void close_all_threads();
    void add_thread(FUNC_CHECKER checker, FUNC_END_CALLBACK end_callback,
                    const QString& source_code, FileInfo* parent);
public slots:
    void update_queue();
};


class FileInfo : public QObject
{
    Q_OBJECT
signals:
    void analysis_results_changed();
    void todo_results_changed();
    void save_breakpoints(QString, QString);
    void text_changed_at(QString, int);
    void edit_goto(QString, int, QString);
    void send_to_help(QString, QString, QString, QString, bool);

public:
    ThreadManager* threadmanager;
    QString filename;
    bool newly_created;
    bool _default;
    QString encoding;
    CodeEditor* editor;
    QStringList path;

    // classes没有用到
    // 实际是QList<QPair<QString, int>>
    QList<QList<QVariant>> analysis_results;
    QList<QList<QVariant>> todo_results; // message, line_number
    QDateTime lastmodified;
    QList<QList<QVariant>> pyflakes_results;
    QList<QList<QVariant>> pep8_results;
public:
    // encoding暂定为QString
    FileInfo(const QString& filename, const QString& encoding, CodeEditor* editor,
             bool _new, ThreadManager* threadmanager);
    QString get_source_code() const;
    void run_code_analysis(bool run_pyflakes, bool run_pep8);
    void pyflakes_analysis_finished(const QList<QList<QVariant>>& results);
    void pep8_analysis_finished(const QList<QList<QVariant>>& results);
    void code_analysis_finished();
    void set_analysis_results(const QList<QList<QVariant>>& results);
    void cleanup_analysis_results();
    void run_todo_finder();
    void todo_finished(const QList<QList<QVariant>>& results);
    void set_todo_results(const QList<QList<QVariant>>& results);
    void cleanup_todo_results();
public slots:
    void text_changed();
    void breakpoints_changed();
};


class StackHistory
{
public:
    QList<size_t> history;
    QList<size_t> id_list;
    EditorStack* editor;

    StackHistory(EditorStack* editor);
    void _update_id_list();
    void refresh();
    int len();
    int getitem(int i);
    bool contains(int value);
    void setitem(int i, int v);
    //delitem
    void reverse();
    void insert(int i, int tab_index);
    void append(int tab_index);
    void remove(int tab_index);
};


class TabSwitcherWidget : public QListWidget
{
    Q_OBJECT
public:
    StackHistory stack_history;
    EditorStack* editor;
    BaseTabs* tabs;
    //id_list 没用到

    TabSwitcherWidget(QWidget* parent, StackHistory stack_history, BaseTabs* tabs);
    void load_data();
    void select_row(int steps);
    void set_dialog_position();
public slots:
    void item_selected(QListWidgetItem* item = nullptr);

protected:
    void keyReleaseEvent(QKeyEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
};


class EditorStack : public QWidget, public Widget_get_shortcut_data
{
    Q_OBJECT
signals:
    void reset_statusbar();
    void readonly_changed(bool);
    void encoding_changed(const QString&);
    void sig_editor_cursor_position_changed(int,int);
    void sig_refresh_eol_chars(const QString&);
    void starting_long_process(const QString&);
    void ending_long_process(const QString&);
    void redirect_stdio(bool);
    void exec_in_extconsole(const QString&, bool);
    void update_plugin_title();
    void editor_focus_changed();
    void zoom_in();
    void zoom_out();
    void zoom_reset();
    void sig_close_file(const QString&, const QString&);
    void file_saved(QString, QString, QString);
    void file_renamed_in_data(QString, QString, QString);
    void create_new_window();
    void opened_files_list_changed();
    void analysis_results_changed();
    void todo_results_changed();
    void update_code_analysis_actions();
    void refresh_file_dependent_actions();
    void refresh_save_all_action();
    void save_breakpoints(const QString&, const QString&);
    void text_changed_at(const QString&, int);
    void current_file_changed(const QString&, int);
    void plugin_load();
    void plugin_load(QString);
    void edit_goto(const QString&, int, const QString&);
    void split_vertically();
    void split_horizontally();
    void sig_new_file();
    void sig_new_file(QString);
    void sig_save_as();
    void sig_prev_edit_pos();
    void sig_prev_cursor();
    void sig_next_cursor();

public:
    StackHistory stack_history;
    ThreadManager* threadmanager;

    QAction* newwindow_action;
    QAction* horsplit_action;
    QAction* versplit_action;
    QAction* close_action;

    QMenu* menu;
    //FileSwitcher* fileswitcher_dlg;
    BaseTabs* tabs;
    TabSwitcherWidget* tabs_switcher;

    FindReplace* find_widget;
    QList<FileInfo*> data;// TODO 存在内存泄露，只能用shared_ptr解决

    QList<QAction*> menu_actions;

    OutlineExplorerWidget* outlineexplorer;
    // help
    // unregister_callback未使用
    bool is_closable;
    QAction* new_action;
    QAction* open_action;
    QAction* save_action;
    QAction* revert_action;
    QString tempfile_path;
    QString title;
    bool pyflakes_enabled;//记得初始化为false
    bool pep8_enabled;
    bool todolist_enabled;
    bool realtime_analysis_enabled;
    bool is_analysis_done;
    bool linenumbers_enabled;
    bool blanks_enabled;
    bool edgeline_enabled;
    int edgeline_column;
    bool codecompletion_auto_enabled;
    bool codecompletion_case_enabled;
    bool codecompletion_enter_enabled;
    bool calltips_enabled;
    bool go_to_definition_enabled;
    bool close_parentheses_enabled;
    bool close_quotes_enabled;
    bool add_colons_enabled;
    bool auto_unindent_enabled;
    QString indent_chars;
    int tab_stop_width_spaces;
    bool help_enabled;
    QFont default_font;//暂定
    bool wrap_enabled;
    bool tabmode_enabled;
    bool intelligent_backspace_enabled;
    bool highlight_current_line_enabled;
    bool highlight_current_cell_enabled;
    bool occurrence_highlighting_enabled;
    int occurrence_highlighting_timeout;
    bool checkeolchars_enabled;
    bool always_remove_trailing_spaces;
    bool focus_to_editor;
    bool create_new_file_if_empty;

    QHash<QString,ColorBoolBool> color_scheme;
    //introspector
    bool __file_status_flag;
    QTimer* analysis_timer;

    QList<ShortCutStrStr> shortcuts;
    QStringList last_closed_files;

    QMessageBox* msgbox;
    QList<QPair<QString, QStringList>> edit_filetypes;
    QString edit_filters;
    bool save_dialog_on_tests;
public:
    EditorStack(QWidget* parent, const QList<QAction*>& actions);

public slots:
    void show_in_external_file_explorer(QStringList fnames = QStringList());
    QList<ShortCutStrStr> create_shortcuts();
    QList<ShortCutStrStr> get_shortcut_data();
    void setup_editorstack(QWidget* parent,QVBoxLayout* layout);
    void add_corner_widgets_to_tabbar(const QList<QPair<int, QToolButton*>>& widgets);

    CodeEditor* clone_editor_from(FileInfo* other_finfo,bool set_current);
    void clone_from(EditorStack* other);
    void open_fileswitcher_dlg();//
    void open_symbolfinder_dlg();//
    EditorStack* get_current_tab_manager();
    void go_to_line(int line = -1);
    void set_or_clear_breakpoint();
    void set_or_edit_conditional_breakpoint();
    void inspect_current_object();//

    void set_closable(bool state);
    void set_io_actions(QAction* new_action,QAction* open_action,
                        QAction* save_action,QAction* revert_action);
    void set_find_widget(FindReplace* find_widget);
    void set_outlineexplorer(OutlineExplorerWidget* outlineexplorer);
    void initialize_outlineexplorer();
    //void add_outlineexplorer_button(self, editor_plugin);
    //void set_help(self, help_plugin);

    void set_tempfile_path(const QString& path);
    void set_title(const QString& text);
    void __update_editor_margins(CodeEditor* editor);

    void __codeanalysis_settings_changed(FileInfo* current_finfo);
    void set_pyflakes_enabled(bool state, FileInfo* current_finfo=nullptr);
    void set_pep8_enabled(bool state, FileInfo* current_finfo=nullptr);

    bool has_markers() const;
    void set_todolist_enabled(bool state,FileInfo* current_finfo=nullptr);
    void set_realtime_analysis_enabled(bool state);
    void set_realtime_analysis_timeout(int timeout);
    void set_linenumbers_enabled(bool state,FileInfo* current_finfo=nullptr);
    void set_blanks_enabled(bool state);
    void set_edgeline_enabled(bool state);
    void set_edgeline_column(int column);

    void set_codecompletion_auto_enabled(bool state);
    void set_codecompletion_case_enabled(bool state);
    void set_codecompletion_enter_enabled(bool state);
    void set_calltips_enabled(bool state);
    void set_go_to_definition_enabled(bool state);
    void set_close_parentheses_enabled(bool state);
    void set_close_quotes_enabled(bool state);
    void set_add_colons_enabled(bool state);
    void set_auto_unindent_enabled(bool state);

    void set_indent_chars(QString indent_chars);
    void set_tab_stop_width_spaces(int tab_stop_width_spaces);
    void set_help_enabled(bool state);
    void set_default_font(const QFont& font,const QHash<QString,ColorBoolBool>& color_scheme=QHash<QString,ColorBoolBool>());
    void set_color_scheme(const QHash<QString,ColorBoolBool>& color_scheme);
    void set_wrap_enabled(bool state);
    void set_tabmode_enabled(bool state);

    void set_intelligent_backspace_enabled(bool state);
    void set_occurrence_highlighting_enabled(bool state);
    void set_occurrence_highlighting_timeout(int timeout);
    void set_highlight_current_line_enabled(bool state);
    void set_highlight_current_cell_enabled(bool state);
    void set_checkeolchars_enabled(bool state);
    void set_always_remove_trailing_spaces(bool state);
    void set_focus_to_editor(bool state);
    //set_introspector(self, introspector)

    int get_stack_index() const;
    FileInfo* get_current_finfo() const;
    CodeEditor* get_current_editor() const;
    int get_stack_count() const;
    void set_stack_index(int index,EditorStack* instance=nullptr);
    void set_tabbar_visible(bool state);
    void remove_from_data(int index);

    QString __modified_readonly_title(QString title,bool is_modified,bool is_readonly);
    QString get_tab_text(int index,bool is_modified=false,bool is_readonly=false);
    QString get_tab_tip(const QString& filename,bool is_modified=false,bool is_readonly=false);
    void add_to_data(FileInfo* finfo, bool set_current);
    //在add_to_data()函数中向主体控件tabs添加CodeEditor
    void __repopulate_stack();
    int rename_in_data(const QString& original_filename,const QString& new_filename);
    void set_stack_title(int index, bool is_modified);

    void __setup_menu();
    QList<QAction*> __get_split_actions();
    void reset_orientation();
    void set_orientation(Qt::Orientation orientation);
    void update_actions();

    QString get_current_filename() const;
    int has_filename(const QString& filename) const;
    CodeEditor* set_current_filename(const QString& filename);
    bool is_file_opened() const;
    int is_file_opened(const QString& filename) const;
    int get_index_from_filename(const QString& filename) const;

    void move_editorstack_data(int start,int end);
    bool close_file(int index=-1, bool force=false);
    void close_all_files();
    void close_all_right();
    void close_all_but_this();
    void add_last_closed_file(const QString& fname);
    QStringList get_last_closed_files() const;
    void set_last_closed_files(const QStringList& fnames);

    bool save_if_changed(bool cancelable=false,int index=-1);
    bool save(int index=-1, bool force=false);
    void file_saved_in_other_editorstack(const QString& original_filename,const QString& filename);
    QString select_savename(const QString& original_filename);
    bool save_as(int index = -1);
    bool save_copy_as(int index = -1);
    void save_all();

    void start_stop_analysis_timer();
    void analyze_script(int index = -1);
    void set_analysis_results(const QString& filename, const QList<QList<QVariant>>& analysis_results);
    QList<QList<QVariant>> get_analysis_results();
    void set_todo_results(const QString& filename, const QList<QList<QVariant>>& todo_results);
    QList<QList<QVariant>> get_todo_results();
    void current_changed(int index);
    int _get_previous_file_index();

    void tab_navigation_mru(bool forward = true);
    void focus_changed();
    void _refresh_outlineexplorer(int index=-1,bool update=true,bool clear=false);
    void __refresh_statusbar(int index);
    void __refresh_readonly(int index);
    void __check_file_status(int index);
    void __modify_stack_title();
    void refresh(int index = -1);
    void modification_changed(int state=-1,int index=-2,size_t editor_id=0);
    void refresh_eol_chars(const QString& eol_chars);

    void reload(int index);
    void revert();
    FileInfo* create_new_editor(const QString& fname,const QString& enc,const QString& txt,
                                bool set_current,bool _new=false,CodeEditor* cloned_from=nullptr);
    void editor_cursor_position_changed(int line,int index);
    void send_to_help(QString qstr1,QString qstr2=QString(),QString qstr3=QString(),
                      QString qstr4=QString(),bool force=false);
    FileInfo* _new(const QString& filename,const QString& encoding,const QString& text,
                   bool default_content=false,bool empty=false);
    FileInfo* load(QString filename,bool set_current=true);
    void set_os_eol_chars(int index = -1);
    void remove_trailing_spaces(int index = -1);
    void fix_indentation(int index = -1);

    void run_selection(); // 运行代码的槽函数
    void run_cell();
    void run_cell_and_advance();
    void advance_cell(bool reverse=false);
    void re_run_last_cell();
protected:
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
};


class EditorSplitter : public QSplitter
{
    Q_OBJECT
public:
    QList<StrStrActions> toolbar_list;
    QList<QPair<QString, QList<QObject*>>> menu_list;

    Editor* plugin;

    std::function<void(EditorStack*)> register_editorstack_cb;
    std::function<bool(EditorStack*)> unregister_editorstack_cb;

    QList<QAction*> menu_actions;
    EditorStack* editorstack;

public:
    EditorSplitter(QWidget* parent,Editor* plugin,
                   const QList<QAction*>& menu_actions,bool first=false,
                   std::function<void(EditorStack*)> register_editorstack_cb=nullptr,
                   std::function<bool(EditorStack*)> unregister_editorstack_cb=nullptr);
    void __give_focus_to_remaining_editor();
    void split(Qt::Orientation orientation=Qt::Vertical);
    QList<QPair<EditorStack*, Qt::Orientation>> iter_editorstacks();
    SplitSettings get_layout_settings();
    void set_layout_settings(const SplitSettings& settings);

public slots:
    void editorstack_closed();
    void editorsplitter_closed();
};


class EditorWidget : public QSplitter
{
    Q_OBJECT
public:
    ReadWriteStatus* readwrite_status;
    EOLStatus* eol_status;
    EncodingStatus* encoding_status;
    CursorPositionStatus* cursorpos_status;
    QList<EditorStack*> editorstacks;
    Editor* plugin;
    FindReplace* find_widget;
    OutlineExplorerWidget* outlineexplorer;
    EditorSplitter* editorsplitter;

    EditorWidget(QMainWindow* parent,Editor* plugin,
                 const QList<QAction*>& menu_actions,bool show_fullpath,
                 bool show_all_files,bool show_comments);
    void register_editorstack(EditorStack* editorstack);
    void __print_editorstacks();
    bool unregister_editorstack(EditorStack* editorstack);
};


class EditorMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    QSize window_size;
    EditorWidget* editorwidget;
    QList<QToolBar*> toolbars;
    QList<QMenu*> menus;

    EditorMainWindow(Editor* plugin,
                     const QList<QAction*>& menu_actions,
                     QList<StrStrActions> toolbar_list,
                     QList<QPair<QString, QList<QObject*>>> menu_list,
                     bool show_fullpath,
                     bool show_all_files,bool show_comments);
    QList<QToolBar*> get_toolbars() const;
    void add_toolbars_to_menu(const QString& menu_title, const QList<QToolBar*>& actions);
    void load_toolbars();
    LayoutSettings get_layout_settings();
    void set_layout_settings(const LayoutSettings& settings);
protected:
    void resizeEvent(QResizeEvent *event);
};


// 这个类只是暂时模拟一下plugins文件夹下Editor类
/*
class EditorPluginExample : public QSplitter, public Register_Editorstack_cb
{
    Q_OBJECT
public:
    QList<EditorStack*> editorstacks;
    QList<EditorMainWindow*> editorwindows;
    QHash<QWidget*, EditorStack*> last_focus_editorstack;
    FindReplace* find_widget;
    OutlineExplorerWidget* outlineexplorer;
    EditorSplitter* editor_splitter;
    QList<QAction*> menu_actions;
    QList<StrStrActions> toolbar_list;
    QList<QPair<QString, QList<QObject*>>> menu_list;

    EditorPluginExample();

public slots:
    void go_to_file(const QString& fname,int lineno,const QString& text);
    void load(const QString& fname);
    void load(QString filenames, int _goto, QString word, QWidget* editorwindow){}
    void register_editorstack(EditorStack* editorstack);
    bool unregister_editorstack(EditorStack* editorstack);
    void clone_editorstack(EditorStack* editorstack);

    void setup_window(QList<StrStrActions> toolbar_list,
                      QList<QPair<QString, QList<QObject*>>> menu_list);
    void create_new_window();
    void register_editorwindow(EditorMainWindow* window);
    void unregister_editorwindow(EditorMainWindow* window);

    QWidget* get_focus_widget();
    void close_file_in_all_editorstacks(QString editorstack_id_str,QString filename);
    void file_saved_in_editorstack(QString editorstack_id_str,
                                   QString original_filename,QString filename);
    void file_renamed_in_data_in_editorstack(QString editorstack_id_str,
                                             QString original_filename,QString filename);
    virtual void register_widget_shortcuts(QWidget* widget);
protected:
    void closeEvent(QCloseEvent *event);
};
*/
