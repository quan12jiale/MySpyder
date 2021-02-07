#include "control.h"

ControlCallTip::ControlCallTip(ControlWidget* text_edit, bool hide_timer_on)
    : QLabel (nullptr, Qt::ToolTip)
{
    this->app = qobject_cast<QApplication*>(QCoreApplication::instance());

    this->hide_timer_on = hide_timer_on;
    this->tip = QString();
    this->_hide_timer = new QBasicTimer;
    this->_text_edit = text_edit;

    this->setFont(text_edit->document()->defaultFont());
    this->setForegroundRole(QPalette::ToolTipText);
    this->setBackgroundRole(QPalette::ToolTipBase);
    this->setPalette(QToolTip::palette());

    this->setAlignment(Qt::AlignLeft);
    this->setIndent(1);
    this->setFrameStyle(QFrame::NoFrame);
    this->setMargin(1 + this->style()->pixelMetric(
            QStyle::PM_ToolTipLabelFrameWidth, nullptr, this));
}

bool ControlCallTip::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == _text_edit) {
        QEvent::Type etype = event->type();

        if (etype == QEvent::KeyPress) {
            QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
            int key = keyEvent->key();
            QTextCursor cursor = _text_edit->textCursor();
            QChar prev_char = _text_edit->get_character(cursor.position(),-1);

            if (key==Qt::Key_Enter || key==Qt::Key_Return ||
                    key==Qt::Key_Down || key==Qt::Key_Up)
                hide();
            else if (key==Qt::Key_Escape) {
                hide();
                return true;
            }
            else if (prev_char == ')')
                hide();
        }

        else if (etype == QEvent::FocusOut)
            hide();

        else if (etype == QEvent::Enter) {
            if (_hide_timer->isActive() &&
                    app->topLevelAt(QCursor::pos())==this)
                _hide_timer->stop();
        }

        else if (etype == QEvent::Leave)
            this->_leave_event_hide();
    }

    return QLabel::eventFilter(obj, event);
}

void ControlCallTip::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == _hide_timer->timerId()) {
        _hide_timer->stop();
        hide();
    }
}

void ControlCallTip::enterEvent(QEvent *event)
{
    QLabel::enterEvent(event);
    if (_hide_timer->isActive() &&
            app->topLevelAt(QCursor::pos())==this)
        _hide_timer->stop();
}

void ControlCallTip::hideEvent(QHideEvent *event)
{
    QLabel::hideEvent(event);
    disconnect(_text_edit,SIGNAL(cursorPositionChanged()),
               this,SLOT(_cursor_position_changed()));
    _text_edit->removeEventFilter(this);
}

void ControlCallTip::leaveEvent(QEvent *event)
{
    QLabel::leaveEvent(event);
    _leave_event_hide();
}

void ControlCallTip::mousePressEvent(QMouseEvent *event)
{
    QLabel::mousePressEvent(event);
    hide();
}

void ControlCallTip::paintEvent(QPaintEvent *event)
{
    QStylePainter painter(this);
    QStyleOptionFrame option;
    option.initFrom(this);
    painter.drawPrimitive(QStyle::PE_PanelTipLabel, option);
    painter.end();

    QLabel::paintEvent(event);
}

void ControlCallTip::setFont(const QFont &font)
{
    QLabel::setFont(font);
}

void ControlCallTip::showEvent(QShowEvent *event)
{
    QLabel::showEvent(event);
    connect(_text_edit,SIGNAL(cursorPositionChanged()),
            this,SLOT(_cursor_position_changed()));
    _text_edit->installEventFilter(this);
}

void ControlCallTip::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    hide();
}

