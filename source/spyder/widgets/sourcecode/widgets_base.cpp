#include "widgets_base.h"
#include "../calltip.h"

static void insert_text_to(QTextCursor* cursor, QString text, const QTextCharFormat& fmt)
{
    // QChar(8) = backspace
    while (true) {
        int index = text.indexOf(QChar(8));
        if (index == -1)
            break;
        cursor->insertText(text.left(index), fmt);
        if (cursor->positionInBlock() > 0)
            cursor->deletePreviousChar();
        text = text.mid(index+1);
    }
    cursor->insertText(text, fmt);
}

CompletionWidget::CompletionWidget(TextEditBaseWidget *parent,
                                   QWidget* ancestor)
    : QListWidget (ancestor)
{
    this->setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    this->textedit = parent;
    this->completion_list = QStringList();
    this->case_sensitive = false;
    this->enter_select = false;
    this->hide();
    connect(this,SIGNAL(itemActivated(QListWidgetItem *)),
            this,SLOT(item_selected(QListWidgetItem *)));
}

void CompletionWidget::setup_appearance(const QSize &size, const QFont &font)
{
    this->resize(size);
    this->setFont(font);
}

void CompletionWidget::show_list(const QList<QPair<QString, QString> >& completion_list, bool automatic)
{
    QStringList types, _completion_list;
    foreach (auto pair, completion_list) {
        types << pair.second;
        _completion_list << pair.first;
    }
    if (_completion_list.size()==1 && !automatic) {
        this->textedit->insert_completion(_completion_list[0]);
        return;
    }

    this->completion_list = _completion_list;
    this->clear();

    QHash<QString,QString> icons_map = {{"instance", "attribute"},
                                        {"statement", "attribute"},
                                        {"method", "method"},
                                        {"function", "function"},
                                        {"class", "class"},
                                        {"module", "module"}};
    this->type_list = types;
	if (std::any_of(types.cbegin(), types.cend(), [](QString element) { return !element.isEmpty(); })) {
        for (int i = 0; i<std::min(_completion_list.size(), types.size()); ++i) {
            QString icon = icons_map.value(types[i],"no_match");
            this->addItem(new QListWidgetItem(ima::icon(icon),_completion_list[i]));
        }
    }
    else
        this->addItems(_completion_list);
    setCurrentRow(0);

    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    this->show();
    this->setFocus();
    this->raise();

    QDesktopWidget* desktop = QApplication::desktop();
    QRect srect = desktop->availableGeometry(desktop->screenNumber(this));
    int screen_right = srect.right();
    int screen_bottom = srect.bottom();

    QPoint point = this->textedit->cursorRect().bottomRight();
    point.setX(point.x()+this->textedit->get_linenumberarea_width());
    point = this->textedit->mapToGlobal(point);

    int comp_right = point.x()+this->width();
    QWidget* ancestor = qobject_cast<QWidget*>(this->parent());
    int anc_right;
    if (ancestor==nullptr)
        anc_right = screen_right;
    else
        anc_right = qMin(ancestor->x()+ancestor->width(), screen_right);

    if (comp_right > anc_right)
        point.setX(point.x()-this->width());

    int comp_bottom = point.y()+this->height();
    ancestor = qobject_cast<QWidget*>(this->parent());
    int anc_bottom;
    if (ancestor==nullptr)
        anc_bottom = screen_bottom;
    else
        anc_bottom = qMin(ancestor->y()+ancestor->height(), screen_bottom);

    int x_position = point.x();
    if (comp_bottom > anc_bottom) {
        point = this->textedit->cursorRect().topRight();
        point = this->textedit->mapToGlobal(point);
        point.setX(x_position);
        point.setY(point.y()-this->height());
    }

    if (ancestor)
        point = ancestor->mapFromGlobal(point);
    this->move(point);

    if (!this->textedit->completion_text.isEmpty())
        this->update_current();
    emit this->sig_show_completions(_completion_list);
}

void CompletionWidget::hide()
{
    QListWidget::hide();
    this->textedit->setFocus();
}

void CompletionWidget::keyPressEvent(QKeyEvent *event)
{
    QString text = event->text();
    int key = event->key();
    Qt::KeyboardModifiers alt = event->modifiers() & Qt::AltModifier;
    Qt::KeyboardModifiers shift = event->modifiers() & Qt::ShiftModifier;
    Qt::KeyboardModifiers ctrl = event->modifiers() & Qt::ControlModifier;
    bool modifier = shift || ctrl || alt;
    if (((key==Qt::Key_Return || key==Qt::Key_Enter) && this->enter_select)
            || key==Qt::Key_Tab)
        this->item_selected();
    else if ((key==Qt::Key_Return || key==Qt::Key_Enter ||
              key==Qt::Key_Left || key==Qt::Key_Right) || (text=="." || text==":")) {
        this->hide();
        this->textedit->keyPressEvent(event);
    }
    else if ((key==Qt::Key_Up || key==Qt::Key_Down || key==Qt::Key_PageUp || key==Qt::Key_PageDown ||
              key==Qt::Key_Home || key==Qt::Key_End || key==Qt::Key_CapsLock) && !modifier) {
        QListWidget::keyPressEvent(event);
    }
    else if (text.size() || key==Qt::Key_Backspace) {
        this->textedit->keyPressEvent(event);
        this->update_current();
    }
    else if (modifier) {
        this->textedit->keyPressEvent(event);
    }
    else {
        this->hide();
        QListWidget::keyPressEvent(event);
    }
}

void CompletionWidget::update_current()
{
    QString completion_text = this->textedit->completion_text;
    if (!completion_text.isEmpty()) {
        bool break_flag = true;
        for (int i = 0; i < this->completion_list.size(); ++i) {
            QString completion = this->completion_list[i];
            if (!this->case_sensitive) {
                qDebug() << completion_text;
                completion = completion.toLower();
                completion_text = completion_text.toLower();
            }
            if (completion.startsWith(completion_text)) {
                this->setCurrentRow(i);
                this->scrollTo(this->currentIndex(),
                               QAbstractItemView::PositionAtTop);
                break_flag = false;
                break;
            }
        }
        if (break_flag)
            this->hide();
    }
    else
        this->hide();
}

