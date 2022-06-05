#include "app/tour.h"
#include "app/mainwindow.h"
#include <cassert>

void myAssert(bool ok)
{
	if (!ok)
	{
		std::exit(0);
	}
}

namespace tour {

const QColor MAIN_TOP_COLOR = QColor::fromRgb(230, 230, 230);
const QColor MAIN_BG_COLOR = QColor::fromRgb(255, 255, 255);


struct SpyderWidgets
{
	// Panes
	const QString ipython_console = "ipyconsole";
	const QString editor = "editor";
	const QString editor_line_number_area = "editor.get_current_editor().linenumberarea";
	const QString editor_scroll_flag_area = "editor.get_current_editor().scrollflagarea";
	const QString file_explorer = "explorer";
	const QString help_plugin = "help";
	const QString variable_explorer = "variableexplorer";
	const QString history_log = "historylog";

	// Toolbars
	const QString toolbars = "";
	const QString toolbars_active = "";
	const QString toolbar_file = "";
	const QString toolbar_edit = "";
	const QString toolbar_run = "";
	const QString toolbar_debug = "";
	const QString toolbar_main = "";

	const QString status_bar = "";
	const QString menu_bar = "";
	const QString menu_file = "";
	const QString menu_edit = "";
};

QList<QMap<QString, QVariant >> get_tours(int index)
{
	return get_tour(index);
}

QList<QMap<QString, QVariant >> get_tour(int index)
{
	thread_local QList<QMap<QString, QVariant>> intro;
	if (intro.isEmpty())
	{
		SpyderWidgets sw;
		const QString qtconsole_link = "https://qtconsole.readthedocs.io/en/stable/index.html";

		QMap<QString, QVariant> tmpMap;
		tmpMap.insert("title", QString("Welcome to the Introduction tour"));
		tmpMap.insert("content", QString("<b>Spyder</b> is a powerful Interactive "
			"Development Environment (or IDE) for the Python "
			"programming language.<br><br>"
			"Here we are going to guide you through its most "
			"important features.<br><br>"
			"Please use the arrow keys or click on the buttons "
			"below to move along the tour."));
		tmpMap.insert("image", QString("tour-spyder-logo.png"));
		intro << tmpMap;

		tmpMap.clear();
		tmpMap.insert("title", QString("The Editor"));
		tmpMap.insert("content", QString("This is the pane where you write Python code before "
			"evaluating it. You can get automatic suggestions "
			"and completions while writing, by pressing the "
			"<b>Tab</b> key next to a given text.<br><br>"
			"The Editor comes "
			"with a line number area (highlighted here in red), "
			"where Spyder shows warnings and syntax errors. They "
			"can help you to detect potential problems before "
			"running the code.<br><br>"
			"You can also set debug breakpoints in the line "
			"number area, by doing a double click next to "
			"a non-empty line."));
		tmpMap.insert("widgets", QString(sw.editor));
		tmpMap.insert("decoration", QString(sw.editor_line_number_area));
		intro << tmpMap;

		tmpMap.clear();
		tmpMap.insert("title", QString("The File Explorer"));
		tmpMap.insert("content", QString("This pane lets you navigate through the directories "
			"and files present in your computer.<br><br>"
			"You can also open any of these files with its "
			"corresponding application, by doing a double "
			"click on it.<br><br>"
			"There is one exception to this rule: plain-text "
			"files will always be opened in the Spyder Editor."));
		tmpMap.insert("widgets", sw.file_explorer);
		tmpMap.insert("interact", true);
		intro << tmpMap;

		tmpMap.clear();
		tmpMap.insert("title", QString("The History Log"));
		tmpMap.insert("content", QString("This pane records all commands introduced in "
			"the Python and IPython consoles."));
		tmpMap.insert("widgets", sw.history_log);
		tmpMap.insert("interact", true);
		intro << tmpMap;
	}

	QList<QMap<QString, QVariant >> tours;

	QMap<QString, QVariant > temp;
	temp.insert("name", QString("Introduction tour"));
	QList<QVariant> introVar;
	for (const QMap<QString, QVariant>& item : intro)
	{
		introVar << QVariant{ item };
	}
	temp.insert("tour", introVar);

	tours << temp;
	if (index == -2)
	{
	}
	else if (index == -1)
	{
	}
	else
	{
		return QList<QMap<QString, QVariant >>() << tours[index];
	}
	return QList<QMap<QString, QVariant >>();
}

FadingDialog::FadingDialog(MainWindow* parent, 
	std::pair<double, double> opacity, 
	std::pair<int, int> duration, 
	QList<QEasingCurve::Type> easing_curve)
	: QDialog(parent)
{
	this->parent = parent;
	this->opacity_min = std::min(opacity.first, opacity.second);
	this->opacity_max = std::max(opacity.first, opacity.second);
	this->duration_fadein = duration.first;
	this->duration_fadeout = duration.second;
	this->easing_curve_in = easing_curve.front();
	this->easing_curve_out = easing_curve.back();

	this->setModal(false);
}

void FadingDialog::_run(const QList<std::function<void()>>& funcs)
{
	for (std::function<void()> func : funcs)
	{
		func();
	}
}

void FadingDialog::_run_before_fade_in()
{
	this->_run(this->_funcs_before_fade_in);
}

void FadingDialog::_run_after_fade_in()
{
	this->_run(this->_funcs_after_fade_in);
}

void FadingDialog::_run_before_fade_out()
{
	this->_run(this->_funcs_before_fade_out);
}

void FadingDialog::_run_after_fade_out()
{
	this->_run(this->_funcs_after_fade_out);
}

void FadingDialog::_set_fade_finished()
{
	this->_fade_running = false;
}

void FadingDialog::_fade_setup()
{
	this->_fade_running = true;
	this->effect = new QGraphicsOpacityEffect(this);
	this->setGraphicsEffect(this->effect);//QWidget takes ownership of effect.
	this->anim = std::make_unique<QPropertyAnimation>(this->effect, QByteArray("opacity"));
}

void FadingDialog::fade_in(QObject* receiver, const char* on_finished_connect)
{
	this->_run_before_fade_in();
	this->_fade_setup();
	this->show();
	this->raise();
	this->anim->setEasingCurve(this->easing_curve_in);
	this->anim->setStartValue(this->opacity_min);
	this->anim->setEndValue(this->opacity_max);
	this->anim->setDuration(this->duration_fadein);
	bool ok = connect(this->anim.get(), SIGNAL(finished()), receiver, on_finished_connect);
	myAssert(ok);
	ok = connect(this->anim.get(), SIGNAL(finished()), this, SLOT(_set_fade_finished()));
	myAssert(ok);
	ok = connect(this->anim.get(), SIGNAL(finished()), this, SLOT(_run_after_fade_in()));
	myAssert(ok);
	this->anim->start();
}

void FadingDialog::fade_out(QObject* receiver, const char* on_finished_connect)
{
	this->_run_before_fade_out();
	this->_fade_setup();
	this->anim->setEasingCurve(this->easing_curve_out);
	this->anim->setStartValue(this->opacity_max);
	this->anim->setEndValue(this->opacity_min);
	this->anim->setDuration(this->duration_fadeout);
	bool ok = connect(this->anim.get(), SIGNAL(finished()), receiver, on_finished_connect);
	myAssert(ok);
	ok = connect(this->anim.get(), SIGNAL(finished()), this, SLOT(_set_fade_finished()));
	myAssert(ok);
	ok = connect(this->anim.get(), SIGNAL(finished()), this, SLOT(_run_after_fade_out()));
	myAssert(ok);
	this->anim->start();
}

bool FadingDialog::is_fade_running() const
{
	return this->_fade_running;
}

void FadingDialog::set_funcs_before_fade_in(const QList<std::function<void()>>& funcs)
{
	this->_funcs_before_fade_in = funcs;
}

void FadingDialog::set_funcs_after_fade_in(const QList<std::function<void()>>& funcs)
{
	this->_funcs_after_fade_in = funcs;
}

void FadingDialog::set_funcs_before_fade_out(const QList<std::function<void()>>& funcs)
{
	this->_funcs_before_fade_out = funcs;
}

void FadingDialog::set_funcs_after_fade_out(const QList<std::function<void()>>& funcs)
{
	this->_funcs_after_fade_out = funcs;
}


FadingCanvas::FadingCanvas(MainWindow* parent, 
	std::pair<double, double> opacity, 
	std::pair<int, int> duration, 
	QList<QEasingCurve::Type> easing_curve, 
	Qt::GlobalColor color, AnimatedTour* tour /*= nullptr*/)
	: FadingDialog(parent, opacity, duration, easing_curve)
{
	this->parent = parent;
	this->tour = tour;

	this->color = color;

	// widget setup
	this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
	this->setAttribute(Qt::WA_TranslucentBackground);
	this->setAttribute(Qt::WA_TransparentForMouseEvents);
	this->setModal(false);
	this->setFocusPolicy(Qt::NoFocus);

	std::function<void()> update_canvas_func = [this]() { this->update_canvas(); };
	std::function<void()> update_widgets_func = [this]() { this->update_widgets(QList<QWidget*>()); };
	std::function<void()> update_decora_func = [this]() { this->update_decoration(QList<QWidget*>()); };

	this->set_funcs_before_fade_in(QList<std::function<void()>>() << update_canvas_func);
	this->set_funcs_after_fade_out(QList<std::function<void()>>() 
		<< update_widgets_func << update_decora_func);
}

void FadingCanvas::set_interaction(bool value)
{
	this->interaction_on = value;
}

void FadingCanvas::update_canvas()
{
	const int w = this->parent->size().width();
	const int h = this->parent->size().height();

	this->path_full = QPainterPath();
	this->path_subtract = QPainterPath();
	this->path_decoration = QPainterPath();
	this->region_mask = QRegion(0, 0, w, h);

	this->path_full.addRect(0, 0, w, h);

	// Add the path
	if (!this->widgets.isEmpty())
	{
		for (QWidget* widget : this->widgets)
		{
			QPainterPath temp_path = QPainterPath();
			QRegion temp_region;
			if (widget)
			{
				widget->raise();
				widget->show();
				QRect geo = widget->frameGeometry();
				const int width = geo.width();
				const int height = geo.height();
				QPoint point = widget->mapTo(this->parent, QPoint(0, 0));
				const int x = point.x();
				const int y = point.y();

				temp_path.addRect(QRectF(x, y, width, height));

				temp_region = QRegion(x, y, width, height);
			}
			if (this->interaction_on)
			{
				this->region_mask = this->region_mask.subtracted(temp_region);
			}
			this->path_subtract = this->path_subtract.united(temp_path);
		}
		this->path_current = this->path_full.subtracted(this->path_subtract);
	}
	else
	{
		this->path_current = this->path_full;
	}

	if (!this->decoration.isEmpty())
	{
		for (QWidget* widget : this->decoration)
		{
			if (!widget)
			{
				continue;
			}
			QPainterPath temp_path = QPainterPath();
			widget->raise();
			widget->show();
			QRect geo = widget->frameGeometry();
			const int width = geo.width();
			const int height = geo.height();
			QPoint point = widget->mapTo(this->parent, QPoint(0, 0));
			const int x = point.x();
			const int y = point.y();
			temp_path.addRect(QRectF(x, y, width, height));

			QRegion temp_region_1 = QRegion(x - 1, y - 1, width + 2, height + 2);
			QRegion temp_region_2 = QRegion(x + 1, y + 1, width - 2, height - 2);
			QRegion temp_region = temp_region_1.subtracted(temp_region_2);

			if (this->interaction_on)
			{
				this->region_mask = this->region_mask.united(temp_region);
			}
			this->path_decoration = this->path_decoration.united(temp_path);
		}
	}
	else
	{
		this->path_decoration.addRect(0, 0, 0, 0);
	}

	// Add a decoration stroke around widget
	this->setMask(this->region_mask);
	this->update();
	this->repaint();
}

void FadingCanvas::update_widgets(const QList<QWidget*>& widgets)
{
	this->widgets = widgets;
}

void FadingCanvas::update_decoration(const QList<QWidget*>& widgets)
{
	this->decoration = widgets;
}

void FadingCanvas::paintEvent(QPaintEvent* AEvent)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	// Decoration
	painter.fillPath(this->path_current, QBrush(this->color));
	painter.strokePath(this->path_decoration, QPen(this->color_decoration,
		this->stroke_decoration));
}

