#include "plugins.h"
#include "app/mainwindow.h"

TabFilter::TabFilter(QTabBar* dock_tabbar, MainWindow* main)
    : QObject ()
{
    this->dock_tabbar = dock_tabbar;
    this->main = main;
    this->moving = false;
    this->from_index = -1;
    this->to_index = -1;
}

SpyderPluginMixin* TabFilter::_get_plugin(int index) const
{
    foreach (SpyderPluginMixin* plugin, this->main->widgetlist) {
        QString tab_text = this->dock_tabbar->tabText(index).replace("&", "");
        if (plugin->get_plugin_title() == tab_text)
            return plugin;
    }
    return nullptr;
}

QList<SpyderPluginMixin*> TabFilter::_get_plugins() const
{
    QList<SpyderPluginMixin*> plugins;
    for (int index = 0; index < dock_tabbar->count(); ++index) {
        SpyderPluginMixin* plugin = this->_get_plugin(index);
        if (plugin != nullptr)
            plugins.append(plugin);
    }
    return plugins;
}

void TabFilter::_fix_cursor(int from_index, int to_index)
{
    int direction = std::abs(to_index - from_index) / (to_index - from_index);

    int tab_width = this->dock_tabbar->tabRect(to_index).width();
    int tab_x_min = this->dock_tabbar->tabRect(to_index).x();
    int tab_x_max = tab_x_min + tab_width;
    int previous_width = dock_tabbar->tabRect(to_index - direction).width();

    int delta = previous_width - tab_width;
    if (delta > 0)
        delta = delta * direction;
    else
        delta = 0;
    QCursor cursor;
    QPoint pos = this->dock_tabbar->mapFromGlobal(cursor.pos());
    int x = pos.x(), y = pos.y();
    if (x < tab_x_min || x > tab_x_max) {
        QPoint new_pos = dock_tabbar->mapToGlobal(QPoint(x+delta, y));
        cursor.setPos(new_pos);
    }
}

bool TabFilter::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    QEvent::Type event_type = event->type();
	switch (event_type)
	{
	case QEvent::MouseButtonPress:
		this->tab_pressed(static_cast<QMouseEvent*>(event));
		return false;
	case QEvent::MouseMove:
		try {
			this->tab_moved(static_cast<QMouseEvent*>(event));
		}
		catch (...) {
		}
		return true;
	case QEvent::MouseButtonRelease:
		this->tab_released(static_cast<QMouseEvent*>(event));
		return true;
	default:
		break;
	}
    return false;
}

void TabFilter::tab_pressed(QMouseEvent *event)
{
    this->from_index = this->dock_tabbar->tabAt(event->pos());
    this->dock_tabbar->setCurrentIndex(this->from_index);

    if (event->button() == Qt::RightButton) {
        if (this->from_index == -1)
            this->show_nontab_menu(event);
        else
            this->show_tab_menu(event);
    }
}

void TabFilter::tab_moved(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        this->to_index = -1;
        return;
    }

    this->to_index = this->dock_tabbar->tabAt(event->pos());

    if (!this->moving && this->from_index != -1 && this->to_index != -1) {
        QApplication::setOverrideCursor(Qt::ClosedHandCursor);
        this->moving = true;
    }

    if (this->to_index == -1)
        this->to_index = this->from_index;

    int from_index = this->from_index;
    int to_index = this->to_index;
    if (from_index != to_index && from_index != -1) {
        this->move_tab(from_index, to_index);
        this->_fix_cursor(from_index, to_index);
        this->from_index = to_index;
    }
}

void TabFilter::tab_released(QMouseEvent *event)
{
    Q_UNUSED(event);
    QApplication::restoreOverrideCursor();
    this->moving = false;
}

void TabFilter::move_tab(int from_index, int to_index)
{
    QList<SpyderPluginMixin*> plugins = this->_get_plugins();
    SpyderPluginMixin* from_plugin = this->_get_plugin(from_index);
    SpyderPluginMixin* to_plugin = this->_get_plugin(to_index);

    int from_idx = plugins.indexOf(from_plugin);
    int to_idx = plugins.indexOf(to_plugin);

    std::swap(plugins[from_idx], plugins[to_idx]);
    for (int i = 0; i < plugins.size()-1; ++i) {
        this->main->tabify_plugins(plugins[i], plugins[i+1]);
    }
    if (from_plugin != nullptr)
        from_plugin->dockwidget->raise();
}

void TabFilter::show_tab_menu(QMouseEvent *event)
{
    this->show_nontab_menu(event);
}

void TabFilter::show_nontab_menu(QMouseEvent *event)
{
    QMenu* menu = this->main->createPopupMenu();
    menu->exec(this->dock_tabbar->mapToGlobal(event->pos()));
}


/********** SpyderDockWidget **********/
SpyderDockWidget::SpyderDockWidget(const QString& title, MainWindow* parent)
    : QDockWidget (title, parent)
{
    this->title = title;
    this->main = parent;
    this->dock_tabbar = nullptr;
    connect(this,SIGNAL(visibilityChanged(bool)),this,SLOT(install_tab_event_filter(bool)));
}

void SpyderDockWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    emit this->plugin_closed();
}

void SpyderDockWidget::install_tab_event_filter(bool value)
{
    Q_UNUSED(value);
    QTabBar* dock_tabbar = nullptr;
    QList<QTabBar *> tabbars = this->main->findChildren<QTabBar *>();
    foreach (QTabBar* tabbar, tabbars) {
        for (int idx = 0; idx < tabbar->count(); ++idx) {
            QString title = tabbar->tabText(idx);
            if (title == this->title) {
                dock_tabbar = tabbar;
                break;
            }
        }
    }

    if (dock_tabbar != nullptr) {
        this->dock_tabbar = dock_tabbar;
        if (this->dock_tabbar->property("filter").isNull()) {
            TabFilter* filter = new TabFilter(this->dock_tabbar, this->main);
            this->dock_tabbar->setProperty("filter", reinterpret_cast<size_t>(filter));
            this->dock_tabbar->installEventFilter(filter);
        }
    }
}


/********** SpyderPluginMixin **********/

void SpyderPluginMixin::__update_plugin_title()
{
    QWidget* win;
    if (this->dockwidget)
        win = this->dockwidget;
    else if (this->mainwindow)
        win = this->mainwindow;
    else
        return;
    win->setWindowTitle(this->get_plugin_title());
}

void SpyderPluginMixin::register_shortcut(QObject *qaction_or_qshortcut, const QString &context,
                                          const QString &name, bool add_sc_to_tip)
{
    this->main->register_shortcut(qaction_or_qshortcut, context,
                                  name, add_sc_to_tip);
}

void SpyderPluginMixin::register_widget_shortcuts(Widget_get_shortcut_data *widget)
{
    foreach (ShortCutStrStr tmp, widget->get_shortcut_data()) {
        QShortcut* qshortcut = tmp.qsc;
        QString context = tmp.context;
        QString name = tmp.name;
        this->register_shortcut(qshortcut, context, name);
    }
}

void SpyderPluginMixin::switch_to_plugin()
{

    if (this->main->last_plugin &&
            this->main->last_plugin->ismaximized &&
            this->main->last_plugin != this)
        this->main->maximize_dockwidget();
    if (!this->toggle_view_action->isChecked())
        this->toggle_view_action->setChecked(true);
    this->visibility_changed(true);

}

void SpyderPluginMixin::visibility_changed(bool enable)
{
    if (this->dockwidget) {
        if (enable) {
            this->dockwidget->raise();
            QWidget* widget = this->get_focus_widget();
            if (widget)
                widget->setFocus();
        }
        bool visible = this->dockwidget->isVisible() || this->ismaximized;
        if (this->DISABLE_ACTIONS_WHEN_HIDDEN)
            toggle_actions(this->plugin_actions, visible);
        this->isvisible = enable && visible;
        if (this->isvisible)
            this->refresh_plugin();
    }
}

void SpyderPluginMixin::plugin_closed()
{
    this->toggle_view_action->setChecked(false);
}

void SpyderPluginMixin::set_option(const QString &option, const QVariant &value)
{
    CONF_set(this->CONF_SECTION, option, value);
}

QVariant SpyderPluginMixin::get_option(const QString &option, const QVariant &_default)
{
    return CONF_get(this->CONF_SECTION, option, _default);
}

QFont SpyderPluginMixin::get_plugin_font(bool rich_text) const
{
    QString option;
    int font_size_delta;
    if (rich_text) {
        option = "rich_font";
        font_size_delta = this->RICH_FONT_SIZE_DELTA;
    }
    else {
        option = "font";
        font_size_delta = this->FONT_SIZE_DELTA;
    }
    return gui::get_font("main", option, font_size_delta);
}

QFont SpyderPluginMixin::get_plugin_font2(const QString& rich_text) const
{
    QString option;
    int font_size_delta;
    if (!rich_text.isEmpty()) {
        option = "rich_font";
        font_size_delta = this->RICH_FONT_SIZE_DELTA;
    }
    else {
        option = "font";
        font_size_delta = this->FONT_SIZE_DELTA;
    }
    return gui::get_font("main", option, font_size_delta);
}

void SpyderPluginMixin::__show_message(const QString &message, int timeout)
{
    this->main->statusBar()->showMessage(message, timeout);
}

void SpyderPluginMixin::starting_long_process(const QString &message)
{
    this->__show_message(message);
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QApplication::processEvents();
}

void SpyderPluginMixin::ending_long_process(const QString &message)
{
    QApplication::restoreOverrideCursor();
    this->__show_message(message, 2000);
    QApplication::processEvents();
}

QHash<QString, ColorBoolBool> SpyderPluginMixin::get_color_scheme() const
{
    return gui::get_color_scheme();
}

void SpyderPluginMixin::toggle_view(bool checked)
{
    if (!this->dockwidget)
        return;
    if (checked) {
        this->dockwidget->show();
        this->dockwidget->raise();
    }
    else
        this->dockwidget->hide();
}