bool ControlCallTip::show_tip(QPoint point, const QString &tip, QStringList wrapped_tiplines)
{
    if (isVisible()) {
        if (this->tip == tip)
            return true;
        else
            hide();
    }

    ControlWidget* text_edit = _text_edit;
    QTextCursor cursor = text_edit->textCursor();
    int search_pos = cursor.position()-1;
    _start_position = _find_parenthesis(search_pos,false).first;

    if (_start_position == -1)
        return false;

    if (hide_timer_on) {
        _hide_timer->stop();

        int hide_time;
        if (wrapped_tiplines.size() == 1) {
            QString args = wrapped_tiplines[0].split('(')[1];
            int nargs = (args.split(',')).size();
            if (nargs == 1)
                hide_time = 1400;
            else if (nargs == 2)
                hide_time = 1600;
            else
                hide_time = 1800;
        }
        else if (wrapped_tiplines.size() == 2) {
            QString args1 = wrapped_tiplines[1].trimmed();
            int nargs1 = (args1.split(',')).size();
            if (nargs1 == 1)
                hide_time = 2500;
            else
                hide_time = 2800;
        }
        else
            hide_time = 3500;
        _hide_timer->start(hide_time, this);
    }

    this->tip = tip;
    setText(tip);
    resize(sizeHint());

    int padding = 3;
    QRect cursor_rect = text_edit->cursorRect(cursor);
    QRect screen_rect = app->desktop()->screenGeometry(text_edit);
    point.setY(point.y()+padding);
    int tip_height = size().height();
    int tip_width = size().width();

    QString vertical = "bottom";
    QString horizontal = "Right";
    if (point.y()+tip_height > screen_rect.height()+screen_rect.y()) {
        QPoint point_ = text_edit->mapToGlobal(cursor_rect.topRight());
        if (point_.y() - tip_height < padding) {
            if (2*point.y() < screen_rect.height())
                vertical = "bottom";
            else
                vertical = "top";
        }
        else
            vertical = "top";
    }
    if (point.x()+tip_width > screen_rect.width()+screen_rect.x()) {
        QPoint point_ = text_edit->mapToGlobal(cursor_rect.topRight());
        if (point_.x() - tip_width < padding) {
            if (2*point.x() < screen_rect.width())
                horizontal = "Right";
            else
                horizontal = "Left";
        }
        else
            horizontal = "Left";
    }

    QPoint pos;
    if (vertical=="top" && horizontal=="Left")
        pos = cursor_rect.topLeft();
    else if (vertical=="top" && horizontal=="Right")
        pos = cursor_rect.topRight();
    else if (vertical=="bottom" && horizontal=="Left")
        pos = cursor_rect.bottomLeft();
    else if (vertical=="bottom" && horizontal=="Right")
        pos = cursor_rect.bottomRight();

    QPoint adjusted_point = text_edit->mapToGlobal(pos);
    if (vertical == "top")
        point.setY(adjusted_point.y() - tip_height - padding);
    if (horizontal == "Left")
        point.setX(adjusted_point.x() - tip_width - padding);

    move(point);
    show();
    return true;
}

QPair<int,int> ControlCallTip::_find_parenthesis(int position, bool forward)
{
    int commas=0, depth = 0;
    QTextDocument* document = _text_edit->document();
    QChar _char = document->characterAt(position);

    int breakflag = 1;
    while (_char.category()!=QChar::Other_Control && position > 0) {
        if (_char == ',' && depth == 0)
            commas++;
        else if (_char == ')') {
            if (forward && depth == 0) {
                breakflag = 0;
                break;
            }
            depth++;
        }
        else if (_char == '(') {
            if (!forward && depth == 0) {
                breakflag = 0;
                break;
            }
            depth--;
        }
        position += (forward) ? 1 : -1;
        _char = document->characterAt(position);
    }
    if (breakflag)
        position--;
    return qMakePair(position, commas);
}

void ControlCallTip::_leave_event_hide()
{
    if (hide_timer_on && !_hide_timer->isActive() &&
            app->topLevelAt(QCursor::pos())!=this)
        _hide_timer->start(800, this);
}

