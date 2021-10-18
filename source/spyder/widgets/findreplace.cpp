#include "findreplace.h"

static bool is_position_sup(int pos1, int pos2)
{
    return pos1 > pos2;
}

static bool is_position_inf(int pos1, int pos2)
{
    return pos1 < pos2;
}

QHash<bool, QString> FindReplace::STYLE = {{false, "background-color:rgb(255, 175, 90);"},
                                           {true, ""}};
QHash<bool, QString> FindReplace::TOOLTIP = {{false, "No matches"},
                                             {true, "Search string"}};


FindReplace::FindReplace(QWidget* parent, bool enable_replace)
    : QWidget (parent)
{
    this->enable_replace = enable_replace;
    this->editor = nullptr;

    QGridLayout* glayout = new QGridLayout;
    glayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(glayout);

    this->close_button = new QToolButton(this);
    connect(this->close_button, SIGNAL(clicked(bool)), this, SLOT(hide()));
    this->close_button->setIcon(ima::icon("DialogCloseButton"));
    glayout->addWidget(this->close_button, 0, 0);

    this->search_text = new PatternComboBox(this, QStringList(),
                                            "Search string", false);
    connect(this, &FindReplace::return_shift_pressed,
            [=](){this->find(false, false, false, false, false);});

    connect(this, &FindReplace::return_pressed,
            [=](){this->find(false, true, false, false, false);});

    connect(search_text->lineEdit(), SIGNAL(textEdited(QString)),
            this, SLOT(text_has_been_edited(QString)));

    this->number_matches_text = new QLabel(this);

    this->previous_button = new QToolButton(this);
    connect(this->previous_button, SIGNAL(clicked(bool)), this, SLOT(find_previous()));
    this->previous_button->setIcon(ima::icon("ArrowUp"));

    this->next_button = new QToolButton(this);
    connect(this->next_button, SIGNAL(clicked(bool)), this, SLOT(find_next()));
    this->next_button->setIcon(ima::icon("ArrowDown"));
    connect(this->next_button, SIGNAL(clicked(bool)), this, SLOT(update_search_combo()));
    connect(this->previous_button, SIGNAL(clicked(bool)), this, SLOT(update_search_combo()));

    this->re_button = new QToolButton(this);
    this->re_button->setIcon(ima::get_icon("regexp.svg"));
    this->re_button->setToolTip("Regular expression");
    this->re_button->setCheckable(true);
    connect(this->re_button, &QAbstractButton::toggled,
            [=](){this->find();});

    this->case_button = new QToolButton(this);
    this->case_button->setIcon(ima::get_icon("upper_lower.png"));
    this->case_button->setToolTip("Case Sensitive");
    this->case_button->setCheckable(true);
    connect(this->case_button, &QAbstractButton::toggled,
            [=](){this->find();});

    this->words_button = new QToolButton(this);
    this->words_button->setIcon(ima::get_icon("whole_words.png"));
    this->words_button->setToolTip("Whole words");
    this->words_button->setCheckable(true);
    connect(this->words_button, &QAbstractButton::toggled,
            [=](){this->find();});

    this->highlight_button = new QToolButton(this);
    this->highlight_button->setIcon(ima::get_icon("highlight.png"));
    this->highlight_button->setToolTip("Highlight matches");
    this->highlight_button->setCheckable(true);
    connect(this->highlight_button, SIGNAL(toggled(bool)),
            this, SLOT(toggle_highlighting(bool)));

    QHBoxLayout* hlayout = new QHBoxLayout;
    this->widgets << this->close_button << this->search_text
                  << this->number_matches_text << this->previous_button
                  << this->next_button << this->re_button << this->case_button
                  << this->words_button << this->highlight_button;
    foreach (QWidget* widget, this->widgets.mid(1))
        hlayout->addWidget(widget);
    glayout->addLayout(hlayout, 0, 1);

    QLabel* replace_with = new QLabel("Replace with:", this);
    this->replace_text = new PatternComboBox(this, QStringList(),
                                             "Replace string", false);
    connect(this->replace_text, &PatternComboBox::valid,
            [=](){this->replace_find(true);});

    this->replace_button = new QToolButton(this);
    this->replace_button->setText("Replace/find next");
    this->replace_button->setIcon(ima::icon("DialogApplyButton"));
    connect(this->replace_button, &QAbstractButton::clicked,
            [=](){this->replace_find();});
    this->replace_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    this->replace_sel_button = new QToolButton(this);
    this->replace_sel_button->setText("Replace selection");
    this->replace_sel_button->setIcon(ima::icon("DialogApplyButton"));
    connect(this->replace_sel_button, &QAbstractButton::clicked,
            [=](){this->replace_find_selection();});
    this->replace_sel_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(this->replace_sel_button, SIGNAL(clicked(bool)), this, SLOT(update_replace_combo()));
    connect(this->replace_sel_button, SIGNAL(clicked(bool)), this, SLOT(update_search_combo()));

    this->replace_all_button = new QToolButton(this);
    this->replace_all_button->setText("Replace all");
    this->replace_all_button->setIcon(ima::icon("DialogApplyButton"));
    connect(this->replace_all_button, &QAbstractButton::clicked,
            [=](){this->replace_find_all();});
    this->replace_all_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(this->replace_all_button, SIGNAL(clicked(bool)), this, SLOT(update_replace_combo()));
    connect(this->replace_all_button, SIGNAL(clicked(bool)), this, SLOT(update_search_combo()));

    this->replace_layout = new QHBoxLayout;
	this->replace_widgets << replace_with << this->replace_text << this->replace_button
            << this->replace_sel_button << this->replace_all_button;
    foreach (QWidget* widget, this->replace_widgets)
        this->replace_layout->addWidget(widget);
    glayout->addLayout(this->replace_layout, 1, 1);

    this->widgets.append(this->replace_widgets);
    this->hide_replace();

    this->search_text->setTabOrder(this->search_text, this->replace_text);

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    this->shortcuts = this->create_shortcuts(parent);

    this->highlight_timer = new QTimer(this);
    this->highlight_timer->setSingleShot(true);
    this->highlight_timer->setInterval(1000);
    connect(this->highlight_timer, SIGNAL(timeout()), this, SLOT(highlight_matches()));
    this->search_text->installEventFilter(this);
}

