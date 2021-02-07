#include "projects.h"
#include "app/mainwindow.h"

Projects::Projects(MainWindow *parent)
    : ProjectExplorerWidget (parent)
{
    this->CONF_SECTION = "project_explorer";

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
    recent_projects = this->get_option("recent_projects", QStringList()).toStringList();
    this->current_active_project = nullptr;
    this->latest_project = nullptr;

    this->initialize_plugin();
    this->setup_project(this->get_active_project_path());
}

void Projects::initialize_plugin()
{
    this->create_toggle_view_action();
    this->plugin_actions = this->get_plugin_actions();

    connect(this, SIGNAL(sig_option_changed(QString, QVariant)),
            this, SLOT(set_option(QString, QVariant)));
    this->setWindowTitle(this->get_plugin_title());
}

QString Projects::get_plugin_title() const
{
    return "Project explorer";
}

QWidget* Projects::get_focus_widget() const
{
    return this->treewidget;
}

QList<QAction*> Projects::get_plugin_actions()
{
    new_project_action = new QAction("New Project...", this);
    connect(new_project_action, SIGNAL(triggered(bool)),
            SLOT(create_new_project()));

    open_project_action = new QAction("Open Project...", this);
    connect(open_project_action, &QAction::triggered,
            [this](){this->open_project();});

    close_project_action = new QAction("Close Project", this);
    connect(close_project_action, SIGNAL(triggered(bool)),
            SLOT(close_project()));

    delete_project_action = new QAction("Delete Project", this);
    connect(delete_project_action, SIGNAL(triggered(bool)),
            SLOT(_delete_project()));

    clear_recent_projects_action = new QAction("Clear this list", this);
    connect(clear_recent_projects_action, SIGNAL(triggered(bool)),
            SLOT(clear_recent_projects()));

    edit_project_preferences_action = new QAction("Project Preferences", this);
    connect(edit_project_preferences_action, SIGNAL(triggered(bool)),
            SLOT(edit_project_preferences()));

    recent_project_menu = new QMenu("Recent Projects", this);
    QAction* explorer_action = this->toggle_view_action;
    if (this->main) {
        this->main->projects_menu_actions << new_project_action
                                          << nullptr
                                          << open_project_action
                                          << close_project_action
                                          << delete_project_action
                                          << nullptr
                                          << recent_project_menu
                                          << explorer_action;
    }
    this->setup_menu_actions();
    return QList<QAction*>();
}

void Projects::register_plugin()
{
    this->main->pythonpath_changed();
    connect(this->main, SIGNAL(restore_scrollbar_position()),
            SLOT(restore_scrollbar_position()));
    connect(this, SIGNAL(sig_pythonpath_changed()),
            this->main, SLOT(pythonpath_changed()));
    connect(this, &Projects::create_module,
            [this](QString fname){this->main->editor->_new(fname);});
    connect(this, &Projects::edit,
            [this](QString fname){this->main->editor->load(fname);});

    connect(this, SIGNAL(removed(const QString&)),
            this->main->editor, SLOT(removed(const QString&)));
    connect(this, SIGNAL(removed_tree(const QString&)),
            this->main->editor, SLOT(removed_tree(const QString&)));
    connect(this, SIGNAL(renamed(const QString&, const QString&)),
            this->main->editor, SLOT(renamed(const QString&, const QString&)));
    connect(this, SIGNAL(renamed_tree(const QString&, const QString&)),
            this->main->editor, SLOT(renamed_tree(const QString&, const QString&)));

    this->main->editor->set_projects(this);
    this->main->add_dockwidget(this);

    connect(this, &Projects::sig_open_file,
            [this](QString fname){this->main->open_file(fname);});
    connect(this, &Projects::sig_new_file,
            [this](QString x){this->main->editor->_new(QString(), nullptr, x);});

    // New project connections. Order matters!
    connect(this, &Projects::sig_project_loaded,
            [this](QString v){this->main->workingdirectory->chdir(v);});
    connect(this, &Projects::sig_project_loaded,
            [this](){this->main->set_window_title();});
    connect(this, &Projects::sig_project_loaded,
            [this](){this->main->editor->setup_open_files();});
    connect(this, SIGNAL(sig_project_loaded(const QString&)), SLOT(update_explorer()));

    connect(this, &Projects::sig_project_closed,
            [this](){this->main->workingdirectory->chdir(this->get_last_working_dir());});
    connect(this, &Projects::sig_project_closed,
            [this](){this->main->set_window_title();});
    connect(this, &Projects::sig_project_closed,
            [this](){this->main->editor->setup_open_files();});

    connect(recent_project_menu, SIGNAL(aboutToShow()), SLOT(setup_menu_actions()));
}

