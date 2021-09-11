#include "plugins/editor/panels/codefolding.h"
#include "plugins/editor/utils/editor.h"
#include "plugins\editor\api\decoration.h"

#include "widgets/sourcecode/codeeditor.h"
#include "str.h"
#include "intervaltree/intervaltree.h"

void FoldingPanel::set_highlight_caret_scope(bool value)
{
	if (value == this->_highlight_caret)
	{
		return;
	}

	this->_highlight_caret = value;
	CodeEditor* editor = this->getEditor();
	if (!editor)
	{
		return;
	}

	if (value)
	{
		this->_block_nbr = -1;
		connect(editor, &CodeEditor::cursorPositionChanged,
			this, &FoldingPanel::_highlight_caret_scope);
	}
	else
	{
		this->_block_nbr = -1;
		disconnect(editor, &CodeEditor::cursorPositionChanged,
			this, &FoldingPanel::_highlight_caret_scope);
	}
}

FoldingPanel::FoldingPanel(bool highlight_caret_scope /*= false*/)
{
	this->_native_icons = false;
	this->_indicators_icons 
		<< "folding.arrow_right_off"
		<< "folding.arrow_right_on"
		<< "folding.arrow_down_off"
		<< "folding.arrow_down_on";
	this->_block_nbr = -1;
	this->_highlight_caret = false;
	this->highlight_caret_scope = highlight_caret_scope;
	this->_indic_size = 16;
	this->_scope_decos.clear();
	this->_block_decos.clear();
	this->setMouseTracking(true);
	this->setScrollable(true);
	this->_mouse_over_line = -1;
	this->_current_scope = std::make_pair(-1, -1);
	this->_prev_cursor = QTextCursor();
	this->context_menu = nullptr;
	this->action_collapse = nullptr;
	this->action_expand = nullptr;
	this->action_collapse_all = nullptr;
	this->action_expand_all = nullptr;
// 		self._original_background = None 没有用到
	this->_highlight_runner = new DelayJobRunner(250);
	this->folding_regions.clear();
	this->folding_status.clear();
	this->folding_levels.clear();
	this->folding_nesting.clear();
}

FoldingPanel::~FoldingPanel()
{
	delete this->_highlight_runner;
}

QHash<int, int> FoldingPanel::__compute_line_offsets(const QString& text, bool reverse /*= false*/) const
{
	QStringList lines = splitlines(text, true);
	QHash<int, int> line_start_offset;
	int offset = 0;
	for (int i = 0; i < lines.size(); i++)
	{
		QString line = lines[i];
		if (!reverse)
		{
			line_start_offset[i + 1] = offset;
		}
		else
		{
			line_start_offset[offset] = i + 1;
		}
		offset += line.size();
	}
	return line_start_offset;
}

/*Update folding panel folding ranges.*/
void FoldingPanel::update_folding(const QList<std::pair<int, int>>& ranges)
{
	if (ranges.isEmpty())
	{
		return;
	}
	QHash<int, int> new_folding_ranges;
	for (const auto& pair : ranges)
	{
		int starting_line = pair.first;
		int ending_line = pair.second;
		if (ending_line > starting_line)
		{
			new_folding_ranges[starting_line + 1] = ending_line + 1;
		}
	}

	QHash<int, int> past_folding_regions = this->folding_regions;
	this->folding_regions = new_folding_ranges;
	QHash<int, bool> folding_status;
	for (auto iter = this->folding_regions.begin(); 
		iter != this->folding_regions.end(); iter++)
	{
		folding_status[iter.key()] = false;
	}

	if (folding_status.size() == this->folding_status.size())
	{
		this->folding_status = folding_status;
		QHash<int, bool> this_folding_status_copy = this->folding_status;
		this->folding_status.clear();
		for (auto iter = folding_status.begin();
			iter != folding_status.end(); iter++)
		{
			this->folding_status[iter.key()] = this_folding_status_copy.value(iter.key());
		}
	}
	else
	{
		// diff, previous_text = self.editor.text_diff
	}

	this->folding_levels.clear();
	this->folding_nesting.clear();
	for (auto iter = this->folding_regions.begin();
		iter != this->folding_regions.end(); iter++)
	{
		this->folding_levels[iter.key()] = 0;
		this->folding_nesting[iter.key()] = -1;
	}
	auto tree = IntervalTree::from_tuples(this->folding_regions.keys(), 
		this->folding_regions.values());
	for (auto iter = this->folding_regions.begin();
		iter != this->folding_regions.end(); iter++)
	{
		int start = iter.key();
		int end = iter.value();
		QSet<Interval> nested_regions = tree->envelop(start, end);
		for (const auto& region : nested_regions)
		{
			if (region.begin != start)
			{
				this->folding_levels[region.begin] += 1;
				this->folding_nesting[region.begin] = start;
			}
		}
	}
	this->update();
	delete tree;
}

