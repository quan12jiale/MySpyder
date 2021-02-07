#include "workingdirectory.h"
#include "app/mainwindow.h"

WorkingDirectory::WorkingDirectory(MainWindow *parent, QString workdir)
    : QToolBar (parent)
{
    this->CONF_SECTION = "workingdir";
    this->LOG_PATH = get_conf_path(CONF_SECTION);

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

    this->setWindowTitle(this->get_plugin_title());
    this->setObjectName(this->get_plugin_title());// Used to save Window state

    this->history = QStringList();
    this->histindex = -1;

    previous_action = new QAction(ima::icon("previous"), "previous", this);
    previous_action->setToolTip("Back");
    connect(previous_action, SIGNAL(triggered()), SLOT(previous_directory()));
    this->addAction(previous_action);

    next_action = new QAction(ima::icon("next"), "next", this);
    next_action->setToolTip("Next");
    connect(next_action, SIGNAL(triggered()), SLOT(next_directory()));
    this->addAction(next_action);

    connect(this, SIGNAL(set_previous_enabled(bool)),
            previous_action, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(set_next_enabled(bool)),
            next_action, SLOT(setEnabled(bool)));

    bool adjust = this->get_option("working_dir_adjusttocontents").toBool();
    this->pathedit = new PathComboBox(this, adjust);
    pathedit->setToolTip("This is the working directory for newly\n"
                         "opened consoles (Python/IPython consoles and\n"
                         "terminals), for the file explorer, for the\n"
                         "find in files plugin and for new files\n"
                         "created in the editor");
    connect(pathedit, &PathComboBox::open_dir,
            [this](QString directory){this->chdir(directory);});
    connect(pathedit,
            static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated),
            [this](QString directory){this->chdir(directory);});
    pathedit->setMaxCount(this->get_option("working_dir_history").toInt());

    QStringList wdhistory = this->load_wdhistory(workdir);
    /*if (workdir.isEmpty()) {
        if (this->get_option("console/use_project_or_home_directory").toBool())
            workdir = get_home_dir();//需要注释这段话，不然会更改当前工作目录，影响图标显示
        else {
            workdir = this->get_option("console/fixed_directory", QString("")).toString();
            QFileInfo info(workdir);
            if (!info.isDir())
                workdir = get_home_dir();
        }
    }*/
    this->chdir(workdir);
    this->pathedit->addItems(wdhistory);
    this->pathedit->selected_text = this->pathedit->currentText();
    this->refresh_plugin();
    this->addWidget(this->pathedit);

    QAction* browse_action = new QAction(ima::icon("DirOpenIcon"), "browse", this);
    browse_action->setToolTip("Browse a working directory");
    connect(browse_action, SIGNAL(triggered()), SLOT(select_directory()));
    this->addAction(browse_action);

    QAction* parent_action = new QAction(ima::icon("up"), "parent", this);
    parent_action->setToolTip("Change to parent directory");
    connect(parent_action, SIGNAL(triggered()), SLOT(parent_directory()));
    this->addAction(parent_action);
}

QString WorkingDirectory::get_plugin_title() const
{
    return "Current working directory";
}

QIcon WorkingDirectory::get_plugin_icon() const
{
    return ima::icon("DirOpenIcon");
}

QList<QAction*> WorkingDirectory::get_plugin_actions()
{
    QList<QAction*> actions;
    actions << nullptr << nullptr;
    return actions;
}

void WorkingDirectory::register_plugin()
{
    connect(this, SIGNAL(redirect_stdio(bool)),
            this->main, SLOT(redirect_internalshell_stdio(bool)));
    // TODO
    //self.main.console.shell.refresh.connect(self.refresh_plugin)
    int iconsize = 24;
    this->setIconSize(QSize(iconsize, iconsize));
    this->main->addToolBar(this);
}

void WorkingDirectory::refresh_plugin()
{
    QString curdir = misc::getcwd_or_home();
    this->pathedit->add_text(curdir);
    this->save_wdhistory();
    emit set_previous_enabled(this->histindex > 0);
    emit set_next_enabled(-1 < histindex && histindex < history.size()-1);
}

