#pragma once

#include "os.h"
#include "str.h"
#include "utils/sourcecode.h"
#include "utils/misc.h"
#include <QtWidgets>

class ControlWidget;
class ControlCallTip : public QLabel
{
    Q_OBJECT
public:
    QApplication* app;

    bool hide_timer_on;
    QString tip;
    QBasicTimer* _hide_timer;
    ControlWidget* _text_edit;

    int _start_position;

    ControlCallTip(ControlWidget* text_edit, bool hide_timer_on = false);

    bool eventFilter(QObject *obj, QEvent *event);
    void timerEvent(QTimerEvent *event);
    void enterEvent(QEvent *event);
    void hideEvent(QHideEvent *event);
    void leaveEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

    void setFont(const QFont& font);
    void showEvent(QShowEvent *event);
    void focusOutEvent(QFocusEvent *event);

    bool show_tip(QPoint point, const QString& tip, QStringList wrapped_tiplines);
    QPair<int,int> _find_parenthesis(int position, bool forward=true);
    void _leave_event_hide();
public slots:
    void _cursor_position_changed();
};

class ControlWidget : public QTextEdit
{
    Q_OBJECT
signals:
    //来自于mixins.py
    void sig_eol_chars_changed(const QString&);

    void visibility_changed(bool);
    void go_to_error(const QString&);
    void focus_changed();
public:

    QString eol_chars;
    int calltip_size;

    int calltip_position;//定义在TextEditBaseWidget中
    int current_prompt_pos;//首次出现在373行 TODO是在哪里初始化的
    int tab_stop_width_spaces;//定义在TextEditBaseWidget中
    ControlCallTip* calltip_widget;//定义在TextEditBaseWidget中

    bool __cursor_changed;

    //help
    bool help_enabled;

    QStringList history;
    int histidx;
    bool hist_wholeline;

    // found_results
    bool calltips;

    ControlWidget(QWidget *parent = nullptr);

    void set_eol_chars(const QString& text);
    QString get_line_separator() const;
    QString get_text_with_eol() const;

    int get_position(const QString& subject) const;
    int get_position(int subject) const;
    QPoint get_coordinates(const QString& position) const;
    QPoint get_coordinates(int position) const;
    QPair<int,int> get_cursor_line_column() const;
    int get_cursor_line_number() const;
    void set_cursor_position(const QString& position);
    void set_cursor_position(int position);
    void move_cursor(int chars=0);
    bool is_cursor_on_first_line() const;
    bool is_cursor_on_last_line() const;
    bool is_cursor_at_end() const;
    bool is_cursor_before(const QString& position,int char_offset=0) const;
    bool is_cursor_before(int position,int char_offset=0) const;
    void __move_cursor_anchor(QString what,QString direction,
                              QTextCursor::MoveMode move_mode);
    void move_cursor_to_next(QString what="word",QString direction="left");
    void clear_selection();
    void extend_selection_to_next(QString what="word",QString direction="left");

    QTextCursor __select_text(const QString& position_from,
                              const QString& position_to);
    QTextCursor __select_text(const QString& position_from,
                              int position_to);
    QTextCursor __select_text(int position_from,
                              const QString& position_to);
    QTextCursor __select_text(int position_from,
                              int position_to);
    QString get_text_line(int line_nb);
    QString get_text(const QString& position_from,
                     const QString& position_to);
    QString get_text(const QString& position_from,
                     int position_to);
    QString get_text(int position_from,
                     const QString& position_to);
    QString get_text(int position_from,
                     int position_to);
    QChar get_character(const QString& position, int offset=0);
    QChar get_character(int position, int offset=0);
    virtual void insert_text(const QString& text);
    void replace_text(const QString& position_from,
                      const QString& position_to,const QString& text);
    void replace_text(const QString& position_from,
                      int position_to,const QString& text);
    void replace_text(int position_from,
                      const QString& position_to,const QString& text);
    void replace_text(int position_from,
                      int position_to,const QString& text);
    void remove_text(const QString& position_from,
                     const QString& position_to);
    void remove_text(const QString& position_from,
                     int position_to);
    void remove_text(int position_from,
                     const QString& position_to);
    void remove_text(int position_from,
                     int position_to);
    QString get_current_word();
    bool is_space(QTextCursor::MoveOperation move);
    QString get_current_line();
    QString get_current_line_to_cursor();
    int get_line_number_at(const QPoint& coordinates);
    QString get_line_at(const QPoint& coordinates);
    QString get_word_at(const QPoint& coordinates);
    int get_block_indentation(int block_nb);
    QPair<int,int> get_selection_bounds();

    bool has_selected_text();
    QString get_selected_text();
    void remove_selected_text();
    void replace(QString text, const QString& pattern=QString());

    QTextCursor find_multiline_pattern(const QRegularExpression& regexp,const QTextCursor& cursor,
                                       QTextDocument::FindFlags findflag);
    bool find_text(QString text,bool changed=true,bool forward=true,bool _case=false,
                   bool words=false, bool regexp=false);
    bool is_editor() { return false; }
    int get_number_matches(QString pattern,QString source_text="",bool _case=false,
                           bool regexp=false);
    int get_match_number(QString pattern,bool _case=false,bool regexp=false);
    //void enter_array_inline() { this->_enter_array(true); }
    //void enter_array_table() { this->_enter_array(false); }
    //void _enter_array(bool inline_);
    //virtual bool is_python_like() = 0;//mixins.py596行，在codeeditor.py中定义
    virtual bool is_python_like() { return true; }

    // TracebackLinksMixin
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);

    // GetHelpMixin
    //void set_help();
    void set_help_enabled(bool state);
    void inspect_current_object();
    void show_object_info(const QString& text, bool call=false, bool force=false);
    //get_last_obj

    // BrowseHistoryMixin
    void clear_line();
    void browse_history(bool backward);
    QPair<QString,int> find_in_history(const QString& tocursor, int start_idx, bool backward);
    void reset_search_pos();

    // ControlWidget
    void showEvent(QShowEvent *event);
    void _key_paren_left(const QString& text);
    void keyPressEvent(QKeyEvent *event);
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);

};