void CompletionWidget::focusOutEvent(QFocusEvent *event)
{
    event->ignore();
#ifdef Q_OS_MAC
    if (event->reason() != Qt::ActiveWindowFocusReason)
        this->hide();
#else
    this->hide();
#endif
}

void CompletionWidget::item_selected(QListWidgetItem *item)
{
    if (item == nullptr)
        item = this->currentItem();
    this->textedit->insert_completion(item->text());
    this->hide();
}


/********** TextEditBaseWidget **********/
TextEditBaseWidget::TextEditBaseWidget(QWidget* parent)
    : BaseEditMixin<QPlainTextEdit> (parent)
{
    BRACE_MATCHING_SCOPE = QPair<QString,QString>("sof","eof");
    cell_separators = QStringList();

    this->setAttribute(Qt::WA_DeleteOnClose);

    this->extra_selections_dict = QHash<QString,QList<QTextEdit::ExtraSelection>>();

    connect(this,SIGNAL(textChanged()),this,SLOT(changed()));
    connect(this,SIGNAL(cursorPositionChanged()),
            this,SLOT(cursor_position_changed()));

    this->indent_chars = QString(4, ' ');
    this->tab_stop_width_spaces = 4;

    if (parent) {
        QWidget* mainwin = parent;
        while (qobject_cast<QMainWindow*>(mainwin) == nullptr) {
            mainwin = qobject_cast<QWidget*>(mainwin->parent());
            if (mainwin == nullptr)
                break;
        }
        if (mainwin)
            parent = mainwin;
    }

    this->completion_widget = new CompletionWidget(this, parent);
    this->codecompletion_auto = false;
    this->codecompletion_case = true;
    this->codecompletion_enter = false;
    this->completion_text = "";
    this->setup_completion();

    this->calltip_widget = new CallTipWidget(this, false);
    this->calltips = true;
    this->calltip_position = -1;//所有int用-1表示Python中的None

    this->has_cell_separators = false;
    this->highlight_current_cell_enabled = false;


    this->currentline_color = QColor(Qt::red).lighter(190);
    this->currentcell_color = QColor(Qt::red).lighter(194);

    this->bracepos = QList<int>();
    this->matched_p_color = QColor(Qt::green);
    this->unmatched_p_color = QColor(Qt::red);

    this->last_cursor_cell = QTextCursor();
}

void TextEditBaseWidget::setup_completion()
{
    // size = (300,180)
    QSize size = CONF_get("main", "completion/size").toSize();
    QFont font = gui::get_font();
    this->completion_widget->setup_appearance(size, font);
}

void TextEditBaseWidget::set_indent_chars(const QString &indent_chars)
{
    this->indent_chars = indent_chars;
}

void TextEditBaseWidget::set_tab_stop_width_spaces(int tab_stop_width_spaces)
{
    this->tab_stop_width_spaces = tab_stop_width_spaces;
    this->update_tab_stop_width_spaces();
}

void TextEditBaseWidget::update_tab_stop_width_spaces()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    this->setTabStopDistance(this->fontMetrics().horizontalAdvance(
                              QString(this->tab_stop_width_spaces, ' ')));
#else
	this->setTabStopWidth(this->fontMetrics().width(
		QString(this->tab_stop_width_spaces, ' ')));
#endif
}

void TextEditBaseWidget::set_palette(const QColor &background, const QColor &foreground)
{
    QPalette palette;
    palette.setColor(QPalette::Base, background);
    palette.setColor(QPalette::Text, foreground);
    this->setPalette(palette);

#ifdef Q_OS_MAC
    if (!this->objectName().isEmpty()) {
        QString style = QString("QPlainTextEdit#%1 {background: %2; color: %3;}")
                .arg(this->objectName()).arg(background.name()).arg(foreground.name());
        this->setStyleSheet(style);
    }
#endif
}

//------Extra selections
int TextEditBaseWidget::extra_selection_length(const QString& key)
{
    QList<QTextEdit::ExtraSelection> selection = this->get_extra_selections(key);
    if (!selection.isEmpty()) {
        QTextCursor cursor = this->extra_selections_dict[key][0].cursor;
        int selection_length = cursor.selectionEnd() - cursor.selectionStart();
        return selection_length;
    }
    else
        return 0;
}

QList<QTextEdit::ExtraSelection> TextEditBaseWidget::get_extra_selections(const QString& key)
{
    return this->extra_selections_dict.value(key,QList<QTextEdit::ExtraSelection>());
}

void TextEditBaseWidget::set_extra_selections(const QString &key, QList<QTextEdit::ExtraSelection> extra_selections)
{
    this->extra_selections_dict[key] = extra_selections;
    // this->extra_selections_dict = OrderedDict
    // TODO实现OrderedDict,具体实现位于odictobject.c，用到了双向链表+字典
}

void TextEditBaseWidget::update_extra_selections()
{
    QList<QTextEdit::ExtraSelection> extra_selections;
    if (this->extra_selections_dict.contains("current_cell"))
        extra_selections.append(this->extra_selections_dict["current_cell"]);
    if (this->extra_selections_dict.contains("current_line"))
        extra_selections.append(this->extra_selections_dict["current_line"]);

    for (auto it = this->extra_selections_dict.begin(); it!=this->extra_selections_dict.end(); ++it) {
        if (!(it.key()=="current_line" || it.key()=="current_cell"))
            extra_selections.append(it.value());
    }
    //下面这一行代码用来使光标所在处单词呈现occurrence_color颜色
    this->setExtraSelections(extra_selections);
}

void TextEditBaseWidget::clear_extra_selections(const QString &key)
{
    this->extra_selections_dict[key] = QList<QTextEdit::ExtraSelection>();
    this->update_extra_selections();
}

void TextEditBaseWidget::changed()
{
    emit this->modificationChanged(this->document()->isModified());
}

//------Highlight current line
void TextEditBaseWidget::highlight_current_line()
{
    QTextEdit::ExtraSelection selection;
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.format.setBackground(currentline_color);
    selection.cursor = this->textCursor();
    selection.cursor.clearSelection();

    QList<QTextEdit::ExtraSelection> extraSelections;
    extraSelections.append(selection);
    this->set_extra_selections("current_line", extraSelections);
    this->update_extra_selections();
}

