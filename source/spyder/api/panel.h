#pragma once
#include <QWidget>
#include <QBrush>
#include <QPen>
#include <QPainter>

#include "api/editorextension.h"

class Panel : public QWidget, public EditorExtension
{
	Q_OBJECT
public:
	enum Position
	{
		TOP,
		LEFT,
		RIGHT,
		BOTTOM,
		FLOATING
	};

	Panel(bool dynamic = false);
	bool getScrollable() const;
	void setScrollable(bool value);
	void on_install(CodeEditor* editor) override;
	void paintEvent(QPaintEvent *event) override;
	//const QRect &geometry() const;
	void set_geometry(const QRect& crect);

public Q_SLOTS:
	void setVisible(bool visible) override;

protected:
	bool dynamic;
	int order_in_zone;
	int position;

private:
	bool _scrollable;
	QBrush _background_brush;
	QPen _foreground_pen;
};