QSize FoldingPanel::sizeHint() const
{
	QFontMetricsF fm(this->getEditor()->font());
	QSize size_hint(qCeil(fm.height()), qCeil(fm.height()));
	if (size_hint.width() > 16)
	{
		size_hint.setWidth(16);
	}
	return size_hint;
}

void FoldingPanel::paintEvent(QPaintEvent *event)
{
	/*
	Paints the fold indicators and the possible fold region background
	on the folding panel.
	*/
	__super::paintEvent(event);
	QPainter painter(this);

	if (this->_mouse_over_line != -1)
	{
		QTextBlock block = getEditor()->document()->findBlockByNumber(_mouse_over_line);
		try
		{
			this->_draw_fold_region_background(block, painter);
		}
		catch (const std::exception&)
		{}
	}

	for (const auto& intIntTB : this->getEditor()->__visible_blocks)
	{
		int top_position = intIntTB.top;
		int line_number = intIntTB.line_number;
		QTextBlock block = intIntTB.block;
		if (this->folding_regions.contains(line_number))
		{
			bool collapsed = this->folding_status[line_number];
			int line_end = this->folding_regions[line_number];
			bool mouse_over = this->_mouse_over_line == line_number;
			this->_draw_fold_indicator(top_position, mouse_over, collapsed, painter);
			if (collapsed)
			{
				bool break_flag = false;
				for (auto deco : this->_block_decos)
				{
					if (deco->block == block)
					{
						break_flag = true;
						break;
					}
				}
				if (!break_flag)
				{
					this->_add_fold_decoration(block, line_end);
				}
			}
			else
			{
				auto block_decos_copy = this->_block_decos;
				for (int i = 0; i < block_decos_copy.size(); i++)
				{
					auto deco = block_decos_copy[i];
					if (deco->block == block)
					{
						this->_block_decos.removeAt(i);
						// self.editor.decorations.remove(deco)
						delete deco;
						break;
					}
				}
			}
		}
	}
}


void FoldingPanel::_draw_fold_region_background(const QTextBlock& block, QPainter& painter)
{
	TextHelper th(this->getEditor());
	int start = block.blockNumber();
	int end = this->folding_regions[start];
	int top;
	if (start > 0)
	{
		top = th.line_pos_from_number(start);
	}
	else
	{
		top = 0;
	}
	int bottom = th.line_pos_from_number(end);
	int h = bottom - top;
	if (h == 0)
	{
		h = this->sizeHint().height();
	}
	int w = this->sizeHint().width();
	this->_draw_rect(QRectF(0, top, w, h), painter);
}

void FoldingPanel::_draw_rect(const QRectF& rect, QPainter& painter)
{
	QColor c = this->getEditor()->sideareas_color;
	QLinearGradient grad(rect.topLeft(), rect.topRight());

	grad.setColorAt(0, c.lighter(110));
	grad.setColorAt(1, c.lighter(130));
	QColor outline = c.darker(100);

	painter.fillRect(rect, grad);
	painter.setPen(QPen(outline));
	painter.drawLine(rect.topLeft() +
		QPointF(1, 0),
		rect.topRight() -
		QPointF(1, 0));
	painter.drawLine(rect.bottomLeft() +
		QPointF(1, 0),
		rect.bottomRight() -
		QPointF(1, 0));
	painter.drawLine(rect.topRight() +
		QPointF(0, 1),
		rect.bottomRight() -
		QPointF(0, 1));
	painter.drawLine(rect.topLeft() +
		QPointF(0, 1),
		rect.bottomLeft() -
		QPointF(0, 1));
}