void TextEditBaseWidget::unhighlight_current_line()
{
    this->clear_extra_selections("current_line");
}

void TextEditBaseWidget::highlight_current_cell()
{
    if (this->cell_separators.isEmpty() ||
            !this->highlight_current_cell_enabled)
        return;
    QTextEdit::ExtraSelection selection;
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.format.setBackground(currentline_color);

    CursorBoolBool res = this->select_current_cell_in_visible_portion();
    selection.cursor = res.cursor;
    bool whole_file_selected = res.file;
    bool whole_screen_selected = res.portion;
    QList<QTextEdit::ExtraSelection> extraSelections;
    extraSelections.append(selection);
    if (whole_file_selected)
        this->clear_extra_selections("current_cell");
    else if (whole_screen_selected) {
        if (this->has_cell_separators) {
            this->set_extra_selections("current_cell",extraSelections);
            this->update_extra_selections();
        }
        else
            this->clear_extra_selections("current_cell");
    }
    else {
        this->set_extra_selections("current_cell",extraSelections);
        this->update_extra_selections();
    }
}

void TextEditBaseWidget::unhighlight_current_cell()
{
    this->clear_extra_selections("current_cell");
}

//------Brace matching
int TextEditBaseWidget::find_brace_match(int position,
                                          QChar brace, bool forward)
{
    QString start_pos = this->BRACE_MATCHING_SCOPE.first;
    QString end_pos = this->BRACE_MATCHING_SCOPE.second;
    QHash<QChar,QChar> bracemap;
    QString text;
    int i_start_open, i_start_close;
    if (forward) {
        bracemap['('] = ')';
        bracemap['['] = ']';
        bracemap['{'] = '}';
        text = this->get_text(position,end_pos);
        i_start_open = 1;
        i_start_close = 1;
    }
    else {
        bracemap[')'] = '(';
        bracemap[']'] = '[';
        bracemap['}'] = '{';
        text = this->get_text(start_pos,position);
        i_start_open = text.size()-1;
        i_start_close = text.size()-1;
    }

    while (true) {
        int i_close;
        if (forward)
            i_close = text.indexOf(bracemap[brace], i_start_close);
        else
            // python中str.rfind(char,0,len)对应qstring.lastIndexOf(char,len-1);
            i_close = text.lastIndexOf(bracemap[brace], i_start_close);
        if (i_close > -1) {
			int i_open;
            if (forward) {
                i_start_close = i_close+1;
                i_open = text.left(i_close).indexOf(brace, i_start_open);
            }
            else {
                i_start_close = i_close-1;
                i_open = text.lastIndexOf(brace, i_start_open);
                if (i_open < i_close)
                    i_open = -1;
            }
            if (i_open > -1) {
                if (forward)
                    i_start_open = i_open+1;
                else
                    i_start_open = i_open-1;
            }
            else {
                if (forward)
                    return position+i_close;
                else
                    return position-(text.size()-i_close);
            }
        }
        else
            return -1;
    }
}

void TextEditBaseWidget::__highlight(const QList<int> &positions, QColor color, bool cancel)
{
    if (cancel) {
        this->clear_extra_selections("brace_matching");
        return;
    }
    QList<QTextEdit::ExtraSelection> extra_selections;
    foreach (int position, positions) {
        if (position > this->get_position("eof"))
            return;
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(color);
        selection.cursor = this->textCursor();
        selection.cursor.clearSelection();
        selection.cursor.setPosition(position);
        selection.cursor.movePosition(QTextCursor::NextCharacter,
                                      QTextCursor::KeepAnchor);
        extra_selections.append(selection);
    }
    this->set_extra_selections("brace_matching", extra_selections);
    this->update_extra_selections();
}

void TextEditBaseWidget::cursor_position_changed()
{
    if (!this->bracepos.isEmpty()) {
        this->__highlight(this->bracepos, QColor(), true);
        this->bracepos.clear();
    }
    QTextCursor cursor = this->textCursor();
    if (cursor.position() == 0)
        return;
    cursor.movePosition(QTextCursor::PreviousCharacter,
                        QTextCursor::KeepAnchor);
    QString text = cursor.selectedText();
    int pos1 = cursor.position();
    int pos2 = -1;
    if (text==")" || text=="]" || text=="}")
        pos2 = this->find_brace_match(pos1,text[0],false);
    else if (text=="(" || text=="[" || text=="{")
        pos2 = this->find_brace_match(pos1,text[0],true);
    else
        return;
    if (pos2 > -1) {
        this->bracepos = {pos1, pos2};
        this->__highlight(this->bracepos, this->matched_p_color);
    }
    else {
        this->bracepos = {pos1};
        this->__highlight(this->bracepos, this->unmatched_p_color);
    }
}


//-----Widget setup and options
void TextEditBaseWidget::set_codecompletion_auto(bool state)
{
    this->codecompletion_auto = state;
}

void TextEditBaseWidget::set_codecompletion_case(bool state)
{
    this->codecompletion_case = state;
    this->completion_widget->case_sensitive = state;
}

void TextEditBaseWidget::set_codecompletion_enter(bool state)
{
    this->codecompletion_enter = state;
    this->completion_widget->enter_select = state;
}

void TextEditBaseWidget::set_calltips(bool state)
{
    this->calltips = state;
}

void TextEditBaseWidget::set_wrap_mode(const QString &mode)
{
    QTextOption::WrapMode wrap_mode;
    if (mode == "word")
        wrap_mode = QTextOption::WrapAtWordBoundaryOrAnywhere;
    else if (mode == "character")
        wrap_mode = QTextOption::WrapAnywhere;
    else
        wrap_mode = QTextOption::NoWrap;
    this->setWordWrapMode(wrap_mode);
}

//------Reimplementing Qt methods
void TextEditBaseWidget::copy()
{
    if (!this->get_selected_text().isEmpty())
        QApplication::clipboard()->setText(this->get_selected_text());
}

