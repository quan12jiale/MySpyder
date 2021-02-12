#pragma once
#include <QTextEdit>
#include <QTextCursor>
#include <QTextBlock>
#include <QVariant>

extern const QHash<QString, int> DRAW_ORDERS;

class TextDecoration;
class Signals : public QObject
{
	Q_OBJECT
signals:
	void clicked(TextDecoration*);
};

class TextDecoration : public QTextEdit::ExtraSelection
{
public:
	//TextDecoration() = default;//ÔÚmain.cppÖÐqRegisterMetaTypeStreamOperators<ColorBoolBool>("ColorBoolBool");
	TextDecoration(QTextDocument* cursor_or_bloc_or_doc, 
		const QHash<QString, QVariant>& kwargs);
	TextDecoration(const QTextBlock& cursor_or_bloc_or_doc, 
		const QHash<QString, QVariant>& kwargs);
	~TextDecoration();
	void set_foreground(const QColor& color);
	void set_background(const QBrush& brush);
	void set_outline(const QColor& color);
	void select_line();
	void set_full_width(bool flag = true, bool clear = true);

private:
	void setup(const QHash<QString, QVariant>& kwargs);

public:
	Signals* _signals;
	int draw_order;
	QString tooltip;
	QTextBlock block;
};
//Q_DECLARE_METATYPE(TextDecoration)