void ControlCallTip::_cursor_position_changed()
{
    QTextCursor cursor = _text_edit->textCursor();
    int position = cursor.position();
    QTextDocument* document = _text_edit->document();
    QChar _char = document->characterAt(position-1);
    if (position <= _start_position)
        hide();
    else if (_char == ')') {
        int pos = _find_parenthesis(position-1,false).first;
        if (pos == -1)
            hide();
    }
}

ControlWidget::ControlWidget(QWidget *parent)
    : QTextEdit (parent)
{
    // BaseEditMixin
    this->eol_chars = QString();
    this->calltip_size = 600;

    // TracebackLinksMixin
    this->__cursor_changed = false;
    this->setMouseTracking(true);

    // GetHelpMixin
    // help;
    this->help_enabled = false;

    // BrowseHistoryMixin
    this->history = QStringList();
    this->histidx = -1;
    this->hist_wholeline = false;

    // ControlWidget
    this->calltip_widget = new ControlCallTip(this, false);
    //this->found_results = [];
    this->calltips = false;
}

// BaseEditMixin
void ControlWidget::set_eol_chars(const QString &text)
{
    QString eol_chars = sourcecode::get_eol_chars(text);
    bool is_document_modified = !eol_chars.isEmpty() && !this->eol_chars.isEmpty();
    this->eol_chars = eol_chars;
    if (is_document_modified) {
        this->document()->setModified(true);
        emit sig_eol_chars_changed(eol_chars);
    }
}

QString ControlWidget::get_line_separator() const
{
    if (!this->eol_chars.isEmpty())
        return this->eol_chars;
    else
        return os::linesep;
}

QString ControlWidget::get_text_with_eol() const
{
    QString utext = this->toPlainText();
    // 这里没有splitlines()然后join()，感觉这样容易出bug
    QString linesep = this->get_line_separator();
    if (utext.endsWith('\n'))
        utext += linesep;
    return utext;
}

int ControlWidget::get_position(const QString& subject) const
{
    QTextCursor cursor = textCursor();
    if (subject == "cursor")
        return cursor.position();
    else if (subject == "sol")
        cursor.movePosition(QTextCursor::StartOfBlock);
    else if (subject == "eol")
        cursor.movePosition(QTextCursor::EndOfBlock);
    else if (subject == "eof")
        cursor.movePosition(QTextCursor::End);
    else if (subject == "sof")
        cursor.movePosition(QTextCursor::Start);
    return cursor.position();
}

int ControlWidget::get_position(int subject) const
{
    return subject;
}

QPoint ControlWidget::get_coordinates(const QString& position) const
{
    int _position = get_position(position);
    QTextCursor cursor = textCursor();
    cursor.setPosition(_position);
    QPoint point = this->cursorRect(cursor).center();
    return point;
}

QPoint ControlWidget::get_coordinates(int position) const
{
    int _position = get_position(position);
    QTextCursor cursor = textCursor();
    cursor.setPosition(_position);
    QPoint point = this->cursorRect(cursor).center();
    return point;
}

QPair<int,int> ControlWidget::get_cursor_line_column() const
{
    QTextCursor cursor = this->textCursor();
    return qMakePair(cursor.blockNumber(), cursor.columnNumber());
}

int ControlWidget::get_cursor_line_number() const
{
    return textCursor().blockNumber()+1;
}

void ControlWidget::set_cursor_position(const QString &position)
{
    int _position = this->get_position(position);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position);
    this->setTextCursor(cursor);
    this->ensureCursorVisible();
}

void ControlWidget::set_cursor_position(int position)
{
    int _position = this->get_position(position);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position);
    this->setTextCursor(cursor);
    this->ensureCursorVisible();
}

void ControlWidget::move_cursor(int chars)
{
    QTextCursor::MoveOperation direction = (chars>0) ? QTextCursor::Right : QTextCursor::Left;
    for (int i=0;i<qAbs(chars);i++)
        this->moveCursor(direction,QTextCursor::MoveAnchor);
}

