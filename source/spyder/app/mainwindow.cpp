#include "mainwindow.h"

QSplashScreen* setup_splash()
{
    if (!running_under_pytest()) {
        QSplashScreen* SPLASH = new QSplashScreen(QPixmap(get_image_path("splash.svg")));
        QFont SPLASH_FONT = SPLASH->font();
        SPLASH_FONT.setPixelSize(10);
        SPLASH->setFont(SPLASH_FONT);
        SPLASH->show();
        SPLASH->showMessage("Initializing...", Qt::AlignBottom | Qt::AlignCenter
                            | Qt::AlignAbsolute, QColor(Qt::white));
        QApplication::processEvents();
        return SPLASH;
    }
    else
        return nullptr;
}

void set_opengl_implementation(const QString& option)
{
    if (option == "software")
        qApp->setAttribute(Qt::AA_UseSoftwareOpenGL);
    else if (option == "desktop")
        qApp->setAttribute(Qt::AA_UseDesktopOpenGL);
    else if (option == "gles")
        qApp->setAttribute(Qt::AA_UseOpenGLES);
}

const QMainWindow::DockOptions MainWindow::DOCKOPTIONS =
        QMainWindow::AllowTabbedDocks | QMainWindow::AllowNestedDocks;
const int MainWindow::CURSORBLINK_OSDEFAULT = QApplication::cursorFlashTime();
const int MainWindow::DEFAULT_LAYOUTS = 4;

MainWindow::MainWindow(const QHash<QString,QVariant>& options)
    : QMainWindow ()
{
    this->splash = setup_splash();

    SPYDER_PATH = get_conf_path("path");
    SPYDER_NOT_ACTIVE_PATH = get_conf_path("not_active_path");

    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);
    this->default_style = qApp->style()->objectName();

    this->dialog_manager = new DialogManager;

    init_workdir = options.value("working_directory", QString()).toString();
    profile = options.value("profile", false).toBool();
    multithreaded = options.value("multithreaded", false).toBool();
    new_instance = options.value("new_instance", false).toBool();
    open_project = options.value("project", QString()).toString();
    window_title = options.value("window_title", QString()).toString();

    qDebug() << "Start of MainWindow constructor";

    this->shortcut_data.clear();

    this->path.clear();
    this->not_active_path.clear();
    this->project_path.clear();
    QFileInfo info(this->SPYDER_PATH);
    QFileInfo fileinfo(this->SPYDER_NOT_ACTIVE_PATH);
    if (info.isFile()) {
        QStringList tmp = encoding::readlines(this->SPYDER_PATH);
        foreach (QString name, tmp) {
            QFileInfo info2(name);
            if (info2.isDir())
                this->path.append(name);
        }
    }
    if (fileinfo.isFile()) {
        QStringList tmp = encoding::readlines(SPYDER_NOT_ACTIVE_PATH);
        foreach (QString name, tmp) {
            QFileInfo info2(name);
            if (info2.isDir())
                this->not_active_path.append(name);
        }
    }
    this->remove_path_from_sys_path();
    this->add_path_to_sys_path();

    this->workingdirectory = nullptr;
    this->editor = nullptr;  
    this->explorer = nullptr;
    this->projects = nullptr;
    this->outlineexplorer = nullptr;
    this->findinfiles = nullptr;
    this->thirdparty_plugins.clear();

    this->check_updates_action = nullptr;
    this->give_updates_feedback = true;

    this->prefs_index = -1;
    this->prefs_dialog_size = QSize();

    // Actions
    this->lock_dockwidgets_action = nullptr;
    this->show_toolbars_action = nullptr;
    this->close_dockwidget_action = nullptr;
    this->undo_action = nullptr;
    this->redo_action = nullptr;
    this->copy_action = nullptr;
    this->cut_action = nullptr;
    this->paste_action = nullptr;
    this->selectall_action = nullptr;
    this->maximize_action = nullptr;
    this->fullscreen_action = nullptr;

    // Menu bars
    this->file_menu = nullptr;
    this->file_menu_actions.clear();
    this->edit_menu = nullptr;
    this->edit_menu_actions.clear();
    this->search_menu = nullptr;
    this->search_menu_actions.clear();
    this->source_menu = nullptr;
    this->source_menu_actions.clear();
    this->run_menu = nullptr;
    this->run_menu_actions.clear();
    this->debug_menu = nullptr;
    this->debug_menu_actions.clear();
    this->consoles_menu = nullptr;
    this->consoles_menu_actions.clear();
    this->projects_menu = nullptr;
    this->projects_menu_actions.clear();
    this->tools_menu = nullptr;
    this->tools_menu_actions.clear();
    this->external_tools_menu = nullptr; // We must keep a reference to this,
    // otherwise the external tools menu is lost after leaving setup method
    this->external_tools_menu_actions.clear();
    this->view_menu = nullptr;
    this->plugins_menu = nullptr;
    this->plugins_menu_actions.clear();
    this->toolbars_menu = nullptr;
    this->help_menu = nullptr;
    this->help_menu_actions.clear();

    // Status bar widgets
    this->mem_status = nullptr;
    this->cpu_status = nullptr;

    // Toolbars
    this->visible_toolbars.clear();
    this->toolbarslist.clear();
    this->main_toolbar = nullptr;
    this->main_toolbar_actions.clear();
    this->file_toolbar = nullptr;
    this->file_toolbar_actions.clear();
    this->edit_toolbar = nullptr;
    this->edit_toolbar_actions.clear();
    this->search_toolbar = nullptr;
    this->search_toolbar_actions.clear();
    this->source_toolbar = nullptr;
    this->source_toolbar_actions.clear();
    this->run_toolbar = nullptr;
    this->run_toolbar_actions.clear();
    this->debug_toolbar = nullptr;
    this->debug_toolbar_actions.clear();
    this->layout_toolbar = nullptr;
    this->layout_toolbar_actions.clear();

    // 458行
    if (running_under_pytest())
        CONF_set("main", "show_internal_errors", false);

    this->set_window_title();

    this->already_closed = false;
    this->is_starting_up = true;
    this->is_setting_up = true;

    this->dockwidgets_locked = CONF_get("main", "panes_locked").toBool();
    this->floating_dockwidgets.clear();
    this->window_size = QSize();
    this->window_position = QPoint();
    this->state_before_maximizing = QByteArray();
    this->current_quick_layout = 4;

    this->last_plugin = nullptr;//502行

    this->saved_normal_geometry = QRect();

    this->last_focused_widget = nullptr;
    this->previous_focused_widget = nullptr;

    this->apply_settings();
    qDebug() << "End of MainWindow constructor";
}

QToolBar* MainWindow::create_toolbar(const QString& title, const QString& object_name, int iconsize)
{
    QToolBar* toolbar = this->addToolBar(title);
    toolbar->setObjectName(object_name);
    toolbar->setIconSize(QSize(iconsize, iconsize));
    this->toolbarslist.append(toolbar);
    return toolbar;
}