bool FindReplace::eventFilter(QObject *widget, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyevent = static_cast<QKeyEvent*>(event);
        int key = keyevent->key();
        Qt::KeyboardModifiers shift = keyevent->modifiers() & Qt::ShiftModifier;

        if (key == Qt::Key_Return) {
            if (shift)
                emit this->return_shift_pressed();
            else
                emit this->return_pressed();
        }

        else if (key == Qt::Key_Tab) {
            if (this->search_text->hasFocus())
                this->replace_text->set_current_text(this->search_text->currentText());
            this->focusNextChild();
        }
    }
    return QWidget::eventFilter(widget, event);
}

QList<ShortCutStrStr> FindReplace::create_shortcuts(QWidget *parent)
{
    QString keystr = gui::get_shortcut("_", "Find next");// F3
    QShortcut* qsc = new QShortcut(QKeySequence(keystr), parent);
    //没有下面这一行无法使用快捷键调出该控件
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(qsc, SIGNAL(activated()), this, SLOT(find_next()));
    ShortCutStrStr findnext(qsc, "_", "Find next");

    keystr = gui::get_shortcut("_", "Find previous");// Shift+F3
    qsc = new QShortcut(QKeySequence(keystr), parent);
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(qsc, SIGNAL(activated()), this, SLOT(find_previous()));
    ShortCutStrStr findprev(qsc, "_", "Find previous");

    keystr = gui::get_shortcut("_", "Find text");// Ctrl+F
    qsc = new QShortcut(QKeySequence(keystr), parent);
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(qsc, SIGNAL(activated()), this, SLOT(myshow()));
    ShortCutStrStr togglefind(qsc, "_", "Find text");

    keystr = gui::get_shortcut("_", "Replace text");// Ctrl+R
    qsc = new QShortcut(QKeySequence(keystr), parent);
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(qsc, SIGNAL(activated()), this, SLOT(show_replace()));
    ShortCutStrStr togglereplace(qsc, "_", "Replace text");

    keystr = gui::get_shortcut("_", "hide find and replace");// Escape
    qsc = new QShortcut(QKeySequence(keystr), parent);
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(qsc, SIGNAL(activated()), this, SLOT(hide()));
    ShortCutStrStr hide(qsc, "_", "hide find and replace");

    QList<ShortCutStrStr> res;
    res << findnext << findprev << togglefind << togglereplace << hide;
    return res;
}