bool ControlWidget::is_cursor_on_first_line() const
{
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock);
    return cursor.atStart();
}

bool ControlWidget::is_cursor_on_last_line() const
{
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::EndOfBlock);
    return cursor.atEnd();
}

bool ControlWidget::is_cursor_at_end() const
{
    return this->textCursor().atEnd();
}

//"""Return True if cursor is before *position*"""
bool ControlWidget::is_cursor_before(const QString &position, int char_offset) const
{
    int _position = this->get_position(position) + char_offset;
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (_position < cursor.position()) {
        cursor.setPosition(_position);
        return this->textCursor() < cursor;//QTextCursor类提供了<运算符,就是比较光标位置
    }
    return true;
}

bool ControlWidget::is_cursor_before(int position, int char_offset) const
{
    int _position = this->get_position(position) + char_offset;
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (_position < cursor.position()) {
        cursor.setPosition(_position);
        return this->textCursor() < cursor;
    }
    return true;
}

void ControlWidget::__move_cursor_anchor(QString what, QString direction,
                                         QTextCursor::MoveMode move_mode)
{
    Q_ASSERT(what == "character" || what == "word" || what == "line");
    if (what == "character") {
        if (direction == "left")
            this->moveCursor(QTextCursor::PreviousCharacter, move_mode);
        else if (direction == "right")
            this->moveCursor(QTextCursor::NextCharacter, move_mode);
    }
    else if (what == "word") {
        if (direction == "left")
            this->moveCursor(QTextCursor::PreviousWord, move_mode);
        else if (direction == "right")
            this->moveCursor(QTextCursor::NextWord, move_mode);
    }
    else if (what == "line") {
        if (direction == "down")
            this->moveCursor(QTextCursor::NextBlock, move_mode);
        else if (direction == "up")
            this->moveCursor(QTextCursor::PreviousBlock, move_mode);
    }
}

void ControlWidget::move_cursor_to_next(QString what, QString direction)
{
    this->__move_cursor_anchor(what, direction, QTextCursor::MoveAnchor);
}

void ControlWidget::clear_selection()
{
    QTextCursor cursor = this->textCursor();
    cursor.clearSelection();
    this->setTextCursor(cursor);
}

void ControlWidget::extend_selection_to_next(QString what, QString direction)
{
    this->__move_cursor_anchor(what,direction,QTextCursor::KeepAnchor);
}


QTextCursor ControlWidget::__select_text(const QString& position_from,
                                         const QString& position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position_from);
    cursor.setPosition(_position_to,QTextCursor::KeepAnchor);
    return cursor;
}

QTextCursor ControlWidget::__select_text(const QString& position_from,
                                         int position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position_from);
    cursor.setPosition(_position_to,QTextCursor::KeepAnchor);
    return cursor;
}

QTextCursor ControlWidget::__select_text(int position_from,
                                         const QString& position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position_from);
    cursor.setPosition(_position_to,QTextCursor::KeepAnchor);
    return cursor;
}

QTextCursor ControlWidget::__select_text(int position_from,
                                         int position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position_from);
    cursor.setPosition(_position_to,QTextCursor::KeepAnchor);
    return cursor;
}

QString ControlWidget::get_text_line(int line_nb)
{
    try {
        // TODO源码是self.toPlainText().splitlines()
        return splitlines(this->toPlainText())[line_nb];
    }
    catch (...) {
        return this->get_line_separator();
    }
}

/*
 * Return text between *position_from* and *position_to*
   Positions may be positions or 'sol', 'eol', 'sof', 'eof' or 'cursor'
*/
QString ControlWidget::get_text(const QString& position_from,
                                const QString& position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    QString text = cursor.selectedText();
    bool all_text = (position_from=="sof") && (position_to=="eof");
    if (!text.isEmpty() && !all_text) {
        while (text.endsWith("\n"))
            text.chop(1);
        // TODO源码是while text.endswith(u"\u2029"):
        //              text = text[:-1]
        while (text.endsWith("\u2029"))
            text.chop(1);
    }
    return text;
}