void FoldingPanel::_draw_fold_indicator(int top, bool mouse_over, bool collapsed, QPainter& painter)
{
	QRect rect(0, top, sizeHint().width(),
		sizeHint().height());
	if (this->_native_icons)
	{
		QStyleOptionViewItem opt;

		opt.rect = rect;
		opt.state = (QStyle::State_Active |
			QStyle::State_Item |
			QStyle::State_Children);
		if (!collapsed)
			opt.state |= QStyle::State_Open;
		if (mouse_over)
		{
			opt.state |= (QStyle::State_MouseOver |
				QStyle::State_Enabled |
				QStyle::State_Selected);
			opt.palette.setBrush(QPalette::Window,
				this->palette().highlight());
		}
		opt.rect.translate(-2, 0);
		this->style()->drawPrimitive(QStyle::PE_IndicatorBranch,
			&opt, &painter, this);
	}
	else
	{
		int index = 0;
		if (!collapsed)
		{
			index = 2;
		}
		if (mouse_over)
		{
			index += 1;
		}
		// ima::icon(this->_indicators_icons[index]).paint(&painter, rect);
	}
}

QTextBlock FoldingPanel::find_parent_scope(QTextBlock block) const
{
	int block_line = block.blockNumber();
	if (!this->folding_regions.contains(block_line))
	{
		for (auto iter= folding_regions.begin(); iter!= folding_regions.end(); iter++)
		{
			int start_line = iter.key();
			int end_line = this->folding_regions[start_line];
			if (end_line > block_line)
			{
				if (start_line < block_line)
				{
					block = getEditor()->document()->findBlockByNumber(start_line);
					break;
				}
			}
		}
	}
	return block;
}

void FoldingPanel::_clear_scope_decos()
{
	// self.editor.decorations.remove(deco)
	qDeleteAll(this->_scope_decos);
	this->_scope_decos.clear();
}

QColor FoldingPanel::_get_scope_highlight_color() const
{
	QColor color = this->getEditor()->sideareas_color;
	if (color.lightness() < 128)
	{
		color = drift_color(color, 130);
	}
	else
	{
		color = drift_color(color, 105);
	}
	return color;
}

void FoldingPanel::_decorate_block(int start, int end)
{
	QColor color = this->_get_scope_highlight_color();
	int draw_order = DRAW_ORDERS.value(QStringLiteral("codefolding"));
	QHash<QString, QVariant> kwargs;
	kwargs["kwargs"] = std::max(0, start - 1);
	kwargs["end_line"] = end;
	kwargs["draw_order"] = draw_order;
	TextDecoration* d = new TextDecoration(getEditor()->document(), kwargs);
	d->set_background(color);
	d->set_full_width(true, false);
	// self.editor.decorations.add(d)
	this->_scope_decos.append(d);
}

void FoldingPanel::_highlight_block(const QTextBlock& block)
{
	int block_line = block.blockNumber();
	int end_line = this->folding_regions.value(block_line);

	auto scope = std::make_pair(block_line, end_line);
	if (this->_current_scope.first == -1 || this->_current_scope != scope)
	{
		this->_current_scope = scope;
		this->_clear_scope_decos();
		int start = block_line;
		int end = end_line;
		if (!this->folding_status.value(start))
		{
			this->_decorate_block(start, end);
		}
	}
}