void FadingCanvas::reject()
{
	if (!this->is_fade_running())
	{
		Qt::Key key = Qt::Key_Escape;
		this->key_pressed = key;
		emit this->sig_key_pressed();
	}
}

void FadingCanvas::mousePressEvent(QMouseEvent* AEvent)
{
	// python源码是pass
	return;
}

void FadingCanvas::focusInEvent(QFocusEvent* AEvent)
{
	// To be used so tips do not appear outside spyder
	if (this->hasFocus())
	{
		this->tour->gain_focus();
	}
}

void FadingCanvas::focusOutEvent(QFocusEvent* AEvent)
{
	if (this->tour->step_current != 0)
	{
		this->tour->lost_focus();
	}
}


FadingTipBox::FadingTipBox(MainWindow* parent, 
	std::pair<double, double> opacity, 
	std::pair<int, int> duration, 
	QList<QEasingCurve::Type> easing_curve, 
	AnimatedTour* tour /*= nullptr*/, 
	QColor color_top /*= QColor()*/, 
	QColor color_back /*= QColor()*/, 
	QColor combobox_background /*= QColor()*/)
	: FadingDialog(parent, opacity, duration, easing_curve)
{
	this->parent = parent;
	this->tour = tour;

	this->setAttribute(Qt::WA_TranslucentBackground);
	this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint |
		Qt::WindowStaysOnTopHint);
	this->setModal(false);

	auto toolbutton = [this](const QIcon& icon)->QToolButton*
	{
		QToolButton* bt = new QToolButton(this);
		bt->setAutoRaise(true);
		bt->setIcon(icon);
		return bt;
	};

	this->button_close = toolbutton(ima::icon("DialogCloseButton"));// tour.close
	this->button_home = toolbutton(ima::icon("MediaSkipBackward"));// tour.home
	this->button_previous = toolbutton(ima::icon("MediaSeekBackward"));// tour.previous
	this->button_end = toolbutton(ima::icon("MediaSkipForward"));// tour.end
	this->button_next = toolbutton(ima::icon("MediaSeekForward"));// tour.next
	this->button_run = new QPushButton("Run code", this);
	this->button_disable = QString();
	//this->button_current = new QToolButton(this);
	this->label_image = new QLabel(this);

	//this->label_title = new QLabel(this);
	this->combo_title = new QComboBox(this);
	this->label_current = new QLabel(this);
	this->label_content = new QLabel(this);

	this->label_content->setMinimumWidth(this->fixed_width);
	this->label_content->setMaximumWidth(this->fixed_width);

	this->label_current->setAlignment(Qt::AlignCenter);

	this->label_content->setWordWrap(true);

	this->widgets << this->label_content //<< this->label_title
		<< this->label_current << this->combo_title
		<< this->button_close << this->button_run << this->button_next
		<< this->button_previous << this->button_end
		<< this->button_home;// << this->button_current;

	const QString arrow = get_image_path("hide.png");

	this->color_top = color_top;
	this->color_back = color_back;
	this->combobox_background = combobox_background;
	const QString style = "QComboBox {{"
					"padding-left: 5px;"
					"background-color: %1;"
					"border-width : 0px;"
					"border-radius: 0px;"
					"min-height:20px;"
					"max-height:20px;"
					"}}"

					"QComboBox::drop-down {{"
					"subcontrol-origin: padding;"
					"subcontrol-position: top left;"
					"border-width: 0px;"
					"}}"

					"QComboBox::down-arrow {{"
					"image: url(%2);"
					"}}";
	this->stylesheet = style.arg(this->combobox_background.name()).arg(arrow);
	// Windows fix, slashes should be always in unix - style
	this->stylesheet = this->stylesheet.replace('\\', '/');

	this->setFocusPolicy(Qt::StrongFocus);
	for (QWidget* widget : this->widgets)
	{
		widget->setFocusPolicy(Qt::NoFocus);
		widget->setStyleSheet(this->stylesheet);
	}
	
	QHBoxLayout* layout_top = new QHBoxLayout();
	layout_top->addWidget(this->combo_title);
	layout_top->addStretch();
	layout_top->addWidget(this->button_close);
	layout_top->addSpacerItem(new QSpacerItem(this->offset_shadow,
		this->offset_shadow));

	QHBoxLayout* layout_content = new QHBoxLayout();
	layout_content->addWidget(this->label_content);
	layout_content->addWidget(this->label_image);
	layout_content->addSpacerItem(new QSpacerItem(5, 5));

	QHBoxLayout* layout_run = new QHBoxLayout();
	layout_run->addStretch();
	layout_run->addWidget(this->button_run);
	layout_run->addStretch();
	layout_run->addSpacerItem(new QSpacerItem(this->offset_shadow,
		this->offset_shadow));

	QHBoxLayout* layout_navigation = new QHBoxLayout();
	layout_navigation->addWidget(this->button_home);
	layout_navigation->addWidget(this->button_previous);
	layout_navigation->addStretch();
	layout_navigation->addWidget(this->label_current);
	layout_navigation->addStretch();
	layout_navigation->addWidget(this->button_next);
	layout_navigation->addWidget(this->button_end);
	layout_navigation->addSpacerItem(new QSpacerItem(this->offset_shadow,
		this->offset_shadow));

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(layout_top);
	layout->addStretch();
	layout->addSpacerItem(new QSpacerItem(15, 15));
	layout->addLayout(layout_content);
	layout->addLayout(layout_run);
	layout->addStretch();
	layout->addSpacerItem(new QSpacerItem(15, 15));
	layout->addLayout(layout_navigation);
	layout->addSpacerItem(new QSpacerItem(this->offset_shadow,
		this->offset_shadow));

	layout->setSizeConstraint(QLayout::SetFixedSize);

	this->setLayout(layout);

	std::function<void()> disableFunc = [this]() { this->_disable_widgets(); };
	std::function<void()> enableFunc = [this]() { this->_enable_widgets(); };
	std::function<void()> setFocusFunc = [this]() { this->setFocus(); };

	this->set_funcs_before_fade_in(QList<std::function<void()>>() << disableFunc);
	this->set_funcs_after_fade_in(QList<std::function<void()>>() << enableFunc << setFocusFunc);
	this->set_funcs_before_fade_out(QList<std::function<void()>>() << disableFunc);

	this->setContextMenuPolicy(Qt::CustomContextMenu);
}

