#pragma once

#include "saxutils.h"
#include "os.h"
#include "str.h"
#include "textwrap.h"

#include "utils/sourcecode.h"
#include "widgets/calltip.h"
#include "widgets/arraybuilder.h"

#include <QtWidgets>


template <typename T>
class BaseEditMixin : public T
{
    //Q_OBJECT
//signals:
    virtual void sig_eol_chars_changed(const QString&){}
public:
    QString eol_chars;
    int calltip_size;

    int calltip_position;//定义在TextEditBaseWidget中
    int current_prompt_pos;//首次出现在373行 TODO是在哪里初始化的
    int tab_stop_width_spaces;//定义在TextEditBaseWidget中
    CallTipWidget* calltip_widget;//定义在TextEditBaseWidget中

public:
    BaseEditMixin(QWidget* parent = nullptr);
    int get_linenumberarea_width() const { return 0; }
    QPair<QString,QStringList> _format_signature(QString text) const;
    void show_calltip(const QString& title,QStringList text,bool signature=false,
                      const QString& color="#2D62FF",int at_line=-1,int at_position=-1);
    void show_calltip(const QString& title,QString text,bool signature=false,
                      const QString& color="#2D62FF",int at_line=-1,int at_position=-1);

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
    QString get_character(const QString& position, int offset=0);
    QString get_character(int position, int offset=0);
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
    void enter_array_inline() { this->_enter_array(true); }
    void enter_array_table() { this->_enter_array(false); }
    void _enter_array(bool inline_);
    virtual bool is_python_like() { return true; }
};

template <typename T>
BaseEditMixin<T>::BaseEditMixin(QWidget *parent)
    : T(parent)
{
    this->eol_chars = QString();
    this->calltip_size = 600;
}

template <typename T>
QPair<QString,QStringList> BaseEditMixin<T>::_format_signature(QString text) const
{
    // 该函数仅被show_calltip(signature=true)时调用
    QStringList formatted_lines;
    QString name = text.split('(')[0];
    QString subsequent_indent(name.size()+1,' ');
    QStringList rows = textwrap::wrap(text, 50, "", subsequent_indent);
    foreach (QString r, rows) {
        r = escape(r);
        r.replace(" ", "&nbsp;");
        QStringList charlist = {"=", ",", "(", ")", "*", "**"};
        foreach (const QString& _char, charlist) {
            r.replace(_char, "<span style=\'color: red; font-weight: bold\'>" + _char + "</span>");
        }
        formatted_lines.append(r);
    }
    QString signature = formatted_lines.join("<br>");
    return qMakePair(signature, rows);
}

template <typename T>
void BaseEditMixin<T>::show_calltip(const QString &title, QStringList text, bool signature,
                                 const QString &color, int at_line, int at_position)
{
    if (text.isEmpty())
        return;

    if (at_position == -1)
        at_position = this->get_position("cursor");
    this->calltip_position = at_position;

    QString _text;
    QStringList wrapped_textlines;
    _text = text.join("\n    ");
    _text.replace('\n', "<br>");
    if (_text.size() > calltip_size)
        _text = _text.left(calltip_size) + " ...";

    QFont font = this->font();
    int size = font.pointSize();
    QString family = font.family();
    QString format1 = QString("<div style=\'font-family: \"%1\"; font-size: %2pt; color: %3\'>")
            .arg(family).arg(size).arg(color);
    QString format2 = QString("<div style=\'font-family: \"%1\"; font-size: %2pt\'>")
            .arg(family).arg((size>9) ? size-1 : size);
    QString tiptext = format1 + QString("<b>%1</b></div>").arg(title) + "<hr>" +
            format2 + _text + "</div>";

    QPoint pt = this->get_coordinates("cursor");
    int cx=pt.x(), cy=pt.y();
    if (at_line != -1) {
        cx = 5;
        QTextCursor cursor = QTextCursor(this->document()->findBlockByNumber(at_line-1));
        cy = this->cursorRect(cursor).top();
    }
    QPoint point = this->mapToGlobal(QPoint(cx, cy));
    point.setX(point.x()+this->get_linenumberarea_width());
    point.setY(point.y()+font.pointSize()+5);
    if (signature)
        this->calltip_widget->show_tip(point, tiptext, wrapped_textlines);
    else
        QToolTip::showText(point, tiptext);
}