void FoldingPanel::mouseMoveEvent(QMouseEvent *event)
{
	__super::mouseMoveEvent(event);
	TextHelper th(this->getEditor());
	int line = th.line_nbr_from_position(event->pos().y());
	if (line >= 0)
	{
		QTextBlock block = getEditor()->document()->findBlockByNumber(line);
		block = this->find_parent_scope(block);
		int line_number = block.blockNumber();
		if (this->folding_regions.contains(line_number))
		{
			if (this->_mouse_over_line == -1)
			{
				QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
			}
			else if (this->_mouse_over_line != block.blockNumber())
			{
				/*
				fold scope changed, a previous block was highlighter so
				we quickly update our highlighting
				*/
				this->_mouse_over_line = block.blockNumber();
				this->_highlight_block(block);
			}
			else
			{
				this->_mouse_over_line = block.blockNumber();
				std::function<void (QTextBlock)> job
					= [&](QTextBlock block)->void { this->_highlight_block(block); };
				this->_highlight_runner->request_job(job, block);
			}
			this->_highight_block = block;
		}
		else
		{
			this->_highlight_runner->cancel_requests();
			this->_mouse_over_line = -1;
			QApplication::restoreOverrideCursor();
		}
		this->repaint();
	}
}

void FoldingPanel::leaveEvent(QEvent *event)
{
	__super::leaveEvent(event);
	QApplication::restoreOverrideCursor();
	this->_highlight_runner->cancel_requests();
	if (!this->highlight_caret_scope)
	{
		this->_clear_scope_decos();
		this->_mouse_over_line = -1;
		this->_current_scope = std::make_pair(-1, -1);
	}
	else
	{
		this->_block_nbr = -1;
		this->_highlight_caret_scope();
	}
	this->getEditor()->repaint();
}

void FoldingPanel::_add_fold_decoration(const QTextBlock& block, int end_line)
{
	int start_line = block.blockNumber();
	QString text = getEditor()->get_text_region(start_line + 1, end_line);
	int draw_order = DRAW_ORDERS.value(QStringLiteral("codefolding"));
	QHash<QString, QVariant> kwargs;
	kwargs["draw_order"] = draw_order;
	TextDecoration* deco = new TextDecoration(block, kwargs);
	connect(deco->_signals, &Signals::clicked, this, &FoldingPanel::_on_fold_deco_clicked);
	deco->tooltip = text;
	deco->block = block;
	deco->select_line();
	deco->set_outline(drift_color(this->_get_scope_highlight_color(), 110));
	deco->set_background(this->_get_scope_highlight_color());
	deco->set_foreground(QColor("#808080"));
	this->_block_decos.append(deco);
	// self.editor.decorations.add(deco)
}

QTextBlock FoldingPanel::_get_block_until_line(QTextBlock block, int end_line) const
{
	while (block.isValid() && block.blockNumber() <= end_line)
	{
		block.setVisible(false);
		block = block.next();
	}
	return block;
}

QTextBlock FoldingPanel::fold_region(QTextBlock block, int start_line, int end_line) const
{
	while (block.isValid() && block.blockNumber() < end_line)
	{
		block.setVisible(false);
		block = block.next();
	}
	return block;
}


void FoldingPanel::unfold_region(QTextBlock block, int start_line, int end_line)
{
	while (block.isValid() && block.blockNumber() < end_line)
	{
		int current_line = block.blockNumber();
		block.setVisible(true);
		bool get_next = true;
		if (this->folding_regions.contains(current_line)
			&& current_line != start_line)
		{
			int block_end = this->folding_regions[current_line];
			if (this->folding_status[current_line])
			{
				get_next = false;
				block = this->_get_block_until_line(block, block_end - 1);
			}
		}
		if (get_next)
		{
			block = block.next();
		}
	}
}

void FoldingPanel::toggle_fold_trigger(const QTextBlock& block)
{
	int start_line = block.blockNumber();
	if (!this->folding_regions.contains(start_line))
	{
		return;
	}
	int end_line = this->folding_regions[start_line];
	if (this->folding_status[start_line])
	{
		this->unfold_region(block, start_line, end_line);
		this->folding_status[start_line] = false;
		if (this->_mouse_over_line != -1)
		{
			this->_decorate_block(start_line, end_line);
		}
	}
	else
	{
		this->fold_region(block, start_line, end_line);
		this->folding_status[start_line] = true;
		this->_clear_scope_decos();
	}
	this->_refresh_editor_and_scrollbars();
}