void TextEditBaseWidget::keyPressEvent(QKeyEvent *event)
{
    QString text = event->text();
	Q_UNUSED(text);
    int key = event->key();
    Qt::KeyboardModifiers ctrl = event->modifiers() & Qt::ControlModifier;
    Qt::KeyboardModifiers meta = event->modifiers() & Qt::MetaModifier;

    if ((ctrl || meta) && key==Qt::Key_C)
        this->copy();
    else
        QPlainTextEdit::keyPressEvent(event);
}


//------Text: get, set, ...
QString TextEditBaseWidget::get_selection_as_executable_code()
{
    QString ls = this->get_line_separator();
    auto _indent = [=](QString line) -> int {return line.size() - lstrip(line).size(); };

    QPair<int,int> pair = this->get_selection_bounds();
    int line_from = pair.first;
    QString text = this->get_selected_text();
    if (text.isEmpty())
        return QString();

    QStringList lines = text.split(ls);
    if (lines.size() >1) {
        int original_indent = _indent(this->get_text_line(line_from));
        text = QString(original_indent-_indent(lines[0]), ' ') + text;
    }

    int min_indent = 999;
    int current_indent = 0;
    lines = text.split(ls);
    for (int i = lines.size()-1; i >= 0; --i) {
        QString line = lines[i];
        if (!line.trimmed().isEmpty()) {
            current_indent = _indent(line);
            min_indent = qMin(current_indent, min_indent);
        }
        else
            lines[i] = QString(current_indent, ' ');
    }
    if (min_indent) {
        for (int i = 0; i < lines.size(); ++i) {
            lines[i] = lines[i].mid(min_indent);
        }
    }

    while (!lines.empty()) {
        QString first_line = lstrip(lines[0]);
        if (first_line=="" || first_line[0]=='#')
            lines.removeAt(0);
        else
            break;
    }

    QRegularExpression varname("[a-zA-Z0-9_]*");
    bool maybe = false;
    QStringList nextexcept;
    for (int n = 0; n < lines.size(); ++n) {
        if (!_indent(lines[n])) {
            QRegularExpressionMatch match = varname.match(lines[n]);
            QString word;
            if (match.capturedStart() == 0)
                word = match.captured();
            if (maybe && !nextexcept.contains(word)) {
                lines[n-1] += ls;
                maybe = false;
            }
            if (!word.isEmpty()) {
                if (word=="def" || word=="for" || word=="while"
                        || word=="with" || word=="class") {
                    maybe = true;
                    nextexcept.clear();
                }
                else if (word == "if") {
                    maybe = true;
                    nextexcept.clear();
                    nextexcept << "elif" << "else";
                }
                else if (word == "try") {
                    maybe = true;
                    nextexcept.clear();
                    nextexcept << "except" << "finally";
                }
            }
        }
    }
    if (maybe) {
        if (lines.back().trimmed() == "")
            lines.back() += ls;
        else
            lines.append(ls);
    }
    return lines.join(ls);
}

QString TextEditBaseWidget::__exec_cell()
{
    QTextCursor init_cursor = QTextCursor(this->textCursor());
    QPair<int,int> pair = this->__save_selection();
    int start_pos=pair.first, end_pos=pair.second;
    QPair<QTextCursor,bool> _pair = this->select_current_cell();
    QTextCursor cursor = _pair.first;
    bool whole_file_selected = _pair.second;
    if (!whole_file_selected)
        this->setTextCursor(cursor);
    QString text = this->get_selection_as_executable_code();
    this->last_cursor_cell = init_cursor;
    this->__restore_selection(start_pos, end_pos);
    if (!text.isEmpty())
        text = rstrip(text);
    return text;
}

QString TextEditBaseWidget::get_cell_as_executable_code()
{
    return this->__exec_cell();
}

QString TextEditBaseWidget::get_last_cell_as_executable_code()
{
    QString text;
    if (!this->last_cursor_cell.isNull()) {
        this->setTextCursor(this->last_cursor_cell);
        this->highlight_current_cell();
        text = this->__exec_cell();
    }
    return text;
}

bool TextEditBaseWidget::is_cell_separator(const QTextCursor &cursor, const QTextBlock &block)
{
    // Return True if cursor (or text block) is on a block separator
    // CodeEditor::set_language()中设置cell_separators=
    // {"#%%", "# %%", "# <codecell>", "# In["}

    //当光标点击超过当前行号的区域，下面这行代码会导致程序异常结束
    Q_ASSERT(!cursor.isNull() || block.isValid());
    // A null cursor is created by the default constructor.
    QString text;
    if (!cursor.isNull()) {
        QTextCursor cursor0 = QTextCursor(cursor);
        cursor0.select(QTextCursor::BlockUnderCursor);
        text = cursor0.selectedText();
    }
    else
        text = block.text();
    if (this->cell_separators.isEmpty())
        return false;
    else
        return startswith(lstrip(text),this->cell_separators);
}

QPair<QTextCursor,bool> TextEditBaseWidget::select_current_cell()
{
    // returns the textCursor and a boolean indicating if the
    // entire file is selected
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock);
    int cur_pos = cursor.position(), prev_pos = cursor.position();

    while (this->is_cell_separator(cursor)) {
        cursor.movePosition(QTextCursor::NextBlock);
        prev_pos = cur_pos;
        cur_pos = cursor.position();
        if (cur_pos == prev_pos)
            return qMakePair(cursor,false);
    }
    prev_pos = cur_pos;

    while (!this->is_cell_separator(cursor)) {
        cursor.movePosition(QTextCursor::PreviousBlock);
        prev_pos = cur_pos;
        cur_pos = cursor.position();
        if (cur_pos == prev_pos) {
            if (this->is_cell_separator(cursor))
                return qMakePair(cursor,false);
            else
                break;
        }
    }
    cursor.setPosition(prev_pos);
    bool cell_at_file_start = cursor.atStart();

    while (!this->is_cell_separator(cursor)) {
        cursor.movePosition(QTextCursor::NextBlock,
                            QTextCursor::KeepAnchor);
        cur_pos = cursor.position();
        if (cur_pos == prev_pos) {
            cursor.movePosition(QTextCursor::EndOfBlock,
                                QTextCursor::KeepAnchor);
            break;
        }
        prev_pos = cur_pos;
    }
    bool cell_at_file_end = cursor.atEnd();
    return qMakePair(cursor, cell_at_file_start && cell_at_file_end);
}

