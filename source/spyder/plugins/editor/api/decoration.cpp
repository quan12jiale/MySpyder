#include "plugins\editor\api\decoration.h"
#include "str.h"

const QHash<QString, int> DRAW_ORDERS = 
{ { QStringLiteral("on_bottom"), 0 },
  { QStringLiteral("current_cell"), 1 },
  { QStringLiteral("codefolding"), 2 },
  { QStringLiteral("current_line"), 3 },
  { QStringLiteral("on_top"), 4 } };

TextDecoration::TextDecoration(QTextDocument* cursor_or_bloc_or_doc, const QHash<QString, QVariant>& kwargs)
{
	this->cursor = QTextCursor(cursor_or_bloc_or_doc);
	this->setup(kwargs);
}

TextDecoration::TextDecoration(const QTextBlock& cursor_or_bloc_or_doc, const QHash<QString, QVariant>& kwargs)
{
	this->cursor = QTextCursor(cursor_or_bloc_or_doc);
	this->setup(kwargs);
}

void TextDecoration::setup(const QHash<QString, QVariant>& kwargs)
{
	this->_signals = new Signals;
	this->draw_order = kwargs.value("draw_order", 0).toInt();
	this->tooltip = kwargs.value("tooltip").toString();
	
	if (kwargs.contains("full_width"))
	{
		this->set_full_width(kwargs.value("full_width").toBool());
	}
	if (kwargs.contains("start_pos"))
	{
		this->cursor.setPosition(kwargs.value("start_pos").toInt());
	}
	if (kwargs.contains("end_pos"))
	{
		this->cursor.setPosition(kwargs.value("end_pos").toInt(), QTextCursor::KeepAnchor);
	}

	if (kwargs.contains("start_line"))
	{
		this->cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
		this->cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
			kwargs.value("start_line").toInt());
	}
	if (kwargs.contains("end_line"))
	{
		this->cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor,
			kwargs.value("end_line").toInt() - kwargs.value("start_line").toInt());
	}
	if (kwargs.contains("font"))
	{
		QFont font = kwargs.value("font").value<QFont>();
		this->format.setFont(font);
	}
}

TextDecoration::~TextDecoration()
{
	delete this->_signals;
}

void TextDecoration::set_foreground(const QColor& color)
{
	this->format.setForeground(color);
}

void TextDecoration::set_background(const QBrush& brush)
{
	this->format.setBackground(brush);
}

void TextDecoration::set_outline(const QColor& color)
{
	this->format.setProperty(QTextFormat::OutlinePen, QPen(color));
}

void TextDecoration::select_line()
{
	this->cursor.movePosition(QTextCursor::StartOfBlock);
	QString text = this->cursor.block().text();
	int lindent = text.size() - lstrip(text).size();
	this->cursor.setPosition(this->cursor.block().position() + lindent);
	this->cursor.movePosition(QTextCursor::EndOfBlock,
		QTextCursor::KeepAnchor);
}

void TextDecoration::set_full_width(bool flag /*= true*/, bool clear /*= true*/)
{
	if (clear)
	{
		this->cursor.clearSelection();
	}
	this->format.setProperty(QTextFormat::FullWidthSelection, flag);
}

