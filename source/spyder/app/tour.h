#pragma once
// Standard library imports
#include <memory>
#include <functional>

// Local imports
#include "config/base.h"
#include "utils/qthelpers.h"
#include "utils/icon_manager.h"

class MainWindow;

namespace tour {

class AnimatedTour;

QList<QMap<QString, QVariant >> get_tours(int index = -2);
QList<QMap<QString, QVariant >> get_tour(int index);

class FadingDialog : public QDialog
{
	Q_OBJECT
Q_SIGNALS:
	void sig_key_pressed();
public:
	FadingDialog(MainWindow* parent,
		std::pair<double, double> opacity,
		std::pair<int, int> duration,
		QList<QEasingCurve::Type> easing_curve);

private:
	void _run(const QList<std::function<void()>>& funcs);
	void _run_before_fade_in();
	void _run_before_fade_out();
	void _fade_setup();

private Q_SLOTS:
	void _run_after_fade_in();
	void _run_after_fade_out();
	void _set_fade_finished();

public:
	void fade_in(QObject* receiver, const char* on_finished_connect);
	void fade_out(QObject* receiver, const char* on_finished_connect);

	bool is_fade_running() const;

	void set_funcs_before_fade_in(const QList<std::function<void()>>& funcs);
	void set_funcs_after_fade_in(const QList<std::function<void()>>& funcs);
	void set_funcs_before_fade_out(const QList<std::function<void()>>& funcs);
	void set_funcs_after_fade_out(const QList<std::function<void()>>& funcs);
private:
	MainWindow* parent;

	double opacity_min;
	double opacity_max;
	int duration_fadein;
	int duration_fadeout;
	QEasingCurve::Type easing_curve_in;
	QEasingCurve::Type easing_curve_out;
	QGraphicsOpacityEffect* effect = nullptr;
	std::unique_ptr<QPropertyAnimation> anim = nullptr;

	bool _fade_running = false;
	QList<std::function<void()>> _funcs_before_fade_in;
	QList<std::function<void()>> _funcs_after_fade_in;
	QList<std::function<void()>> _funcs_before_fade_out;
	QList<std::function<void()>> _funcs_after_fade_out;
};


class FadingCanvas : public FadingDialog
{
	Q_OBJECT
public:
	FadingCanvas(MainWindow* parent,
		std::pair<double, double> opacity,
		std::pair<int, int> duration,
		QList<QEasingCurve::Type> easing_curve,
		Qt::GlobalColor color, AnimatedTour* tour = nullptr);

	void set_interaction(bool value);
	void update_widgets(const QList<QWidget*>& widgets);
	void update_decoration(const QList<QWidget*>& widgets);

	void paintEvent(QPaintEvent* AEvent) override;
	void reject() override;
	void mousePressEvent(QMouseEvent* AEvent) override;
	void focusInEvent(QFocusEvent* AEvent) override;
	void focusOutEvent(QFocusEvent* AEvent) override;

public Q_SLOTS:
	void update_canvas();
private:
	MainWindow* parent;
	AnimatedTour* tour;

	Qt::GlobalColor color;						 // Canvas color
	Qt::GlobalColor color_decoration = Qt::red;  // Decoration color
	int stroke_decoration = 2;					 // width in pixels for decoration

	QRegion region_mask;
	QRegion region_subtract;
	QRegion region_decoration;

	QList<QWidget*> widgets;
	QList<QWidget*> decoration;
	bool interaction_on = false;

	QPainterPath path_current;
	QPainterPath path_subtract;
	QPainterPath path_full;
	QPainterPath path_decoration;

	Qt::Key key_pressed;
};


class FadingTipBox : public FadingDialog
{
	Q_OBJECT
public:
	FadingTipBox(MainWindow* parent,
		std::pair<double, double> opacity,
		std::pair<int, int> duration,
		QList<QEasingCurve::Type> easing_curve,
		AnimatedTour* tour = nullptr,
		QColor color_top = QColor(),
		QColor color_back = QColor(),
		QColor combobox_background = QColor());

