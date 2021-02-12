#include "plugins/editor/utils/editor.h"
#include "widgets/sourcecode/codeeditor.h"

QColor drift_color(const QColor& base_color, int factor /*= 110*/)
{
	if (base_color.lightness() > 128)
	{
		return base_color.darker(factor);
	}
	else
	{
		if (base_color == QColor("#000000"))
		{
			return drift_color(QColor("#101010"), factor + 20);
		}
		else
		{
			return base_color.lighter(factor + 10);
		}
	}
}

DelayJobRunner::DelayJobRunner(int delay /*= 500*/)
{
	this->_timer = new QTimer(this);
	this->delay = delay;
	connect(this->_timer, &QTimer::timeout, this, &DelayJobRunner::_exec_requested_job);
	this->_job = nullptr;
}

void DelayJobRunner::request_job(const std::function<void()>& job)
{
	this->cancel_requests();
	this->_job = job;
	this->_timer->start(this->delay);
}

void DelayJobRunner::cancel_requests()
{
	this->_timer->stop();
	this->_job = nullptr;
}

void DelayJobRunner::_exec_requested_job()
{
	this->_timer->stop();
	this->_job();
}


TextHelper::TextHelper(CodeEditor* editor)
{
	this->_editor = editor;
}

int TextHelper::line_count() const
{
	return this->_editor->document()->blockCount();
}

QTextCursor TextHelper::_move_cursor_to(int line) const
{
	QTextCursor cursor = this->_editor->textCursor();
	QTextBlock block = this->_editor->document()->findBlockByNumber(line - 1);
	cursor.setPosition(block.position());
	return cursor;
}

QTextCursor TextHelper::select_lines(int start /*= 0*/, int end /*= -1*/, bool apply_selection /*= true*/) const
{
	CodeEditor* editor = this->_editor;
	if (end == -1)
	{
		end = this->line_count() - 1;
	}
	if (start < 0)
	{
		start = 0;
	}
	QTextCursor text_cursor = this->_move_cursor_to(start);
	if (end > start)
	{
		text_cursor.movePosition(QTextCursor::Down,
			QTextCursor::KeepAnchor, end - start);
		text_cursor.movePosition(QTextCursor::EndOfLine,
			QTextCursor::KeepAnchor);
	}
	else if (end < start)
	{
		// don't miss end of line !
		text_cursor.movePosition(QTextCursor::EndOfLine,
			QTextCursor::MoveAnchor);
		text_cursor.movePosition(QTextCursor::Up,
			QTextCursor::KeepAnchor, start - end);
		text_cursor.movePosition(QTextCursor::StartOfLine,
			QTextCursor::KeepAnchor);
	}
	else
	{
		text_cursor.movePosition(QTextCursor::EndOfLine,
			QTextCursor::KeepAnchor);
	}
	if (apply_selection)
	{
		editor->setTextCursor(text_cursor);
	}
	return text_cursor;
}

int TextHelper::line_pos_from_number(int line_number) const
{
	CodeEditor* editor = this->_editor;
	QTextBlock block = editor->document()->findBlockByNumber(line_number);
	if (block.isValid())
	{
		return editor->blockBoundingGeometry(block).translated(
			editor->contentOffset()).top();
	}
	if (line_number <= 0)
	{
		return 0;
	}
	else
	{
		return editor->blockBoundingGeometry(block.previous()).translated(
			editor->contentOffset()).bottom();
	}
}

int TextHelper::line_nbr_from_position(int y_pos) const
{
	CodeEditor* editor = this->_editor;
	int height = editor->fontMetrics().height();
	for (const auto& intIntTB : editor->__visible_blocks)
	{
		int top = intIntTB.top;
		if (top <= y_pos && y_pos <= top + height)
		{
			return intIntTB.line_number;
		}
	}
	return -1;
}

void TextHelper::mark_whole_doc_dirty()
{
	QTextCursor text_cursor = this->_editor->textCursor();
	text_cursor.select(QTextCursor::Document);
	this->_editor->document()->markContentsDirty(text_cursor.selectionStart(),
		text_cursor.selectionEnd());
}

