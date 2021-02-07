#include "plugins_findinfiles.h"
#include "app/mainwindow.h"

FindInFiles::FindInFiles(MainWindow *parent)
    : FindInFilesWidget (parent)
{
    this->CONF_SECTION = "find_in_files";

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
    connect(this, SIGNAL(toggle_visibility(bool)), this, SLOT(toggle(bool)));
}

void FindInFiles::toggle(bool state)
{
    if (this->dockwidget)
        this->dockwidget->setVisible(state);
}

void FindInFiles::refreshdir()
{
    this->find_options->set_directory(misc::getcwd_or_home());
}

void FindInFiles::set_project_path(const QString &path)
{
    this->find_options->set_project_path(path);
}

void FindInFiles::set_current_opened_file(const QString &path)
{
    this->find_options->set_file_path(path);
}

void FindInFiles::unset_project_path()
{
    this->find_options->disable_project_search();
}

void FindInFiles::findinfiles_callback()
{
    QWidget* tmp = QApplication::focusWidget();
    if (!this->ismaximized) {
        this->dockwidget->setVisible(true);
        this->dockwidget->raise();
    }
    QString text = "";
    TextEditBaseWidget* widget = qobject_cast<TextEditBaseWidget*>(tmp);
    if (widget && widget->has_selected_text())
        text = widget->get_selected_text();
    this->set_search_text(text);
    if (!text.isEmpty())
        this->find();
}


/********** SpyderPluginMixin **********/
void FindInFiles::switch_to_plugin()
{
    this->findinfiles_callback();
    SpyderPluginMixin::switch_to_plugin();
}

void FindInFiles::initialize_plugin()
{
    this->create_toggle_view_action();
    this->plugin_actions = this->get_plugin_actions();

    connect(this, SIGNAL(sig_option_changed(QString, QVariant)),
            this, SLOT(set_option(QString, QVariant)));
    this->setWindowTitle(this->get_plugin_title());
}


void FindInFiles::update_margins()
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

QPair<SpyderDockWidget*,Qt::DockWidgetArea> FindInFiles::create_dockwidget()
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

QMainWindow* FindInFiles::create_mainwindow()
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

void FindInFiles::create_toggle_view_action()
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

QString FindInFiles::get_plugin_title() const
{
    return "Find in files";
}

QWidget* FindInFiles::get_focus_widget() const
{
    return this->find_options->search_text;
}

void FindInFiles::register_plugin()
{
    this->main->add_dockwidget(this);
    connect(this, &FindInFiles::edit_goto,
            [this](QString fname, int _goto, QString word){this->main->editor->load(fname, _goto, word);});
    connect(this, SIGNAL(redirect_stdio(bool)),
            this->main, SLOT(redirect_internalshell_stdio(bool)));
    connect(this->main->workingdirectory, SIGNAL(refresh_findinfiles()),
            SLOT(refreshdir()));
    // self.main.projects
    // self.main.projects
    connect(this->main->editor, SIGNAL(open_file_update(const QString&)),
            SLOT(set_current_opened_file(const QString&)));

    QAction* findinfiles_action = new QAction("&Find in files", this);
    findinfiles_action->setIcon(ima::icon("findf"));
    connect(findinfiles_action, SIGNAL(triggered(bool)), this, SLOT(switch_to_plugin()));
    findinfiles_action->setShortcut(QKeySequence(this->shortcut));
    findinfiles_action->setShortcutContext(Qt::WidgetShortcut);
    findinfiles_action->setToolTip("Search text in multiple files");

    this->main->search_menu_actions << nullptr << findinfiles_action;
    this->main->search_toolbar_actions << nullptr << findinfiles_action;
}

bool FindInFiles::closing_plugin(bool cancelable)
{
    Q_UNUSED(cancelable);
    this->closing_widget();
    StruToSave options = this->find_options->get_options_to_save();
    if (!options.search_text.isEmpty()) {
        QStringList search_text = options.search_text;
        bool text_re = options.text_re;
        QStringList exclude = options.exclude;
        int exclude_idx = options.exclude_idx;
        bool exclude_re = options.exclude_re;
        bool more_options = options.more_options;
        bool case_sensitive = options.case_sensitive;
        QStringList path_history = options.path_history;

        int hist_limit = 15;
        search_text = search_text.mid(0, hist_limit);
        exclude = exclude.mid(0, hist_limit);
        if (path_history.size() > hist_limit)
            path_history = path_history.mid(path_history.size() - hist_limit);
        this->set_option("search_text", search_text);
        this->set_option("search_text_regexp", text_re);
        this->set_option("exclude", exclude);
        this->set_option("exclude_idx", exclude_idx);
        this->set_option("exclude_regexp", exclude_re);
        this->set_option("more_options", more_options);
        this->set_option("case_sensitive", case_sensitive);
        this->set_option("path_history", path_history);
    }
    return true;
}

static void test()
{
    FindInFiles widget;
    widget.show();
}