	void set_data(QString title, QString content,
		QString current, QString image,
		QStringList run,
		QStringList frames = QStringList(),
		int step = -1);
	void set_pos(double x, double y);
	void build_paths();

	void paintEvent(QPaintEvent* AEvent) override;
	void keyReleaseEvent(QKeyEvent* AEvent) override;
	void mousePressEvent(QMouseEvent* AEvent) override;
	void focusOutEvent(QFocusEvent* AEvent) override;

	void context_menu_requested(const QPoint& AEvent);
	void reject() override;

private:
	void _disable_widgets();
	void _enable_widgets();

private:
	friend class AnimatedTour;
	//QPropertyAnimation* holder;//python需要该成员维护引用计数，C++不需要
	MainWindow* parent;
	AnimatedTour* tour;

	// frames = None 未使用
	int offset_shadow = 0;
	int fixed_width = 300;

	Qt::Key key_pressed = Qt::Key_unknown;

	QToolButton* button_close;
	QToolButton* button_home;
	QToolButton* button_previous;
	QToolButton* button_end;
	QToolButton* button_next;
	QPushButton* button_run;
	QString button_disable;
	//QToolButton* button_current;
	QLabel* label_image;

	//QLabel* label_title;
	QComboBox* combo_title;
	QLabel* label_current;
	QLabel* label_content;

	QList<QWidget*> widgets;

	QColor color_top;
	QColor color_back;
	QColor combobox_background;

	QString stylesheet;

	QPainterPath round_rect_path;
	QPainterPath top_rect_path;

	//QPixmap image;只在set_data函数中使用，因此改为局部变量，并重命名为imagePixmap
};


class AnimatedTour : public QWidget
{
	Q_OBJECT
public:
	explicit AnimatedTour(MainWindow* parent);

private Q_SLOTS:
	void _resized(QResizeEvent* AEvent);
	void _moved(QMoveEvent* AEvent);
	void _close_canvas();
	void _move_step();
	void _hiding();
	void go_to_step(int number);
private:
	void _clear_canvas();
	void _set_modal(bool value, const QList<QWidget*>& widgets);
	std::pair<QWidget*, QDockWidget*> _process_widgets(
		const QString& names, MainWindow* spy_window) const;
	void _set_data();
	void _locate_tip_box();
	void _check_buttons();
	void _key_pressed();

public:
	void run_code();
	void set_tour(int index, const QMap<QString, QVariant >& frames,
		MainWindow* spy_window);
	void start_tour();
	void close_tour();

	void hide_tips();
	void unhide_tips();
	void next_step();
	void previous_step();

	void last_step();
	void first_step();
	void lost_focus();
	void gain_focus();
	bool any_has_focus() const;

private:
	friend class FadingCanvas;
	MainWindow* parent;

	std::pair<int, int> duration_canvas = std::make_pair(666, 666);
	std::pair<int, int> duration_tips = std::make_pair(333, 333);
	std::pair<double, double> opacity_canvas = std::make_pair(0.0, 0.7);
	std::pair<double, double> opacity_tips = std::make_pair(0.0, 1.0);
	Qt::GlobalColor color = Qt::black;
	QList<QEasingCurve::Type> easing_curve =
		QList<QEasingCurve::Type>() << QEasingCurve::Linear;

	int current_step = 0;
	int step_current = 0;
	int steps = 0;

	FadingCanvas* canvas;
	FadingTipBox* tips;

	QList<QMap<QString, QVariant>> frames;
	MainWindow* spy_window = nullptr;//set_tour()函数中赋值,在_set_data()函数中使用

	QWidget* widgets = nullptr;
	QDockWidget* dockwidgets = nullptr;
	QWidget* decoration = nullptr;
	QStringList run;

	bool is_tour_set = false;
	bool is_running = false;

	bool setting_data = false;
	bool hidden = false;

	int active_tour_index;//set_tour()函数中赋值，在close_tour函数中使用
	int last_frame_active;
};

} // namespace tour