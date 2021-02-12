#include "api/panel.h"
#include "widgets/sourcecode/codeeditor.h"

Panel::Panel(bool dynamic)
{
	/*
	Specifies whether the panel is dynamic. A dynamic panel is a panel
	that will be shown/hidden depending on the context.
	Dynamic panel should not appear in any GUI menu
	*/
	this->dynamic = dynamic;
	/*
	Panel order into the zone it is installed to. This value is
	automatically set when installing the panel but it can be changed
	later (negative values can also be used).
	*/
	this->order_in_zone = -1;
	this->_scrollable = false;
	// Position in the editor (top, left, right, bottom)
	this->position = -1;
}

bool Panel::getScrollable() const
{
	return this->_scrollable;
}

void Panel::setScrollable(bool value)
{
	this->_scrollable = value;
}

void Panel::on_install(CodeEditor* editor)
{
	__super::on_install(editor);
	this->setParent(editor);
	this->setPalette(qApp->palette());
	this->setFont(qApp->font());
	//this->getEditor()->panels;
	this->_background_brush = QBrush(QColor(
		this->palette().window().color()));
	this->_foreground_pen = QPen(QColor(
		this->palette().windowText().color()));

	if (this->position == this->Position::FLOATING)
		this->setAttribute(Qt::WA_TransparentForMouseEvents);
}

void Panel::paintEvent(QPaintEvent *event)
{
	if (this->isVisible() && this->position != this->Position::FLOATING)
	{
		this->_background_brush = QBrush(QColor(
			this->getEditor()->sideareas_color));
		this->_foreground_pen = QPen(QColor(
			this->palette().windowText().color()));
		QPainter painter(this);
		painter.fillRect(event->rect(), this->_background_brush);
	}
}

void Panel::setVisible(bool visible)
{
	__super::setVisible(visible);
	if (this->getEditor())
	{
		this->getEditor(); // panels.refresh()
	}
}

void Panel::set_geometry(const QRect& crect)
{
	QRect oGeometry = this->geometry();
	int x0 = oGeometry.x();
	int y0 = oGeometry.y();
	int width = crect.width();
	int height = crect.height();

	// Calculate editor coordinates with their offsets
	QPointF offset = this->getEditor()->contentOffset();
	double x = getEditor()->blockBoundingGeometry(getEditor()->firstVisibleBlock())
		.translated(offset).left()
		+ getEditor()->document()->documentMargin();
	// + self.editor.panels.margin_size(Panel.Position.LEFT)
	double y = crect.top();// + self.editor.panels.margin_size(Panel.Position.TOP)

	this->setGeometry(qCeil(x + x0), qCeil(y + y0),
		width, height);
}