QString ControlWidget::get_text(const QString& position_from,
                                int position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    QString text = cursor.selectedText();
    if (!text.isEmpty()) {
        while (text.endsWith("\n"))
            text.chop(1);
        // TODO源码是while text.endswith(u"\u2029"):
        //              text = text[:-1]
        while (text.endsWith("\u2029"))
            text.chop(1);
    }
    return text;
}

QString ControlWidget::get_text(int position_from,
                                const QString& position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    QString text = cursor.selectedText();
    if (!text.isEmpty()) {
        while (text.endsWith("\n"))
            text.chop(1);
        // TODO源码是while text.endswith(u"\u2029"):
        //              text = text[:-1]
        while (text.endsWith("\u2029"))
            text.chop(1);
    }
    return text;
}

QString ControlWidget::get_text(int position_from,
                                int position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    QString text = cursor.selectedText();
    if (!text.isEmpty()) {
        while (text.endsWith("\n"))
            text.chop(1);
        // TODO源码是while text.endswith(u"\u2029"):
        //              text = text[:-1]
        while (text.endsWith("\u2029"))
            text.chop(1);
    }
    return text;
}

QChar ControlWidget::get_character(const QString& position, int offset)
{
    int _position = this->get_position(position) + offset;
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (_position < cursor.position()) {
        cursor.setPosition(_position);
        cursor.movePosition(QTextCursor::Right,
                            QTextCursor::KeepAnchor);
        return cursor.selectedText()[0];
    }
    else
        return QChar();
}

QChar ControlWidget::get_character(int position, int offset)
{
    int _position = this->get_position(position) + offset;
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (_position < cursor.position()) {
        cursor.setPosition(_position);
        cursor.movePosition(QTextCursor::Right,
                            QTextCursor::KeepAnchor);
        return cursor.selectedText()[0];
    }
    else
        return QChar();
}

void ControlWidget::insert_text(const QString &text)
{
    if (!this->isReadOnly())
        this->textCursor().insertText(text);
}

void ControlWidget::replace_text(const QString& position_from,
                                 const QString& position_to, const QString &text)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
    cursor.insertText(text);
}

void ControlWidget::replace_text(const QString& position_from,
                                 int position_to, const QString &text)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
    cursor.insertText(text);
}

void ControlWidget::replace_text(int position_from,
                                 const QString& position_to, const QString &text)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
    cursor.insertText(text);
}

void ControlWidget::replace_text(int position_from,
                                 int position_to, const QString &text)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
    cursor.insertText(text);
}

void ControlWidget::remove_text(const QString& position_from,
                                const QString& position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
}

void ControlWidget::remove_text(const QString& position_from,
                                int position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
}

void ControlWidget::remove_text(int position_from,
                                const QString& position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
}

void ControlWidget::remove_text(int position_from,
                                int position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
}