void FadingTipBox::_disable_widgets()
{
	for (QWidget* widget : this->widgets)
	{
		widget->setDisabled(true);
	}
}

void FadingTipBox::_enable_widgets()
{
	this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint |
		Qt::WindowStaysOnTopHint);
	for (QWidget* widget : this->widgets)
	{
		widget->setDisabled(false);
	}

	if (this->button_disable == "previous")
	{
		this->button_previous->setDisabled(true);
		this->button_home->setDisabled(true);
	}
	else if (this->button_disable == "next")
	{
		this->button_next->setDisabled(true);
		this->button_end->setDisabled(true);
	}
}

void FadingTipBox::set_data(QString title, QString content, 
	QString current, QString image, 
	QStringList run,
	QStringList frames /*= QStringList()*/, 
	int step /*= -1*/)
{
	//this->label_title->setText(title);
	this->combo_title->clear();
	this->combo_title->addItems(frames);
	this->combo_title->setCurrentIndex(step);

	this->label_current->setText(current);
	//this->button_current->setText(current);
	this->label_content->setText(content);

	if (image.isEmpty())
	{
		this->label_image->setFixedHeight(1);
		this->label_image->setFixedWidth(1);
	}
	else
	{
		QString extension = image.split('.').back();
		QPixmap imagePixmap = QPixmap(get_image_path(image), extension.toStdString().c_str());
		this->label_image->setPixmap(imagePixmap);
		this->label_image->setFixedSize(imagePixmap.size());
	}

	if (run.isEmpty())
	{
		this->button_run->setVisible(false);
	}
	else
	{
		this->button_run->setDisabled(false);
		this->button_run->setVisible(true);
	}

	// Refresh layout
	this->layout()->activate();
}