CursorBoolBool TextEditBaseWidget::select_current_cell_in_visible_portion()
{
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock);
    int cur_pos = cursor.position(), prev_pos = cursor.position();

    int beg_pos = this->cursorForPosition(QPoint(0,0)).position();
    QPoint bottom_right = QPoint(this->viewport()->width() - 1,
                                 this->viewport()->height() - 1);
    int end_pos = this->cursorForPosition(bottom_right).position();

    while (this->is_cell_separator(cursor)) {
        cursor.movePosition(QTextCursor::NextBlock);
        int prev_pos = cur_pos;
        cur_pos = cursor.position();
        if (cur_pos == prev_pos)
            return CursorBoolBool(cursor,false,false);
    }
    prev_pos = cur_pos;
    while (!this->is_cell_separator(cursor)
           && cursor.position() >= beg_pos) {
        cursor.movePosition(QTextCursor::PreviousBlock);
        prev_pos = cur_pos;
        cur_pos = cursor.position();
        if (cur_pos == prev_pos) {
            if (this->is_cell_separator(cursor))
                return CursorBoolBool(cursor,false,false);
            else
                break;
        }
    }
    bool cell_at_screen_start = cursor.position() <= beg_pos;
    cursor.setPosition(prev_pos);
    bool cell_at_file_start = cursor.atStart();
    if (!cell_at_file_start) {
        cursor.movePosition(QTextCursor::PreviousBlock);
        cursor.movePosition(QTextCursor::NextBlock,
                            QTextCursor::KeepAnchor);
    }

    while (!this->is_cell_separator(cursor)
           && cursor.position() <= end_pos) {
        cursor.movePosition(QTextCursor::NextBlock,
                            QTextCursor::KeepAnchor);
        cur_pos = cursor.position();
        if (cur_pos == prev_pos) {
            cursor.movePosition(QTextCursor::EndOfBlock,
                                QTextCursor::KeepAnchor);
            break;
        }
        prev_pos = cur_pos;
    }
    bool cell_at_file_end = cursor.atEnd();
    bool cell_at_screen_end = cursor.position() >= end_pos;
    return CursorBoolBool(cursor,
                          cell_at_file_start && cell_at_file_end,
                          cell_at_screen_start && cell_at_screen_end);
}

void TextEditBaseWidget::go_to_next_cell()
{
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::NextBlock);
    int cur_pos = cursor.position(), prev_pos = cursor.position();
    while (!this->is_cell_separator(cursor)) {
        cursor.movePosition(QTextCursor::NextBlock);
        prev_pos = cur_pos;
        cur_pos = cursor.position();
        if (cur_pos == prev_pos)
            return;
    }
    this->setTextCursor(cursor);
}

void TextEditBaseWidget::go_to_previous_cell()
{
    QTextCursor cursor = this->textCursor();
    int cur_pos = cursor.position(), prev_pos = cursor.position();

    if (this->is_cell_separator(cursor)) {
        cursor.movePosition(QTextCursor::PreviousBlock);
        cur_pos = prev_pos = cursor.position();
    }

    while (!this->is_cell_separator(cursor)) {
        cursor.movePosition(QTextCursor::PreviousBlock);
        prev_pos = cur_pos;
        cur_pos = cursor.position();
        if (cur_pos == prev_pos)
            return;
    }
    this->setTextCursor(cursor);
}

int TextEditBaseWidget::get_line_count()
{
    return blockCount();
}

QPair<int,int> TextEditBaseWidget::__save_selection()
{
    QTextCursor cursor = this->textCursor();
    return qMakePair(cursor.selectionStart(), cursor.selectionEnd());
}

void TextEditBaseWidget::__restore_selection(int start_pos, int end_pos)
{
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(start_pos);
    cursor.setPosition(end_pos,QTextCursor::KeepAnchor);
    this->setTextCursor(cursor);
}

void TextEditBaseWidget::__duplicate_line_or_selection(bool after_current_line)
{
    QTextCursor cursor = this->textCursor();
    cursor.beginEditBlock();
    QPair<int,int> pair = this->__save_selection();
    int start_pos=pair.first, end_pos=pair.second;
    if (!cursor.selectedText().isEmpty()) {
        cursor.setPosition(end_pos);
        cursor.movePosition(QTextCursor::StartOfBlock,
                            QTextCursor::KeepAnchor);
        if (cursor.selectedText().isEmpty()) {
            cursor.movePosition(QTextCursor::PreviousBlock);
            end_pos = cursor.position();
        }
    }

    cursor.setPosition(start_pos);
    cursor.movePosition(QTextCursor::StartOfBlock);
    while (cursor.position() <= end_pos) {
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        if (cursor.atEnd()) {
            QTextCursor cursor_temp = QTextCursor(cursor);
            cursor_temp.clearSelection();
            cursor_temp.insertText(this->get_line_separator());
            break;
        }
        cursor.movePosition(QTextCursor::NextBlock,QTextCursor::KeepAnchor);
    }
    QString text = cursor.selectedText();
    cursor.clearSelection();

    if (!after_current_line) {
        cursor.setPosition(start_pos);
        cursor.movePosition(QTextCursor::StartOfBlock);
        start_pos += text.size();
        end_pos += text.size();
    }

    cursor.insertText(text);
    cursor.endEditBlock();
    this->setTextCursor(cursor);
    this->__restore_selection(start_pos, end_pos);
}

void TextEditBaseWidget::duplicate_line()
{
    this->__duplicate_line_or_selection(true);
}

void TextEditBaseWidget::copy_line()
{
    this->__duplicate_line_or_selection(false);
}