bool ControlWidget::is_space(QTextCursor::MoveOperation move)
{
    QTextCursor curs = this->textCursor();
    curs.movePosition(move, QTextCursor::KeepAnchor);
    return curs.selectedText().trimmed().isEmpty();
}
QString ControlWidget::get_current_word()
{
    QTextCursor cursor = this->textCursor();
    if (cursor.hasSelection()) {
        cursor.setPosition(qMin(cursor.selectionStart(),
                                cursor.selectionEnd()));
    }
    else {
        if (is_space(QTextCursor::NextCharacter)) {
            if (is_space(QTextCursor::PreviousCharacter))
                return QString();
            cursor.movePosition(QTextCursor::WordLeft);
        }
    }

    cursor.select(QTextCursor::WordUnderCursor);
    QString text = cursor.selectedText();

    QRegularExpression rule("([^\\d\\W]\\w*)", QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpressionMatchIterator iterator = rule.globalMatch(text);
    if (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        return match.captured();
    }

    return QString();
}

QString ControlWidget::get_current_line()
{
    QTextCursor cursor = this->textCursor();
    cursor.select(QTextCursor::BlockUnderCursor);
    return cursor.selectedText();
}

QString ControlWidget::get_current_line_to_cursor()
{
    return this->get_text(this->current_prompt_pos, "cursor");
}

int ControlWidget::get_line_number_at(const QPoint& coordinates)
{
    QTextCursor cursor = this->cursorForPosition(coordinates);
    return cursor.blockNumber()-1;
}

QString ControlWidget::get_line_at(const QPoint &coordinates)
{
    QTextCursor cursor = this->cursorForPosition(coordinates);
    cursor.select(QTextCursor::BlockUnderCursor);
    return cursor.selectedText().replace("\u2029","");
}

QString ControlWidget::get_word_at(const QPoint &coordinates)
{
    QTextCursor cursor = this->cursorForPosition(coordinates);
    cursor.select(QTextCursor::WordUnderCursor);
    return cursor.selectedText();
}

int ControlWidget::get_block_indentation(int block_nb)
{
    QString text = this->document()->findBlockByNumber(block_nb).text();
    QString after(this->tab_stop_width_spaces,' ');
    text.replace("\t",after);
    return text.size() - lstrip(text).size();
}

QPair<int,int> ControlWidget::get_selection_bounds()
{
    QTextCursor cursor = this->textCursor();
    int start=cursor.selectionStart(), end=cursor.selectionEnd();
    QTextBlock block_start = this->document()->findBlock(start);
    QTextBlock block_end = this->document()->findBlock(end);
    if (block_start.blockNumber() <= block_end.blockNumber())
        return qMakePair(block_start.blockNumber(), block_end.blockNumber());
    else
        return qMakePair(block_end.blockNumber(), block_start.blockNumber());
}

bool ControlWidget::has_selected_text()
{
    QString text = this->textCursor().selectedText();
    if (text.isEmpty())
        return false;
    else
        return true;
}

QString ControlWidget::get_selected_text()
{
    // \u2029是unicode编码的段落分割符
    return this->textCursor().selectedText().replace("\u2029",
                                                     this->get_line_separator());
}

void ControlWidget::remove_selected_text()
{
    this->textCursor().removeSelectedText();
}

void ControlWidget::replace(QString text, const QString &pattern)
{
    QTextCursor cursor = this->textCursor();
    cursor.beginEditBlock();
    QString seltxt;
    if (!pattern.isEmpty())
        seltxt = cursor.selectedText();
    cursor.removeSelectedText();
    if (!pattern.isEmpty()) {
        QRegularExpression re(pattern);
        text = seltxt.replace(re, text);
    }
    cursor.insertText(text);
    cursor.endEditBlock();
}

QTextCursor ControlWidget::find_multiline_pattern(const QRegularExpression& regexp,const QTextCursor& cursor,
                                                   QTextDocument::FindFlags findflag)
{
    QString pattern = regexp.pattern();
    QString text = toPlainText();
    QRegularExpression regobj(pattern);
    if (!regobj.isValid())
        return QTextCursor();

    QRegularExpressionMatch match;
    if (findflag & QTextDocument::FindBackward) {
        int offset = qMin(cursor.selectionEnd(), cursor.selectionStart());
        text = text.left(offset);
        QList<QRegularExpressionMatch> matches;
        QRegularExpressionMatchIterator iterator = regobj.globalMatch(text.left(offset));
        while (iterator.hasNext())
            matches.append(iterator.next());
        if (!matches.empty())
            match = matches.last();
        else
            return QTextCursor();
    }
    else {
        int offset = qMax(cursor.selectionEnd(), cursor.selectionStart());
        match = regobj.match(text,offset);
    }
    if (match.isValid()) {
        int pos1 = match.capturedStart();
        int pos2 = match.capturedEnd();
        QTextCursor fcursor = this->textCursor();
        fcursor.setPosition(pos1);
        fcursor.setPosition(pos2, QTextCursor::KeepAnchor);
        return fcursor;
    }
    return QTextCursor();
}

bool ControlWidget::find_text(QString text, bool changed, bool forward,
                              bool _case, bool words, bool regexp)
{
    QTextCursor cursor = this->textCursor();
    QTextDocument::FindFlags findflag = QTextDocument::FindFlag();

    if (!forward)
        findflag = findflag | QTextDocument::FindBackward;
    if (_case)
        findflag = findflag | QTextDocument::FindCaseSensitively;

    QList<QTextCursor::MoveOperation> moves = {QTextCursor::NoMove};
    if (forward) {
        moves.append(QTextCursor::NextWord);
        moves.append(QTextCursor::Start);
        if (changed) {
            if (!cursor.selectedText().isEmpty()) {
                int new_position = qMin(cursor.selectionStart(),
                                        cursor.selectionEnd());
                cursor.setPosition(new_position);
            }
            else
                cursor.movePosition(QTextCursor::PreviousWord);
        }
    }
    else
        moves.append(QTextCursor::End);

    if (!regexp)
        text = QRegularExpression::escape(text);

    QRegularExpression pattern((words ? QString("\\b%1\\b").arg(text) : text));
    if (_case)
        pattern.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

    foreach (QTextCursor::MoveOperation move, moves) {
        cursor.movePosition(move);
        QTextCursor found_cursor;
        if (regexp && text.contains("\\n")) {
            found_cursor = this->find_multiline_pattern(pattern, cursor, findflag);
        }
        else {
            found_cursor = this->document()->find(pattern, cursor, findflag);
        }
        if (!found_cursor.isNull()) {
            this->setTextCursor(found_cursor);
            return true;
        }
    }

    return false;
}


//Get the number of matches for the searched text.
int ControlWidget::get_number_matches(QString pattern, QString source_text,
                                      bool _case, bool regexp)
{
    if (pattern.isEmpty())
        return 0;
    if (regexp == false)
        pattern = QRegularExpression::escape(pattern);
    if (source_text.isEmpty())
        source_text = toPlainText();

    QRegularExpression regobj(pattern);
    if (_case == false)
        regobj.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    if (!regobj.isValid())
        return -1;//-1表示pattern不合法

    int number_matches = 0;
    QRegularExpressionMatchIterator iterator = regobj.globalMatch(source_text);
    while (iterator.hasNext()) {
        iterator.next();//必须调用next使迭代器前进
        number_matches++;
    }

    return number_matches;
}

int ControlWidget::get_match_number(QString pattern, bool _case, bool regexp)
{
    int position = this->textCursor().position();
    QString source_text = this->get_text("sof",position);
    int match_number = this->get_number_matches(pattern,source_text,_case,regexp);
    return match_number;
}

// TracebackLinksMixin
void ControlWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QTextEdit::mouseReleaseEvent(event);
    QString text = this->get_line_at(event->pos());
    if (misc::get_error_match(text) && !has_selected_text()) {
        emit go_to_error(text);
    }
}