void FadingTipBox::set_pos(double x, double y)
{
	this->move(QPoint(qCeil(x), qCeil(y)));
}

void FadingTipBox::build_paths()
{
	QRect geo = this->geometry();
	const int radius = 0;
	const int shadow = this->offset_shadow;
// 	const int x0 = geo.x();
// 	const int y0 = geo.y();
	const int width = geo.width() - shadow;
	const int height = geo.height() - shadow;

	int left = 0;
	int top = 0;
	int right = width;
	const int bottom = height;

	this->round_rect_path = QPainterPath();
	this->round_rect_path.moveTo(right, top + radius);
	this->round_rect_path.arcTo(right - radius, top, radius, radius, 0.0,
		90.0);
	this->round_rect_path.lineTo(left + radius, top);
	this->round_rect_path.arcTo(left, top, radius, radius, 90.0, 90.0);
	this->round_rect_path.lineTo(left, bottom - radius);
	this->round_rect_path.arcTo(left, bottom - radius, radius, radius, 180.0,
		90.0);
	this->round_rect_path.lineTo(right - radius, bottom);
	this->round_rect_path.arcTo(right - radius, bottom - radius, radius, radius,
		270.0, 90.0);
	this->round_rect_path.closeSubpath();

	// Top path
	int header = 36;
	int offset = 2;
	left = offset;
	top = offset;
	right = width - (offset);
	this->top_rect_path = QPainterPath();
	this->top_rect_path.lineTo(right, top + radius);
	this->top_rect_path.moveTo(right, top + radius);
	this->top_rect_path.arcTo(right - radius, top, radius, radius, 0.0, 90.0);
	this->top_rect_path.lineTo(left + radius, top);
	this->top_rect_path.arcTo(left, top, radius, radius, 90.0, 90.0);
	this->top_rect_path.lineTo(left, top + header);
	this->top_rect_path.lineTo(right, top + header);
}