void MainWindow::setup()
{
    qDebug() << "*** Start of MainWindow setup ***";
    qDebug() << "  ..core actions";

    close_dockwidget_action = new QAction("Close current pane", this);
    close_dockwidget_action->setIcon(ima::icon("DialogCloseButton"));
    connect(close_dockwidget_action, SIGNAL(triggered()), SLOT(close_current_dockwidget()));
    close_dockwidget_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(close_dockwidget_action, "_", "Close pane");
#if 1
	lock_dockwidgets_action = create_action(this, "Lock panes",
		nullptr,
		QString(), QIcon(), QString(), 
		SLOT(toggle_lock_dockwidgets(bool)),
		QString(), QAction::NoRole, 
		Qt::ApplicationShortcut);
#else
    lock_dockwidgets_action = new QAction("Lock panes", this);
    connect(lock_dockwidgets_action, SIGNAL(toggled(bool)), SLOT(toggle_lock_dockwidgets(bool)));
    lock_dockwidgets_action->setCheckable(true);
    lock_dockwidgets_action->setShortcutContext(Qt::ApplicationShortcut);
#endif
    this->register_shortcut(lock_dockwidgets_action, "_", "Lock unlock panes");

    toggle_next_layout_action = new QAction("Use next layout", this);
    connect(toggle_next_layout_action, SIGNAL(triggered()), SLOT(toggle_next_layout()));
    toggle_next_layout_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(toggle_next_layout_action, "_", "Use next layout");

    toggle_previous_layout_action = new QAction("Use previous layout", this);
    connect(toggle_previous_layout_action, SIGNAL(triggered()), SLOT(toggle_previous_layout()));
    toggle_previous_layout_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(toggle_previous_layout_action, "_", "Use previous layout");

    file_switcher_action = new QAction("File switcher...", this);
    file_switcher_action->setIcon(ima::icon("filelist"));
    file_switcher_action->setToolTip("Fast switch between files");
    connect(file_switcher_action, &QAction::triggered, [this](){this->open_fileswitcher();});
    file_switcher_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(file_switcher_action, "_", "File switcher");

    symbol_finder_action = new QAction("Symbol finder...", this);
    symbol_finder_action->setIcon(ima::icon("symbol_find"));
    symbol_finder_action->setToolTip("Fast symbol search in file");
    connect(symbol_finder_action, SIGNAL(triggered()), SLOT(open_symbolfinder()));
    symbol_finder_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(symbol_finder_action, "_", "symbol finder", true);

    this->file_toolbar_actions.clear();
    file_toolbar_actions << file_switcher_action << symbol_finder_action;
#if 1
	auto create_edit_action = [this](const QString& text, 
		const QString& tr_text, const QIcon& icon) -> QAction*
	{
		const QStringList textseq = text.split(' ');
		QString method_name = textseq[0].toLower() + textseq.mid(1).join("");
		QAction* action = create_action(this, tr_text,
			SLOT(global_callback()),
			QString(), icon, QString(),
			nullptr,
			method_name, QAction::NoRole,
			Qt::WidgetShortcut);
		this->register_shortcut(action, "Editor", text);
		return action;
	};

	undo_action = create_edit_action("Undo", "Undo", ima::icon("undo"));
	redo_action = create_edit_action("Redo", "Redo", ima::icon("redo"));
	copy_action = create_edit_action("Copy", "Copy", ima::icon("editcopy"));
	cut_action = create_edit_action("Cut", "Cut", ima::icon("editcut"));
	paste_action = create_edit_action("Paste", "Paste", ima::icon("editpaste"));
	selectall_action = create_edit_action("Select All", "Select All", ima::icon("selectall"));
#else
    undo_action = new QAction(ima::icon("undo"), "Undo", this);
    connect(undo_action, SIGNAL(triggered()), SLOT(global_callback()));
    undo_action->setData("undo");
    undo_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(undo_action, "Editor", "Undo");

    redo_action = new QAction(ima::icon("redo"), "Redo", this);
    connect(redo_action, SIGNAL(triggered()), SLOT(global_callback()));
    redo_action->setData("redo");
    redo_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(redo_action, "Editor", "Redo");

    copy_action = new QAction(ima::icon("editcopy"), "Copy", this);
    connect(copy_action, SIGNAL(triggered()), SLOT(global_callback()));
    copy_action->setData("copy");
    copy_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(copy_action, "Editor", "Copy");

    cut_action = new QAction(ima::icon("editcut"), "Cut", this);
    connect(cut_action, SIGNAL(triggered()), SLOT(global_callback()));
    cut_action->setData("cut");
    cut_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(cut_action, "Editor", "Cut");

    paste_action = new QAction(ima::icon("editpaste"), "Paste", this);
    connect(paste_action, SIGNAL(triggered()), SLOT(global_callback()));
    paste_action->setData("paste");
    paste_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(paste_action, "Editor", "Paste");

    selectall_action = new QAction(ima::icon("selectall"), "Select All", this);
    connect(selectall_action, SIGNAL(triggered()), SLOT(global_callback()));
    selectall_action->setData("selectAll");
    selectall_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(selectall_action, "Editor", "Select All");
#endif
    this->edit_menu_actions.clear();
    edit_menu_actions << undo_action << redo_action
                      << nullptr << cut_action << copy_action
                      << paste_action << selectall_action;

    //namespace = nullptr;,用于Console的构造函数
    qDebug() << "  ..toolbars";

    file_menu = this->menuBar()->addMenu("&File");
    file_toolbar = this->create_toolbar("File toolbar", "file_toolbar");

    edit_menu = this->menuBar()->addMenu("&Edit");
    edit_toolbar = this->create_toolbar("Edit toolbar", "edit_toolbar");

    search_menu = this->menuBar()->addMenu("&Search");
    search_toolbar = this->create_toolbar("Search toolbar", "search_toolbar");

    source_menu = this->menuBar()->addMenu("Sour&ce");
    source_toolbar = this->create_toolbar("Source toolbar", "source_toolbar");

    run_menu = this->menuBar()->addMenu("&Run");
    run_toolbar = this->create_toolbar("Run toolbar", "run_toolbar");

    debug_menu = this->menuBar()->addMenu("&Debug");
    debug_toolbar = this->create_toolbar("Debug toolbar", "debug_toolbar");

    consoles_menu = this->menuBar()->addMenu("C&onsoles");

    projects_menu = this->menuBar()->addMenu("&Projects");
    connect(projects_menu, SIGNAL(aboutToShow()), SLOT(valid_project()));

    tools_menu = this->menuBar()->addMenu("&Tools");

    view_menu = this->menuBar()->addMenu("&View");

    help_menu = this->menuBar()->addMenu("&Help");

    QStatusBar* status = this->statusBar();
    status->setObjectName("StatusBar");
    status->showMessage("Welcome to Spyder!", 5000);

    qDebug() << "  ..tools";

    QAction* prefs_action = new QAction(ima::icon("configure"), "Pre&ferences", this);
    connect(prefs_action, SIGNAL(triggered()), SLOT(edit_preferences()));
    prefs_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(prefs_action, "_", "Preferences", true);
#if 1
	QAction* spyder_path_action = create_action(this, "PYTHONPATH manager", 
		SLOT(path_manager_callback()),
		QString(), ima::icon("pythonpath"), "Python Path Manager", 
		nullptr,
		QString(), QAction::ApplicationSpecificRole);
#else
    QAction* spyder_path_action = new QAction(ima::icon("pythonpath"), "PYTHONPATH manager", this);
    spyder_path_action->setToolTip("Python Path Manager");
    connect(spyder_path_action, SIGNAL(triggered()), SLOT(path_manager_callback()));
    spyder_path_action->setMenuRole(QAction::ApplicationSpecificRole);
#endif
    QAction* update_modules_action = new QAction("Update module names list", this);
    update_modules_action->setToolTip("Refresh list of module names available in PYTHONPATH");
    //triggered=lambda: module_completion.reset()

    QAction* reset_spyder_action = new QAction("Reset Spyder to factory defaults", this);
    connect(reset_spyder_action, SIGNAL(triggered()), SLOT(reset_spyder()));

    this->tools_menu_actions.clear();
    tools_menu_actions << prefs_action << spyder_path_action;

    //if WinUserEnvDialog is not nullptr;:该类需要windows注册表头文件

    tools_menu_actions << reset_spyder_action << nullptr
                       << update_modules_action;

    external_tools_menu = new QMenu("External Tools");
    external_tools_menu_actions.clear();

    wp_action = new QAction(ima::icon("winpython.svg"), "WinPython control panel", this);
    //triggered=lambda: programs.run_python_script('winpython', 'controlpanel')

    // 从730行到781行是调用qt设计师QAction的代码，暂不实现

    maximize_action = new QAction("", this);
    connect(maximize_action, &QAction::triggered, [this](){this->maximize_dockwidget();});
    maximize_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(maximize_action, "_", "Maximize pane");
    this->__update_maximize_action();

    fullscreen_action = new QAction("Fullscreen mode", this);
    connect(fullscreen_action, SIGNAL(triggered()), SLOT(toggle_fullscreen()));
    fullscreen_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(fullscreen_action, "_", "Fullscreen mode", true);

    this->main_toolbar_actions.clear();
    main_toolbar_actions << maximize_action
                         << fullscreen_action
                         << nullptr
                         << prefs_action << spyder_path_action;
    main_toolbar = create_toolbar("Main toolbar", "main_toolbar");

    qDebug() << "  ..plugin: internal console";
    //this->console = Console(self, namespace, exitfunc=this->closing,
    //this->console.register_plugin()

    qDebug() << "  ..plugin: working directory";
    this->workingdirectory = new WorkingDirectory(this, this->init_workdir);
    this->workingdirectory->register_plugin();
    this->toolbarslist.append(this->workingdirectory);

    if (CONF_get("help", "enable").toBool()) {
        this->set_splash("Loading help...");
        //spyder.plugins.help import Help
    }

    if (CONF_get("outline_explorer", "enable").toBool()) {
        this->set_splash("Loading outline explorer...");
        this->outlineexplorer = new OutlineExplorer(this);
        this->outlineexplorer->register_plugin();
    }

    this->set_splash("Loading editor...");
    this->editor = new Editor(this);
    this->editor->register_plugin();
	auto plugin = static_cast<SpyderPluginMixin*>(this->editor);//该步转换是必须的：多继承下指针偏移
	this->setProperty("editor", QVariant(reinterpret_cast<qulonglong>(plugin)));

    QAction* quit_action = new QAction(ima::icon("exit"), "&Quit", this);
    // triggered=this->console.quit
    quit_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(quit_action, "_", "Quit");

    QAction* restart_action = new QAction(ima::icon("restart"), "&Restart", this);
    restart_action->setToolTip("Restart");
    connect(restart_action, &QAction::triggered, [this](){this->restart();});
    restart_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(restart_action, "_", "Restart");

    file_menu_actions << file_switcher_action
                      << symbol_finder_action << nullptr
                      << restart_action << quit_action;
    this->set_splash("");

    qDebug() << "  ..widgets";

    if (CONF_get("explorer", "enable").toBool()) {
        this->set_splash("Loading file explorer...");
        this->explorer = new Explorer(this);
        this->explorer->register_plugin();
		auto plugin = static_cast<SpyderPluginMixin*>(this->explorer);
		this->setProperty("explorer", QVariant(reinterpret_cast<qulonglong>(plugin)));
    }

    if (CONF_get("historylog", "enable").toBool()) {
        this->set_splash("Loading history plugin...");
        this->historylog = new HistoryLog(this);
        this->historylog->register_plugin();
		auto plugin = static_cast<SpyderPluginMixin*>(this->historylog);
		this->setProperty("historylog", QVariant(reinterpret_cast<qulonglong>(plugin)));
    }

    if (CONF_get("onlinehelp", "enable").toBool()) {
        this->set_splash("Loading online help...");
        //plugins.onlinehelp import OnlineHelp
    }

    this->set_splash("Loading project explorer...");
    this->projects = new Projects(this);
    this->projects->register_plugin();
    this->project_path = this->projects->get_pythonpath(true);


    if (CONF_get("find_in_files", "enable").toBool()) {
		const QStringList external_path_history = CONF_get("find_in_files", "path_history", QStringList()).toStringList();
        this->findinfiles = new FindInFiles(this, external_path_history);
        this->findinfiles->register_plugin();
    }

    this->set_splash("Loading namespace browser...");
    //plugins.variableexplorer import VariableExplorer

    this->set_splash("Loading IPython console...");
    //plugins.ipythonconsole import IPythonConsole

    this->set_splash("Setting up main window...");

    // Help menu
    QAction* trouble_action = new QAction("Troubleshooting...", this);
    connect(trouble_action, SIGNAL(triggered()), SLOT(trouble_guide()));

    QAction* dep_action = new QAction(ima::icon("advanced"), "Dependencies...", this);
    connect(dep_action, SIGNAL(triggered()), SLOT(show_dependencies()));

    QAction* report_action = new QAction(ima::icon("bug"), "Report issue...", this);
    connect(report_action, &QAction::triggered, [this](){this->report_issue();});

    QAction* support_action = new QAction("Spyder support...", this);
    connect(support_action, SIGNAL(triggered()), SLOT(google_group()));

    check_updates_action = new QAction("Check for updates...", this);
    connect(check_updates_action, &QAction::triggered, [this](){this->check_updates();});

    // Spyder documentation
    QString spyder_doc = "https://docs.spyder-ide.org/";
    QAction* doc_action = new QAction(ima::icon("DialogHelpButton"), "Spyder documentation", this);
    // triggered=lambda: programs.start_file(spyder_doc)
    this->register_shortcut(doc_action, "_", "spyder documentation");

    QAction* tut_action = new QAction("Spyder tutorial", this);
    //triggered=this->help.show_tutorial
#if 1
	QAction* shortcuts_action = create_action(this, "Shortcuts Summary", 
		SLOT(show_shortcuts_dialog()),
		"Meta+F1");
#else
    QAction* shortcuts_action = new QAction("Shortcuts Summary", this);
    shortcuts_action->setShortcut(QKeySequence("Meta+F1"));
    connect(shortcuts_action, SIGNAL(triggered()), SLOT(show_shortcuts_dialog()));
#endif
    //from spyder.app import tour
	this->tour = new tour::AnimatedTour(this);
    tours_menu = new QMenu("Interactive tours");
    tour_menu_actions.clear();
	this->tours_available = tour::get_tours(0);
	for (int i = 0; i < this->tours_available.size(); i++)
	{
		const QMap<QString, QVariant > tour_available = this->tours_available[i];
		this->tours_available[i]["last"] = 0;//在spyder启动时将上次的索引设为0
		QString tour_name = tour_available["name"].toString();

		QAction* temp_action = create_action(this, tour_name,
			SLOT(show_tour()));
		tour_menu_actions << temp_action;
	}

    tours_menu->addActions(tour_menu_actions);

    this->help_menu_actions.clear();
    help_menu_actions << doc_action << tut_action << shortcuts_action
                      << tours_menu
                      << nullptr << trouble_action
                      <<report_action << dep_action
                     << check_updates_action << support_action
                     << nullptr;

    QMenu* ipython_menu = new QMenu("IPython documentation", this);
    QAction* intro_action = new QAction("Intro to IPython", this);
    //triggered=this->ipyconsole.show_intro

    QAction* quickref_action = new QAction("Quick reference", this);
    //triggered=this->ipyconsole.show_quickref

    QAction* guiref_action = new QAction("Console help", this);
    //triggered=this->ipyconsole.show_guiref

    QList<QAction*> actions;
    actions << intro_action << guiref_action << quickref_action;
    add_actions(ipython_menu, actions);
    this->help_menu_actions.append(ipython_menu);

    //1002行到1029行不实现

    QMenu* web_resources = new QMenu("Online documentation", this);
    //webres_actions = create_module_bookmark_actions(self,this->BOOKMARKS)
    //webres_actions.insert(2, nullptr;)

    //add_actions(web_resources, webres_actions)
    this->help_menu_actions.append(web_resources);

    //qta_act = create_program_action(self, _("Qt documentation"),
    //this->help_menu_actions += [qta_act, nullptr;]

    QAction* about_action = new QAction("About Spyder...", this);
    about_action->setIcon(ima::icon("MessageBoxInformation"));
    connect(about_action, SIGNAL(triggered()), SLOT(about()));

    this->help_menu_actions << nullptr << about_action;

    // 1057行
    mem_status = new MemoryStatus(this, status);
    cpu_status = new CPUStatus(this, status);
    this->apply_statusbar_settings();
    //1061行到1076h行第三方插件不实现

    this->plugins_menu = new QMenu("Panes", this);

    toolbars_menu = new QMenu("Toolbars", this);
    quick_layout_menu = new QMenu("Window layouts", this);
    this->quick_layout_set_menu();

    this->view_menu->addMenu(this->plugins_menu);
    actions.clear();
    actions << lock_dockwidgets_action
            << close_dockwidget_action
            << maximize_action
            << nullptr;
    add_actions(view_menu, actions);
    show_toolbars_action = new QAction("Show toolbars", this);
    connect(show_toolbars_action, SIGNAL(triggered()), SLOT(show_toolbars()));
    show_toolbars_action->setShortcutContext(Qt::ApplicationShortcut);
    this->register_shortcut(show_toolbars_action, "_", "Show toolbars");

    view_menu->addMenu(toolbars_menu);
    view_menu->addAction(show_toolbars_action);

    QList<QObject*> objects;
    objects << nullptr
            << quick_layout_menu
            << toggle_previous_layout_action
            << toggle_next_layout_action
            << nullptr
            << fullscreen_action;
    add_actions(view_menu, objects);

    if (!external_tools_menu_actions.isEmpty()) {
        QAction* external_tools_act = new QAction("External Tools", this);
        external_tools_act->setMenu(external_tools_menu);
        tools_menu_actions << nullptr << external_tools_act;
    }

    // Filling out menu/toolbar entries:
    add_actions(this->file_menu, this->file_menu_actions);
    add_actions(this->edit_menu, this->edit_menu_actions);
    add_actions(this->search_menu, this->search_menu_actions);
    add_actions(this->source_menu, this->source_menu_actions);
    add_actions(this->run_menu, this->run_menu_actions);
    add_actions(this->debug_menu, this->debug_menu_actions);
    add_actions(this->consoles_menu, this->consoles_menu_actions);
    add_actions(this->projects_menu, this->projects_menu_actions);
    add_actions(this->tools_menu, this->tools_menu_actions);
    add_actions(this->external_tools_menu,
                this->external_tools_menu_actions);
    add_actions(this->help_menu, this->help_menu_actions);

    add_actions(this->main_toolbar, this->main_toolbar_actions);
    add_actions(this->file_toolbar, this->file_toolbar_actions);
    add_actions(this->edit_toolbar, this->edit_toolbar_actions);
    add_actions(this->search_toolbar, this->search_toolbar_actions);
    add_actions(this->source_toolbar, this->source_toolbar_actions);
    add_actions(this->debug_toolbar, this->debug_toolbar_actions);
    add_actions(this->run_toolbar, this->run_toolbar_actions);

    this->apply_shortcuts();

    emit all_actions_defined();

    qDebug() << "Setting up window...";
    this->setup_layout(false);// 这里设置窗口布局


    if (this->splash) // 1164行
        this->splash->hide();

    if (CONF_get("main", "tear_off_menus").toBool()) {
        foreach (QObject* obj, this->menuBar()->children()) {
            QMenu* child = qobject_cast<QMenu*>(obj);
            if (child && child != this->help_menu)
                child->setTearOffEnabled(true);
        }
    }

    foreach (QObject* obj, this->menuBar()->children()) {
        QMenu* child = qobject_cast<QMenu*>(obj);
        if (child) {
            connect(child, SIGNAL(aboutToShow()), SLOT(update_edit_menu()));
            connect(child, SIGNAL(aboutToShow()), SLOT(update_search_menu()));
        }
    }

    qDebug() << "*** End of MainWindow setup ***";
    this->is_starting_up = false;
}

