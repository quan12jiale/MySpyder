#include "kill_ring.h"

KillRing::KillRing()
{
    this->clear();
}

void KillRing::clear()
{
    this->_index = -1;
    this->_ring = QStringList();
}

void KillRing::kill(const QString &text)
{
    this->_ring.append(text);
}

QString KillRing::yank()
{
    this->_index = this->_ring.size();
    return this->rotate();
}

QString KillRing::rotate()
{
    this->_index -= 1;
    if (this->_index >= 0)
        return this->_ring[this->_index];
    return QString();
}


//============QtKillRing
QtKillRing::QtKillRing(QPlainTextEdit* text_edit)
    : QObject (text_edit)
{
    this->_ring.reset(new KillRing());
    this->_prev_yank = QString();
    this->_skip_cursor = false;
    this->_text_edit= text_edit;

    connect(text_edit,SIGNAL(cursorPositionChanged()),
            this,SLOT(_cursor_position_changed()));
}

void QtKillRing::clear()
{
    this->_ring->clear();
    this->_prev_yank = QString();
}

void QtKillRing::kill(const QString &text)
{
    this->_ring->kill(text);
}

void QtKillRing::kill_cursor(QTextCursor *cursor)
{
    QString text = cursor->selectedText();
    if (!text.isEmpty()) {
        cursor->removeSelectedText();
        this->kill(text);
    }
}

void QtKillRing::yank()
{
    QString text = this->_ring->yank();
    if (!text.isEmpty()) {
        this->_skip_cursor = true;
        QTextCursor cursor = this->_text_edit->textCursor();
        cursor.insertText(text);
        this->_prev_yank = text;
    }
}

void QtKillRing::rotate()
{
    if (!this->_prev_yank.isEmpty()) {
        QString text = this->_ring->rotate();
        if (!text.isEmpty()) {
            this->_skip_cursor = true;
            QTextCursor cursor = this->_text_edit->textCursor();
            cursor.movePosition(QTextCursor::Left,
                                QTextCursor::KeepAnchor,
                                this->_prev_yank.size());
            cursor.insertText(text);
            this->_prev_yank = text;
        }
    }
}

void QtKillRing::_cursor_position_changed()
{
    if (this->_skip_cursor)
        this->_skip_cursor = false;
    else
        this->_prev_yank = QString();
}
