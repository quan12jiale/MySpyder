#ifndef FINDREPLACE_H
#define FINDREPLACE_H

#include "str.h"
#include "config/gui.h"
#include "utils/icon_manager.h"
#include "utils/misc.h"
#include "utils/qthelpers.h"
#include "widgets/comboboxes.h"
#include "widgets/sourcecode/codeeditor.h"

#include <QToolButton>

class FindReplace : public QWidget, public Widget_get_shortcut_data
{
    Q_OBJECT
signals:
    void visibility_changed(bool);
    void return_shift_pressed();
    void return_pressed();
public:
    //0:False; 1:True, 2:None, 3:regexp_error
    static QHash<bool, QString> STYLE;
    static QHash<bool, QString> TOOLTIP;

    bool enable_replace;
    CodeEditor* editor;
    bool is_code_editor;
    QToolButton* close_button;
    PatternComboBox* search_text;
    QLabel* number_matches_text;
    QToolButton* previous_button;
    QToolButton* next_button;
    QToolButton* re_button;
    QToolButton* case_button;
    QToolButton* words_button;
    QToolButton* highlight_button;
    QList<QWidget*> widgets;
    PatternComboBox* replace_text;
    QToolButton* replace_button;
    QToolButton* replace_sel_button;
    QToolButton* replace_all_button;
    QHBoxLayout* replace_layout;
    QList<QWidget*> replace_widgets;

    QList<ShortCutStrStr> shortcuts;
    QTimer* highlight_timer;

    FindReplace(QWidget* parent, bool enable_replace=false);
    bool eventFilter(QObject* widget, QEvent* event);
    QList<ShortCutStrStr> create_shortcuts(QWidget* parent);
    QList<ShortCutStrStr> get_shortcut_data();
    void toggle_replace_widgets();
    void refresh();
    void set_editor(CodeEditor* editor, bool refresh=true);
    void clear_matches();
    bool find(bool changed=true, bool forward=true, bool rehighlight=true,
              bool start_highlight_timer=false, bool multiline_replace_check=true);
    void change_number_matches(int current_match=0,int total_matches=0);
public slots:
    void highlight_matches();
    void update_search_combo();
    void update_replace_combo();
    void toggle_highlighting(bool state);
    void myshow(bool hide_replace=true);//
    void hide();
    void show_replace();
    void hide_replace();
    bool find_next();
    bool find_previous();
    void text_has_been_edited(const QString& text);
    void replace_find(bool focus_replace_text=false,bool replace_all=false);
    void replace_find_all(bool focus_replace_text=false);
    void replace_find_selection(bool focus_replace_text=false);
};

#endif // FINDREPLACE_H
