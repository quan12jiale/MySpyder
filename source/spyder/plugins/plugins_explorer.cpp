#include "plugins_explorer.h"
#include "app/mainwindow.h"

Explorer::Explorer(MainWindow *parent)
    : ExplorerWidget (parent)
{
    this->CONF_SECTION = "explorer";

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
}

void Explorer::initialize_plugin()
{
    this->create_toggle_view_action();
    this->plugin_actions = this->get_plugin_actions();

    connect(this, SIGNAL(sig_option_changed(QString, QVariant)),
            this, SLOT(set_option(QString, QVariant)));
    this->setWindowTitle(this->get_plugin_title());
}

void Explorer::update_margins()
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

QPair<SpyderDockWidget*,Qt::DockWidgetArea> Explorer::create_dockwidget()
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

QMainWindow* Explorer::create_mainwindow()
{
    // Note: this method is currently not used
    this->mainwindow = new QMainWindow;
    QMainWindow* mainwindow = this->mainwindow;
    mainwindow->setAttribute(Qt::WA_DeleteOnClose);
    QIcon icon = this->get_plugin_icon();
    mainwindow->setWindowIcon(icon);
    mainwindow->setWindowTitle(this->get_plugin_title());
    mainwindow->setCentralWidget(this);
    this->refresh_plugin2();
    return mainwindow;
}

void Explorer::create_toggle_view_action()
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

QString Explorer::get_plugin_title() const
{
    return "File explorer";
}

QWidget* Explorer::get_focus_widget() const
{
    return this->treewidget;
}

void Explorer::register_plugin()
{
    this->main->add_dockwidget(this);
    connect(this, &Explorer::edit, [this](QString fname){this->main->editor->load(fname);});
    connect(this, SIGNAL(removed(const QString&)), this->main->editor, SLOT(removed(const QString&)));
    connect(this, SIGNAL(removed_tree(const QString&)), this->main->editor, SLOT(removed_tree(const QString&)));

    connect(this, SIGNAL(renamed(const QString&, const QString&)),
            this->main->editor, SLOT(renamed(const QString&, const QString&)));
    connect(this, SIGNAL(renamed_tree(const QString&, const QString&)),
            this->main->editor, SLOT(renamed_tree(const QString&, const QString&)));

    connect(this->main->editor, SIGNAL(open_dir(QString)), SLOT(chdir(QString)));
    connect(this, &Explorer::create_module, [this](QString fname){this->main->editor->_new(fname);});

    connect(this->main->workingdirectory, &WorkingDirectory::set_explorer_cwd,
            [this](QString directory){this->refresh_plugin2(directory, true);});
    connect(this, &Explorer::open_dir,
            [this](QString dirname){this->main->workingdirectory->chdir(dirname, false, false, true);});

    connect(this, &ExplorerWidget::sig_open_file, [this](QString fname){this->main->open_file(fname);});
    connect(this, &Explorer::sig_new_file, [this](QString fname){this->main->editor->_new(fname);});
}

void Explorer::refresh_plugin2(const QString &new_path, bool force_current)
{
    this->treewidget->update_history(new_path);
    this->treewidget->refresh(new_path, force_current);
}

void Explorer::chdir(const QString& directory)
{
    this->treewidget->chdir(directory);
}
