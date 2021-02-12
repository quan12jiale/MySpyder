#pragma once
#include <QMenu>
#include <QTextCursor>
#include <QTextBlock>

#include "api/panel.h"
class DelayJobRunner;
class TextDecoration;

class FoldingPanel : public Panel
{
	Q_OBJECT
signals:
	void trigger_state_changed(const QTextBlock&, bool);
	void collapse_all_triggered();
	void expand_all_triggered();

public:
	void set_highlight_caret_scope(bool value);

public:
	FoldingPanel(bool highlight_caret_scope = false);
	~FoldingPanel();
	QHash<int, int> __compute_line_offsets(const QString& text, bool reverse = false) const;
	void update_folding(const QList<std::pair<int,int>>& ranges);
	QSize sizeHint() const override;
	void paintEvent(QPaintEvent *event) override;
	void _draw_fold_region_background(const QTextBlock& block, QPainter& painter);
	void _draw_rect(const QRectF& rect, QPainter& painter);
	void _draw_fold_indicator(int top, bool mouse_over, bool collapsed, QPainter& painter);
	
	QTextBlock find_parent_scope(QTextBlock block) const;
	void _clear_scope_decos();
	QColor _get_scope_highlight_color() const;
	void _decorate_block(int start, int end);
	void _highlight_block(const QTextBlock& block);

	void mouseMoveEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void _add_fold_decoration(const QTextBlock& block, int end_line);
	QTextBlock _get_block_until_line(QTextBlock block, int end_line) const;

	QTextBlock fold_region(QTextBlock block, int start_line, int end_line) const;
	void unfold_region(QTextBlock block, int start_line, int end_line);
	void toggle_fold_trigger(const QTextBlock& block);
	void mousePressEvent(QMouseEvent *event) override;
	
public slots:
	void _on_fold_deco_clicked(TextDecoration* deco);
	void on_state_changed(bool state);
	void _on_key_pressed(QKeyEvent* event);

public:
	static void _show_previous_blank_lines(const QTextBlock& block);

	void refresh_decorations(bool force = false);
	void _refresh_editor_and_scrollbars();
	void collapse_all();

public slots:
	void _clear_block_deco();

public:
	void expand_all();
	void _on_action_toggle();
	void _on_action_collapse_all_triggered();
	void _on_action_expand_all_triggered();

public slots:
	void _highlight_caret_scope();

public:
	void clone_settings(FoldingPanel* original);
	bool is_collapsed(const QTextBlock& block) const;
	std::pair<int, int> get_range(const QTextBlock& block) const;

private:
	bool _native_icons;
	QStringList _indicators_icons;
	int _block_nbr;
	bool _highlight_caret;
	bool highlight_caret_scope;
	int _indic_size;
	QList<TextDecoration*> _scope_decos;
	QList<TextDecoration*> _block_decos;
	int _mouse_over_line;
	std::pair<int, int> _current_scope;
	QTextCursor _prev_cursor;
	QMenu* context_menu;
	QAction* action_collapse;
	QAction* action_expand;
	QAction* action_collapse_all;
	QAction* action_expand_all;
	// _original_background
	DelayJobRunner* _highlight_runner;
	QHash<int, int> folding_regions;
	QHash<int, bool> folding_status;
	QHash<int, int> folding_levels;
	QHash<int, int> folding_nesting;

	QTextBlock _highight_block;//仅在mouseMoveEvent493行出现过一次
};