void TextEditBaseWidget::__move_line_or_selection(bool after_current_line)
{
    QTextCursor cursor = this->textCursor();
    cursor.beginEditBlock();
    QPair<int,int> pair = this->__save_selection();
    int start_pos=pair.first, end_pos=pair.second;
    bool last_line = false;

    cursor.setPosition(start_pos);
    cursor.movePosition(QTextCursor::StartOfBlock);
    start_pos = cursor.position();

    cursor.setPosition(end_pos);
    if (!cursor.atBlockStart() || end_pos==start_pos) {
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextBlock);
    }
    end_pos = cursor.position();

    if (cursor.atEnd()) {
        if (!cursor.atBlockStart() || end_pos==start_pos)
            last_line = true;
    }

    cursor.setPosition(start_pos);
    if (cursor.atStart() && !after_current_line) {
        cursor.endEditBlock();
        this->setTextCursor(cursor);
        this->__restore_selection(start_pos, end_pos);
        return;
    }

    cursor.setPosition(end_pos, QTextCursor::KeepAnchor);
    if (last_line && after_current_line) {
        cursor.endEditBlock();
        this->setTextCursor(cursor);
        this->__restore_selection(start_pos, end_pos);
        return;
    }

    QString sel_text = cursor.selectedText();
    cursor.removeSelectedText();

    if (after_current_line) {
        QString text = cursor.block().text();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        sel_text = os::linesep + sel_text.chopped(1);
#else
		sel_text = os::linesep + sel_text.right(1);
#endif
        cursor.movePosition(QTextCursor::EndOfBlock);
        start_pos += text.size()+1;
        end_pos += text.size();
        if (!cursor.atEnd())
            end_pos += 1;
    }
    else {
        if (last_line) {
            cursor.deletePreviousChar();
            sel_text = sel_text + os::linesep;
            cursor.movePosition(QTextCursor::StartOfBlock);
            end_pos += 1;
        }
        else
            cursor.movePosition(QTextCursor::PreviousBlock);
        QString text = cursor.block().text();
        start_pos -= text.size()+1;
        end_pos -= text.size()+1;
    }

    cursor.insertText(sel_text);
    cursor.endEditBlock();
    this->setTextCursor(cursor);
    this->__restore_selection(start_pos, end_pos);
}

void TextEditBaseWidget::move_line_up()
{
    this->__move_line_or_selection(false);
}

void TextEditBaseWidget::move_line_down()
{
    this->__move_line_or_selection(true);
}

void TextEditBaseWidget::go_to_new_line()
{
    this->stdkey_end(false, false);
    this->insert_text(this->get_line_separator());
}

void TextEditBaseWidget::extend_selection_to_complete_lines()
{
    QTextCursor cursor = this->textCursor();
    int start_pos=cursor.selectionStart(), end_pos=cursor.selectionEnd();
    cursor.setPosition(start_pos);
    cursor.setPosition(end_pos,QTextCursor::KeepAnchor);
    if (cursor.atBlockStart()) {
        cursor.movePosition(QTextCursor::PreviousBlock,
                            QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock,
                            QTextCursor::KeepAnchor);
    }
    this->setTextCursor(cursor);
}

void TextEditBaseWidget::delete_line()
{
    QTextCursor cursor = this->textCursor();
    int start_pos, end_pos;
    if (this->has_selected_text()) {
        this->extend_selection_to_complete_lines();
        start_pos=cursor.selectionStart();
        end_pos=cursor.selectionEnd();
        cursor.setPosition(start_pos);
    }
    else {
        start_pos = cursor.position();
        end_pos = cursor.position();
    }
    cursor.beginEditBlock();
    cursor.setPosition(start_pos);
    cursor.movePosition(QTextCursor::StartOfBlock);
    while (cursor.position() <= end_pos) {
        cursor.movePosition(QTextCursor::EndOfBlock,QTextCursor::KeepAnchor);
        if (cursor.atEnd())
            break;
        cursor.movePosition(QTextCursor::NextBlock,QTextCursor::KeepAnchor);
    }
    cursor.removeSelectedText();
    cursor.endEditBlock();
    this->ensureCursorVisible();
}

void TextEditBaseWidget::set_selection(int start, int end)
{
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    this->setTextCursor(cursor);
}

void TextEditBaseWidget::truncate_selection(const QString& position_from)
{
    int _position_from = this->get_position(position_from);
    QTextCursor cursor = this->textCursor();
    int start=cursor.selectionStart(), end=cursor.selectionEnd();
    if (start < end)
        start = qMax(_position_from, start);
    else
        end = qMax(_position_from, end);
    this->set_selection(start, end);
}

void TextEditBaseWidget::truncate_selection(int position_from)
{
    int _position_from = this->get_position(position_from);
    QTextCursor cursor = this->textCursor();
    int start=cursor.selectionStart(), end=cursor.selectionEnd();
    if (start < end)
        start = qMax(_position_from, start);
    else
        end = qMax(_position_from, end);
    this->set_selection(start, end);
}

void TextEditBaseWidget::restrict_cursor_position(const QString& position_from,
                                                  const QString& position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    int cursor_position = cursor.position();
    if (cursor_position<_position_from || cursor_position>_position_to)
        this->set_cursor_position(_position_to);
}

void TextEditBaseWidget::restrict_cursor_position(const QString& position_from,
                                                  int position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    int cursor_position = cursor.position();
    if (cursor_position<_position_from || cursor_position>_position_to)
        this->set_cursor_position(_position_to);
}

void TextEditBaseWidget::restrict_cursor_position(int position_from,
                                                  const QString& position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    int cursor_position = cursor.position();
    if (cursor_position<_position_from || cursor_position>_position_to)
        this->set_cursor_position(_position_to);
}

void TextEditBaseWidget::restrict_cursor_position(int position_from,
                                                  int position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    int cursor_position = cursor.position();
    if (cursor_position<_position_from || cursor_position>_position_to)
        this->set_cursor_position(_position_to);
}


//------Code completion / Calltips
void TextEditBaseWidget::hide_tooltip_if_necessary(int key)
{
    try {
        QString calltip_char = this->get_character(this->calltip_position);
        bool before = this->is_cursor_before(this->calltip_position,1);
        bool other = key==Qt::Key_ParenRight || key==Qt::Key_Period || key==Qt::Key_Tab;
        if ((calltip_char!="?" || calltip_char!="(") || before || other)
            QToolTip::hideText();
    } catch (...) {
        QToolTip::hideText();
    }
}