void FoldingPanel::mousePressEvent(QMouseEvent *event)
{
	if (this->_mouse_over_line != -1)
	{
		QTextBlock block = getEditor()->document()->findBlockByNumber(_mouse_over_line);
		this->toggle_fold_trigger(block);
	}
}

void FoldingPanel::_on_fold_deco_clicked(TextDecoration* deco)
{
	this->toggle_fold_trigger(deco->block);
}

void FoldingPanel::on_state_changed(bool state)
{
	CodeEditor* editor = this->getEditor();
	if (state)
	{
		connect(editor, &CodeEditor::sig_key_pressed,
			this, &FoldingPanel::_on_key_pressed);
		if (this->_highlight_caret)
		{
			connect(editor, &CodeEditor::cursorPositionChanged,
				this, &FoldingPanel::_highlight_caret_scope);
			this->_block_nbr = -1;
		}
		connect(editor, &CodeEditor::new_text_set,
			this, &FoldingPanel::_clear_block_deco);
	}
	else
	{
		disconnect(editor, &CodeEditor::sig_key_pressed,
			this, &FoldingPanel::_on_key_pressed);
		if (this->_highlight_caret)
		{
			disconnect(editor, &CodeEditor::cursorPositionChanged,
				this, &FoldingPanel::_highlight_caret_scope);
			this->_block_nbr = -1;
		}
		disconnect(editor, &CodeEditor::new_text_set,
			this, &FoldingPanel::_clear_block_deco);
	}
}

void FoldingPanel::_on_key_pressed(QKeyEvent* event)
{
	bool delete_request = event->key() == Qt::Key_Backspace
		|| event->key() == Qt::Key_Delete;
	if (!event->text().isEmpty() || delete_request)
	{
		QTextCursor cursor = getEditor()->textCursor();
		QList<int> positions_to_check;
		if (cursor.hasSelection())
		{
			positions_to_check << cursor.selectionStart() 
				<< cursor.selectionEnd();
		}
		else
		{
			positions_to_check << cursor.position();
		}
		for (const int& pos : positions_to_check)
		{
			QTextBlock block = getEditor()->document()->findBlock(pos);
			int start_line = block.blockNumber() + 1;
			if (this->folding_regions.contains(start_line)
				&& this->folding_status[start_line])
			{
				int end_line = this->folding_regions[start_line];
				this->toggle_fold_trigger(block.next());
				if (delete_request && cursor.hasSelection())
				{
					QTextCursor tc = TextHelper(getEditor()).select_lines(start_line, end_line);
					int start, end;
					if (tc.selectionStart() > cursor.selectionStart())
					{
						start = cursor.selectionStart();
					}
					else
					{
						start = tc.selectionStart();
					}
					if (tc.selectionEnd() < cursor.selectionEnd())
					{
						end = cursor.selectionEnd();
					}
					else
					{
						end = tc.selectionEnd();
					}
					tc.setPosition(start);
					tc.setPosition(end, QTextCursor::KeepAnchor);
					getEditor()->setTextCursor(tc);
					this->folding_regions.remove(start_line);
					this->folding_status.remove(start_line);
				}
			}
		}
	}
}

void FoldingPanel::_show_previous_blank_lines(const QTextBlock& block)
{
	QTextBlock pblock = block.previous();
	while (pblock.blockNumber() >= 0
		&& pblock.text().trimmed().isEmpty())
	{
		pblock.setVisible(true);
		pblock = pblock.previous();
	}
}