template <typename T>
void BaseEditMixin<T>::show_calltip(const QString &title, QString text, bool signature,
                                 const QString &color, int at_line, int at_position)
{
    if (text.isEmpty())
        return;

    if (at_position == -1)
        at_position = this->get_position("cursor");
    this->calltip_position = at_position;

    QString _text;
    QStringList wrapped_textlines;
    if (signature) {
        auto pair = _format_signature(text);
        _text = pair.first;
        wrapped_textlines = pair.second;
    }
    else {
        _text.replace('\n', "<br>");
        if (_text.size() > calltip_size)
            _text = _text.left(calltip_size) + " ...";
    }

    QFont font = this->font();
    int size = font.pointSize();
    QString family = font.family();
    QString format1 = QString("<div style=\'font-family: \"%1\"; font-size: %2pt; color: %3\'>")
            .arg(family).arg(size).arg(color);
    QString format2 = QString("<div style=\'font-family: \"%1\"; font-size: %2pt\'>")
            .arg(family).arg((size>9) ? size-1 : size);
    QString tiptext = format1 + QString("<b>%1</b></div>").arg(title) + "<hr>" +
            format2 + _text + "</div>";

    QPoint pt = this->get_coordinates("cursor");
    int cx=pt.x(), cy=pt.y();
    if (at_line != -1) {
        cx = 5;
        QTextCursor cursor = QTextCursor(this->document()->findBlockByNumber(at_line-1));
        cy = this->cursorRect(cursor).top();
    }
    QPoint point = this->mapToGlobal(QPoint(cx, cy));
    point.setX(point.x()+this->get_linenumberarea_width());
    point.setY(point.y()+font.pointSize()+5);
    if (signature)
        this->calltip_widget->show_tip(point, tiptext, wrapped_textlines);
    else
        QToolTip::showText(point, tiptext);
}

template <typename T>
void BaseEditMixin<T>::set_eol_chars(const QString &text)
{
    QString eol_chars = sourcecode::get_eol_chars(text);
    bool is_document_modified = (!eol_chars.isEmpty()) && (!this->eol_chars.isEmpty());
    this->eol_chars = eol_chars;
    if (is_document_modified) {
        this->document()->setModified(true);
        emit this->sig_eol_chars_changed(eol_chars);
    }
}

template <typename T>
QString BaseEditMixin<T>::get_line_separator() const
{
    if (!this->eol_chars.isEmpty())
        return this->eol_chars;
    else
        return os::linesep;
}

template <typename T>
QString BaseEditMixin<T>::get_text_with_eol() const
{
    QString utext = this->toPlainText();
    // 这里没有splitlines()然后join()，感觉这样容易出bug
    //QStringList lines = splitlines(utext);
    //QString linesep = this->get_line_separator();
    //QString txt = lines.join(linesep);
    //if (utext.endsWith("\n"))
    //    utext += linesep;
    return utext;
}

template <typename T>
int BaseEditMixin<T>::get_position(const QString& subject) const
{
    QTextCursor cursor = this->textCursor();
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

template <typename T>
int BaseEditMixin<T>::get_position(int subject) const
{
    return subject;
}

template <typename T>
QPoint BaseEditMixin<T>::get_coordinates(const QString& position) const
{
    int _position = get_position(position);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position);
    QPoint point = this->cursorRect(cursor).center();
    return point;
}

template <typename T>
QPoint BaseEditMixin<T>::get_coordinates(int position) const
{
    int _position = get_position(position);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position);
    QPoint point = this->cursorRect(cursor).center();
    return point;
}

template <typename T>
QPair<int,int> BaseEditMixin<T>::get_cursor_line_column() const
{
    QTextCursor cursor = this->textCursor();
    return QPair<int,int>(cursor.blockNumber(), cursor.columnNumber());
}

template <typename T>
int BaseEditMixin<T>::get_cursor_line_number() const
{
    return this->textCursor().blockNumber()+1;
}

template <typename T>
void BaseEditMixin<T>::set_cursor_position(const QString &position)
{
    int _position = this->get_position(position);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position);
    this->setTextCursor(cursor);
    this->ensureCursorVisible();
}

template <typename T>
void BaseEditMixin<T>::set_cursor_position(int position)
{
    int _position = this->get_position(position);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position);
    this->setTextCursor(cursor);
    this->ensureCursorVisible();
}

template <typename T>
void BaseEditMixin<T>::move_cursor(int chars)
{
    QTextCursor::MoveOperation direction = (chars>0) ? QTextCursor::Right : QTextCursor::Left;
    for (int i=0;i<qAbs(chars);i++)
        this->moveCursor(direction,QTextCursor::MoveAnchor);
}

template <typename T>
bool BaseEditMixin<T>::is_cursor_on_first_line() const
{
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock);
    return cursor.atStart();
}