void TextEditBaseWidget::show_completion_widget(const QList<QPair<QString, QString> > &textlist, bool automatic)
{
    this->completion_widget->show_list(textlist, automatic);
}

void TextEditBaseWidget::hide_completion_widget()
{
    this->completion_widget->hide();
}

static bool comp(const QPair<QString, QString>&pair1,const QPair<QString, QString>&pair2)
{
    if (pair1.first.toLower().compare(pair2.first.toLower()) <= 0)
        return true;
    return false;
}
void TextEditBaseWidget::show_completion_list(QList<QPair<QString, QString> > completions,
                                              const QString completion_text, bool automatic)
{
    if (completions.empty())
        return;
    if (completions[0].second.isNull()) {
        for (int i = 0; i < completions.size(); ++i) {
            completions[i].second = "";
        }
    }
    if (completions.size()==1 && completions[0].first==completion_text)
        return;
    this->completion_text = completion_text;

    QSet<QPair<QString, QString>> underscore;
    foreach (auto pair, completions) {
        if (pair.first.startsWith('_'))
            underscore.insert(pair);
    }

    completions = (QSet<QPair<QString, QString>>::fromList(completions) - underscore).toList();
    std::sort(completions.begin(),completions.end(), comp);
    QList<QPair<QString, QString>> underscore_list = underscore.toList();
    std::sort(underscore_list.begin(),underscore_list.end(), comp);
    completions.append(underscore_list);
    this->show_completion_widget(completions, automatic);
}

void TextEditBaseWidget::select_completion_list()
{
    this->completion_widget->item_selected();
}

void TextEditBaseWidget::insert_completion(const QString &text)
{
    if (!text.isEmpty()) {
        QTextCursor cursor = this->textCursor();
        cursor.movePosition(QTextCursor::PreviousCharacter,
                            QTextCursor::KeepAnchor,
                            this->completion_text.size());
        cursor.removeSelectedText();
        this->insert_text(text);
    }
}

bool TextEditBaseWidget::is_completion_widget_visible()
{
    return this->completion_widget->isVisible();
}


//------Standard keys
void TextEditBaseWidget::stdkey_clear()
{
    if (!this->has_selected_text())
        this->moveCursor(QTextCursor::NextCharacter,QTextCursor::KeepAnchor);
    this->remove_selected_text();
}

void TextEditBaseWidget::stdkey_backspace()
{
    if (!this->has_selected_text())
        this->moveCursor(QTextCursor::PreviousCharacter,
                         QTextCursor::KeepAnchor);
    this->remove_selected_text();
}

QTextCursor::MoveMode TextEditBaseWidget::__get_move_mode(bool shift)
{
    if (shift)
        return QTextCursor::KeepAnchor;
    else
        return QTextCursor::MoveAnchor;
}

void TextEditBaseWidget::stdkey_up(bool shift)
{
    this->moveCursor(QTextCursor::Up, this->__get_move_mode(shift));
}

void TextEditBaseWidget::stdkey_down(bool shift)
{
    this->moveCursor(QTextCursor::Down, this->__get_move_mode(shift));
}

void TextEditBaseWidget::stdkey_tab()
{
    this->insert_text(this->indent_chars);
}

void TextEditBaseWidget::stdkey_home(bool shift, bool ctrl,
                                     const QString& prompt_pos)
{
    QTextCursor::MoveMode move_mode = this->__get_move_mode(shift);
    if (ctrl)
        this->moveCursor(QTextCursor::Start, move_mode);
    else {
        QTextCursor cursor = this->textCursor();
        int start_position;
        if (prompt_pos.isEmpty())
            start_position = this->get_position("sol");
        else
            start_position = this->get_position(prompt_pos);
        QString text = this->get_text(start_position,"eol");
        int indent_pos = start_position+text.size()-lstrip(text).size();
        if (cursor.position() != indent_pos)
            cursor.setPosition(indent_pos, move_mode);
        else
            cursor.setPosition(start_position, move_mode);
        this->setTextCursor(cursor);
    }
}

void TextEditBaseWidget::stdkey_end(bool shift, bool ctrl)
{
    QTextCursor::MoveMode move_mode = this->__get_move_mode(shift);
    if (ctrl)
        this->moveCursor(QTextCursor::End, move_mode);
    else
        this->moveCursor(QTextCursor::EndOfBlock, move_mode);
}


//----Qt Events
void TextEditBaseWidget::mousePressEvent(QMouseEvent *event)
{
#ifdef Q_OS_UNIX
    ;
#else
    this->calltip_widget->hide();
    QPlainTextEdit::mousePressEvent(event);
#endif
}

void TextEditBaseWidget::focusInEvent(QFocusEvent *event)
{
    emit this->focus_changed();
    emit this->focus_in();
    this->highlight_current_cell();
    QPlainTextEdit::focusInEvent(event);
}

void TextEditBaseWidget::focusOutEvent(QFocusEvent *event)
{
    emit this->focus_changed();
    QPlainTextEdit::focusOutEvent(event);
}

void TextEditBaseWidget::wheelEvent(QWheelEvent *event)
{
#ifndef Q_OS_MAC
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() < 0)
            emit zoom_out();
        else if (event->angleDelta().y() > 0)
            emit zoom_in();
        return;
    }
#endif
    QPlainTextEdit::wheelEvent(event);
    this->highlight_current_cell();
}


/********** QtANSIEscapeCodeHandler**********/
QtANSIEscapeCodeHandler::QtANSIEscapeCodeHandler()
    : ANSIEscapeCodeHandler ()
{
    this->base_format = QTextCharFormat();
    this->current_format = QTextCharFormat();
}

void QtANSIEscapeCodeHandler::set_light_background(bool state)
{
    if (state) {
        this->default_foreground_color = 30;
        this->default_background_color = 47;
    }
    else {
        this->default_foreground_color = 37;
        this->default_background_color = 40;
    }
}

void QtANSIEscapeCodeHandler::set_base_format(const QTextCharFormat &base_format)
{
    this->base_format = base_format;
}

QTextCharFormat QtANSIEscapeCodeHandler::get_format() const
{
    return this->current_format;
}