QList<ShortCutStrStr> FindReplace::get_shortcut_data()
{
    return shortcuts;
}

void FindReplace::update_search_combo()
{
    emit this->search_text->lineEdit()->returnPressed();
}

void FindReplace::update_replace_combo()
{
    emit this->replace_text->lineEdit()->returnPressed();
}

void FindReplace::toggle_replace_widgets()
{
    if (this->enable_replace) {
        if (this->replace_widgets[0]->isVisible()) {
            this->hide_replace();
            this->hide();
        }
        else {
            this->show_replace();
            if (this->search_text->currentText().size() > 0)
                this->replace_text->setFocus();
        }
    }
}

void FindReplace::toggle_highlighting(bool state)
{
    if (this->editor) {
        if (state)
            this->highlight_matches();
        else
            this->clear_matches();
    }
}

void FindReplace::myshow(bool hide_replace)
{
    QWidget::show();
    emit this->visibility_changed(true);
    this->change_number_matches();
    if (this->editor != nullptr) {
        if (hide_replace) {
            if (this->replace_widgets[0]->isVisible())
                this->hide_replace();

        }
        QString text = editor->get_selected_text();

        if (hide_replace || splitlines(text).size() <= 1) {
            bool highlighted = true;
            if (text.isEmpty()) {
                highlighted = false;

                QTextCursor cursor = editor->textCursor();
                cursor.select(QTextCursor::WordUnderCursor);
                text = cursor.selectedText();
            }

            if ((!text.isEmpty() && this->search_text->currentText().isEmpty()) || highlighted) {
                this->search_text->setEditText(text);
                this->search_text->lineEdit()->selectAll();
                this->refresh();
            }
            else
                this->search_text->lineEdit()->selectAll();
        }
        this->search_text->setFocus();
    }
}

void FindReplace::hide()
{
    foreach (QWidget* widget, this->replace_widgets)
        widget->hide();
    QWidget::hide();
    emit this->visibility_changed(false);
    if (this->editor) {
        this->editor->setFocus();
        this->clear_matches();
    }
}

void FindReplace::show_replace()
{
    this->myshow(false);
    foreach (QWidget* widget, this->replace_widgets)
        widget->show();
}

void FindReplace::hide_replace()
{
    foreach (QWidget* widget, this->replace_widgets)
        widget->hide();
}

void FindReplace::refresh()
{
    if (this->isHidden()) {
        if (this->editor != nullptr)
            this->clear_matches();
        return;
    }
    bool state = this->editor != nullptr;
    foreach (QWidget* widget, this->widgets)
        widget->setEnabled(state);
    if (state)
        this->find();
}

void FindReplace::set_editor(CodeEditor *editor, bool refresh)
{
    //editor在源码中可能是CodeEditor或WebEngineView
    this->editor = editor;
    this->words_button->setVisible(true);
    this->re_button->setVisible(true);
    //this->re_button->setVisible(! editor->inherits("QWebEngineView"));
    this->is_code_editor = editor != nullptr;
    this->highlight_button->setVisible(this->is_code_editor);
    if (refresh)
        this->refresh();
    if (this->isHidden() && this->editor != nullptr)
        this->clear_matches();
}

