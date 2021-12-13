#pragma once

#include "str.h"
#include "config/gui.h"
#include "utils/encoding.h"
#include "utils/qthelpers.h"
#include "utils/syntaxhighlighters.h"
#include "widgets/editortools.h"
#include "widgets/sourcecode/widgets_base.h"
#include "widgets/sourcecode/kill_ring.h"
#include <QDebug>
#include <QPrinter>

class CodeEditor;
enum HighlighterEnumType;

class GoToLineDialog : public QDialog
{
    Q_OBJECT
public:
    int lineno;
    CodeEditor* editor;
    QLineEdit* lineedit;
public:
    GoToLineDialog(CodeEditor* editor);
    int get_line_number() const;

public slots:
    void text_has_changed(const QString& text);
};


class LineNumberArea : public QWidget
{
    Q_OBJECT
public:
    LineNumberArea(CodeEditor* editor);
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    CodeEditor* code_editor;
};


class ScrollFlagArea : public QWidget
{
    Q_OBJECT
public:
    static const int WIDTH;
    static const int FLAGS_DX;
    static const int FLAGS_DY;
    ScrollFlagArea(CodeEditor* editor);
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    double get_scale_factor(bool slider=false) const;
    double value_to_position(int y,bool slider=false) const;
    double position_to_value(int y,bool slider=false) const;
    QRect make_flag_qrect(int position) const;
    QRect make_slider_range(int value) const;
    void wheelEvent(QWheelEvent *event) override;
    CodeEditor* code_editor;
};


class EdgeLine : public QWidget
{
    Q_OBJECT
public:
    EdgeLine(QWidget* editor);
    void paintEvent(QPaintEvent* event) override;
    QWidget* code_editor;
    int column;
};

class BlockUserData : public QTextBlockUserData
{
public:
    BlockUserData(CodeEditor* editor);
	~BlockUserData();
    bool is_empty();

    CodeEditor* editor;
    bool breakpoint;
    QString breakpoint_condition;
    QList<QPair<QString,bool>> code_analysis;
    QString todo;
};

QString get_file_language(const QString& filename,QString text=QString());


struct IntIntTextblock {
    int top;
    int line_number;
    QTextBlock block;
    IntIntTextblock():top(-1), line_number(-1), block(QTextBlock()) {}
    IntIntTextblock(int _top,int _line_number, const QTextBlock& _block)
        : top(_top), line_number(_line_number), block(_block) {}
};

class CodeEditor : public TextEditBaseWidget, public Widget_get_shortcut_data
{
    Q_OBJECT
public:
	friend class Panel;
	friend class TextHelper;

    QHash<HighlighterEnumType,QString> LANGUAGES;
    QStringList TAB_ALWAYS_INDENTS;
signals:
    void painted(QPaintEvent*);
    void breakpoints_changed();
    void get_completions(bool);
    void go_to_definition(int);
    void sig_show_object_info(int);
    void run_selection();
    void run_cell_and_advance();
    void run_cell();
    void re_run_last_cell();
    void go_to_definition_regex(int);
    void sig_cursor_position_changed(int,int);
    void focus_changed();
    void sig_new_file(const QString&);

	void sig_key_pressed(QKeyEvent*);
	void new_text_set();

public:
    bool edge_line_enabled;
    EdgeLine* edge_line;
    bool blanks_enabled;
    bool markers_margin;
    int markers_margin_width;

    QPixmap error_pixmap;
    QPixmap warning_pixmap;
    QPixmap todo_pixmap;
    QPixmap bp_pixmap;
    QPixmap bpc_pixmap;

    bool linenumbers_margin;
    bool linenumberarea_enabled;
    LineNumberArea* linenumberarea;
    int linenumberarea_pressed;
    int linenumberarea_released;

    QColor occurrence_color;
    QColor ctrl_click_color;
    QColor sideareas_color;
    //QColor matched_p_color;
    //QColor unmatched_p_color;
    QColor normal_color;
    QColor comment_color;
    QColor linenumbers_color;

	HighlighterEnumType highlighter_class;
    sh::BaseSH* highlighter;
    QHash<QString,ColorBoolBool> color_scheme;
    bool highlight_current_line_enabled;

    bool scrollflagarea_enabled;
    ScrollFlagArea* scrollflagarea;
    QString warning_color;
    QString error_color;
    QString todo_color;
    QString breakpoint_color;

    size_t document_id;
    int __find_first_pos;
    int __find_flags;

    bool supported_language;
    bool supported_cell_language;
    // classfunc_match
    QString comment_string;
    QtKillRing* _kill_ring;

    QList<BlockUserData*> blockuserdata_list;

    QTimer* timer_syntax_highlight;
    bool occurrence_highlighting;
    QTimer* occurrence_timer;
    QList<int> occurrences;
    QList<int> found_results;
    QColor found_results_color;