void QtANSIEscapeCodeHandler::set_style()
{
    if (this->current_format.isEmpty()) {
        Q_ASSERT(!this->base_format.isEmpty());
        this->current_format = QTextCharFormat(this->base_format);
    }

    QBrush qcolor;
    if (this->foreground_color != -1)
        qcolor = this->base_format.foreground();
    else {
        QString cstr = this->ANSI_COLORS[this->foreground_color-30][this->intensity];
        qcolor = QColor(cstr);
    }
    this->current_format.setForeground(qcolor);

    if (this->background_color != -1)
        qcolor = this->base_format.background();
    else {
        QString cstr = this->ANSI_COLORS[this->background_color-40][this->intensity];
        qcolor = QColor(cstr);
    }
    this->current_format.setBackground(qcolor);

    QFont font = this->current_format.font();
    font.setItalic(this->italic);
    font.setBold(this->bold);
    font.setUnderline(this->underline);
    this->current_format.setFont(font);
}

/********** ConsoleFontStyle **********/
void inverse_color(QColor* color)
{
    color->setHsv(color->hue(), color->saturation(), 255-color->value());
}

ConsoleFontStyle::ConsoleFontStyle(const QString& foregroundcolor,const QString&  backgroundcolor,
                                   bool bold,bool italic,bool underline)
{
    this->foregroundcolor = foregroundcolor;
    this->backgroundcolor = backgroundcolor;
    this->bold = bold;
    this->italic = italic;
    this->underline = underline;
}

void ConsoleFontStyle::apply_style(QFont font,bool light_background,bool is_default)
{
    this->format = QTextCharFormat();
    this->format.setFont(font);
    QColor foreground = QColor(this->foregroundcolor);
    if (!light_background && is_default)
        inverse_color(&foreground);
    this->format.setForeground(foreground);
    QColor background = QColor(this->backgroundcolor);
    if (!light_background)
        inverse_color(&background);
    this->format.setBackground(background);
    font = this->format.font();
    font.setBold(this->bold);
    font.setItalic(this->italic);
    font.setUnderline(this->underline);
    this->format.setFont(font);
}


/********** ConsoleBaseWidget **********/
ConsoleBaseWidget::ConsoleBaseWidget(QWidget* parent)
    : TextEditBaseWidget (parent)
{
    this->BRACE_MATCHING_SCOPE = QPair<QString,QString>("sol", "eol");
    this->COLOR_PATTERN = QRegularExpression("\\x01?\\x1b\\[(.*?)m\\x02?");

    this->light_background = true;
    this->setMaximumBlockCount(300);
    this->ansi_handler = new QtANSIEscapeCodeHandler;
    this->setUndoRedoEnabled(false);
    connect(this, &ConsoleBaseWidget::userListActivated,
            [=](int, QString text){emit this->completion_widget_activated(text);});

    this->default_style = new ConsoleFontStyle("#000000", "#FFFFFF",
                                               false, false, false);
    this->error_style = new ConsoleFontStyle("#FF0000", "#FFFFFF",
                                               false, false, false);
    this->traceback_link_style = new ConsoleFontStyle("#0000FF", "#FFFFFF",
                                               true, false, true);
    this->prompt_style = new ConsoleFontStyle("#00AA00", "#FFFFFF",
                                               true, false, false);
    this->font_styles.clear();
    this->font_styles << this->default_style << this->error_style
                      << this->traceback_link_style << this->prompt_style;
    this->set_pythonshell_font();
    this->setMouseTracking(true);
}

void ConsoleBaseWidget::set_light_background(bool state)
{
    this->light_background = state;
    if (state)
        this->set_palette(QColor(Qt::white), QColor(Qt::darkGray));
    else
        this->set_palette(QColor(Qt::black), QColor(Qt::lightGray));
    this->ansi_handler->set_light_background(state);
    this->set_pythonshell_font();
}

void ConsoleBaseWidget::insert_text(const QString &text)
{
    this->textCursor().insertText(text, this->default_style->format);
}

void ConsoleBaseWidget::paste()
{
    if (this->has_selected_text())
        this->remove_selected_text();
    this->insert_text(QApplication::clipboard()->text());
}

void ConsoleBaseWidget::append_text_to_shell(QString text, bool error, bool prompt)
{
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (text.contains('\r')) {
        text.replace("\r\n", "\n");
        text.replace('\r', '\n');
    }
    while (true) {
        // QChar(12) = '\x0c'
        int index = text.indexOf(QChar(12));
        if (index == -1)
            break;
        text = text.mid(index+1);
        this->clear();
    }
    if (error) {
        bool is_traceback = false;
        foreach (QString tmp, splitlines(text, true)) {
            if (tmp.startsWith("  File") && !tmp.startsWith("  File \"<")) {
                is_traceback = true;
                cursor.insertText("  ", this->default_style->format);
                cursor.insertText(tmp.mid(2), this->traceback_link_style->format);
            }
            else
                cursor.insertText(tmp, this->error_style->format);
        }
        emit exception_occurred(text, is_traceback);
    }
    else if (prompt)
        insert_text_to(&cursor, text, this->prompt_style->format);
    else {
        int last_end = 0;
        QRegularExpressionMatchIterator iterator = this->COLOR_PATTERN.globalMatch(text);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
            insert_text_to(&cursor, text.mid(last_end, match.capturedStart()-last_end),
                           this->default_style->format);
            last_end = match.capturedEnd();
            foreach (QString _c, match.captured(1).split(';')) {
                bool ok;
                int code = _c.toInt(&ok);
                if (ok)
                    this->ansi_handler->set_code(code);
                else
                    break;
            }
            this->default_style->format = this->ansi_handler->get_format();
        }
        insert_text_to(&cursor, text.mid(last_end), this->default_style->format);
    }

    this->set_cursor_position("eof");
    this->setCurrentCharFormat(this->default_style->format);
}

void ConsoleBaseWidget::set_pythonshell_font(const QFont &font)
{
    foreach (ConsoleFontStyle* style, this->font_styles) {
        style->apply_style(font, this->light_background, style == this->default_style);
    }
    this->ansi_handler->set_base_format(this->default_style->format);
}