template <typename T>
bool BaseEditMixin<T>::is_cursor_on_last_line() const
{
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::EndOfBlock);
    return cursor.atEnd();
}

template <typename T>
bool BaseEditMixin<T>::is_cursor_at_end() const
{
    return this->textCursor().atEnd();
}

//"""Return True if cursor is before *position*"""
template <typename T>
bool BaseEditMixin<T>::is_cursor_before(const QString &position, int char_offset) const
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

template <typename T>
bool BaseEditMixin<T>::is_cursor_before(int position, int char_offset) const
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

template <typename T>
void BaseEditMixin<T>::__move_cursor_anchor(QString what, QString direction,
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

template <typename T>
void BaseEditMixin<T>::move_cursor_to_next(QString what, QString direction)
{
    this->__move_cursor_anchor(what, direction, QTextCursor::MoveAnchor);
}

template <typename T>
void BaseEditMixin<T>::clear_selection()
{
    QTextCursor cursor = this->textCursor();
    cursor.clearSelection();
    this->setTextCursor(cursor);
}

template <typename T>
void BaseEditMixin<T>::extend_selection_to_next(QString what, QString direction)
{
    this->__move_cursor_anchor(what,direction,QTextCursor::KeepAnchor);
}

template <typename T>
QTextCursor BaseEditMixin<T>::__select_text(const QString& position_from,
                                         const QString& position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position_from);
    cursor.setPosition(_position_to,QTextCursor::KeepAnchor);
    return cursor;
}

template <typename T>
QTextCursor BaseEditMixin<T>::__select_text(const QString& position_from,
                                         int position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position_from);
    cursor.setPosition(_position_to,QTextCursor::KeepAnchor);
    return cursor;
}

template <typename T>
QTextCursor BaseEditMixin<T>::__select_text(int position_from,
                                         const QString& position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position_from);
    cursor.setPosition(_position_to,QTextCursor::KeepAnchor);
    return cursor;
}

template <typename T>
QTextCursor BaseEditMixin<T>::__select_text(int position_from,
                                         int position_to)
{
    int _position_from = this->get_position(position_from);
    int _position_to = this->get_position(position_to);
    QTextCursor cursor = this->textCursor();
    cursor.setPosition(_position_from);
    cursor.setPosition(_position_to,QTextCursor::KeepAnchor);
    return cursor;
}

template <typename T>
QString BaseEditMixin<T>::get_text_line(int line_nb)
{
    QTextBlock block  = this->document()->findBlockByLineNumber(line_nb);
    return block.text();
}

/*
 * Return text between *position_from* and *position_to*
   Positions may be positions or 'sol', 'eol', 'sof', 'eof' or 'cursor'
*/
template <typename T>
QString BaseEditMixin<T>::get_text(const QString& position_from,
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
        while (text.endsWith(QChar(QChar::ParagraphSeparator)))
            text.chop(1);
    }
    return text;
}

template <typename T>
QString BaseEditMixin<T>::get_text(const QString& position_from,
                                int position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    QString text = cursor.selectedText();
    if (!text.isEmpty()) {
        while (text.endsWith("\n"))
            text.chop(1);
        // TODO源码是while text.endswith(u"\u2029"):
        //              text = text[:-1]
        while (text.endsWith(QChar(QChar::ParagraphSeparator)))
            text.chop(1);
    }
    return text;
}

template <typename T>
QString BaseEditMixin<T>::get_text(int position_from,
                                const QString& position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    QString text = cursor.selectedText();
    if (!text.isEmpty()) {
        while (text.endsWith("\n"))
            text.chop(1);
        // TODO源码是while text.endswith(u"\u2029"):
        //              text = text[:-1]
        while (text.endsWith(QChar(QChar::ParagraphSeparator)))
            text.chop(1);
    }
    return text;
}

template <typename T>
QString BaseEditMixin<T>::get_text(int position_from,
                                int position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    QString text = cursor.selectedText();
    if (!text.isEmpty()) {
        while (text.endsWith("\n"))
            text.chop(1);
        // TODO源码是while text.endswith(u"\u2029"):
        //              text = text[:-1]
        while (text.endsWith(QChar(QChar::ParagraphSeparator)))
            text.chop(1);
    }
    return text;
}

template <typename T>
QString BaseEditMixin<T>::get_character(const QString& position, int offset)
{
    int _position = this->get_position(position) + offset;
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (_position < cursor.position()) {
        cursor.setPosition(_position);
        cursor.movePosition(QTextCursor::Right,
                            QTextCursor::KeepAnchor);
        return cursor.selectedText();
    }
    else
        return "";
}