void FadingTipBox::paintEvent(QPaintEvent* AEvent)
{
	this->build_paths();

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	painter.fillPath(this->round_rect_path, this->color_back);
	painter.fillPath(this->top_rect_path, this->color_top);
	painter.strokePath(this->round_rect_path, QPen(Qt::gray, 1));
}

void FadingTipBox::keyReleaseEvent(QKeyEvent* AEvent)
{
	int key = AEvent->key();
	this->key_pressed = static_cast<Qt::Key>(key);

	const QSet<Qt::Key> keys = { Qt::Key_Right, 
		Qt::Key_Left, Qt::Key_Down, Qt::Key_Up,
		Qt::Key_Escape, Qt::Key_PageUp, Qt::Key_PageDown,
		Qt::Key_Home, Qt::Key_End, Qt::Key_Menu };

	if (keys.contains(this->key_pressed))
	{
		if (!this->is_fade_running())
		{
			emit this->sig_key_pressed();
		}
	}
}

void FadingTipBox::mousePressEvent(QMouseEvent* AEvent)
{
	// Raise the main application window on click
	this->parent->raise();
	this->raise();

	if (AEvent->button() == Qt::RightButton)
	{
		// python源码是pass
		return;
	}
}

void FadingTipBox::focusOutEvent(QFocusEvent* AEvent)
{
	this->tour->lost_focus();
}