bool FindReplace::find_next()
{
    bool state = this->find(false, true, false, false, false);
    this->editor->setFocus();
    this->search_text->add_current_text();
    return state;
}

bool FindReplace::find_previous()
{
    bool state = this->find(false, false, false, false, false);
    this->editor->setFocus();
    return state;
}

void FindReplace::text_has_been_edited(const QString& text)
{
    Q_UNUSED(text);
    this->find(true, true, true, true);
}

void FindReplace::highlight_matches()
{
    if (this->is_code_editor && this->highlight_button->isChecked()) {
        QString text = this->search_text->currentText();
        bool words = this->words_button->isChecked();
        bool regexp = this->re_button->isChecked();
        editor->highlight_found_results(text, words, regexp);
    }
}

void FindReplace::clear_matches()
{
    if (this->is_code_editor) {
        editor->clear_found_results();
    }
}

bool FindReplace::find(bool changed, bool forward, bool rehighlight,
                       bool start_highlight_timer, bool multiline_replace_check)
{
    if (multiline_replace_check && this->replace_widgets[0]->isVisible() &&
            splitlines(editor->get_selected_text()).size() > 1)
        return false;
    QString text = this->search_text->currentText();
    if (text.size() == 0) {
        this->search_text->lineEdit()->setStyleSheet("");
        if (!this->is_code_editor)
            editor->find_text("");
        this->change_number_matches();
        return false;
    }
    else {
        bool _case = this->case_button->isChecked();
        bool words = this->words_button->isChecked();
        bool regexp = this->re_button->isChecked();
        bool found = editor->find_text(text, changed, forward,
                                       _case, words, regexp);
        QString stylesheet = this->STYLE[found];
        QString tooltip = this->TOOLTIP[found];
        if (!found && regexp) {
            QString error_msg = misc::regexp_error_msg(text);
            if (!error_msg.isEmpty()) {
                stylesheet = "background-color:rgb(255, 80, 80);";
                tooltip = "Regular expression error: " + error_msg;
            }
        }
        this->search_text->lineEdit()->setStyleSheet(stylesheet);
        this->search_text->setToolTip(tooltip);

        if (this->is_code_editor && found) {
            if (rehighlight || editor->found_results.isEmpty()) {
                this->highlight_timer->stop();
                if (start_highlight_timer)
                    this->highlight_timer->start();
                else
                    this->highlight_matches();
            }
        }
        else
            this->clear_matches();

        int number_matches = editor->get_number_matches(text, "", _case, regexp);
        int match_number = editor->get_match_number(text, _case, regexp);
        this->change_number_matches(match_number, number_matches);
        return found;
    }
}