template <typename T>
QString BaseEditMixin<T>::get_character(int position, int offset)
{
    int _position = this->get_position(position) + offset;
    QTextCursor cursor = this->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (_position < cursor.position()) {
        cursor.setPosition(_position);
        cursor.movePosition(QTextCursor::Right,
                            QTextCursor::KeepAnchor);
        return cursor.selectedText();
    }
    else
        return "";
}

template <typename T>
void BaseEditMixin<T>::insert_text(const QString &text)
{
    if (!this->isReadOnly()) {
        this->textCursor().insertText(text);
    }
}

template <typename T>
void BaseEditMixin<T>::replace_text(const QString& position_from,
                                 const QString& position_to, const QString &text)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
    cursor.insertText(text);
}

template <typename T>
void BaseEditMixin<T>::replace_text(const QString& position_from,
                                 int position_to, const QString &text)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
    cursor.insertText(text);
}

template <typename T>
void BaseEditMixin<T>::replace_text(int position_from,
                                 const QString& position_to, const QString &text)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
    cursor.insertText(text);
}

template <typename T>
void BaseEditMixin<T>::replace_text(int position_from,
                                 int position_to, const QString &text)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
    cursor.insertText(text);
}

template <typename T>
void BaseEditMixin<T>::remove_text(const QString& position_from,
                                const QString& position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
}

template <typename T>
void BaseEditMixin<T>::remove_text(const QString& position_from,
                                int position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
}

template <typename T>
void BaseEditMixin<T>::remove_text(int position_from,
                                const QString& position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
}

template <typename T>
void BaseEditMixin<T>::remove_text(int position_from,
                                int position_to)
{
    QTextCursor cursor = this->__select_text(position_from,position_to);
    cursor.removeSelectedText();
}

template <typename T>
bool BaseEditMixin<T>::is_space(QTextCursor::MoveOperation move)
{
    QTextCursor curs = this->textCursor();
    curs.movePosition(move, QTextCursor::KeepAnchor);
    return curs.selectedText().trimmed().isEmpty();
}

template <typename T>
QString BaseEditMixin<T>::get_current_word()
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

    // \\d匹配任意数字,\\w匹配数字字母下划线
    QRegularExpression rule("([^\\d\\W]\\w*)", QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpressionMatchIterator iterator = rule.globalMatch(text);
    if (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        return match.captured();
    }

    return QString();
}

template <typename T>
QString BaseEditMixin<T>::get_current_line()
{
    QTextCursor cursor = this->textCursor();
    cursor.select(QTextCursor::BlockUnderCursor);
    return cursor.selectedText();
}

template <typename T>
QString BaseEditMixin<T>::get_current_line_to_cursor()
{
    return this->get_text(this->current_prompt_pos, "cursor");
}

template <typename T>
int BaseEditMixin<T>::get_line_number_at(const QPoint& coordinates)
{
    QTextCursor cursor = this->cursorForPosition(coordinates);
    return cursor.blockNumber()-1;
}

template <typename T>
QString BaseEditMixin<T>::get_line_at(const QPoint &coordinates)
{
    QTextCursor cursor = this->cursorForPosition(coordinates);
    cursor.select(QTextCursor::BlockUnderCursor);
    return cursor.selectedText().replace(QChar(QChar::ParagraphSeparator),"");
}

template <typename T>
QString BaseEditMixin<T>::get_word_at(const QPoint &coordinates)
{
    QTextCursor cursor = this->cursorForPosition(coordinates);
    cursor.select(QTextCursor::WordUnderCursor);
    return cursor.selectedText();
}

template <typename T>
int BaseEditMixin<T>::get_block_indentation(int block_nb)
{
    QString text = this->document()->findBlockByNumber(block_nb).text();
    QString after(this->tab_stop_width_spaces,' ');
    text.replace("\t",after);
    return text.size() - lstrip(text).size();
}

template <typename T>
QPair<int,int> BaseEditMixin<T>::get_selection_bounds()
{
    QTextCursor cursor = this->textCursor();
    int start=cursor.selectionStart(), end=cursor.selectionEnd();
    QTextBlock block_start = this->document()->findBlock(start);
    QTextBlock block_end = this->document()->findBlock(end);
    if (block_start.blockNumber() <= block_end.blockNumber())
        return QPair<int,int>(block_start.blockNumber(), block_end.blockNumber());
    else
        return QPair<int,int>(block_end.blockNumber(), block_start.blockNumber());
}