void ControlWidget::mouseMoveEvent(QMouseEvent *event)
{
    QString text = this->get_line_at(event->pos());
    if (misc::get_error_match(text)) {
        if (!this->__cursor_changed) {
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
            this->__cursor_changed = true;
        }
        event->accept();
        return;
    }
    if (this->__cursor_changed) {
        QApplication::restoreOverrideCursor();
        this->__cursor_changed = false;
    }
    QTextEdit::mouseMoveEvent(event);
}

void ControlWidget::leaveEvent(QEvent *event)
{
    if (this->__cursor_changed) {
        QApplication::restoreOverrideCursor();
        this->__cursor_changed = false;
    }
    QTextEdit::leaveEvent(event);
}

// GetHelpMixin
void ControlWidget::set_help_enabled(bool state)
{
    this->help_enabled = state;
}

void ControlWidget::inspect_current_object()
{
    QString text = "";
    QString text1 = this->get_text("sol", "cursor");
    QRegularExpression re("([a-zA-Z_]+[0-9a-zA-Z_\\.]*)");
    QStringList tl1;
    QRegularExpressionMatchIterator it = re.globalMatch(text1);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        tl1.append(match.captured());
    }
    if (!tl1.isEmpty() && text1.endsWith(tl1.last()))
        text += tl1.last();

    QString text2 = this->get_text("cursor", "eol");
    QRegularExpression re2("([0-9a-zA-Z_\\.]+[0-9a-zA-Z_\\.]*)");
    QStringList tl2;
    QRegularExpressionMatchIterator it2 = re2.globalMatch(text2);
    if (it2.hasNext()) {
        QRegularExpressionMatch match = it2.next();
        tl2.append(match.captured());
    }
    if (!tl2.isEmpty() && text2.startsWith(tl2.first()))
        text += tl2.first();
    //if (!text.isEmpty())
}