void FindReplace::replace_find(bool focus_replace_text, bool replace_all)
{
    if (this->editor != nullptr) {
        QString replace_text = this->replace_text->currentText();
        QString search_text = this->search_text->currentText();
        QRegularExpression re_pattern;

        if (this->re_button->isChecked()) {
            re_pattern = QRegularExpression(search_text);
            if (re_pattern.isValid())
                replace_text.replace(re_pattern, "");
            else
                return;
        }

        bool _case = this->case_button->isChecked();
        bool first = true;
        QTextCursor cursor;
		bool hasBeginEditBlock = false;
        while (true) {
            bool wrapped = false;
            int position = 0, position0 = 0;
            QString seltxt;
            if (first) {
                seltxt = editor->get_selected_text();
                QString cmptxt1, cmptxt2;
                if (_case) {
                    cmptxt1 = search_text;
                    cmptxt2 = seltxt;
                }
                else {
                    cmptxt1 = search_text.toLower();
                    cmptxt2 = seltxt.toLower();
                }

                if (re_pattern == QRegularExpression()) {
                    bool has_selected = editor->has_selected_text();
                    if (has_selected && cmptxt1 == cmptxt2)
                        ;
                    else {
                        if (! this->find(false, true, false)) {
                            break;
                        }
                    }
                }
                else {
                    int len = 0;
                    QRegularExpressionMatchIterator it = re_pattern.globalMatch(cmptxt2);
                    while (it.hasNext()) {
                        it.next();
                        len++;
                    }
                    if (len > 0)
                        ;
                    else {
                        if (! this->find(false, true, false))
                            break;
                    }
                }
                first = false;
                wrapped = false;
                position = editor->get_position("cursor");
                position0 = position;
                cursor = editor->textCursor();
                cursor.beginEditBlock();
				hasBeginEditBlock = true;
            }
            else {
                int position1 = editor->get_position("cursor");
                if (is_position_inf(position1, position0+replace_text.size()-search_text.size()+1))
                    wrapped = true;
                if (wrapped) {
                    if (position1 == position || is_position_sup(position1, position))
                        break;
                }
                if (position1 == position0)
                    break;
                position0 = position1;
            }

            if (re_pattern == QRegularExpression()) {
                cursor.removeSelectedText();
                cursor.insertText(replace_text);

            }
            else {
                seltxt = cursor.selectedText();
                cursor.removeSelectedText();
                cursor.insertText(seltxt.replace(re_pattern, replace_text));
            }
            if (this->find_next()) {
                QTextCursor found_cursor = editor->textCursor();
                cursor.setPosition(found_cursor.selectionStart(),
                                   QTextCursor::MoveAnchor);
                cursor.setPosition(found_cursor.selectionEnd(),
                                   QTextCursor::KeepAnchor);
            }
            else
                break;
            if (!replace_all) {
                break;
            }
        }

        if (hasBeginEditBlock) { // !cursor.isNull()
            // 调用cursor.endEditBlock()就会报错"QList<T>::operator[]", "index out of range"
            cursor.endEditBlock();
        }

        if (focus_replace_text)
            this->replace_text->setFocus();
    }

}

void FindReplace::replace_find_all(bool focus_replace_text)
{
    this->replace_find(focus_replace_text, true);
}

void FindReplace::replace_find_selection(bool focus_replace_text)
{
    if (this->editor) {
        QString replace_text = this->replace_text->currentText();
        QString search_text = this->search_text->currentText();
        bool _case = this->case_button->isChecked();
        bool words = this->words_button->isChecked();
        QRegularExpression::PatternOptions re_flags;
        if (_case)
            re_flags = QRegularExpression::MultilineOption;
        else
            re_flags = QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption;

        QRegularExpression re_pattern;
        QString pattern;
        if (this->re_button->isChecked())
            pattern = search_text;
        else {
            pattern = QRegularExpression::escape(search_text);
            replace_text = QRegularExpression::escape(replace_text);
        }
        if (words)
            pattern = "\\b" + pattern + "\\b";

        re_pattern = QRegularExpression(pattern, re_flags);
        if (re_pattern.isValid())
            replace_text.replace(re_pattern, "");
        else
            return;

        QString selected_text = editor->get_selected_text();
        QString replacement = selected_text;
        replacement.replace(re_pattern, replace_text);
        if (replacement != selected_text) {
            QTextCursor cursor = editor->textCursor();
            cursor.beginEditBlock();
            cursor.removeSelectedText();
            if (! this->re_button->isChecked())
                replacement.replace(QRegularExpression("\\\\(?![nrtf])(.)"), "\\1");
            cursor.insertText(replacement);
            cursor.endEditBlock();
        }
        if (focus_replace_text)
            this->replace_text->setFocus();
        else
            this->editor->setFocus();
    }
}

void FindReplace::change_number_matches(int current_match, int total_matches)
{
    if (current_match && total_matches) {
        QString matches_string = QString("%1 %2 %3").arg(current_match).arg("of").arg(total_matches);
        this->number_matches_text->setText(matches_string);
    }
    else if (total_matches) {
        QString matches_string = QString("%1 %2").arg(total_matches).arg("matches");
        this->number_matches_text->setText(matches_string);
    }
    else
        this->number_matches_text->setText("no matches");
}