QStringList WorkingDirectory::load_wdhistory(QString workdir) const
{
    QFileInfo info(this->LOG_PATH);
    QStringList wdhistory;
    if (info.isFile()) {
        wdhistory = encoding::readlines(this->LOG_PATH);
        QStringList tmp(wdhistory);
        wdhistory.clear();
        foreach (QString name, tmp) {
            QFileInfo fileinfo(name);
            if (fileinfo.isDir())
                wdhistory.append(name);
        }
    }
    else {
        if (workdir.isEmpty())
            workdir = get_home_dir();
        wdhistory = QStringList(workdir);
    }
    return wdhistory;
}

void WorkingDirectory::save_wdhistory()
{
    QStringList list;
    for (int index = 0; index < this->pathedit->count(); ++index) {
        list.append(this->pathedit->itemText(index));
    }
    encoding::writelines(list, this->LOG_PATH);
}

void WorkingDirectory::select_directory()
{
    emit redirect_stdio(false);
    QString directory = QFileDialog::getExistingDirectory(this->main, "Select directory",
                                                          misc::getcwd_or_home());
    if (!directory.isEmpty())
        this->chdir(directory);
    emit redirect_stdio(true);
}

void WorkingDirectory::previous_directory()
{
    if (this->histindex >= 0)
        this->histindex -= 1;
    this->chdir("", true);
}

void WorkingDirectory::next_directory()
{
    this->histindex += 1;
    this->chdir("", true);
}

void WorkingDirectory::parent_directory()
{
    QString directory = misc::getcwd_or_home() + '/' + os::pardir;
    this->chdir(directory);
}

void WorkingDirectory::chdir(QString directory, bool browsing_history,
                             bool refresh_explorer, bool refresh_console)
{
    if (!directory.isEmpty()) {
        QFileInfo info(directory);
        directory = info.absoluteFilePath();
    }

    if (browsing_history) {
        if (this->histindex >=0 && this->histindex < this->history.size())
            directory = this->history[this->histindex];
    }
    else if (this->history.contains(directory)) {
        this->histindex = this->history.indexOf(directory);
    }
    else {
        if (this->histindex == -1)
            this->history.clear();
        else {
            this->history = this->history.mid(0, this->histindex+1);
        }
        this->history.append(directory);
        this->histindex = this->history.size()-1;
    }

    //QDir::setCurrent设置Qt应用程序工作目录，这里控制工作目录工具栏
    //记得同步把python cmd cd到directory
    bool ok = QDir::setCurrent(directory);
    if (ok) {
        if (refresh_explorer)
            emit set_explorer_cwd(directory);
        if (refresh_console)
            emit set_current_console_wd(directory);
        emit refresh_findinfiles();
    }
    else {
        if (this->histindex >=0 && this->histindex < this->history.size())
            this->history.removeAt(this->histindex);
    }
    this->refresh_plugin();
}

//------ SpyderPluginWidget API ---------
void WorkingDirectory::initialize_plugin()
{
    this->create_toggle_view_action();
    this->plugin_actions = this->get_plugin_actions();

    connect(this, SIGNAL(sig_option_changed(QString, QVariant)),
            this, SLOT(set_option(QString, QVariant)));
    this->setWindowTitle(this->get_plugin_title());
}


void WorkingDirectory::update_margins()
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

QPair<SpyderDockWidget*,Qt::DockWidgetArea> WorkingDirectory::create_dockwidget()
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

QMainWindow* WorkingDirectory::create_mainwindow()
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

void WorkingDirectory::create_toggle_view_action()
{
    QString title = this->get_plugin_title();
    if (this->CONF_SECTION == "editor")
        title = "Editor";
    QAction* action = new QAction(title, this);
    connect(action, &QAction::toggled, [this](bool checked){this->toggle_view(checked);});
    action->setShortcut(QKeySequence(this->shortcut));
    action->setShortcutContext(Qt::WidgetShortcut);
    this->toggle_view_action = action;
}