void ControlWidget::show_object_info(const QString &text, bool call, bool force)
{}

// BrowseHistoryMixin
void ControlWidget::clear_line()
{
    this->remove_text(this->current_prompt_pos, "eof");
}

void ControlWidget::browse_history(bool backward)
{
    if (this->is_cursor_before("eol") && this->hist_wholeline)
        this->hist_wholeline = false;
    QString tocursor = this->get_current_line_to_cursor();
    QPair<QString,int> pair = this->find_in_history(tocursor, histidx, backward);
    QString text = pair.first;
    this->histidx = pair.second;

    if (!text.isEmpty()) {
        if (this->hist_wholeline) {
            this->clear_line();
            this->insert_text(text);
        }
        else {
            int cursor_position = this->get_position("cursor");
            this->remove_text("cursor", "eol");
            this->insert_text(text);
            this->set_cursor_position(cursor_position);
        }
    }

}

QPair<QString, int> ControlWidget::find_in_history(const QString &tocursor, int start_idx, bool backward)
{
    if (start_idx == -1)
        start_idx = this->history.size();
    int step = backward ? -1 : 1;
    int idx = start_idx;
    if (tocursor.size() == 0 || this->hist_wholeline) {
        idx += step;
        if (idx >= history.size() || history.size() == 0)
            return QPair<QString, int>("", history.size());
        else if (idx < 0)
            idx = 0;
        hist_wholeline = true;
        return QPair<QString, int>(history[idx], idx);
    }
    else {
        for (int index = 0; index < history.size(); ++index) {
            idx = (start_idx+step*(index+1)) % history.size();
            QString entry = history[idx];
            if (entry.startsWith(tocursor))
                return QPair<QString, int>(entry.mid(tocursor.size()), idx);
        }
    }
    return QPair<QString, int>(QString(), start_idx);
}

void ControlWidget::reset_search_pos()
{
    this->histidx = -1;
}

// ControlWidget
void ControlWidget::showEvent(QShowEvent *event)
{
    emit visibility_changed(true);
}


void ControlWidget::_key_paren_left(const QString &text)
{
    //this->current_prompt_pos = this->parentWidget()->_prompt_pos;
    if (!this->get_current_line_to_cursor().isEmpty()) {

    }
}

void ControlWidget::keyPressEvent(QKeyEvent *event)
{
    QString text = event->text();
    int key = event->key();
    //

    QTextEdit::keyPressEvent(event);
}

void ControlWidget::focusInEvent(QFocusEvent *event)
{
    emit focus_changed();
    QTextEdit::focusInEvent(event);
}

void ControlWidget::focusOutEvent(QFocusEvent *event)
{
    emit focus_changed();
    QTextEdit::focusOutEvent(event);
}