void MainWindow::post_visible_setup()
{
    emit restore_scrollbar_position();

    foreach (QDockWidget* widget, this->floating_dockwidgets) {
        widget->setFloating(true);
    }

    this->create_plugins_menu();
    this->create_toolbars_menu();

    toolbars_visible = CONF_get("main", "toolbars_visible").toBool();
    this->load_last_visible_toolbars();

    lock_dockwidgets_action->setChecked(this->dockwidgets_locked);
    this->apply_panes_settings();

    //if this->console.dockwidget.isVisible() and DEV is nullptr;:
    //    this->console.toggle_view_action.setChecked(False)
    //    this->console.dockwidget.hide()

    // 1237行到1246行需要实现

    if (!this->open_project.isEmpty()) {
        this->projects->open_project(this->open_project);
    }
    else {
        this->projects->reopen_last_project();

        if (this->projects->get_active_project() == nullptr)
            this->editor->setup_open_files();
    }

    if (!DEV && CONF_get("main", "check_updates_on_startup").toBool()) {
        this->give_updates_feedback = false;
        this->check_updates(true);
    }

    this->report_missing_dependencies();

    this->is_setting_up = false;
}

void MainWindow::set_window_title()
{
    QString title;
    if (DEV) {
        title = QString("Spyder %1 (Python %2.%3)").arg("3.3.3")
                .arg(3).arg(7);
    }
    else {
        title = QString("Spyder (Python %1.%2)")
                .arg(3).arg(7);
    }

    if (DEBUG)
        title += QString(" [DEBUG MODE %1]").arg(DEBUG);

    if (!this->window_title.isEmpty())
        title += " -- " + this->window_title;

    if (this->projects) {
        QString path = this->projects->get_active_project_path();
        if (!path.isEmpty()) {
            path.replace(get_home_dir(), "~");
            title = QString("%1 - %2").arg(path).arg(title);
        }
    }

    this->base_title = title;
    this->setWindowTitle(this->base_title);
}

void MainWindow::report_missing_dependencies()
{}

