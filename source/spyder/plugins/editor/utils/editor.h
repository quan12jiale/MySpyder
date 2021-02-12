#pragma once
#include <QTimer>
#include <QTextCursor>
#include <functional>

class CodeEditor;

QColor drift_color(const QColor& base_color, int factor = 110);

class DelayJobRunner : public QObject
{
	Q_OBJECT
public:
	DelayJobRunner(int delay = 500);
	void request_job(const std::function<void()>& job);
	void cancel_requests();

private slots:
	void _exec_requested_job();

private:
	QTimer* _timer;
	int delay;
	std::function<void()> _job;
};


class TextHelper
{
public:
	TextHelper(CodeEditor* editor);
	int line_count() const;
	QTextCursor _move_cursor_to(int line) const;
	QTextCursor select_lines(int start = 0, int end = -1, bool apply_selection = true) const;
	int line_pos_from_number(int line_number) const;
	int line_nbr_from_position(int y_pos) const;
	void mark_whole_doc_dirty();

private:
	CodeEditor* _editor;
};