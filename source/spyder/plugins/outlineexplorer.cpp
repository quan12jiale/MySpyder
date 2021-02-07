#include "outlineexplorer.h"
#include "app/mainwindow.h"

OutlineExplorer::OutlineExplorer(MainWindow *parent)
    : OutlineExplorerWidget (parent)
{
    this->CONF_SECTION = "outline_explorer";

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

    this->shortcut = CONF_get("shortcuts", QString("_/switch to %1").arg(CONF_SECTION)).toString();
    this->toggle_view_action = nullptr;

    // 子类
    this->initialize_plugin();

    this->treewidget->header()->hide();
    this->load_config();
}

/********** SpyderPluginMixin **********/

void OutlineExplorer::initialize_plugin()
{
    this->create_toggle_view_action();
    this->plugin_actions = this->get_plugin_actions();

    connect(this, SIGNAL(sig_option_changed(QString, QVariant)),
            this, SLOT(set_option(QString, QVariant)));
    this->setWindowTitle(this->get_plugin_title());
}


void OutlineExplorer::update_margins()
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

QPair<SpyderDockWidget*,Qt::DockWidgetArea> OutlineExplorer::create_dockwidget()
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

QMainWindow* OutlineExplorer::create_mainwindow()
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

void OutlineExplorer::create_toggle_view_action()
{
    QString title = this->get_plugin_title();
    if (this->CONF_SECTION == "editor")
        title = "Editor";
    QAction* action = new QAction(title, this);
    connect(action, &QAction::toggled, [this](bool checked){this->toggle_view(checked);});
    action->setCheckable(true);// 得加上这句，才可以选中
    action->setShortcut(QKeySequence(this->shortcut));
    action->setShortcutContext(Qt::WidgetShortcut);
    this->toggle_view_action = action;
}

QString OutlineExplorer::get_plugin_title() const
{
    return "Outline";
}

QIcon OutlineExplorer::get_plugin_icon() const
{
    return ima::icon("outline_explorer");
}

QWidget* OutlineExplorer::get_focus_widget() const
{
    return this->treewidget;
}

void OutlineExplorer::register_plugin()
{
    connect(this->main, SIGNAL(restore_scrollbar_position()),
            this, SLOT(restore_scrollbar_position()));
    this->main->add_dockwidget(this);
}

bool OutlineExplorer::closing_plugin(bool cancelable)
{
    Q_UNUSED(cancelable);
    this->save_config();
    return true;
}

void OutlineExplorer::visibility_changed(bool enable)
{
    SpyderPluginMixin::visibility_changed(enable);
    if (enable)
        emit outlineexplorer_is_visible();
}

void OutlineExplorer::restore_scrollbar_position()
{
    QPoint scrollbar_pos = this->get_option("scrollbar_position", QPoint()).toPoint();
    if (scrollbar_pos != QPoint())
        this->treewidget->set_scrollbar_position(scrollbar_pos);
}

void OutlineExplorer::save_config()
{
    QHash<QString, QVariant> dict = this->get_options();
    for (auto it=dict.begin();it!=dict.end(); it++) {
        QString option = it.key();
        QVariant value = it.value();
        this->set_option(option, value);
    }
}

void OutlineExplorer::load_config()
{
    QHash<QString, QVariant> tmp = this->get_option("expanded_state",QHash<QString, QVariant>()).toHash();
    if (!tmp.isEmpty()) {
        QHash<uint,bool> expanded_state;
        for (auto it=tmp.begin();it!=tmp.end();it++) {
            QString key = it.key();
            QVariant value = it.value();
            expanded_state[key.toUInt()] = value.toBool();
        }
        this->treewidget->set_expanded_state(expanded_state);
    }
}