void FoldingPanel::refresh_decorations(bool force /*= false*/)
{
	CodeEditor* editor = this->getEditor();
	QTextCursor cursor = editor->textCursor();
	if (this->_prev_cursor.isNull() || force
		|| this->_prev_cursor.blockNumber() != cursor.blockNumber())
	{
		// self.editor.decorations.remove(deco)
		for (auto deco : this->_block_decos)
		{
			deco->set_outline(drift_color(
				_get_scope_highlight_color(), 110));
			deco->set_background(_get_scope_highlight_color());
			// self.editor.decorations.add(deco)
		}
	}
	this->_prev_cursor = cursor;
}

void FoldingPanel::_refresh_editor_and_scrollbars()
{
	TextHelper th(this->getEditor());
	th.mark_whole_doc_dirty();
	this->getEditor()->repaint();
	QSize s = this->getEditor()->size();
	s.setWidth(s.width() + 1);
	this->getEditor()->resizeEvent(new QResizeEvent(this->getEditor()->size(), s));
}

void FoldingPanel::collapse_all()
{
	this->_clear_block_deco();
	QTextBlock block = this->getEditor()->document()->firstBlock();
	while (block.isValid())
	{
		int line_number = block.blockNumber();
		if (this->folding_regions.contains(line_number))
		{
			int end_line = this->folding_regions[line_number];
			this->fold_region(block, line_number, end_line);
		}
		block = block.next();
	}
	this->_refresh_editor_and_scrollbars();
	QTextCursor tc = this->getEditor()->textCursor();
	tc.movePosition(QTextCursor::Start);
	this->getEditor()->setTextCursor(tc);
	emit this->collapse_all_triggered();
}

void FoldingPanel::_clear_block_deco()
{
	// self.editor.decorations.remove(deco)
	qDeleteAll(this->_block_decos);
	this->_block_decos.clear();
}

void FoldingPanel::expand_all()
{
	QTextBlock block = this->getEditor()->document()->firstBlock();
	while (block.isValid())
	{
		int line_number = block.blockNumber();
		if (this->folding_regions.contains(line_number))
		{
			int end_line = this->folding_regions[line_number];
			this->unfold_region(block, line_number, end_line);
		}
		block = block.next();
	}
	this->_clear_block_deco();
	this->_refresh_editor_and_scrollbars();
	emit this->expand_all_triggered();
}

void FoldingPanel::_on_action_toggle()
{
	QTextBlock block = getEditor()->textCursor().block();
	block = this->find_parent_scope(block);
	this->toggle_fold_trigger(block);
}

void FoldingPanel::_on_action_collapse_all_triggered()
{
	this->collapse_all();
}

void FoldingPanel::_on_action_expand_all_triggered()
{
	this->expand_all();
}

void FoldingPanel::_highlight_caret_scope()
{
	QTextCursor cursor = this->getEditor()->textCursor();
	int block_nbr = cursor.blockNumber();
	if (this->_block_nbr != block_nbr)
	{
		QTextBlock block = this->find_parent_scope(cursor.block());
		int line_number = block.blockNumber();
		if (this->folding_regions.contains(line_number))
		{
			this->_mouse_over_line = block.blockNumber();
			this->_highlight_block(block);
		}
		else
		{
			this->_clear_scope_decos();
		}
	}
	this->_block_nbr = block_nbr;
}

void FoldingPanel::clone_settings(FoldingPanel* original)
{
	this->_native_icons = original->_native_icons;
	this->_indicators_icons = original->_indicators_icons;
	set_highlight_caret_scope(original->_highlight_caret);
	// self.custom_fold_region_background = original.custom_fold_region_background
}

bool FoldingPanel::is_collapsed(const QTextBlock& block) const
{
	int line_number = block.blockNumber();
	return this->folding_regions.contains(line_number)
		&& this->folding_status.contains(line_number);
}

std::pair<int, int> FoldingPanel::get_range(const QTextBlock& block) const
{
	int line_number = block.blockNumber();
	int end_number = line_number;
	if (this->folding_regions.contains(line_number))
	{
		end_number = this->folding_regions[line_number];
	}
	return std::make_pair(line_number, end_number);
}