WindowSettings MainWindow::load_window_settings(const QString &prefix, bool _default,
                                                const QString &section)
{
    std::function<QVariant(const QString&, const QString&)> get_func;
    if (_default)
        get_func = [](const QString& section, const QString& option)->QVariant{return get_default(section, option);};
    else
        get_func = [](const QString& section, const QString& option)->QVariant{return CONF_get(section, option);};

    QSize window_size = get_func(section, prefix+"size").toSize();
    QSize prefs_dialog_size = get_func(section, prefix+"prefs_dialog_size").toSize();
    QString hexstate;
    if (!_default)
        hexstate = CONF_get(section, prefix+"state", QString()).toString();
    QPoint pos = get_func(section, prefix+"position").toPoint();

    int width = pos.x();
    int height = pos.y();
    QRect screen_shape = QApplication::desktop()->geometry();
    int current_width = screen_shape.width();
    int current_height = screen_shape.height();
    if (current_width < width || current_height < height)
        pos = get_default(section, prefix+"position").toPoint();

    bool is_maximized = get_func(section, prefix+"is_maximized").toBool();
    bool is_fullscreen = get_func(section, prefix+"is_fullscreen").toBool();
    return WindowSettings(hexstate, window_size, prefs_dialog_size,
                          pos, is_maximized, is_fullscreen);
}

WindowSettings MainWindow::get_window_settings()
{
    QSize window_size = this->window_size;
    bool is_fullscreen = this->isFullScreen();
    bool is_maximized;
    if (is_fullscreen)
        is_maximized = this->maximized_flag;
    else
        is_maximized = this->isMaximized();
    QPoint pos = this->window_position;
    QSize prefs_dialog_size = this->prefs_dialog_size;
    QString hexstate = qbytearray_to_str(this->saveState());
    return WindowSettings(hexstate, window_size, prefs_dialog_size,
                          pos, is_maximized, is_fullscreen);
}

void MainWindow::set_window_settings(QString hexstate, QSize window_size, QSize prefs_dialog_size,
                                     QPoint pos, bool is_maximized, bool is_fullscreen)
{
    this->setUpdatesEnabled(false);
    this->window_size = window_size;
    this->prefs_dialog_size = prefs_dialog_size;
    this->window_position = pos;
    this->setWindowState(Qt::WindowNoState);
    this->resize(this->window_size);
    this->move(this->window_position);

    if (!hexstate.isEmpty()) {
        /*QSettings settings(getIniPath(), QSettings::Format::IniFormat);
        QString run_count = "run_count";
        if (settings.value(run_count, 0).toInt() == 0) {
            settings.setValue(run_count, 1);
        }
        else if (settings.value(run_count).toInt() == 1) {
            settings.setValue(run_count, 2);
        }
        else*/ {
            // !!!当添加新控件时，或第一次打开，位置不对，可以先禁用这里
            this->restoreState(QByteArray::fromHex(hexstate.toUtf8()));
        }
        foreach (QObject* obj, this->children()) {
            QDockWidget* widget = qobject_cast<QDockWidget*>(obj);
            if (widget && widget->isFloating()) {
                floating_dockwidgets.append(widget);
                widget->setFloating(false);
            }
        }
    }

    if (is_fullscreen)
        this->setWindowState(Qt::WindowFullScreen);
    this->__update_fullscreen_action();

    if (is_fullscreen)
        this->maximized_flag = is_maximized;
    else if (is_maximized)
        this->setWindowState(Qt::WindowMaximized);
    this->setUpdatesEnabled(true);
}

void MainWindow::save_current_window_settings(const QString &prefix, const QString &section,
                                              bool none_state)
{
    QSize win_size = this->window_size;
    QSize prefs_size = this->prefs_dialog_size;

    CONF_set(section, prefix+"size", win_size);
    CONF_set(section, prefix+"prefs_dialog_size", prefs_size);
    CONF_set(section, prefix+"is_maximized", this->isMaximized());
    CONF_set(section, prefix+"is_fullscreen", this->isFullScreen());
    QPoint pos = this->window_position;
    CONF_set(section, prefix+"position", pos);
    this->maximize_dockwidget(true);
    if (none_state)
        CONF_set(section, prefix+"state", QString());
    else {
        QByteArray qba = this->saveState();
        CONF_set(section, prefix+"state", qbytearray_to_str(qba));
    }
    CONF_set(section, prefix+"statusbar", !this->statusBar()->isHidden());
}

void MainWindow::tabify_plugins(SpyderPluginMixin *first, SpyderPluginMixin *second)
{
    this->tabifyDockWidget(first->dockwidget, second->dockwidget);
}

// --- Layouts
void MainWindow::setup_layout(bool _default)
{
    QString prefix = "window/";
    WindowSettings settings = this->load_window_settings(prefix, _default);
    QString hexstate = settings.hexstate;

    this->first_spyder_run = false;
	// 之所以之前第一次启动dockwidget布局不对。因为MainWindow::add_dockwidget函数调用QMainWindow::addDockWidget函数，Qt::Orientation默认是Qt::Vertical
	// 然后这里又因为笔误，没有进入if分支，去调用setup_default_layouts函数调整dockwidget间的布局关系
    if (hexstate.isEmpty()) {
		// First Spyder execution:
        this->setWindowState(Qt::WindowMaximized);
        this->first_spyder_run = true;
        this->setup_default_layouts(4, settings);//default为4

        QString section = "quick_layouts";
        QStringList order;
        if (_default)
            order = get_default(section, "order").toStringList();
        else
            order = CONF_get(section, "order").toStringList();

        if (_default) {
            CONF_set(section, "active", order);
            CONF_set(section, "order", order);
            CONF_set(section, "names", order);
        }

        for (int index = 0; index < order.size(); ++index) {
            QString name = order[index];
            QString prefix = QString("layout_%1").arg(index);
            this->save_current_window_settings(prefix, section, true);
        }

        prefix = "layout_default/";
        section = "quick_layouts";
        this->save_current_window_settings(prefix, section, true);
        this->current_quick_layout = 4;//4代表default

        this->quick_layout_set_menu();
    }
    hexstate = settings.hexstate;
    QSize window_size = settings.window_size;
    QSize prefs_dialog_size = settings.prefs_dialog_size;
    QPoint pos = settings.pos;
    bool is_maximized = settings.is_maximized;
    bool is_fullscreen = settings.is_fullscreen;
    this->set_window_settings(hexstate, window_size, prefs_dialog_size,
                              pos, is_maximized, is_fullscreen);

    foreach (SpyderPluginMixin* plugin, this->widgetlist) {
        try {
            plugin->initialize_plugin_in_mainwindow_layout();
        } catch (const std::exception& error) {
            qDebug() << __FILE__ << __LINE__ << error.what();
        }
    }
}

void MainWindow::setup_default_layouts(int index, const WindowSettings& settings)
{
	// Setup default layouts when run for the first time
    this->maximize_dockwidget(true);
    QString hexstate = settings.hexstate;
    QSize window_size = settings.window_size;
    QSize prefs_dialog_size = settings.prefs_dialog_size;
    QPoint pos = settings.pos;
    bool is_maximized = settings.is_maximized;
    bool is_fullscreen = settings.is_fullscreen;
    this->set_window_settings(hexstate, window_size, prefs_dialog_size,
                              pos, is_maximized, is_fullscreen);
    this->setUpdatesEnabled(false);


    double width_fraction[4] = {0.0, 0.55, 0.0, 0.45};

    QList<QList<double>> height_fraction;
    height_fraction.append(QList<double>({1.0}));
    height_fraction.append(QList<double>({1.0}));
    height_fraction.append(QList<double>({1.0}));
    height_fraction.append(QList<double>({0.46, 0.54}));

    QList<QList<QList<SpyderPluginMixin*>>> widgets_layout;

    QList<QList<SpyderPluginMixin*>> tmp;
    tmp.append(QList<SpyderPluginMixin*>({this->projects}));
    widgets_layout.append(tmp);

    tmp.clear();
    tmp.append(QList<SpyderPluginMixin*>({this->editor}));
    widgets_layout.append(tmp);

    tmp.clear();
    tmp.append(QList<SpyderPluginMixin*>({this->outlineexplorer}));
    widgets_layout.append(tmp);

    tmp.clear();
    tmp.append(QList<SpyderPluginMixin*>({this->explorer, this->findinfiles}));
    tmp.append(QList<SpyderPluginMixin*>({this->historylog}));//在这一行加ipyconsole
    widgets_layout.append(tmp);

    QList<SpyderPluginMixin*> widgets;
    foreach (auto column, widgets_layout) {
        foreach (auto row, column) {
            foreach (SpyderPluginMixin* widget, row) {
                if (widget)
                    widgets.append(widget);
            }
        }
    }

    // Make every widget visible
    foreach (SpyderPluginMixin* widget, widgets) {
        widget->toggle_view(true);
        QAction* action = widget->toggle_view_action;
        action->setChecked(widget->dockwidget->isVisible());
    }

    // Set the widgets horizontally
    for (int i = 0; i < widgets.size()-1; ++i) {
        SpyderPluginMixin* first = widgets[i];
        SpyderPluginMixin* second = widgets[i+1];
        if (first && second)
            this->splitDockWidget(first->dockwidget, second->dockwidget,
                                  Qt::Horizontal);
    }

    // Arrange rows vertically
    foreach (auto column, widgets_layout) {
        for (int i = 0; i < column.size()-1; ++i) {
            auto first_row = column[i];
            auto second_row = column[i+1];
			if (!first_row[0] || !second_row[0]) {
				continue;
			}
            this->splitDockWidget(first_row[0]->dockwidget,
                    second_row[0]->dockwidget,
                    Qt::Vertical);
        }
    }

    // Tabify
    foreach (auto column, widgets_layout) {
        foreach (auto row, column) {
            for (int i = 0; i < row.size()-1; ++i) {
                auto first = row[i];
                auto second = row[i+1];
                if (first && second)
					//将this->explorer, this->findinfiles放到一个页签
                    this->tabify_plugins(first, second);
            }

			// Raise front widget per row
			if (!row[0]) {
				continue;
			}
            row[0]->dockwidget->show();
            row[0]->dockwidget->raise();
        }
    }

	this->_layout_widget_info.clear();
    int width = this->window_size.width();
    int height = this->window_size.height();

    // fix column width 源码中也注释了设置固定宽度
//     for (int c = 0; c < widgets_layout.size(); ++c) {
// 		SpyderDockWidget* widget = widgets_layout[c][0][0]->dockwidget;
// 		int min_width = widget->minimumWidth(), max_width = widget->maximumWidth();
// 		QHash<QString, size_t> info;
// 		info["widget"] = reinterpret_cast<size_t>(widget);
// 		info["min width"] = min_width;
// 		info["max width"] = max_width;
// 		this->_layout_widget_info.append(info);
//         int new_width = static_cast<int>(width_fraction[c] * width * 0.95);
//         widget->setMinimumWidth(new_width);
//         widget->setMaximumWidth(new_width);
//         widget->updateGeometry();
//     }

    // fix column height
    for (int c = 0; c < widgets_layout.size(); ++c) {
        auto column = widgets_layout[c];
        for (int r = 0; r < column.size()-1; ++r) {
			SpyderPluginMixin* widget = column[r][0];
			if (!widget) {
				continue;
			}
			SpyderDockWidget* dockwidget = widget->dockwidget;
			int dock_min_h = dockwidget->minimumHeight();
			int dock_max_h = dockwidget->maximumHeight();
			QHash<QString, size_t> info;
			info["widget"] = reinterpret_cast<size_t>(widget);
			info["dock min height"] = dock_min_h;
			info["dock max height"] = dock_max_h;
			this->_layout_widget_info.append(info);
            int new_height = static_cast<int>(height_fraction[c][r] * height * 0.95);
			dockwidget->setMinimumHeight(new_height);
			dockwidget->setMaximumHeight(new_height);
        }
    }

    this->_custom_layout_timer = new QTimer(this);
    connect(_custom_layout_timer, SIGNAL(timeout()), this, SLOT(layout_fix_timer()));
    this->_custom_layout_timer->setSingleShot(true);
    this->_custom_layout_timer->start(5000);
}