    QAction* gotodef_action;
    bool tab_indents;
    bool tab_mode;
    bool intelligent_backspace;
    bool go_to_definition_enabled;
    bool close_parentheses_enabled;
    bool close_quotes_enabled;
    bool add_colons_enabled;
    bool auto_unindent_enabled;

    bool __cursor_changed;
    QList<QList<QVariant>> breakpoints;
    QList<ShortCutStrStr> shortcuts;

    QList<IntIntTextblock> __visible_blocks;

    QAction* undo_action;
    QAction* redo_action;
    QAction* cut_action;
    QAction* copy_action;
    QAction* paste_action;

    QAction* clear_all_output_action;
    QAction* ipynb_convert_action;
    //QAction* gotodef_action;

    QAction* run_cell_action;
    QAction* run_cell_and_advance_action;
    QAction* re_run_last_cell_action;
    QAction* run_selection_action;

    QMenu* menu;
    QMenu* readonly_menu;

public:
    CodeEditor(QWidget* parent=nullptr);
    QList<ShortCutStrStr> create_shortcuts();
    QList<ShortCutStrStr> get_shortcut_data() override;
    void closeEvent(QCloseEvent *event) override;
    size_t get_document_id();
    void set_as_clone(CodeEditor* editor);
    void toggle_wrap_mode(bool enable);
    void setup_editor(const QHash<QString, QVariant>& kwargs);
    void setup_editor(bool linenumbers=true,const QString& language=QString(),bool markers=false,
                      const QFont& font=QFont(),const QHash<QString,ColorBoolBool>& color_scheme=QHash<QString,ColorBoolBool>(),bool wrap=false,bool tab_mode=true,
                      bool intelligent_backspace=true,bool highlight_current_line=true,
                      bool highlight_current_cell=true,bool occurrence_highlighting=true,
                      bool scrollflagarea=true,bool edge_line=true,int edge_line_column=79,
                      bool codecompletion_auto=false,bool codecompletion_case=true,
                      bool codecompletion_enter=false,bool show_blanks=false,
                      bool calltips=false,bool go_to_definition=false,
                      bool close_parentheses=true,bool close_quotes=false,
                      bool add_colons=true,bool auto_unindent=true,const QString& indent_chars=QString(4,' '),
                      int tab_stop_width_spaces=4,CodeEditor* cloned_from=nullptr,const QString& filename=QString(),
                      int occurrence_timeout=1500);

    void set_tab_mode(bool enable);
    void toggle_intelligent_backspace(bool state);
    void set_go_to_definition_enabled(bool enable);
    void set_close_parentheses_enabled(bool enable);
    void set_close_quotes_enabled(bool enable);
    void set_add_colons_enabled(bool enable);
    void set_auto_unindent_enabled(bool enable);
    void set_occurrence_highlighting(bool enable);
    void set_occurrence_timeout(int timeout);
    void set_highlight_current_line(bool enable);
    void set_highlight_current_cell(bool enable);
    void set_language(const QString& language,const QString& filename=QString());
    void _set_highlighter(HighlighterEnumType sh_class);

    bool is_json();
    bool is_python();
    bool is_cpp();
    bool is_cython();
    bool is_enmal();
    bool is_python_like() override;
    void intelligent_tab();
    void intelligent_backtab();

    void rehighlight();
    void rehighlight_cells();
    void setup_margins(bool linenumbers=true,bool markers=true);
    void remove_trailing_spaces();
    void fix_indentation();
    QString get_current_object();

    QTextCursor __find_first(const QString& text);
    QTextCursor __find_next(const QString& text,QTextCursor cursor);

    void __clear_occurrences();
    void __highlight_selection(const QString& key,const QTextCursor& cursor,const QColor& foreground_color=QColor(),
                               const QColor& background_color=QColor(),const QColor&  underline_color=QColor(),
                               QTextCharFormat::UnderlineStyle underline_style=QTextCharFormat::SpellCheckUnderline,
                               bool update=false);


    void highlight_found_results(QString pattern,bool words=false,bool regexp=false);
    void clear_found_results();


    int get_markers_margin();

    void set_linenumberarea_enabled(bool state);
    int get_linenumberarea_width();
    int compute_linenumberarea_width();

    void linenumberarea_paint_event(QPaintEvent* event);
    //void draw_pixmap(int ytop,const QPixmap& pixmap,QPainter* painter,int font_height);
    int __get_linenumber_from_mouse_event(QMouseEvent* event);
    void linenumberarea_mousemove_event(QMouseEvent* event);
    void linenumberarea_mousedoubleclick_event(QMouseEvent* event);
    void linenumberarea_mousepress_event(QMouseEvent* event);
    void linenumberarea_mouserelease_event(QMouseEvent* event);
    void linenumberarea_select_lines(int linenumber_pressed,
                                     int linenumber_released);