void FadingTipBox::context_menu_requested(const QPoint& AEvent)
{
	QPoint pos{ AEvent.x(), AEvent.y() };
	QMenu menu(this);

	QAction* action_title = create_action(this, "Go to step: ");
	action_title->setDisabled(true);
	add_actions(&menu, QList<QAction*>() << action_title);

	menu.popup(this->mapToGlobal(pos));
}

void FadingTipBox::reject()
{
	if (!this->is_fade_running())
	{
		Qt::Key key = Qt::Key_Escape;
		this->key_pressed = key;
		emit this->sig_key_pressed();
	}
}

AnimatedTour::AnimatedTour(MainWindow* parent)
	: QWidget(parent)
{
	this->parent = parent;

	// Widgets
	this->canvas = new FadingCanvas(this->parent, this->opacity_canvas,
		this->duration_canvas, this->easing_curve,
		this->color, this);
	this->tips = new FadingTipBox(this->parent, this->opacity_tips,
		this->duration_tips, this->easing_curve,
		this, MAIN_TOP_COLOR,
		MAIN_BG_COLOR,
		MAIN_TOP_COLOR);

	// Widgets setup
	// Needed to fix spyder - ide / spyder#2204.
	this->setAttribute(Qt::WA_TransparentForMouseEvents);

	// Signals and slots
	connect(this->tips->button_next, &QAbstractButton::clicked,
		this, &AnimatedTour::next_step);
	connect(this->tips->button_previous, &QAbstractButton::clicked,
		this, &AnimatedTour::previous_step);
	connect(this->tips->button_close, &QAbstractButton::clicked,
		this, &AnimatedTour::close_tour);
	connect(this->tips->button_run, &QAbstractButton::clicked,
		this, &AnimatedTour::run_code);
	connect(this->tips->button_home, &QAbstractButton::clicked,
		this, &AnimatedTour::first_step);
	connect(this->tips->button_end, &QAbstractButton::clicked,
		this, &AnimatedTour::last_step);

	connect(this->tips->button_run, &QAbstractButton::clicked,
		this, [this]() { this->tips->button_run->setDisabled(true); });
	bool ok = connect(this->tips->combo_title, SIGNAL(currentIndexChanged(int)),
		this, SLOT(go_to_step(int)));
	myAssert(ok);

	// Main window move or resize
	connect(this->parent, &MainWindow::sig_resized,
		this, &AnimatedTour::_resized);
	connect(this->parent, &MainWindow::sig_moved,
		this, &AnimatedTour::_moved);

	// To capture the arrow keys that allow moving the tour
	connect(this->tips, &FadingTipBox::sig_key_pressed,
		this, &AnimatedTour::_key_pressed);

	// To control the focus of tour
	this->setting_data = false;
	this->hidden = false;
}

void AnimatedTour::_resized(QResizeEvent* AEvent)
{
	if (this->is_running)
	{
		QSize size = AEvent->size();
		this->canvas->setFixedSize(size);
		this->canvas->update_canvas();

		if (this->is_tour_set)
		{
			this->_set_data();
		}
	}
}

void AnimatedTour::_moved(QMoveEvent* AEvent)
{
	if (this->is_running)
	{
		QPoint pos = AEvent->pos();
		this->canvas->move(pos);

		if (this->is_tour_set)
		{
			this->_set_data();
		}
	}
}

void AnimatedTour::_close_canvas()
{
	this->tips->hide();
	this->canvas->fade_out(this->canvas, SLOT(hide()));
}

void AnimatedTour::_clear_canvas()
{
	// TODO: Add option to also make it white... might be usefull?
	// Make canvas black before transitions
	this->canvas->update_widgets(QList<QWidget*>());
	this->canvas->update_decoration(QList<QWidget*>());
	this->canvas->update_canvas();
}

void AnimatedTour::_move_step()
{
	this->_set_data();

	// Show/raise the widget so it is located first!
	QDockWidget* widgets = this->dockwidgets;
	if (widgets)
	{
		widgets->show();
		widgets->raise();
	}

	this->_locate_tip_box();

	// Change in canvas only after fadein finishes, for visual aesthetics
	this->tips->fade_in(this->canvas, SLOT(update_canvas()));
	this->tips->raise();
}

void AnimatedTour::_set_modal(bool value, const QList<QWidget*>& widgets)
{
#ifdef Q_OS_WIN
	for (QWidget* widget : widgets)
	{
		QDialog* dlg = qobject_cast<QDialog*>(widget);
		if (dlg)
		{
			dlg->setModal(value);
		}
		widget->hide();
		widget->show();
	}
#endif
}