void MainWindow::layout_fix_timer()
{
    const QList<QHash<QString, size_t>> info = this->_layout_widget_info;
	for (const QHash<QString, size_t>& i : info)
	{
		SpyderPluginMixin* widget = reinterpret_cast<SpyderPluginMixin*>(i["widget"]);
		SpyderDockWidget* dockwidget = widget->dockwidget;
		if (i.contains("dock min height"))
		{
			dockwidget->setMinimumHeight(i["dock min height"]);
			dockwidget->setMaximumHeight(i["dock max height"]);
		}
		dockwidget->updateGeometry();
	}
    this->setUpdatesEnabled(true);
}

void MainWindow::toggle_previous_layout()
{
    this->toggle_layout("previous");
}

void MainWindow::toggle_next_layout()
{
    this->toggle_layout("next");
}

void MainWindow::toggle_layout(const QString &direction)
{
    QStringList names = CONF_get("quick_layouts", "names").toStringList();
    QStringList order = CONF_get("quick_layouts", "order").toStringList();
    QStringList active = CONF_get("quick_layouts", "active").toStringList();

    if (active.size() == 0)
        return;

    QList<int> layout_index;
    layout_index << 4;
    foreach (QString name, order) {
        if (active.contains(name))
            layout_index.append(names.indexOf(name));
    }

    int current_layout = this->current_quick_layout;
    QHash<QString,int> dic;
    dic["next"] = 1;
    dic["previous"] = -1;

    int current_index = 0;
    if (layout_index.contains(current_layout))
        current_index = layout_index.indexOf(current_layout);

    int new_index = (current_index + dic[direction]) % layout_index.size();
    //当现在为default,current_index=0,不进行上下切换,new_index=0,
    //所以layout_index[new_index]=4
    this->quick_layout_switch(layout_index[new_index]);
}

void MainWindow::quick_layout_set_menu()
{
    QStringList names = CONF_get("quick_layouts", "names").toStringList();
    QStringList order = CONF_get("quick_layouts", "order").toStringList();
    QStringList active = CONF_get("quick_layouts", "active").toStringList();

    QList<QAction*> ql_actions;
    QAction* action = new QAction("Spyder Default Layout", this);
    connect(action, &QAction::triggered, [this](){this->quick_layout_switch(4);});

    foreach (QString name, order) {
        if (active.contains(name)) {
            int index = names.indexOf(name);

            QAction* qli_act = new QAction(name, this);
            connect(qli_act, &QAction::triggered,
                    [this,index](){this->quick_layout_switch(index);});

            ql_actions.append(qli_act);
        }
    }

    ql_save = new QAction("Save current layout", this);
    connect(ql_save, SIGNAL(triggered()), SLOT(quick_layout_save()));
    ql_save->setShortcutContext(Qt::ApplicationShortcut);

    ql_preferences = new QAction("Layout preferences", this);
    connect(ql_preferences, SIGNAL(triggered()), SLOT(quick_layout_settings()));
    ql_preferences->setShortcutContext(Qt::ApplicationShortcut);

    ql_reset = new QAction("Reset to spyder default", this);
    connect(ql_reset, SIGNAL(triggered()), SLOT(reset_window_layout()));

    this->register_shortcut(ql_save, "_", "Save current layout");
    this->register_shortcut(ql_preferences, "_", "Layout preferences");

    ql_actions << nullptr;
    ql_actions << ql_save << ql_preferences << ql_reset;

    this->quick_layout_menu->clear();
    add_actions(quick_layout_menu, ql_actions);

    if (order.size() == 0)
        ql_preferences->setEnabled(false);
    else
        ql_preferences->setEnabled(true);
}