    void add_remove_breakpoint(int line_number=-1,QString condition=QString(),
                               bool edit_condition=false);
    QList<QList<QVariant>> get_breakpoints();
    void clear_breakpoints();
    void set_breakpoints(const QList<QVariant>& breakpoints);


    void show_object_info(int position);

    void set_edge_line_enabled(bool state);
    void set_edge_line_column(int column);

    void set_blanks_enabled(bool state);

    void set_scrollflagarea_enabled(bool state);
    int get_scrollflagarea_width();

    void scrollflagarea_paint_event(QPaintEvent* event);
    void resizeEvent(QResizeEvent *event) override;
    void __set_scrollflagarea_geometry(const QRect& contentrect);

    bool viewportEvent(QEvent *event) override;

    void _apply_highlighter_color_scheme();
    void apply_highlighter_settings(const QHash<QString,ColorBoolBool>& color_scheme=QHash<QString,ColorBoolBool>());
    QHash<int,sh::OutlineExplorerData> get_outlineexplorer_data();
    void set_font(const QFont& font,const QHash<QString,ColorBoolBool>& color_scheme=QHash<QString,ColorBoolBool>());
    void set_color_scheme(const QHash<QString,ColorBoolBool>& color_scheme);
    void set_text(const QString& text);
    void set_text_from_file(const QString& filename,QString language=QString());
    void append(const QString& text);

    void get_block_data(const QTextBlock& block);
    void get_fold_level(int block_nb);

    void go_to_line(int line,const QString& word="");
    void exec_gotolinedialog();
    void cleanup_code_analysis();
    void process_code_analysis(const QList<QList<QVariant>>& check_results);
    //bool is_line_splitted(int line_no,QTextDocument* document);
    void __show_code_analysis_results(int line_number,const QList<QPair<QString,bool>>& code_analysis);
    int go_to_next_warning();
    int go_to_previous_warning();

    int go_to_next_todo();
    void process_todo(const QList<QList<QVariant>>& todo_results);

    void add_prefix(const QString& prefix);
    void __is_cursor_at_start_of_block(QTextCursor* cursor);
    void remove_suffix(const QString& suffix);
    void remove_prefix(const QString& prefix);

    bool fix_indent(bool forward=true,bool comment_or_string=false);
    bool simple_indentation(bool forward=true,bool comment_or_string=false);
    bool fix_indent_smart(bool forward=true,bool comment_or_string=false);

    void indent(bool force=false);
    void indent_or_replace();
    void unindent(bool force=false);

    void comment();
    void uncomment();
    QString __blockcomment_bar();

    QTextCursor _get_word_start_cursor(int position);
    QTextCursor _get_word_end_cursor(int position);


    QString __get_current_color(QTextCursor cursor=QTextCursor());
    bool in_comment_or_string(QTextCursor cursor=QTextCursor());
    bool __colon_keyword(QString text);
    bool __forbidden_colon_end_char(QString text);
    bool __unmatched_braces_in_line(const QString& text,const QChar& closing_braces_type=QChar(0));
    bool __has_colon_not_in_brackets(const QString& text);
    bool autoinsert_colons();
    QString __unmatched_quotes_in_line(QString text);
    QString __next_char();
    bool __in_comment();
    void autoinsert_quotes(int key);

    void setup_context_menu();
    void keyReleaseEvent(QKeyEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    void handle_parentheses(const QString& text);
    void mouseMoveEvent(QMouseEvent *event) override;
	void setPlainText(const QString &text);
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void update_visible_blocks(QPaintEvent *event);
    QList<IntIntTextblock> visible_blocks();
    bool is_editor();

public slots:
    void cb_maker(int attr);
    void update_linenumberarea_width(int new_block_count=-1);
    void update_linenumberarea(const QRect& qrect,int dy);
    void __cursor_position_changed();
    void update_breakpoints();
    void run_pygments_highlighter();
    void __mark_occurrences();
    void __text_has_changed();
    void _draw_editor_cell_divider();

    void do_completion(bool automatic=false);
    void do_go_to_definition();
    void toggle_comment();
    void blockcomment();
    void unblockcomment();
    void transform_to_uppercase();
    void transform_to_lowercase();

    void kill_line_end();
    void kill_line_start();
    void kill_prev_word();
    void kill_next_word();

    void _delete();
    void paste();
    void center_cursor_on_next_focus();
    void clear_all_output();
    void convert_notebook();
    void go_to_definition_from_cursor(QTextCursor cursor=QTextCursor());
};


class Printer : public QPrinter
{
public:
    QFont header_font;
    QString date; //当前日期时间的字符串表示
    Printer(QPrinter::PrinterMode mode = ScreenResolution, const QFont& header_font=QFont());
    // formatPage(self, painter, drawing, area, pagenr)
};


class TestWidget : public QSplitter
{
public:
    TestWidget(QWidget *parent);
    void load(const QString& filename);

public:
    CodeEditor* editor;
    OutlineExplorerWidget* classtree;
};