QIcon SpyderPluginMixin::get_plugin_icon() const
{
    return ima::icon("outline_explorer");
}

QList<QAction*> SpyderPluginMixin::get_plugin_actions()
{
    QList<QAction*> actions;
    return actions;
}


/********** SpyderPluginWidget **********/
SpyderPluginWidget::SpyderPluginWidget(MainWindow *parent)
    : QWidget (parent)
{
    this->FONT_SIZE_DELTA = 0;
    this->RICH_FONT_SIZE_DELTA = 0;
    this->IMG_PATH = "images";
    this->ALLOWED_AREAS = Qt::AllDockWidgetAreas;
    this->LOCATION = Qt::LeftDockWidgetArea;
    this->FEATURES = QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable;
    this->DISABLE_ACTIONS_WHEN_HIDDEN = true;

    this->main = parent;
    this->dockwidget = nullptr;
    this->mainwindow = nullptr;
    this->ismaximized = false;
    this->isvisible = false;

    QPair<bool,QString> pair = this->check_compatibility();
    bool check_compatibility = pair.first;
    QString message = pair.second;
    if (!check_compatibility)
        this->show_compatibility_message(message);
}

void SpyderPluginWidget::initialize_plugin()
{
    this->create_toggle_view_action();
    this->plugin_actions = this->get_plugin_actions();

    connect(this, SIGNAL(show_message(const QString&, int)),
            this, SLOT(__show_message(const QString&, int)));
    connect(this, SIGNAL(update_plugin_title()),
            this, SLOT(__update_plugin_title()));
    connect(this, SIGNAL(sig_option_changed(QString, QVariant)),
            this, SLOT(set_option(QString, QVariant)));
    this->setWindowTitle(this->get_plugin_title());
}

void SpyderPluginWidget::update_margins()
{
    QLayout* layout = this->layout();
    int left, top, right, bottom;
    layout->getContentsMargins(&left, &top, &right, &bottom);
    this->default_margins = QMargins(left, top, right, bottom);
    if (CONF_get("main", "use_custom_margin").toBool()) {
        int margin = CONF_get("main", "custom_margin").toInt();
        layout->setContentsMargins(margin, margin, margin, margin);
    }
    else
        layout->setContentsMargins(this->default_margins);
}

QPair<SpyderDockWidget*,Qt::DockWidgetArea> SpyderPluginWidget::create_dockwidget()
{
    SpyderDockWidget* dock = new SpyderDockWidget(this->get_plugin_title(), this->main);

    dock->setObjectName(this->metaObject()->className() + QString("_dw"));
    dock->setAllowedAreas(this->ALLOWED_AREAS);
    dock->setFeatures(this->FEATURES);
    dock->setWidget(this);
    this->update_margins();
    connect(dock, SIGNAL(visibilityChanged(bool)), this, SLOT(visibility_changed(bool)));
    connect(dock, SIGNAL(plugin_closed()), this, SLOT(plugin_closed()));
    this->dockwidget = dock;
    if (!this->shortcut.isEmpty()) {
        QShortcut* sc = new QShortcut(QKeySequence(this->shortcut), this->main);
        connect(sc, SIGNAL(activated()), this, SLOT(switch_to_plugin()));
        QString name = QString("Switch to %1").arg(CONF_SECTION);
        this->register_shortcut(sc, "_", name);
    }
    return QPair<SpyderDockWidget*,Qt::DockWidgetArea>(dock, LOCATION);
}

QMainWindow* SpyderPluginWidget::create_mainwindow()
{
    // Note: this method is currently not used
    this->mainwindow = new QMainWindow;
    QMainWindow* mainwindow = this->mainwindow;
    mainwindow->setAttribute(Qt::WA_DeleteOnClose);
    QIcon icon = this->get_plugin_icon();
    mainwindow->setWindowIcon(icon);
    mainwindow->setWindowTitle(this->get_plugin_title());
    mainwindow->setCentralWidget(this);
    this->refresh_plugin();
    return mainwindow;
}

void SpyderPluginWidget::create_toggle_view_action()
{
    QString title = this->get_plugin_title();
    if (this->CONF_SECTION == "editor")
        title = "Editor";
    QAction* action = new QAction(title, this);
    connect(action, &QAction::toggled, [this](bool checked){this->toggle_view(checked);});
    action->setCheckable(true);// 得加上这句，才可以选中
    if (!this->shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(this->shortcut));
        action->setShortcutContext(Qt::WidgetShortcut);
    }

    this->toggle_view_action = action;
}

QPair<bool,QString> SpyderPluginWidget::check_compatibility() const
{
    QString message = "";
    bool valid = true;
    return QPair<bool,QString>(valid, message);
}

void SpyderPluginWidget::show_compatibility_message(const QString &message)
{
    QMessageBox* messageBox = new QMessageBox(this);
    messageBox->setWindowModality(Qt::NonModal);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->setWindowTitle("Compatibility Check");
    messageBox->setText(message);
    messageBox->setStandardButtons(QMessageBox::Ok);
    messageBox->show();
}