std::pair<QWidget*, QDockWidget*> AnimatedTour::_process_widgets(
	const QString& names, MainWindow* spy_window) const
{
	QString base = names.split('.')[0];
	auto temp = reinterpret_cast<SpyderPluginMixin*>(spy_window
		->property(base.toStdString().c_str()).toULongLong());
	if (!temp)
	{
		return {nullptr, nullptr};
	}
	auto pwidget = dynamic_cast<QWidget*>(temp);
	if (names.contains("get_current_editor()"))
	{
		Editor* pEditor = dynamic_cast<Editor*>(temp);
		if (!pEditor)
		{
			return {nullptr, nullptr};
		}
		CodeEditor* pCodeEditor = pEditor->get_current_editor();
		if (!pCodeEditor)
		{
			return {nullptr, nullptr};
		}
		const QString suffix = names.split('.').back();
		if (suffix == "linenumberarea")
		{
			// 这里不关心dockwidget
			return std::make_pair(pCodeEditor->linenumberarea, nullptr);
		}
	}
	QDockWidget* pdockwidget = temp->dockwidget;
	return std::make_pair(pwidget, pdockwidget);
}

void AnimatedTour::_set_data()
{
	this->setting_data = true;
	const int step = this->step_current;
	const int steps = this->steps;
	const QList<QMap<QString, QVariant>> frames = this->frames;
	const QString current = QString("%1/%2").arg(step + 1).arg(steps);
	const QMap<QString, QVariant> frame = frames[step];

	const QStringList combobox_frames = [&frames]()->QStringList
	{
		QStringList res;
		for (int i = 0; i < frames.size(); i++)
		{
			const QMap<QString, QVariant> f = frames[i];
			res << QString("%1. %2").arg(i+1).arg(f["title"].toString());
		}
		return res;
	}();

	QString title, content, image;
	this->widgets = nullptr;
	this->dockwidgets = nullptr;
	this->decoration = nullptr;
	this->run = QStringList();

	if (frame.contains("title"))
	{
		title = frame["title"].toString();
	}

	if (frame.contains("content"))
	{
		content = frame["content"].toString();
	}

	if (frame.contains("widgets"))
	{
		const QString widget_names = frame["widgets"].toString();
		std::pair<QWidget*, QDockWidget*> tmpPair = 
			this->_process_widgets(widget_names, this->spy_window);
		this->widgets = tmpPair.first;
		this->dockwidgets = tmpPair.second;
	}

	if (frame.contains("decoration"))
	{
		const QString widget_names = frame["decoration"].toString();
		std::pair<QWidget*, QDockWidget*> tmpPair =
			this->_process_widgets(widget_names, this->spy_window);
		this->decoration = tmpPair.first;//Python源码中获取的是second
	}

	if (frame.contains("image"))
	{
		image = frame["image"].toString();
	}

	if (frame.contains("interact"))
	{
		bool interact = frame["interact"].toBool();
		this->canvas->set_interaction(interact);
		this->_set_modal(!interact, QList<QWidget*>() << this->tips);
	}
	else
	{
		this->canvas->set_interaction(false);
		this->_set_modal(true, QList<QWidget*>() << this->tips);
	}

	if (frame.contains("run"))
	{
		this->run = frame["run"].toStringList();
	}

	this->tips->set_data(title, content, current, image, run,
		combobox_frames, step);
	this->_check_buttons();

	// Make canvas black when starting a new place of decoration
	this->canvas->update_widgets(QList<QWidget*>() << dockwidgets);
	this->canvas->update_decoration(QList<QWidget*>() << decoration);
	this->setting_data = false;
}

void AnimatedTour::_locate_tip_box()
{
	// Store the dimensions of the main window
	QRect geo = this->parent->frameGeometry();
	int x = geo.x();
	int y = geo.y();
	int width = geo.width();
	int height = geo.height();

	const int x_main = x;
	const int y_main = y;
	const int width_main = width;
	const int height_main = height;

	const int delta = 20;

	// Here is the tricky part to define the best position for the tip widget
	if (this->dockwidgets)
	{
		geo = this->dockwidgets->geometry();
		x = geo.x();
		y = geo.y();
		width = geo.width();
		height = geo.height();

		QPoint point = this->dockwidgets->mapToGlobal(QPoint(0, 0));
		const int x_glob = point.x();
		const int y_glob = point.y();

		// Check if is too tall and put to the side
		const int y_fac = height * 100 / height_main;

		if (y_fac > 60)
		{
			if (x < this->tips->width())
			{
				x = x_glob + width + delta;
				y = y_glob + height / 2 - this->tips->height() / 2;
			}
			else
			{
				x = x_glob - this->tips->width() - delta;
				y = y_glob + height / 2 - this->tips->height() / 2;
			}
		}
		else
		{
			if (y < this->tips->height())
			{
				x = x_glob + width / 2 - this->tips->width() / 2;
				y = y_glob + height + delta;
			}
			else
			{
				x = x_glob + width / 2 - this->tips->width() / 2;
				y = y_glob - delta - this->tips->height();
			}
		}
	}
	else
	{
		// Center on parent
		x = x_main + width_main / 2 - this->tips->width() / 2;
		y = y_main + height_main / 2 - this->tips->height() / 2;
	}

	this->tips->set_pos(x, y);
}