bool Projects::closing_plugin(bool cancelable)
{
    Q_UNUSED(cancelable);
    this->save_config();
    this->closing_widget();
    return true;
}

void Projects::switch_to_plugin()
{
    if (this->main->last_plugin &&
            this->main->last_plugin->ismaximized &&
            this->main->last_plugin != this)
        this->main->maximize_dockwidget();

    if (this->get_option("visible_if_project_open").toBool()) {
        if (!this->toggle_view_action->isChecked())
            this->toggle_view_action->setChecked(true);
        this->visibility_changed(true);
    }
}


void Projects::update_margins()
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

QPair<SpyderDockWidget*,Qt::DockWidgetArea> Projects::create_dockwidget()
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

QMainWindow* Projects::create_mainwindow()
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

void Projects::create_toggle_view_action()
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

// ------ Public API -------
void Projects::setup_menu_actions()
{
    this->recent_project_menu->clear();
    this->recent_projects_actions.clear();
    if (!this->recent_projects.isEmpty()) {
        QStringList tmp(this->recent_projects);
        foreach (QString project, tmp) {
            if (this->is_valid_project(project)) {
                QString name = project;
                name.replace(get_home_dir(), "~");
                QAction* action = new QAction(ima::icon("project"), name, this);
                connect(action, &QAction::triggered,
                        [this, project](){this->open_project(project);});
                this->recent_projects_actions.append(action);
            }
            else
                this->recent_projects.removeOne(project);
        }
        recent_projects_actions << nullptr << clear_recent_projects_action;
    }
    else
        recent_projects_actions << clear_recent_projects_action;
    add_actions(recent_project_menu, recent_projects_actions);
    this->update_project_actions();
}

void Projects::update_project_actions()
{
    if (!this->recent_projects.isEmpty())
        this->clear_recent_projects_action->setEnabled(true);
    else
        this->clear_recent_projects_action->setEnabled(false);

    bool active = !this->get_active_project_path().isEmpty();
    close_project_action->setEnabled(active);
    delete_project_action->setEnabled(active);
    edit_project_preferences_action->setEnabled(active);
}

void Projects::edit_project_preferences()
{
    qDebug() << __func__;
}

void Projects::create_new_project()
{
    this->switch_to_plugin();
    EmptyProject* active_project = this->current_active_project;
    ProjectDialog* dlg = new ProjectDialog(this);
    connect(dlg, SIGNAL(sig_project_creation_requested(QString, QString, QStringList)),
            SLOT(_create_project(QString)));
    connect(dlg, SIGNAL(sig_project_creation_requested(QString, QString, QStringList)),
            this, SIGNAL(sig_project_created(QString, QString, QStringList)));
    if (dlg->exec()) {
        if (active_project == nullptr &&
                this->get_option("visible_if_project_open").toBool())
            this->show_explorer();
        emit sig_pythonpath_changed();
        this->restart_consoles();//创建新项目 重启控制台
    }
}

void Projects::_create_project(const QString &path)
{
    this->open_project(path);
    this->setup_menu_actions();
    this->add_to_recent(path);
}

void Projects::open_project(QString path, bool restart_consoles,
                            bool save_previous_files)
{
    this->switch_to_plugin();
    if (path.isEmpty()) {
        QString basedir = get_home_dir();
        path = QFileDialog::getExistingDirectory(this,
                                                 "Open project",
                                                 basedir);
        if (!this->is_valid_project(path)) {
            if (!path.isEmpty())
                QMessageBox::critical(this, "Error",
                                      QString("<b>%1</b> is not a Spyder project!")
                                      .arg(path));
            return;
        }
    }

    this->add_to_recent(path);

    if (current_active_project == nullptr) {
        if (save_previous_files && this->main->editor)
            this->main->editor->save_open_files();
        if (this->main->editor)
            this->main->editor->set_option("last_working_dir",
                                           misc::getcwd_or_home());
        if (this->get_option("visible_if_project_open").toBool())
            this->show_explorer();
    }
    else {
        if (this->main->editor)
            this->set_project_filenames(
                    this->main->editor->get_open_filenames());
    }

    current_active_project = new EmptyProject(path);
    latest_project = new EmptyProject(path);
    this->set_option("current_project_path", this->get_active_project_path());

    this->setup_menu_actions();
    emit sig_project_loaded(path);
    emit sig_pythonpath_changed();

    if (restart_consoles)
        this->restart_consoles();//创建新项目 重启控制台
}

