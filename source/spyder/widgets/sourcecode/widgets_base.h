#pragma once

#include "os.h"
#include "str.h"
#include "config/gui.h"

#include "widgets/mixins.h"
#include "widgets/sourcecode/terminal.h"

class TextEditBaseWidget;
class CompletionWidget : public QListWidget
{
    Q_OBJECT
public:
    CompletionWidget(TextEditBaseWidget* parent, QWidget* ancestor);
    void setup_appearance(const QSize& size,const QFont& font);
    void show_list(const QList<QPair<QString,QString>>& completion_list,bool automatic=true);

    void keyPressEvent(QKeyEvent *event) override;
    void update_current();
    void focusOutEvent(QFocusEvent *event) override;
signals:
    void sig_show_completions(const QStringList&);
public slots:
    void hide();
    void item_selected(QListWidgetItem *item=nullptr);

public:
    TextEditBaseWidget* textedit;
    QStringList completion_list;
    QStringList type_list;
    bool case_sensitive;
    bool enter_select;
};


struct CursorBoolBool
{
    QTextCursor cursor;
    bool file,portion;
    CursorBoolBool(const QTextCursor& _cursor,bool _file,bool _portion):
    cursor(_cursor),file(_file),portion(_portion){}
};

class TextEditBaseWidget : public BaseEditMixin<QPlainTextEdit>
{
    Q_OBJECT
public:
    QPair<QString,QString> BRACE_MATCHING_SCOPE;
    QStringList cell_separators;
signals:
    void focus_in();
    void zoom_in();
    void zoom_out();
    void zoom_reset();
    void focus_changed();
    void sig_eol_chars_changed(const QString&) override;
public:

    TextEditBaseWidget(QWidget* parent = nullptr);
    void setup_completion();
    void set_indent_chars(const QString& indent_chars);
    void set_tab_stop_width_spaces(int tab_stop_width_spaces);
    void update_tab_stop_width_spaces();
    void set_palette(const QColor& background,const QColor& foreground);

    int extra_selection_length(const QString& key);
    QList<QTextEdit::ExtraSelection> get_extra_selections(const QString& key);
    void set_extra_selections(const QString& key,QList<QTextEdit::ExtraSelection> extra_selections);
    void update_extra_selections();
    void clear_extra_selections(const QString& key);


    void highlight_current_line();
    void unhighlight_current_line();
    void highlight_current_cell();
    void unhighlight_current_cell();

    int find_brace_match(int position,QChar brace,bool forward);
    void __highlight(const QList<int>& positions,QColor color=QColor(),bool cancel=false);


    void set_codecompletion_auto(bool state);
    void set_codecompletion_case(bool state);
    void set_codecompletion_enter(bool state);
    void set_calltips(bool state);
    void set_wrap_mode(const QString& mode=QString());

    void keyPressEvent(QKeyEvent *event) override;

    QString get_selection_as_executable_code();
    QString __exec_cell();
    QString get_cell_as_executable_code();
    QString get_last_cell_as_executable_code();

    bool is_cell_separator(const QTextCursor& cursor=QTextCursor(),
                           const QTextBlock& block=QTextBlock());
    QPair<QTextCursor,bool> select_current_cell();
    CursorBoolBool select_current_cell_in_visible_portion();
    void go_to_next_cell();
    void go_to_previous_cell();
    int get_line_count();
    QPair<int,int> __save_selection();
    void __restore_selection(int start_pos,int end_pos);
    void __duplicate_line_or_selection(bool after_current_line=true);


    void __move_line_or_selection(bool after_current_line=true);


    void extend_selection_to_complete_lines();

    void set_selection(int start,int end);
    void truncate_selection(const QString& position_from);
    void truncate_selection(int position_from);
    void restrict_cursor_position(const QString& position_from,
                                  const QString& position_to);
    void restrict_cursor_position(const QString& position_from,
                                  int position_to);
    void restrict_cursor_position(int position_from,
                                  const QString& position_to);
    void restrict_cursor_position(int position_from,
                                  int position_to);

    void hide_tooltip_if_necessary(int key);
    void show_completion_widget(const QList<QPair<QString,QString>>& textlist,
                                bool automatic=true);
    void hide_completion_widget();
    void show_completion_list(QList<QPair<QString,QString>> completions,const QString completion_text="",
                              bool automatic=true);
    void  select_completion_list();
    void insert_completion(const QString& text);
    bool is_completion_widget_visible();

    void stdkey_clear();
    void stdkey_backspace();
    QTextCursor::MoveMode __get_move_mode(bool shift);
    void stdkey_up(bool shift);
    void stdkey_down(bool shift);
    void stdkey_tab();
    void stdkey_home(bool shift,bool ctrl,
                     const QString& prompt_pos=QString());
    void stdkey_end(bool shift,bool ctrl);
    virtual void stdkey_pageup() {}
    virtual void stdkey_pagedown() {}
    virtual void stdkey_escape() {}

    void mousePressEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
public slots:
    void copy();

    void changed();
    void cursor_position_changed();

    void duplicate_line();
    void copy_line();
    void delete_line();
    void move_line_up();
    void move_line_down();
    void go_to_new_line();
public:
    QHash<QString,QList<QTextEdit::ExtraSelection>> extra_selections_dict;
    QString indent_chars;

    CompletionWidget* completion_widget;
    bool codecompletion_auto;
    bool codecompletion_case;
    bool codecompletion_enter;
    QString completion_text;

    bool calltips;

    bool has_cell_separators;
    bool highlight_current_cell_enabled;

    QColor currentline_color;
    QColor currentcell_color;

    QList<int> bracepos;
    QColor matched_p_color;
    QColor unmatched_p_color;
    QTextCursor last_cursor_cell;
};


class QtANSIEscapeCodeHandler : public ANSIEscapeCodeHandler
{
public:
    QTextCharFormat base_format;

    QtANSIEscapeCodeHandler();
    void set_light_background(bool state);
    void set_base_format(const QTextCharFormat& base_format);
    QTextCharFormat get_format() const;
    void set_style();
};

struct ConsoleFontStyle
{
    QString foregroundcolor;
    QString backgroundcolor;
    bool bold;
    bool italic;
    bool underline;
    QTextCharFormat format;
    ConsoleFontStyle(const QString& foregroundcolor,const QString&  backgroundcolor,
                     bool bold,bool italic,bool underline);
    void apply_style(QFont font,bool light_background,bool is_default);
};


class ConsoleBaseWidget : public TextEditBaseWidget
{
    Q_OBJECT
signals:
    void exception_occurred(const QString&, bool);
    void userListActivated(int, const QString&);
    void completion_widget_activated(const QString&);

public:
    QRegularExpression COLOR_PATTERN;
    bool light_background;

    QtANSIEscapeCodeHandler* ansi_handler;
    ConsoleFontStyle* default_style;
    ConsoleFontStyle* error_style;
    ConsoleFontStyle* traceback_link_style;
    ConsoleFontStyle* prompt_style;
    QList<ConsoleFontStyle*> font_styles;
public:
    ConsoleBaseWidget(QWidget* parent = nullptr);
    void set_light_background(bool state);
    void insert_text(const QString& text) override;
    void append_text_to_shell(QString text, bool error, bool prompt);
    void set_pythonshell_font(const QFont& font=QFont());
public slots:
    void paste();
};