void AnimatedTour::_check_buttons()
{
	this->tips->button_disable = QString();

	if (this->step_current == 0)
	{
		this->tips->button_disable = "previous";
	}
	else if (this->step_current == this->steps - 1)
	{
		this->tips->button_disable = "next";
	}
}

void AnimatedTour::_key_pressed()
{
	const Qt::Key key = this->tips->key_pressed;
	switch (key)
	{
	case Qt::Key_Right:
	case Qt::Key_Down:
	case Qt::Key_PageDown:
	{
		if (this->step_current != this->steps - 1)
		{
			this->next_step();
		}
	}
		break;
	case Qt::Key_Left:
	case Qt::Key_Up:
	case Qt::Key_PageUp:
	{
		if (this->step_current != 0)
		{
			this->previous_step();
		}
	}
		break;
	case Qt::Key_Escape:
		this->close_tour();
		break;
	case Qt::Key_Home:
	{
		if (this->step_current != 0)
		{
			this->first_step();
		}
	}
		break;
	case Qt::Key_End:
	{
		if (this->step_current != this->steps - 1)
		{
			this->last_step();
		}
	}
		break;
	case Qt::Key_Menu:
	{
		QPoint pos = this->tips->label_current->pos();
		this->tips->context_menu_requested(pos);
	}
		break;
	default:
		break;
	}
}

void AnimatedTour::_hiding()
{
	this->hidden = true;
	this->tips->hide();
}

void AnimatedTour::run_code()
{
	// TODO Not impl，需要使用到console
}

void AnimatedTour::set_tour(int index, const QMap<QString, QVariant >& frames,
	MainWindow* spy_window)
{
	this->spy_window = spy_window;
	this->active_tour_index = index;
	this->last_frame_active = frames["last"].toInt();
	{
		const QList<QVariant> varList = frames["tour"].toList();
		this->frames.clear();
		for (const QVariant& var : varList)
		{
			this->frames << var.toMap();
		}
	}
	this->steps = this->frames.size();

	this->is_tour_set = true;
}

void AnimatedTour::start_tour()
{
	QRect geo = this->parent->geometry();
	const int x = geo.x();
	const int y = geo.y();
	const int width = geo.width();
	const int height = geo.height();

	// FIXME: reset step to last used value
	// Reset step to begining
	this->step_current = this->last_frame_active;

	// Adjust the canvas size to match the main window size
	this->canvas->setFixedSize(width, height);
	this->canvas->move(QPoint(x, y));
	this->canvas->fade_in(this, SLOT(_move_step()));
	this->_clear_canvas();

	this->is_running = true;
}

void AnimatedTour::close_tour()
{
	this->tips->fade_out(this, SLOT(_close_canvas()));
	this->canvas->set_interaction(false);
	this->_set_modal(true, QList<QWidget*>() << this->tips);
	this->canvas->hide();

	try
	{
		// set the last played frame by updating the available tours in
		// parent. This info will be lost on restart.
		this->parent->tours_available[this->active_tour_index]["last"] = 
			this->step_current;
	}
	catch (const std::exception&)
	{
		
	}

	this->is_running = false;
}

void AnimatedTour::hide_tips()
{
	this->_clear_canvas();
	this->tips->fade_out(this, SLOT(_hiding()));
}

void AnimatedTour::unhide_tips()
{
	this->_clear_canvas();
	this->_move_step();
	this->hidden = false;
}

void AnimatedTour::next_step()
{
	this->_clear_canvas();
	this->step_current += 1;
	this->tips->fade_out(this, SLOT(_move_step()));
}

void AnimatedTour::previous_step()
{
	this->_clear_canvas();
	this->step_current -= 1;
	this->tips->fade_out(this, SLOT(_move_step()));
}

void AnimatedTour::go_to_step(int number)
{
	this->_clear_canvas();
	this->step_current = number;
	this->tips->fade_out(this, SLOT(_move_step()));
}

void AnimatedTour::last_step()
{
	this->go_to_step(this->steps - 1);
}

void AnimatedTour::first_step()
{
	this->go_to_step(0);
}

void AnimatedTour::lost_focus()
{
	if (this->is_running && (!this->any_has_focus()) &&
		(!this->setting_data) && (!this->hidden))
	{
		this->hide_tips();
	}
}

void AnimatedTour::gain_focus()
{
	if (this->is_running && this->any_has_focus() &&
		(!this->setting_data) && this->hidden)
	{
		this->unhide_tips();
	}
}

bool AnimatedTour::any_has_focus() const
{
	bool f = (this->hasFocus() || this->parent->hasFocus() ||
		this->tips->hasFocus() || this->canvas->hasFocus());
	return f;
}

} // namespace tour