void Projects::close_project()
{
    if (current_active_project) {
        this->switch_to_plugin();
        QString path = current_active_project->root_path;
        current_active_project = nullptr;
        this->set_option("current_project_path", QString());
        this->setup_menu_actions();

        emit sig_project_closed(path);
        emit sig_pythonpath_changed();

        if (this->dockwidget) {
            this->set_option("visible_if_project_open",
                             this->dockwidget->isVisible());
            this->dockwidget->close();
        }
        this->clear();
        this->restart_consoles();//关闭项目 重启控制台

        if (this->main->editor)
            this->set_project_filenames(
                    this->main->editor->get_open_filenames());
    }
}

void Projects::_delete_project()
{
    if (this->current_active_project) {
        this->switch_to_plugin();
        this->delete_project();
    }
}

void Projects::clear_recent_projects()
{
    this->recent_projects.clear();
    this->setup_menu_actions();
}

EmptyProject* Projects::get_active_project() const
{
    return this->current_active_project;
}

void Projects::reopen_last_project()
{
    QString current_project_path = this->get_option("current_project_path",
                                                    QString()).toString();
    if (!current_project_path.isEmpty() &&
            this->is_valid_project(current_project_path)) {
        this->open_project(current_project_path,
                           false, false);
        this->load_config();
    }
}

QStringList Projects::get_project_filenames()
{
    QStringList recent_files;
    if (current_active_project)
        recent_files = current_active_project->get_recent_files();
    else if (latest_project)
        recent_files = latest_project->get_recent_files();
    return recent_files;
}

void Projects::set_project_filenames(const QStringList &recent_files)
{
    if (current_active_project &&
            this->is_valid_project(current_active_project->root_path))
        current_active_project->set_recent_files(recent_files);
}

QString Projects::get_active_project_path()
{
    QString active_project_path;
    if (current_active_project)
        active_project_path = current_active_project->root_path;
    return active_project_path;
}

QStringList Projects::get_pythonpath(bool at_start)
{
    QString current_path;
    if (at_start)
        current_path = this->get_option("current_project_path",
                                        QString()).toString();
    else
        current_path = this->get_active_project_path();
    if (current_path.isEmpty())
        return QStringList();
    else
        return QStringList(current_path);
}

QString Projects::get_last_working_dir()
{
    return this->main->editor->get_option("last_working_dir",
                                          misc::getcwd_or_home()).toString();
}

void Projects::save_config()
{
    this->set_option("recent_projects", recent_projects);
    this->set_option("expanded_state", this->treewidget->get_expanded_state());
    this->set_option("scrollbar_position",
                     this->treewidget->get_scrollbar_position());
    if (current_active_project && this->dockwidget)
        this->set_option("visible_if_project_open",
                         this->dockwidget->isVisible());
}

void Projects::load_config()
{
    QStringList expanded_state = this->get_option("expanded_state",
                                                  QStringList()).toStringList();

    if (!expanded_state.isEmpty())
        this->treewidget->set_expanded_state(expanded_state);
}

void Projects::restore_scrollbar_position()
{
    QPoint scrollbar_pos = this->get_option("scrollbar_position",
                                            QPoint()).toPoint();
    if (!scrollbar_pos.isNull())
        this->treewidget->set_scrollbar_position(scrollbar_pos);
}

void Projects::update_explorer()
{
    this->setup_project(this->get_active_project_path());
}

void Projects::show_explorer()
{
    if (this->dockwidget) {
        if (this->dockwidget->isHidden())
            this->dockwidget->show();
        this->dockwidget->raise();
        this->dockwidget->update();
    }
}

void Projects::restart_consoles()
{
    // TODO
    //self.main.ipyconsole.restart()
}

bool Projects::is_valid_project(const QString &path) const
{
    QString spy_project_dir = path + '/' + ".spyproject";
    QFileInfo info1(path);
    QFileInfo info2(spy_project_dir);
    if (info1.isDir() && info2.isDir())
        return true;
    else
        return false;
}

void Projects::add_to_recent(const QString &project)
{
    if (!recent_projects.contains(project)) {
        recent_projects.insert(0, project);
        recent_projects = recent_projects.mid(0, 10);
    }
}