template <typename T>
bool BaseEditMixin<T>::has_selected_text()
{
    QString text = this->textCursor().selectedText();
    if (text.isEmpty())
        return false;
    else
        return true;
}

template <typename T>
QString BaseEditMixin<T>::get_selected_text()
{
    // \u2029是unicode编码的段落分割符 ParagraphSeparator = 0x2029
    return this->textCursor().selectedText().replace(QChar(QChar::ParagraphSeparator),
                                                     this->get_line_separator());
}

template <typename T>
void BaseEditMixin<T>::remove_selected_text()
{
    this->textCursor().removeSelectedText();
}

template <typename T>
void BaseEditMixin<T>::replace(QString text, const QString &pattern)
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

template <typename T>
QTextCursor BaseEditMixin<T>::find_multiline_pattern(const QRegularExpression& regexp,const QTextCursor& cursor,
                                                   QTextDocument::FindFlags findflag)
{
    QString pattern = regexp.pattern();
    QString text = this->toPlainText();
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

template <typename T>
bool BaseEditMixin<T>::find_text(QString text, bool changed, bool forward,
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    QRegularExpression pattern((words ? QString("\\b%1\\b").arg(text) : text));
    if (_case)
        pattern.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
#else
    QRegExp pattern((words ? QString("\\b%1\\b").arg(text) : text),
                    (_case ? Qt::CaseSensitive : Qt::CaseInsensitive),
                    QRegExp::RegExp2);
#endif

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
template <typename T>
int BaseEditMixin<T>::get_number_matches(QString pattern, QString source_text,
                                      bool _case, bool regexp)
{
    if (pattern.isEmpty())
        return 0;
    if (regexp == false)
        pattern = QRegularExpression::escape(pattern);
    if (source_text.isEmpty())
        source_text = this->toPlainText();

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

template <typename T>
int BaseEditMixin<T>::get_match_number(QString pattern, bool _case, bool regexp)
{
    int position = this->textCursor().position();
    QString source_text = this->get_text("sof",position);
    int match_number = this->get_number_matches(pattern,source_text,_case,regexp);
    return match_number;
}

template <typename T>
void BaseEditMixin<T>::_enter_array(bool inline_)
{
    int offset = this->get_position("cursor") - this->get_position("sol");
    QRect rect = this->cursorRect();
    NumpyArrayDialog* dlg = new NumpyArrayDialog(this, inline_, offset);

    int x = rect.left();
    x = x + this->get_linenumberarea_width() - 14;
    int y = rect.top() + (rect.bottom() - rect.top())/2;
    y = y - dlg->height()/2 - 3;

    QPoint pos = QPoint(x, y);
    dlg->move(this->mapToGlobal(pos));

    bool python_like_check;
    QString suffix;
    if (this->is_editor()) {
        python_like_check = this->is_python_like();
        suffix = "\n";
    }
    else {
        python_like_check = true;
        suffix = "";
    }

    if (python_like_check && dlg->exec()) {
        QString text = dlg->text() + suffix;
        if (text != "") {
            QTextCursor cursor = this->textCursor();
            cursor.beginEditBlock();
            cursor.insertText(text);
            cursor.endEditBlock();
        }
    }
}


/*
class BaseEditMixin : public QPlainTextEdit
{
    Q_OBJECT
signals:
    void sig_eol_chars_changed(const QString&);
public:
    QString eol_chars;
    int calltip_size;

    int calltip_position;//定义在TextEditBaseWidget中
    int current_prompt_pos;//首次出现在373行 TODO是在哪里初始化的
    int tab_stop_width_spaces;//定义在TextEditBaseWidget中
    CallTipWidget* calltip_widget;//定义在TextEditBaseWidget中

public:
    BaseEditMixin(QWidget* parent = nullptr);

    int get_linenumberarea_width() const { return 0; }
    QPair<QString,QStringList> _format_signature(QString text) const;
    void show_calltip(const QString& title,QStringList text,bool signature=false,
                      const QString& color="#2D62FF",int at_line=-1,int at_position=-1);
    void show_calltip(const QString& title,QString text,bool signature=false,
                      const QString& color="#2D62FF",int at_line=0,int at_position=0);

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
    QString get_character(const QString& position, int offset=0);
    QString get_character(int position, int offset=0);
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
    void enter_array_inline() { this->_enter_array(true); }
    void enter_array_table() { this->_enter_array(false); }
    void _enter_array(bool inline_);
    //virtual bool is_python_like() = 0;//mixins.py596行，在codeeditor.py中定义
    virtual bool is_python_like() { return true; }
};
*/