void MainWindow::reset_window_layout()
{
    QMessageBox::StandardButton answer = QMessageBox::warning(this, "Warning",
                                                              "Window layout will be reset to default settings: "
                                                              "this affects window position, size and dockwidgets.\n"
                                                              "Do you want to continue?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::Yes)
        this->setup_layout(true);
}

void MainWindow::quick_layout_save()
{
    qDebug() << __func__;
    // 该函数需要plugins.layoutdialog.LayoutSaveDialog
    // 懒得写
}

void MainWindow::quick_layout_settings()
{
    qDebug() << __func__;
    // 该函数需要plugins.layoutdialog.LayoutSettingsDialog
    // 懒得写
}

void MainWindow::quick_layout_switch(int index)
{
    QString section = "quick_layouts";
    WindowSettings settings;
    try {
        settings = this->load_window_settings(QString("layout_%1/").arg(index),
                                                             false, section);
        QString hexstate = settings.hexstate;

        if (hexstate.isEmpty()) {
            if (index >= this->DEFAULT_LAYOUTS+1) {
                QMessageBox::critical(this, "Warning",
                                      "Error opening the custom layout.  Please close"
                                      " Spyder and try again.  If the issue persists,"
                                      " then you must use 'Reset to Spyder default' "
                                      "from the layout menu.");
                return;
            }
            this->setup_default_layouts(index, settings);
        }
    } catch (...) {
        QMessageBox::critical(this, "Warning",
                              QString("Quick switch layout #%s has not yet "
                                      "been defined.").arg(index));
        return;
    }

    QString hexstate = settings.hexstate;
    QSize window_size = settings.window_size;
    QSize prefs_dialog_size = settings.prefs_dialog_size;
    QPoint pos = settings.pos;
    bool is_maximized = settings.is_maximized;
    bool is_fullscreen = settings.is_fullscreen;
    this->set_window_settings(hexstate, window_size, prefs_dialog_size,
                              pos, is_maximized, is_fullscreen);
    this->current_quick_layout = index;

    foreach (SpyderPluginMixin* plugin, this->widgetlist) {
        QAction* action = plugin->toggle_view_action;
        if (action)
            action->setChecked(plugin->dockwidget->isVisible());
    }
}

// --- Show/Hide toolbars
void MainWindow::_update_show_toolbars_action()
{
    QString text, tip;
    if (this->toolbars_visible) {
        text = "Hide toolbars";
        tip = "Hide toolbars";
    }
    else {
        text = "Show toolbars";
        tip = "Show toolbars";
    }
    this->show_toolbars_action->setText(text);
    this->show_toolbars_action->setToolTip(tip);
}

void MainWindow::save_visible_toolbars()
{
    QStringList toolbars;
    foreach (QToolBar* toolbar, this->visible_toolbars) {
        toolbars.append(toolbar->objectName());
    }
    CONF_set("main", "last_visible_toolbars", toolbars);
}

void MainWindow::get_visible_toolbars()
{
    QList<QToolBar*> toolbars;
    foreach (QToolBar* toolbar, this->toolbarslist) {
        if (toolbar->toggleViewAction()->isChecked())
            toolbars.append(toolbar);
    }
    this->visible_toolbars = toolbars;
}

void MainWindow::load_last_visible_toolbars()
{
    QStringList toolbars_names = CONF_get("main", "last_visible_toolbars",
                                          QStringList()).toStringList();

    if (!toolbars_names.isEmpty()) {
        QHash<QString,QToolBar*> dic;
        foreach (QToolBar* toolbar, this->toolbarslist)
            dic[toolbar->objectName()] = toolbar;

        QList<QToolBar*> toolbars;
        foreach (QString name, toolbars_names) {
            if (dic.contains(name))
                toolbars.append(dic[name]);
        }
        this->visible_toolbars = toolbars;
    }
    else
        this->get_visible_toolbars();
    this->_update_show_toolbars_action();
}

void MainWindow::show_toolbars()
{
    bool value = ! this->toolbars_visible;
    CONF_set("main", "toolbars_visible", value);
    if (value)
        this->save_visible_toolbars();
    else
        this->get_visible_toolbars();

    foreach (QToolBar* toolbar, this->visible_toolbars) {
        toolbar->toggleViewAction()->setChecked(value);
        toolbar->setVisible(value);
    }

    this->toolbars_visible = value;
    this->_update_show_toolbars_action();
}

// --- Other
void MainWindow::valid_project()
{
    QString path = this->projects->get_active_project_path();

    if (!path.isEmpty()) {
        if (!this->projects->is_valid_project(path)) {
            if (!path.isEmpty())
                QMessageBox::critical(this, "Error",
                                      QString("<b>%1</b> is no longer a valid Spyder project! "
                                              "Since it is the current active project, it will "
                                              "be closed automatically.").arg(path));
            this->projects->close_project();
        }
    }
}

void MainWindow::free_memory()
{
    //gc.collect()
}

void MainWindow::plugin_focus_changed()
{
    this->update_edit_menu();
    this->update_search_menu();
}

//show_shortcuts和hide_shortcuts是Mac系统下需要的槽函数，这里无需实现
void MainWindow::show_shortcuts(const QString &menu)
{
    Q_UNUSED(menu);
}

void MainWindow::hide_shortcuts(const QString &menu)
{
    Q_UNUSED(menu);
}

FocusWidgetProperties MainWindow::get_focus_widget_properties()
{
    QWidget* widget = QApplication::focusWidget();
    FocusWidgetProperties res;//默认初始化widget为空指针

    TextEditBaseWidget* base_widget = qobject_cast<TextEditBaseWidget*>(widget);
    ControlWidget* ctl_widget = qobject_cast<ControlWidget*>(widget);

    bool not_readonly = false;
    if (base_widget)
        not_readonly = !base_widget->isReadOnly();
    else if (ctl_widget)
        not_readonly = !ctl_widget->isReadOnly();
    if (base_widget || ctl_widget) {
        bool console = ctl_widget != nullptr;
        bool readwrite_editor = not_readonly && !console;

        res.widget = widget;
        res.console = console;
        res.not_readonly = not_readonly;
        res.readwrite_editor = readwrite_editor;
    }
    return res;
}

void MainWindow::update_edit_menu()
{
    FocusWidgetProperties tmp = this->get_focus_widget_properties();
    if (tmp.widget == nullptr)
        return;

    QWidget* widget = tmp.widget;
    bool console = tmp.console;
    bool not_readonly = tmp.not_readonly;
    bool readwrite_editor = tmp.readwrite_editor;

    if (!console && not_readonly && !this->editor->is_file_opened())
        return;

    foreach (QAction* child, this->edit_menu->actions())
        child->setEnabled(false);

    this->selectall_action->setEnabled(true);

    TextEditBaseWidget* base_widget = qobject_cast<TextEditBaseWidget*>(widget);
    ControlWidget* ctl_widget = qobject_cast<ControlWidget*>(widget);

    if (base_widget) {
        undo_action->setEnabled(readwrite_editor &&
                                base_widget->document()->isUndoAvailable());
        redo_action->setEnabled(readwrite_editor &&
                                base_widget->document()->isRedoAvailable());

        bool has_selection = base_widget->has_selected_text();
        copy_action->setEnabled(has_selection);
        cut_action->setEnabled(has_selection && not_readonly);
        paste_action->setEnabled(not_readonly);

        if (!console && not_readonly) {
            foreach (QObject* obj, this->editor->edit_menu_actions) {
                QAction* action = qobject_cast<QAction*>(obj);
                action->setEnabled(true);
            }
        }
    }
    else if (ctl_widget) {
        undo_action->setEnabled(readwrite_editor &&
                                ctl_widget->document()->isUndoAvailable());
        redo_action->setEnabled(readwrite_editor &&
                                ctl_widget->document()->isRedoAvailable());

        bool has_selection = ctl_widget->has_selected_text();
        copy_action->setEnabled(has_selection);
        cut_action->setEnabled(has_selection && not_readonly);
        paste_action->setEnabled(not_readonly);
    }
}

void MainWindow::update_search_menu()
{
    // Disabling all actions except the last one
    // (which is Find in files) to begin with
    QList<QAction*> actions = this->search_menu->actions();
    actions.removeLast();
    foreach (QAction* child, actions)
        child->setEnabled(false);

    FocusWidgetProperties tmp = this->get_focus_widget_properties();
    if (tmp.widget == nullptr)
        return;

    bool console = tmp.console;
    bool readwrite_editor = tmp.readwrite_editor;

    if (!console) {
        foreach (QAction* action, this->search_menu->actions()) {
            try {
                action->setEnabled(true);
            } catch (...) {
            }
        }
    }
    // Disable the replace action for read-only files
    QAction* act = qobject_cast<QAction*>(this->search_menu_actions[3]);
    act->setEnabled(readwrite_editor);
}

void MainWindow::create_plugins_menu()
{
    QStringList order;
    order << "editor"<< "console"<< "ipython_console"<< "variable_explorer"
          << "help"<< QString()<< "explorer"<< "outline_explorer"
          << "project_explorer"<< "find_in_files"<< QString()<< "historylog"
          << "profiler"<< "breakpoints"<< "pylint"<< QString()
          << "onlinehelp"<< "internal_console";
    QList<QAction*> tmp;
    for (int i = 0; i < order.size(); ++i)
        tmp.append(nullptr);

    foreach (SpyderPluginMixin* plugin, this->widgetlist) {
        QAction* action = plugin->toggle_view_action;
        action->setChecked(plugin->dockwidget->isVisible());
        QString name = plugin->CONF_SECTION;
        int pos = order.indexOf(name);

        if (pos > -1)
            tmp[pos] = action;
        else
            tmp.append(action);
    }
    QList<QAction*> actions = tmp;
    foreach (QAction* action, tmp) {
        if (action == nullptr)
            actions.removeOne(action);
    }
    this->plugins_menu_actions = actions;
    add_actions(this->plugins_menu, actions);
}

void MainWindow::create_toolbars_menu()
{
    QStringList order;
    order << "file_toolbar"<< "run_toolbar"<< "debug_toolbar"
          << "main_toolbar"<< "Global working directory"<< QString()
          << "search_toolbar"<< "edit_toolbar"<< "source_toolbar";
    QList<QAction*> tmp;
    for (int i = 0; i < order.size(); ++i)
        tmp.append(nullptr);
    foreach (QToolBar* toolbar, this->toolbarslist) {
        QAction* action = toolbar->toggleViewAction();
        QString name = toolbar->objectName();
        int pos = order.indexOf(name);
        if (pos > -1)
            tmp[pos] = action;
        else
            tmp.append(action);
    }
    add_actions(this->toolbars_menu, tmp);
}

QMenu* MainWindow::createPopupMenu()
{
    QMenu* menu = new QMenu("", this);
    QList<QObject*> actions;
    actions.append(this->help_menu_actions.mid(0,3));
    actions.append(nullptr);
    actions.append(this->help_menu_actions.last());
    add_actions(menu, actions);
    return menu;
}

void MainWindow::set_splash(const QString &message)
{
    if (this->splash == nullptr)
        return;
    if (!message.isEmpty())
        qDebug() << message;
    this->splash->show();
    this->splash->showMessage(message, Qt::AlignBottom | Qt::AlignCenter
                              | Qt::AlignAbsolute, QColor(Qt::white));
    QApplication::processEvents();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (this->closing(true))
        event->accept();
    else
        event->ignore();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (!this->isMaximized() && !this->fullscreen_flag)
        this->window_size = this->size();
    QMainWindow::resizeEvent(event);

    emit sig_resized(event);
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    if (!this->isMaximized() && !this->fullscreen_flag)
        this->window_position = this->pos();
    QMainWindow::moveEvent(event);

    emit sig_moved(event);
}

void MainWindow::hideEvent(QHideEvent *event)
{
    foreach (SpyderPluginMixin* plugin, this->widgetlist) {
		QWidget* pwidget = dynamic_cast<QWidget*>(plugin);
        if (pwidget && pwidget->isAncestorOf(this->last_focused_widget))
            plugin->visibility_changed(true);
    }
    QMainWindow::hideEvent(event);
}

void MainWindow::change_last_focused_widget(QWidget *old, QWidget *now)
{
    if (now == nullptr && QApplication::activeWindow() != nullptr) {
        QApplication::activeWindow()->setFocus();
        this->last_focused_widget = QApplication::focusWidget();
    }
    else if (now != nullptr)
        this->last_focused_widget = now;

    this->previous_focused_widget = old;
}

bool MainWindow::closing(bool cancelable)
{
    if (this->already_closed || this->is_starting_up)
        return true;
    if (cancelable && CONF_get("main", "prompt_on_exit").toBool()) {
        QMessageBox::StandardButton reply = QMessageBox::critical(this, "Spyder",
                                                                  "Do you really want to exit?",
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
            return false;
    }
    QString prefix = "window/";
    this->save_current_window_settings(prefix);

    foreach (SpyderPluginMixin* plugin, this->thirdparty_plugins) {
        if (!plugin->closing_plugin(cancelable))
            return false;
    }
    foreach (SpyderPluginMixin* widget, this->widgetlist) {
        if (!widget->closing_plugin(cancelable))
            return false;
    }
    this->dialog_manager->close_all();
    if (this->toolbars_visible)
        this->save_visible_toolbars();
    this->already_closed = true;
    return true;
}

void MainWindow::add_dockwidget(SpyderPluginMixin *child)
{
    QPair<SpyderDockWidget*,Qt::DockWidgetArea> pair = child->create_dockwidget();
    SpyderDockWidget* dockwidget = pair.first;
    Qt::DockWidgetArea location = pair.second;
    if (CONF_get("main", "vertical_dockwidget_titlebars").toBool()) {
        dockwidget->setFeatures(dockwidget->features() |
                                QDockWidget::DockWidgetVerticalTitleBar);
    }

    this->addDockWidget(location, dockwidget);
    this->widgetlist.append(child);
}

void MainWindow::close_current_dockwidget()
{
    QWidget* widget = QApplication::focusWidget();
    foreach (SpyderPluginMixin* plugin, this->widgetlist) {
		QWidget* pwidget = dynamic_cast<QWidget*>(plugin);
        if (pwidget && pwidget->isAncestorOf(widget)) {
            //plugin->dockwidget->hide();v3.3.3是这样实现的，会导致dockwidget隐藏，但View菜单对应QAction还处于选中状态
			plugin->toggle_view_action->setChecked(false);//v4.0.1是这样实现的，通过调用toggle_view槽函数隐藏dockwidget
            break;
        }
    }
}

void MainWindow::toggle_lock_dockwidgets(bool value)
{
    this->dockwidgets_locked = value;
    this->apply_panes_settings();
    CONF_set("main", "panes_locked", value);
}

void MainWindow::__update_maximize_action()
{
    QString text, tip;
    QIcon icon;
    if (this->state_before_maximizing.isEmpty()) {
        text = "Maximize current pane";
        tip = "Maximize current pane";
        icon = ima::icon("maximize");
    }
    else {
        text = "Restore current pane";
        tip = "Restore pane to its original size";
        icon = ima::icon("unmaximize");
    }
    this->maximize_action->setText(text);
    this->maximize_action->setIcon(icon);
    this->maximize_action->setToolTip(tip);
}

void MainWindow::maximize_dockwidget(bool restore)
{
	/*
	First call: maximize current dockwidget
	Second call (or restore=True): restore original window layout
	*/
    if (this->state_before_maximizing.isEmpty()) {
        if (restore)
            return;

        this->state_before_maximizing = this->saveState();
        QWidget* focus_widget = QApplication::focusWidget();
        foreach (SpyderPluginMixin* plugin, this->widgetlist) {
            plugin->dockwidget->hide();
			QWidget* pwidget = dynamic_cast<QWidget*>(plugin);
            if (pwidget && pwidget->isAncestorOf(focus_widget))
                this->last_plugin = plugin;
        }

        if (this->last_plugin == nullptr)
            this->last_plugin = this->editor;

        this->last_plugin->dockwidget->toggleViewAction()->setDisabled(true);
		QWidget* pwidget = dynamic_cast<QWidget*>(this->last_plugin);
        if (pwidget) {
            this->setCentralWidget(pwidget);
        }
        this->last_plugin->ismaximized = true;

        // Workaround to solve an issue with editor's outline explorer:
		if (pwidget) {
			pwidget->show();
		}
        this->last_plugin->visibility_changed(true);
        if (this->last_plugin == static_cast<SpyderPluginMixin*>(this->editor)) {
			// Automatically show the outline if the editor was maximized:
            this->addDockWidget(Qt::RightDockWidgetArea,
                                this->outlineexplorer->dockwidget);
            this->outlineexplorer->dockwidget->show();
        }
    }
    else {
		QWidget* pwidget = dynamic_cast<QWidget*>(this->last_plugin);
        if (pwidget)
            this->last_plugin->dockwidget->setWidget(pwidget);
        this->last_plugin->dockwidget->toggleViewAction()->setEnabled(true);
        this->setCentralWidget(nullptr);
        this->last_plugin->ismaximized = false;
        this->restoreState(this->state_before_maximizing);
        this->state_before_maximizing = QByteArray();
        this->last_plugin->get_focus_widget()->setFocus();
    }
    this->__update_maximize_action();
}

void MainWindow::__update_fullscreen_action()
{
    QIcon icon;
    if (this->fullscreen_flag)
        icon = ima::icon("window_nofullscreen");
    else
        icon = ima::icon("window_fullscreen");
    this->fullscreen_action->setIcon(icon);
}

void MainWindow::toggle_fullscreen()
{
    if (this->fullscreen_flag) {
        this->fullscreen_flag = false;
        if (os::name == "nt") {
            this->setWindowFlags(this->windowFlags() ^
                                 (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint));
            this->setGeometry(this->saved_normal_geometry);
        }
        this->showNormal();
        if (this->maximized_flag)
            this->showMaximized();
    }
    else {
        this->maximized_flag = this->isMaximized();
        this->fullscreen_flag = true;
        this->saved_normal_geometry = this->normalGeometry();
        if (os::name == "nt") {
            this->setWindowFlags(this->windowFlags()
                                 | Qt::FramelessWindowHint
                                 | Qt::WindowStaysOnTopHint);
            QRect r = QApplication::desktop()->screenGeometry();
            this->setGeometry(r.left() - 1, r.top() - 1,
                              r.width() + 2, r.height() + 2);
            this->showNormal();
        }
        else
            this->showFullScreen();
    }
    this->__update_fullscreen_action();
}

//源码2357行
void MainWindow::add_to_toolbar()
{
    // 该函数在整个项目中都没有被调用
}

void MainWindow::about()
{
    QMessageBox* msgBox = new QMessageBox(this);
    msgBox->setText("<b>MySpyder</b>"
                    "<br>MySpyder is a powerful scientific environment written in Qt, for Python, "
                    "and designed by and for scientists, engineers and data analysts. "
                    "<br>It mainly refers to the source code of spyder written in PyQt. "
                    "For more general information about MySpyder, please "
                    "visit <a href=https://github.com/quan12jiale>github_url</a>.");
    msgBox->setWindowTitle("About MySpyder");
    msgBox->setStandardButtons(QMessageBox::Ok);
    QIcon tmp = QIcon(get_image_path("spyder.svg"));
    msgBox->setIconPixmap(tmp.pixmap(QSize(64, 64)));
    msgBox->setTextInteractionFlags(Qt::LinksAccessibleByMouse |
                                    Qt::TextSelectableByMouse);
    msgBox->exec();
}

void MainWindow::show_dependencies()
{
    qDebug() << __func__;
}

QString MainWindow::render_issue(QString description, const QString &traceback)
{
    QHash<QString,QString> versions = get_versions();

    QString revision = "";
    if (!versions["revision"].isEmpty())
        revision = versions["revision"];

    if (description.isEmpty())
        description = "### What steps reproduce the problem?";

    QString error_section = "";
    if (!traceback.isEmpty())
        error_section = QString("### Traceback\n"
                                "```python-traceback\n"
                                "%1\n"
                                "```").arg(traceback);

    QString issue_template = QString("## Description\n\n"
                                     "%1\n\n"
                                     "%2\n\n"
                                     "## Versions\n\n"
                                     "* Spyder version: %3 %4\n"
                                     "* Python version: %5\n"
                                     "* Qt version: %6\n"
                                     "* %7 version: %8\n"
                                     "* Operating System: %9 %10\n\n"
                                     "### Dependencies\n")
            .arg(description)
            .arg(error_section)
            .arg(versions["spyder"])
            .arg(revision)
            .arg(versions["python"])
            .arg(versions["qt"])
            .arg(versions["qt_api"])
            .arg(versions["qt_api_ver"])
            .arg(versions["system"])
            .arg(versions["release"]);
    return issue_template;
}

void MainWindow::report_issue(const QString &body, const QString &title, bool open_webpage)
{
    if (body.isEmpty()) {
        SpyderErrorDialog* report_dlg = new SpyderErrorDialog(this, true);
        report_dlg->show();
    }
    else {
        if (open_webpage) {
            QUrl url(__project_url__ + "/issues/new");
            QUrlQuery query;
            query.addQueryItem("body", body);
            if (!title.isEmpty())
                query.addQueryItem("title", title);
            url.setQuery(query);
            QDesktopServices::openUrl(url);
        }
    }
}

void MainWindow::trouble_guide()
{
    QUrl url(__trouble_url__);
    QDesktopServices::openUrl(url);
}

void MainWindow::google_group()
{
    QUrl url(__forum_url__);
    QDesktopServices::openUrl(url);
}

void MainWindow::global_callback()
{
    QWidget* widget = QApplication::focusWidget();
    QAction* action = qobject_cast<QAction*>(this->sender());
    QString callback = action->data().toString();
#if 1
	std::string method_name = callback.toStdString();
	bool success = QMetaObject::invokeMethod(widget, method_name.c_str());
	Q_UNUSED(success);
	method_name.clear();
#else
    TextEditBaseWidget* base_widget = qobject_cast<TextEditBaseWidget*>(widget);
    ControlWidget* ctl_widget = qobject_cast<ControlWidget*>(widget);

    if (base_widget) {
        if (callback == "undo")
            base_widget->undo();
        else if (callback == "redo")
            base_widget->redo();
        else if (callback == "copy")
            base_widget->copy();
        else if (callback == "cut")
            base_widget->cut();
        else if (callback == "paste")
            base_widget->paste();
        else if (callback == "selectAll")
            base_widget->selectAll();
    }
    else if (ctl_widget) {
        if (callback == "undo")
            ctl_widget->undo();
        else if (callback == "redo")
            ctl_widget->redo();
        else if (callback == "copy")
            ctl_widget->copy();
        else if (callback == "cut")
            ctl_widget->cut();
        else if (callback == "paste")
            ctl_widget->paste();
        else if (callback == "selectAll")
            ctl_widget->selectAll();
    }
#endif
}

void MainWindow::redirect_internalshell_stdio(bool state)
{
    qDebug() << __func__;
    //this->console.shell.interpreter.redirect_stds()
}

void MainWindow::open_external_console(QString fname, QString wdir, QString args, bool interact, bool debug,
                                       bool python, QString python_args, bool systerm, bool post_mortem)
{
    Q_UNUSED(python);
    Q_UNUSED(post_mortem);
    if (systerm) {
        try {
            QString executable;
            if (CONF_get("main_interpreter", "default").toBool())
                executable = misc::get_python_executable();
            else
                executable = CONF_get("main_interpreter", "executable").toString();
            programs::run_python_script_in_terminal(fname, wdir, args, interact,
                                                    debug, python_args,executable);
        } catch (...) {
            QMessageBox::critical(this, "Run",
                                  QString("Running an external system terminal "
                                          "is not supported on platform %1.")
                                  .arg(os::name));
        }
    }
}

void MainWindow::execute_in_external_console(const QString& lines, bool focus_to_editor)
{
    // console = this->ipyconsole
    // console.execute_code(lines) 执行代码

    if (focus_to_editor)
        this->editor->switch_to_plugin();
}

void MainWindow::open_file(QString fname, bool external)
{
    QFileInfo info(fname);
    QString ext = "." + info.suffix();
    if (encoding::is_text_file(fname))
        this->editor->load(fname);
    //elif this->variableexplorer is not nullptr; and ext in IMPORT_EXT:
    else if (!external) {
        fname = file_uri(fname);
        programs::start_file(fname);
    }
}

void MainWindow::open_external_file(const QString& fname)
{
    QFileInfo info(fname);
    QString CWD = misc::getcwd_or_home();
    QString tmp = CWD+'/'+fname;
    QFileInfo fileinfo(tmp);
    if (info.isFile())
        this->open_file(fname, true);
    else if (fileinfo.isFile())
        this->open_file(tmp, true);
}

QStringList MainWindow::get_spyder_pythonpath() const
{
    QStringList active_path;
    foreach (QString p, this->path) {
        if (!this->not_active_path.contains(p))
            active_path.append(p);
    }
    active_path.append(this->project_path);
    return active_path;
}

void MainWindow::add_path_to_sys_path()
{
    //sys.path.insert(1, path)
}

void MainWindow::remove_path_from_sys_path()
{
    //sys.path.remove(path)
}

void MainWindow::path_manager_callback()
{
    this->remove_path_from_sys_path();
    QStringList project_path = this->projects->get_pythonpath();
    PathManager* dialog = new PathManager(this, this->path, project_path,
                                          this->not_active_path, true);
    connect(dialog, SIGNAL(redirect_stdio(bool)),
            SLOT(redirect_internalshell_stdio(bool)));
    dialog->exec();
    this->add_path_to_sys_path();
    encoding::writelines(this->path, this->SPYDER_PATH);
    encoding::writelines(this->not_active_path, SPYDER_NOT_ACTIVE_PATH);

    emit sig_pythonpath_changed();
}

void MainWindow::pythonpath_changed()
{
    this->remove_path_from_sys_path();
    this->project_path = this->projects->get_pythonpath();
    this->add_path_to_sys_path();

    emit sig_pythonpath_changed();
}

void MainWindow::win_env()
{
    //this->dialog_manager.show(WinUserEnvDialog(self))
}

void MainWindow::apply_settings()
{
    if (is_gtk_desktop() && QStyleFactory::keys().contains("GTK+")) {
        try {
            qApp->setStyle("gtk+");
        } catch (...) {
        }
    }
    else {
        QString style_name = CONF_get("main", "windows_style",
                                      this->default_style).toString();
        QStyle* style = QStyleFactory::create(style_name);
        if (style) {
            style->setProperty("name", style_name);
            qApp->setStyle(style);
        }
    }

    QMainWindow::DockOptions _default = this->DOCKOPTIONS;
    if (CONF_get("main", "vertical_tabs").toBool())
        _default |= QMainWindow::VerticalTabs;
    if (CONF_get("main", "animated_docks").toBool())
        _default |= QMainWindow::AnimatedDocks;
    this->setDockOptions(_default);

    this->apply_panes_settings();
    this->apply_statusbar_settings();

    if (CONF_get("main", "use_custom_cursor_blinking").toBool())
        qApp->setCursorFlashTime(CONF_get("main", "custom_cursor_blinking").toInt());
    else
        qApp->setCursorFlashTime(this->CURSORBLINK_OSDEFAULT);
}

void MainWindow::apply_panes_settings()
{
    foreach (SpyderPluginMixin* child, this->widgetlist) {
        QDockWidget::DockWidgetFeatures features = child->FEATURES;
        if (CONF_get("main", "vertical_dockwidget_titlebars").toBool())
            features |= QDockWidget::DockWidgetVerticalTitleBar;
        if (!this->dockwidgets_locked)
            features |= QDockWidget::DockWidgetMovable;
        child->dockwidget->setFeatures(features);
        child->update_margins();
    }
}

void MainWindow::apply_statusbar_settings()
{
    bool show_status_bar = CONF_get("main", "show_status_bar").toBool();
    this->statusBar()->setVisible(show_status_bar);

    if (show_status_bar) {
        if (this->mem_status != nullptr) {
            QString name = "memory_usage";
            mem_status->setVisible(CONF_get("main", QString("%1/enable").arg(name)).toBool());
            mem_status->set_interval(CONF_get("main", QString("%1/timeout").arg(name)).toInt());
        }
        if (this->cpu_status != nullptr) {
            QString name = "cpu_usage";
            cpu_status->setVisible(CONF_get("main", QString("%1/enable").arg(name)).toBool());
            cpu_status->set_interval(CONF_get("main", QString("%1/timeout").arg(name)).toInt());
        }
    }
    else
        return;
}

void MainWindow::edit_preferences()
{
    ConfigDialog* dlg = new ConfigDialog(this);
    connect(dlg, SIGNAL(size_change(const QSize&)), SLOT(set_prefs_size(const QSize&)));
    if (this->prefs_dialog_size.isValid())
        dlg->resize(this->prefs_dialog_size);

    MainConfigPage* main_con_page = new MainConfigPage(dlg, this);
    main_con_page->initialize();
    dlg->add_page(main_con_page);

    MainInterpreterConfigPage* interpreter = new MainInterpreterConfigPage(dlg, this);
    interpreter->initialize();
    dlg->add_page(interpreter);

    dlg->show();
    dlg->check_all_settings();
    connect(dlg->pages_widget, SIGNAL(currentChanged(int)), SLOT(__preference_page_changed(int)));
    dlg->exec();

    // 该函数调用了plugin.create_configwidget()函数
    // create_configwidget函数用到了CONFIGWIDGET_CLASS类，
    // plugins/editor定义的第一个类EditorConfigPage
}

void MainWindow::__preference_page_changed(int index)
{
    this->prefs_index = index;
}

void MainWindow::set_prefs_size(const QSize& size)
{
    this->prefs_dialog_size = size;
}

void MainWindow::register_shortcut(QObject *qaction_or_qshortcut, const QString &context,
                                   const QString &name, bool add_sc_to_tip)
{
    this->shortcut_data.append(ShortCutStrStrBool(qaction_or_qshortcut, context,
                                                  name, add_sc_to_tip));
}

void MainWindow::apply_shortcuts()
{
    QList<int> toberemoved;
    for (int index = 0; index < this->shortcut_data.size(); ++index) {
        ShortCutStrStrBool tmp = shortcut_data[index];
        QObject* qobject = tmp.qaction_or_qshortcut;
        QString context = tmp.context;
        QString name = tmp.name;
        bool add_sc_to_tip = tmp.add_sc_to_tip;
        QKeySequence keyseq(gui::get_shortcut(context, name));

        try {
            QAction* action = qobject_cast<QAction*>(qobject);
            QShortcut* shortcut = qobject_cast<QShortcut*>(qobject);
            if (action) {
                action->setShortcut(keyseq);
                if (add_sc_to_tip)
                    add_shortcut_to_tooltip(action, context, name);
            }
            else if (shortcut)
                shortcut->setKey(keyseq);
        } catch (...) {
            toberemoved.append(index);
        }
    }

    std::sort(toberemoved.begin(),toberemoved.end());
    std::reverse(toberemoved.begin(),toberemoved.end());
    for (int i = 0; i < toberemoved.size(); ++i) {
        int index = toberemoved[i];
        this->shortcut_data.removeAt(index);
    }
}

void MainWindow::show_shortcuts_dialog()
{
    //widgets.shortcutssummary
    qDebug() << __func__;
}

void MainWindow::start_open_files_server()
{
    qDebug() << __func__;
}

void MainWindow::reset_spyder()
{
    QMessageBox::StandardButton answer = QMessageBox::warning(this, "Warning",
                                                              "Spyder will restart and reset to default settings: <br><br>"
                                                              "Do you want to continue?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::Yes)
        this->restart(true);
}

void MainWindow::restart(bool reset)
{
    qDebug() << __func__;
}

void MainWindow::show_tour()
{
    this->maximize_dockwidget(true);
	const int index = 0;
	QMap<QString, QVariant > frames = this->tours_available[index];
	this->tour->set_tour(index, frames, this);
	this->tour->start_tour();
}

void MainWindow::open_fileswitcher(bool symbol)
{
    qDebug() << __func__;
}

void MainWindow::open_symbolfinder()
{
    this->open_fileswitcher(true);
}

void MainWindow::add_to_fileswitcher(Editor *plugin, BaseTabs *tabs,
                                     QList<FileInfo *> data, QIcon icon)
{
    qDebug() << __func__;
}

void MainWindow::_check_updates_ready()
{
    qDebug() << __func__;
    //workers.updates import WorkerUpdates
}

void MainWindow::check_updates(bool startup)
{
    qDebug() << __func__;
    //workers.updates import WorkerUpdates
    //WorkerUpdates类需要ssl、urllib等库
}

bool MainWindow::_test_setting_opengl(const QString &option)
{
    if(option == "software")
        return QCoreApplication::testAttribute(Qt::AA_UseSoftwareOpenGL);
    else if(option == "desktop")
        return QCoreApplication::testAttribute(Qt::AA_UseDesktopOpenGL);
    else if(option == "gles")
        return QCoreApplication::testAttribute(Qt::AA_UseOpenGLES);
    return false;
}

void initialize()
{
    QIcon APP_ICON = QIcon(get_image_path("spyder.svg"));
    qApp->setWindowIcon(APP_ICON);

    if (CONF_get("main", "opengl").toString() != "automatic") {
        QString option = CONF_get("main", "opengl").toString();
        set_opengl_implementation(option);
    }
}

static void test()
{   
    MainWindow* win = new MainWindow;
    win->setup();
    win->show();
    win->post_visible_setup();

    //if main.console:
    QObject::connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
            win, SLOT(change_last_focused_widget(QWidget *, QWidget *)));
}
