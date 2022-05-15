#include "explorer.h"
#include "widgets/projects/projects_explorer.h"
#include "plugins/plugins_explorer.h"


void open_file_in_external_explorer(QString filename)
{
    QFileInfo info(filename);
    filename = info.absolutePath();
    if (os::name == "nt") {
        qDebug() << __func__ << filename;
        QString program = "cmd";
        QStringList arguments = {"/c", "start", filename};
        // cmd /c      执行字符串指定的命令然后终止
        QProcess::execute(program, arguments);
    }
}

void show_in_external_file_explorer(const QStringList& fnames)
{
    foreach (QString fname, fnames)
        open_file_in_external_explorer(fname);
}

QString fixpath(const QString& path)
{
    // 返回绝对路径
    QFileInfo info(path);
    return info.absoluteFilePath();
}

void create_script(const QString& fname)
{
    QStringList list = {"# -*- coding: utf-8 -*-", "", ""};
    QString text = list.join(os::linesep);
    bool ok = encoding::write(text, fname);
    if (ok == false) {
        QFileInfo info(fname);
        QMessageBox::critical(nullptr, "Save Error",
                              QString("<b>Unable to save file '%1'</b>"
                                      "<br><br>Error message:<br>")
                              .arg(info.fileName()));
    }
}


QHash<QString,QString> IconProvider::APP_FILES  =
{{"zip", "ArchiveFileIcon"},
 {"x-tar", "ArchiveFileIcon"},
 {"x-7z-compressed", "ArchiveFileIcon"},
 {"rar", "ArchiveFileIcon"},

 {"vnd.ms-powerpoint", "PowerpointFileIcon"},
 {"vnd.openxmlformats-officedocument.presentationml.presentation", "PowerpointFileIcon"},
 {"msword", "WordFileIcon"},
 {"vnd.openxmlformats-officedocument.wordprocessingml.document", "WordFileIcon"},
 {"vnd.ms-excel", "ExcelFileIcon"},
 {"vnd.openxmlformats-officedocument.spreadsheetml.sheet", "ExcelFileIcon"},
 {"pdf", "PDFIcon"}};

QHash<QString,QString> IconProvider::OFFICE_FILES =
{{".xlsx", "ExcelFileIcon"},
 {".docx", "WordFileIcon"},
 {".pptx", "PowerpointFileIcon"}};

IconProvider::IconProvider(QTreeView* treeview)
    : QFileIconProvider ()
{
    this->treeview = treeview;
    application_icons = APP_FILES;
}

QIcon IconProvider::icon(const QFileInfo &qfileinfo) const
{
    if (qfileinfo.isDir())
        return ima::icon("DirOpenIcon");
    else {
        // osp.basename()   <==>   info.fileName()
        QString basename = qfileinfo.fileName();
        QString extension = "." + qfileinfo.suffix();
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(qfileinfo);
        QString mime_type = mime.name();
        QIcon icon = ima::icon("FileIcon");

        if (OFFICE_FILES.contains(extension))
            icon = ima::icon(OFFICE_FILES[extension]);

        if (!mime_type.isEmpty()) {
            QString file_type, bin_name;
            try {
                QStringList list = mime_type.split('/');
                file_type = list[0];
                bin_name = list[1];
            } catch (...) {
                file_type = "text";
            }
            if (file_type == "text")
                icon = ima::icon("TextFileIcon");
            else if (file_type == "audio")
                icon = ima::icon("AudioFileIcon");
            else if (file_type == "video")
                icon = ima::icon("VideoFileIcon");
            else if (file_type == "image")
                icon = ima::icon("ImageFileIcon");
            else if (file_type == "application") {
                if (application_icons.contains(bin_name))
                    icon = ima::icon(application_icons[bin_name]);
            }
        }
        return icon;
    }
}


/********** DirView **********/
DirView::DirView(QWidget* parent)
    : QTreeView (parent)
{
    this->name_filters = QStringList("*.py");
    this->parent_widget = parent;
    this->show_all = false;
    this->menu = nullptr;
    this->common_actions = QList<QObject*>();
    this->__expanded_state = QStringList();
    this->_to_be_loaded = QStringList();
    this->fsmodel = nullptr;
    this->setup_fs_model();
    this->_scrollbar_positions = QPoint();
}

void DirView::setup_fs_model()
{
    QDir::Filters filters = QDir::AllDirs | QDir::Files | QDir::Drives | QDir::NoDotAndDotDot;
    fsmodel = new QFileSystemModel(this);
    fsmodel->setFilter(filters);
    fsmodel->setNameFilterDisables(false);
}

void DirView::install_model()
{
    this->setModel(this->fsmodel);
}

void DirView::setup_view()
{
    this->install_model();
    connect(fsmodel, &QFileSystemModel::directoryLoaded,
            [this](){this->resizeColumnToContents(0);});
    this->setAnimated(false);
    this->setSortingEnabled(true);
    this->sortByColumn(0, Qt::AscendingOrder);
    connect(fsmodel, &QFileSystemModel::modelReset, this, &DirView::reset_icon_provider);
    this->reset_icon_provider();
    this->filter_directories();
}

void DirView::set_name_filters(const QStringList &name_filters)
{
    this->name_filters = name_filters;
    fsmodel->setNameFilters(name_filters);
}

void DirView::set_show_all(bool state)
{
    if (state)
        fsmodel->setNameFilters(QStringList());
    else
        fsmodel->setNameFilters(name_filters);
}

QString DirView::get_filename(const QModelIndex &index) const
{
    if (index.isValid()) {
        QFileInfo info(fsmodel->filePath(index));
        return info.canonicalFilePath();
    }
    return QString();
}

QModelIndex DirView::get_index(const QString &filename) const
{
    return fsmodel->index(filename);
}

QStringList DirView::get_selected_filenames() const
{
    QStringList list;
    if (this->selectionMode() == ExtendedSelection) {
        foreach (QModelIndex idx, this->selectedIndexes()) {
            QString filename = this->get_filename(idx);
            list.append(filename);
        }
    }
    else {
        list.append(this->get_filename(this->currentIndex()));
    }
    return list;
}

QString DirView::get_dirname(const QModelIndex &index) const
{
    QString fname = this->get_filename(index);
    if (!fname.isEmpty()) {
        QFileInfo info(fname);
        if (info.isDir())
            return fname;
        else
            return info.absolutePath();
    }
    return QString();
}

//---- Tree view widget
void DirView::setup(const QStringList &name_filters, bool show_all)
{
    this->setup_view();
    this->set_name_filters(name_filters);
    this->show_all = show_all;
    this->menu = new QMenu(this);
    common_actions = this->setup_common_actions();
}

void DirView::reset_icon_provider()
{
    fsmodel->setIconProvider(new IconProvider(this));
}

//---- Context menu
QList<QObject*> DirView::setup_common_actions()
{
    QAction* filters_action = new QAction("Edit filename filters...", this);
    filters_action->setIcon(ima::icon("filter"));
    connect(filters_action, SIGNAL(triggered(bool)),this,SLOT(edit_filter()));
    filters_action->setShortcutContext(Qt::WindowShortcut);

    QAction* all_action = new QAction("Show all files", this);
    connect(all_action, SIGNAL(toggled(bool)),this,SLOT(toggle_all(bool)));
    all_action->setCheckable(true);
    all_action->setShortcutContext(Qt::WindowShortcut);

    all_action->setChecked(this->show_all);
    this->toggle_all(this->show_all);

    QList<QObject*> actions;
    actions << filters_action << all_action;
    return actions;
}

void DirView::edit_filter()
{
    bool valid;
    QString filters = QInputDialog::getText(this, "Edit filename filters",
                                            "Name filters:",
                                            QLineEdit::Normal,
                                            name_filters.join(", "), &valid);
    if (valid) {
        QStringList list;
        foreach (QString f, filters.split(',')) {
            list.append(f.trimmed());
        }
        ExplorerWidget* widget1 = qobject_cast<ExplorerWidget*>(parent_widget);
        ProjectExplorerWidget* widget2 = qobject_cast<ProjectExplorerWidget*>(parent_widget);
        if (widget1)
            emit widget1->sig_option_changed("name_filters", list);
        else if (widget2)
            emit widget2->sig_option_changed("name_filters", list);
        this->set_name_filters(list);
    }
}

void DirView::toggle_all(bool checked)
{
    ExplorerWidget* widget1 = qobject_cast<ExplorerWidget*>(parent_widget);
    ProjectExplorerWidget* widget2 = qobject_cast<ProjectExplorerWidget*>(parent_widget);
    if (widget1)
        emit widget1->sig_option_changed("show_all", checked);
    else if (widget2)
        emit widget2->sig_option_changed("show_all", checked);
    this->show_all = checked;
    this->set_show_all(checked);
}

QList<QObject*> DirView::create_file_new_actions(const QStringList &fnames)
{
    QList<QObject*> actions;
    if (fnames.isEmpty())
        return actions;

    QAction* new_file_act = new QAction("File...", this);
    new_file_act->setIcon(ima::icon("filenew"));
    connect(new_file_act, &QAction::triggered, [=](){this->new_file(fnames.last());});
    new_file_act->setShortcutContext(Qt::WindowShortcut);

    QAction* new_module_act = new QAction("Module...", this);
    new_module_act->setIcon(ima::icon("spyder"));
    connect(new_module_act, &QAction::triggered, [=](){this->new_module(fnames.last());});
    new_module_act->setShortcutContext(Qt::WindowShortcut);

    QAction* new_folder_act = new QAction("Folder...", this);
    new_folder_act->setIcon(ima::icon("folder_new"));
    connect(new_folder_act, &QAction::triggered, [=](){this->new_folder(fnames.last());});
    new_folder_act->setShortcutContext(Qt::WindowShortcut);

    QAction* new_package_act = new QAction("Package...", this);
    new_package_act->setIcon(ima::icon("package_new"));
    connect(new_package_act, &QAction::triggered, [=](){this->new_package(fnames.last());});
    new_package_act->setShortcutContext(Qt::WindowShortcut);

    actions << new_file_act << new_folder_act << nullptr
            << new_module_act << new_package_act;
    return actions;
}

QList<QObject*> DirView::create_file_import_actions(const QStringList &fnames)
{
    Q_UNUSED(fnames);
    QList<QObject*> actions;
    return actions;
}

bool all_files(const QStringList &fnames)
{
    foreach (QString fn, fnames) {
        QFileInfo info(fn);
        if (!info.isFile())
            return false;
    }
    return true;
}

bool all_modules(const QStringList &fnames)
{
    QStringList suffixes = {"py", "pyw", "ipy"};
    foreach (QString fn, fnames) {
        QFileInfo info(fn);
        if (!suffixes.contains(info.suffix()))
            return false;
    }
    return true;
}

bool all_notebooks(const QStringList &fnames)
{
    foreach (QString fn, fnames) {
        QFileInfo info(fn);
        if (info.suffix() != "ipynb")
            return false;
    }
    return true;
}

bool all_valid(const QStringList &fnames)
{
    foreach (QString fn, fnames) {
        if (!encoding::is_text_file(fn))
            return false;
    }
    return true;
}

bool all_samedir(const QStringList &fnames, const QString& basedir)
{
    foreach (QString fn, fnames) {
        QFileInfo info(fn);
        if (fixpath(info.absolutePath()) != basedir)
            return false;
    }
    return true;
}

bool all_isdir(const QStringList &fnames)
{
    foreach (QString fn, fnames) {
        QFileInfo info(fn);
        if (!info.isDir())
            return false;
    }
    return true;
}

QList<QObject*> DirView::create_file_manage_actions(const QStringList &fnames)
{
    bool only_files = all_files(fnames);
    bool only_modules = all_modules(fnames);
    bool only_notebooks = all_notebooks(fnames);
    bool only_valid = all_valid(fnames);

    QAction* run_action = new QAction("Run", this);
    run_action->setIcon(ima::icon("run"));
    connect(run_action, &QAction::triggered, [=](){this->run();});
    run_action->setShortcutContext(Qt::WindowShortcut);

    QAction* edit_action = new QAction("Edit", this);
    edit_action->setIcon(ima::icon("edit"));
    connect(edit_action, SIGNAL(triggered(bool)),this,SLOT(clicked()));
    edit_action->setShortcutContext(Qt::WindowShortcut);

    QAction* move_action = new QAction("Move...", this);
    move_action->setIcon(ima::get_icon("move.png"));
    connect(move_action, &QAction::triggered, [=](){this->move();});
    move_action->setShortcutContext(Qt::WindowShortcut);

    QAction* delete_action = new QAction("Delete...", this);
    delete_action->setIcon(ima::icon("editdelete"));
    connect(delete_action, &QAction::triggered, [=](){this->_delete();});
    delete_action->setShortcutContext(Qt::WindowShortcut);

    QAction* rename_action = new QAction("Rename...", this);
    rename_action->setIcon(ima::icon("rename"));
    connect(rename_action, &QAction::triggered, [=](){this->rename();});
    rename_action->setShortcutContext(Qt::WindowShortcut);

    QAction* open_action = new QAction("Open", this);
    connect(open_action, &QAction::triggered, [=](){this->open();});
    open_action->setShortcutContext(Qt::WindowShortcut);

    QAction* ipynb_convert_action = new QAction("Convert to Python script", this);
    ipynb_convert_action->setIcon(ima::icon("python"));
    connect(ipynb_convert_action, SIGNAL(triggered(bool)),this,SLOT(convert_notebooks()));
    ipynb_convert_action->setShortcutContext(Qt::WindowShortcut);

    QList<QObject*> actions;
    if (only_modules)
        actions.append(run_action);
    if (only_valid && only_files)
        actions.append(edit_action);
    else
        actions.append(open_action);

    QString text = "Show in external file explorer";

    QAction* external_fileexp_action = new QAction(text, this);
    connect(external_fileexp_action, &QAction::triggered, [=](){this->show_in_external_file_explorer();});
    external_fileexp_action->setShortcutContext(Qt::WindowShortcut);

    actions.append(external_fileexp_action);
    actions << delete_action << rename_action;

    Q_ASSERT(!fnames.isEmpty());

    QFileInfo info(fnames[0]);
    QString basedir = info.absolutePath();
    if (all_samedir(fnames, basedir))
        actions.append(move_action);
    actions.append(nullptr);
    if (only_notebooks && /* DISABLES CODE */ (false))
        actions.append(ipynb_convert_action);

    QString dirname;
    if (info.isDir())
        dirname = fnames[0];
    else
        dirname = info.absolutePath();

    // 暂不实现版本控制
    if (fnames.size() == /* DISABLES CODE */ (1) && false) {
        actions.append(nullptr);
    }

    return actions;
}

QList<QObject*> DirView::create_folder_manage_actions(const QStringList &fnames)
{
    QList<QObject*> actions;
    QString _title = "Open IPython console here";
    QAction* action = new QAction(_title, this);
    connect(action, &QAction::triggered, [=](){this->open_interpreter(fnames);});
    action->setShortcutContext(Qt::WindowShortcut);
    actions.append(action);
    return actions;
}

QList<QObject*> DirView::create_context_menu_actions()
{
    QList<QObject*> actions;
    QStringList fnames = this->get_selected_filenames();
    QList<QObject*> new_actions = create_file_new_actions(fnames);
    if (new_actions.size() > 1) {
        QMenu* new_act_menu = new QMenu("New", this);
        add_actions(new_act_menu, new_actions);
        actions.append(new_act_menu);
    }
    else
        actions.append(new_actions);

    QList<QObject*> import_actions = create_file_import_actions(fnames);
    if (import_actions.size() > 1) {
        QMenu* import_act_menu = new QMenu("Import", this);
        add_actions(import_act_menu, import_actions);
        actions.append(import_act_menu);
    }
    else
        actions.append(import_actions);

    if (!actions.isEmpty())
        actions.append(nullptr);
    if (!fnames.isEmpty())
        actions.append(this->create_file_manage_actions(fnames));

    if (!actions.isEmpty())
        actions.append(nullptr);
    if (!fnames.isEmpty() && all_isdir(fnames))
        actions.append(this->create_folder_manage_actions(fnames));

    if (!actions.isEmpty())
        actions.append(nullptr);
    actions.append(this->common_actions);
    return actions;
}

void DirView::update_menu()
{
    this->menu->clear();
    add_actions(this->menu, this->create_context_menu_actions());
}

//---- Events
bool DirView::viewportEvent(QEvent *event)
{
    this->executeDelayedItemsLayout();
    return QTreeView::viewportEvent(event);
}

void DirView::contextMenuEvent(QContextMenuEvent *event)
{
    try {
        this->update_menu();
        this->menu->popup(event->globalPos());
    } catch (...) {
    }
}

void DirView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        this->clicked();
        break;
    case Qt::Key_F2:
        this->rename();
        break;
    case Qt::Key_Delete:
        this->_delete();
        break;
    case Qt::Key_Backspace:
        this->go_to_parent_directory();
        break;
    default:
        QTreeView::keyPressEvent(event);
        break;
    }
}

void DirView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QTreeView::mouseDoubleClickEvent(event);
    this->clicked();
}

void DirView::clicked()
{
    QStringList fnames = get_selected_filenames();
    foreach (QString fname, fnames) {
        QFileInfo info(fname);
        if (info.isDir()) {
            this->directory_clicked(fname);
        }
        else {
            this->open(QStringList(fname));
        }
    }
}

void DirView::dragEnterEvent(QDragEnterEvent *event)
{
    event->setAccepted(event->mimeData()->hasFormat("text/plain"));
}

void DirView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("text/plain")) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
        event->ignore();
}

void DirView::startDrag(Qt::DropActions dropActions)
{
    Q_UNUSED(dropActions);

    QMimeData *data = new QMimeData;
    QList<QUrl> urls;
    foreach (QString fname, this->get_selected_filenames())
        urls.append(QUrl(fname));
    data->setUrls(urls);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(data);
    drag->exec();
}

//---- File/Directory actions
void DirView::open(QStringList fnames)
{
    if (fnames.isEmpty())
        fnames = this->get_selected_filenames();
    foreach (QString fname, fnames) {
        QFileInfo info(fname);
        if (info.isFile() && encoding::is_text_file(fname)) {
            ExplorerWidget* widget1 = qobject_cast<ExplorerWidget*>(parent_widget);
            ProjectExplorerWidget* widget2 = qobject_cast<ProjectExplorerWidget*>(parent_widget);
            if (widget1)
                emit widget1->sig_open_file(fname);
            else if (widget2)
                emit widget2->sig_open_file(fname);
        }
        else {
            this->open_outside_spyder(QStringList(fname));
        }
    }
}

void DirView::open_outside_spyder(QStringList fnames)
{
    std::sort(fnames.begin(), fnames.end());
    foreach (QString tmp, fnames) {
        QString path = file_uri(tmp);
        bool ok = programs::start_file(path);
        if (ok == false) {
            Explorer* parent = qobject_cast<Explorer*>(parent_widget);
            if (parent)
                emit parent->edit(path);
        }
    }
}

void DirView::open_interpreter(QStringList fnames)
{
    std::sort(fnames.begin(), fnames.end());
    foreach (QString path, fnames) {
        Explorer* parent = qobject_cast<Explorer*>(parent_widget);
        if (parent)
            emit parent->open_interpreter(path);
    }
}

void DirView::run(QStringList fnames)
{
    if (fnames.isEmpty())
        fnames = get_selected_filenames();
    foreach (QString fname, fnames) {
        Explorer* parent = qobject_cast<Explorer*>(parent_widget);
        if (parent)
            emit parent->run(fname);
    }
}

void DirView::remove_tree(const QString &dirname)
{
    QDir dir(dirname);
    dir.removeRecursively();
    //当文件夹下包含只读文件，该函数返回false;
    //源码在这种情况下，更改该文件的权限，再进行删除
    //这里没有进行这种处理
}

int DirView::delete_file(const QString &fname, bool multiple, int yes_to_all)
{
    QMessageBox::StandardButtons buttons;
    if (multiple)
        buttons = QMessageBox::Yes|QMessageBox::YesAll|
                  QMessageBox::No|QMessageBox::Cancel;
    else
        buttons = QMessageBox::Yes|QMessageBox::No;
    QFileInfo info(fname);
    if (yes_to_all == -1) { // 源码是if yes_to_all is None:
        QMessageBox::StandardButton answer = QMessageBox::warning(this,"Delete",
                                                                   QString("Do you really want to delete <b>%1</b>")
                                                                  .arg(info.fileName()),
                                                                   buttons);
        if (answer == QMessageBox::No)
            return yes_to_all;
        else if (answer == QMessageBox::Cancel)
            return 0;
        else if (answer == QMessageBox::YesAll)
            yes_to_all = 1;
    }
    try {
        if (info.isFile()) {
            misc::remove_file(fname);
            Explorer* parent = qobject_cast<Explorer*>(parent_widget);
            if (parent)
                emit parent->removed(fname);
        }
        else {
            this->remove_tree(fname);
            Explorer* parent = qobject_cast<Explorer*>(parent_widget);
            if (parent)
                emit parent->removed_tree(fname);
        }
        return yes_to_all;
    } catch (std::exception error) {
        QString action_str = "delete";
        QMessageBox::critical(this, "Project Explorer",
                              QString("<b>Unable to %1 <i>%2</i></b><br><br>Error message:<br>%3")
                              .arg(action_str).arg(fname).arg(error.what()));
    }
    return 0;
}

void DirView::_delete(QStringList fnames)
{
    if (fnames.isEmpty())
        fnames = this->get_selected_filenames();
    bool multiple = fnames.size() > 1;
    int yes_to_all = -1; //源码是yes_to_all = None
    foreach (QString fname, fnames) {
        QString spyproject_path = fname + "/.spyproject";
        QFileInfo info(fname);
        if (info.isDir() && QFileInfo::exists(spyproject_path)) {
            QMessageBox::information(this, "File explorer",
                                     "The current directory contains a "
                                     "project.<br><br>"
                                     "If you want to delete"
                                     " the project, please go to "
                                     "<b>Projects</b> &raquo; <b>Delete"
                                     "Project</b>");
        }
        else {
            yes_to_all = this->delete_file(fname, multiple, yes_to_all);
            if (yes_to_all == 0) //注意是not yes_to_all
                break;
        }
    }
}

void DirView::convert_notebook(const QString &fname)
{
    Q_UNUSED(fname);
}

//@Slot()
void DirView::convert_notebooks()
{

}

QString DirView::rename_file(const QString &fname)
{
    QFileInfo info(fname);
    bool valid;
    QString path = QInputDialog::getText(this, "Rename",
                                         "New name:", QLineEdit::Normal,
                                         info.fileName(),&valid);
    if (valid) {
        path = info.absolutePath() + '/' + path;
        if (path == fname)
            return QString();
        if (QFileInfo::exists(path)) {
            QFileInfo pathinfo(path);
            if (QMessageBox::warning(this, "Rename",
                                     QString("Do you really want to rename <b>%s</b> and "
                                             "overwrite the existing file <b>%s</b>?").arg(info.fileName()).arg(pathinfo.fileName()),
                                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
                return QString();
        }
        try {
            misc::rename_file(fname, path);
            if (info.isFile()) {
                Explorer* parent = qobject_cast<Explorer*>(parent_widget);
                if (parent)
                    emit parent->renamed(fname, path);
            }
            else {
                Explorer* parent = qobject_cast<Explorer*>(parent_widget);
                if (parent)
                    emit parent->renamed_tree(fname, path);
            }
            return path;
        } catch (std::exception error) {
            QMessageBox::critical(this, "Rename",
                                  QString("<b>Unable to rename file <i>%1</i></b>"
                                          "<br><br>Error message:<br>%2").
                                  arg(info.fileName()).arg(error.what()));
        }
    }
    return QString();
}

void DirView::show_in_external_file_explorer(QStringList fnames)
{
    if (fnames.isEmpty())
        fnames = this->get_selected_filenames();
    ::show_in_external_file_explorer(fnames);
}

void DirView::rename(QStringList fnames)
{
    if (fnames.isEmpty())
        fnames = this->get_selected_filenames();
    foreach (QString fname, fnames) {
        this->rename_file(fname);
    }
}

void DirView::move(QStringList fnames, const QString &directory)
{
    if (fnames.isEmpty())
        fnames = this->get_selected_filenames();
    if (fnames.isEmpty())
        return;
    QFileInfo info(fnames[0]);
    QString orig = info.absolutePath();
    QString folder;
    while (true) {
        Explorer* parent = qobject_cast<Explorer*>(parent_widget);
        if (parent)
            emit parent->redirect_stdio(false);
        if (directory.isEmpty())
            folder = QFileDialog::getExistingDirectory(this, "Select directory",
                                                       orig);
        else
            folder = directory;
        if (parent)
            emit parent->redirect_stdio(true);
        if (!folder.isEmpty()) {
            folder = fixpath(folder);
            if (folder != orig)
                break;
        }
        else
            return;
    }
    foreach (QString fname, fnames) {
        QFileInfo fileinfo(fname);
        QString basename = fileinfo.fileName();
        try {
            misc::move_file(fname, folder+'/'+basename);
        } catch (std::exception error) {
            QMessageBox::critical(this, "Error",
                                  QString("<b>Unable to move <i>%1</i></b>"
                                          "<br><br>Error message:<br>%2")
                                  .arg(basename).arg(error.what()));
        }
    }
}

QString DirView::create_new_folder(QString current_path, const QString &title,
                                   const QString &subtitle, bool is_package)
{
    if (current_path.isNull())
        current_path = "";
    QFileInfo info(current_path);
    if (info.isFile())
        current_path = info.absolutePath();
    bool valid;
    QString name = QInputDialog::getText(this, title, subtitle,
                                         QLineEdit::Normal, "", &valid);
    if (valid) {
        QString dirname = current_path + '/' + name;
        QDir dir(current_path);
        if (dir.mkdir(name) == false) {
            QMessageBox::critical(this, title,
                                  QString("<b>Unable to create folder <i>%1</i></b>"
                                          "<br><br>Error message:<br>").arg(dirname));
        }
        if (is_package) {
            QString fname = dirname + '/' + "__init__.py";
            QFile file(fname);
            bool ok = file.open(QIODevice::WriteOnly);
            if (ok) {
                file.write("#");
                file.close();
                return dirname;
            }
            else {
                QMessageBox::critical(this, title,
                                      QString("<b>Unable to create file <i>%1</i></b>"
                                              "<br><br>Error message:<br>").arg(fname));
            }
        }
    }
    return QString();
}

void DirView::new_folder(const QString &basedir)
{
    QString title = "New folder";
    QString subtitle = "Folder name:";
    this->create_new_folder(basedir, title, subtitle, false);
}

void DirView::new_package(const QString &basedir)
{
    QString title = "New package";
    QString subtitle = "Package name:";
    this->create_new_folder(basedir, title, subtitle, true);
}

QString DirView::create_new_file(QString current_path, const QString &title, const QString &filters, std::function<void(QString)> create_func)
{
    if (current_path.isNull())
        current_path = "";
    QFileInfo info(current_path);
    if (info.isFile())
        current_path = info.absolutePath();
    Explorer* parent = qobject_cast<Explorer*>(parent_widget);
    if (parent)
        emit parent->redirect_stdio(false);
    QString fname = QFileDialog::getSaveFileName(this,title,current_path,filters);
    if (parent)
        emit parent->redirect_stdio(true);
    if (!fname.isEmpty()) {
        try {
            create_func(fname);
            return fname;
        } catch (std::exception error) {
            QMessageBox::critical(this, "New file",
                                  QString("<b>Unable to create file <i>%1</i>"
                                          "</b><br><br>Error message:<br>%2")
                                  .arg(fname).arg(error.what()));
        }
    }
    return QString();
}

void DirView::new_file(const QString &basedir)
{
    QString title = "New file";
    QString filters = "All files (*)";

    std::function<void(QString)> create_func = [](QString fname)
    {QFileInfo info(fname);
    QStringList suffixes = {"py", "pyw", "ipy"};
    if (suffixes.contains(info.suffix()))
        create_script(fname);
    else {
        QFile file(fname);
        file.open(QIODevice::WriteOnly);
        file.write("");
        file.close();
    }};

    QString fname = create_new_file(basedir, title, filters, create_func);
    if (!fname.isEmpty()) {
        this->open(QStringList(fname));
    }
}

void DirView::new_module(QString basedir)
{
    QString title = "New module";
    QString filters = "Python scripts (*.py *.pyw *.ipy)";

    std::function<void(QString)> create_func = [this](QString fname)
    {Explorer* parent = qobject_cast<Explorer*>(this->parent_widget);
    if (parent)
        emit parent->create_module(fname);};

    this->create_new_file(basedir, title, filters, create_func);
}

// vcs_command(self, fnames, action)

QPoint DirView::get_scrollbar_position() const
{
    return QPoint(this->horizontalScrollBar()->value(),
                  this->verticalScrollBar()->value());
}

void DirView::set_scrollbar_position(const QPoint &position)
{
    this->_scrollbar_positions = position;
    if (_to_be_loaded.size() == 0)
        this->restore_scrollbar_positions();
}

void DirView::restore_scrollbar_positions()
{
    int hor = _scrollbar_positions.x();
    int ver = _scrollbar_positions.y();
    this->horizontalScrollBar()->setValue(hor);
    this->verticalScrollBar()->setValue(ver);
}

QStringList DirView::get_expanded_state()
{
    this->save_expanded_state();
    return __expanded_state;
}

void DirView::set_expanded_state(const QStringList &state)
{
    __expanded_state = state;
    this->restore_expanded_state();
}

void DirView::save_expanded_state()
{
    QAbstractItemModel* model = this->model();
    // friend关键字 友元类不是相互的，所以这种方法不行
    // 要不弄一个继承QFileSystemModel,把persistentIndexList()改成public的,然后setModel
    //persistentIndexList()是QAbstractItemModel的protected成员函数，
    //而该类没有继承自QAbstractItemModel，无法调用
    /*if (model) {
        __expanded_state.clear();
        foreach (QModelIndex idx, model->persistentIndexList()) {
            if (this->isExpanded(idx));
            __expanded_state.append(get_filename(idx));
        }
    }*/
}

void DirView::restore_directory_state(const QString &fname)
{
    QFileInfo info(fname);
    QString root = info.canonicalFilePath();
    if (!QFileInfo::exists(root))
        return;
    QDir dir(root);
    dir.setFilter(QDir::Files | QDir::Dirs);
    foreach (QFileInfo info, dir.entryInfoList()) {
        QString path = info.absoluteFilePath();
        if (info.isDir() && __expanded_state.contains(path)) {
            __expanded_state.removeOne(path);
            _to_be_loaded.append(path);
            this->setExpanded(this->get_index(path), true);
        }
    }
    if (__expanded_state.isEmpty())
        disconnect(fsmodel, SIGNAL(directoryLoaded(QString)),
                   this,SLOT(restore_directory_state(QString)));
}

void DirView::follow_directories_loaded(const QString &fname)
{
    if (_to_be_loaded.isEmpty())
        return;
    QFileInfo info(fname);
    QString path = info.canonicalFilePath();
    if (_to_be_loaded.contains(path))
        _to_be_loaded.removeOne(path);
    if (_to_be_loaded.size() == 0) {
        disconnect(fsmodel, SIGNAL(directoryLoaded(QString)),
                   this,SLOT(restore_directory_state(QString)));
        if (_scrollbar_positions != QPoint())
            QTimer::singleShot(50,this,SLOT(restore_scrollbar_positions()));
    }
}

void DirView::restore_expanded_state()
{
    if (!__expanded_state.isEmpty()) {
        connect(fsmodel, SIGNAL(directoryLoaded(QString)),
                           this,SLOT(restore_directory_state(QString)));
        connect(fsmodel, SIGNAL(directoryLoaded(QString)),
                           this,SLOT(follow_directories_loaded(QString)));
    }
}

void DirView::filter_directories()
{
    QModelIndex index = this->get_index(".spyproject");
    if (index.isValid())
        this->setRowHidden(index.row(), index.parent(), true);
}


/********** ProxyModel **********/
ProxyModel::ProxyModel(QObject* parent)
    : QSortFilterProxyModel (parent)
{
    this->root_path = QString();
    this->path_list = QStringList();
    this->setDynamicSortFilter(true);
}

void ProxyModel::setup_filter(const QString &root_path, const QStringList &path_list)
{
    QFileInfo info(root_path);
    this->root_path = info.canonicalFilePath();
    this->path_list.clear();
    foreach (QString p,path_list) {
        QFileInfo fileinfo(p);
        this->path_list.append(fileinfo.canonicalFilePath());
    }
    this->invalidateFilter();
}

void ProxyModel::sort(int column, Qt::SortOrder order)
{
    this->sourceModel()->sort(column, order);
}

bool ProxyModel::filterAcceptsRow(int row, const QModelIndex &parent_index) const
{
    if (root_path.isEmpty())
        return true;
    QModelIndex index = this->sourceModel()->index(row, 0, parent_index);
    QFileSystemModel* model = qobject_cast<QFileSystemModel*>(this->sourceModel());
    QFileInfo info(model->filePath(index));
    QString path = info.canonicalFilePath();
    if (root_path.startsWith(path))
        return true;
    else {
        foreach (QString p,path_list) {
            //sep = "\\";altsep = "/";
            if (path == p || path.startsWith(p+os::altsep))
                return true;
        }
        return false;
    }
}

QVariant ProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ToolTipRole) {
        Q_ASSERT(path_list.size() > 0);
        //sep = "\\";altsep = "/";
        QString root_dir = path_list[0].split(os::altsep).last();
        if (index.data().toString() == root_dir)
            return root_path + '/' + root_dir;
    }
    return QSortFilterProxyModel::data(index, role);
}


/********** FilteredDirView **********/
FilteredDirView::FilteredDirView(QWidget* parent)
    : DirView (parent)
{
    this->proxymodel = nullptr;
    this->setup_proxy_model();
    this->root_path = QString();
}

void FilteredDirView::setup_proxy_model()
{
    proxymodel = new ProxyModel(this);
    proxymodel->setSourceModel(fsmodel);
}

void FilteredDirView::install_model()
{
    if (!root_path.isEmpty())
        this->setModel(proxymodel);
}

void FilteredDirView::set_root_path(const QString &root_path)
{
    this->root_path = root_path;
    this->install_model();
    QModelIndex index = fsmodel->setRootPath(root_path);
    this->setRootIndex(proxymodel->mapFromSource(index));
}

QModelIndex FilteredDirView::get_index(const QString &filename) const
{
    QModelIndex index = fsmodel->index(filename);
    if (index.isValid() && index.model() == fsmodel)
        return proxymodel->mapFromSource(index);
    return QModelIndex();
}

void FilteredDirView::set_folder_names(const QStringList &folder_names)
{
    QStringList path_list;
    foreach (QString dirname, folder_names) {
        path_list.append(root_path + '/' + dirname);
    }
    proxymodel->setup_filter(root_path, path_list);
}

QString FilteredDirView::get_filename(const QModelIndex &index) const
{
    if (index.isValid()) {
        QString path = fsmodel->filePath(proxymodel->mapToSource(index));
        QFileInfo info(path);
        return info.canonicalFilePath();
    }
    return QString();
}

void FilteredDirView::setup_project_view()
{
    for (int i=1;i<=3;i++)
        this->hideColumn(i);
    this->setHeaderHidden(true);
    this->filter_directories();
}


/********** ExplorerTreeWidget **********/
ExplorerTreeWidget::ExplorerTreeWidget(QWidget* parent,int show_cd_only)
    : DirView (parent)
{
    this->history = QStringList();
    this->histindex = -1;

    this->show_cd_only = show_cd_only;
    this->__original_root_index = QModelIndex();
    this->__last_folder = QString();

    this->menu = nullptr;
    this->common_actions = QList<QObject*>();

    this->setDragEnabled(true);
}

QList<QObject*> ExplorerTreeWidget::setup_common_actions()
{
    QList<QObject*> actions = DirView::setup_common_actions();
    if (show_cd_only == -1)
        show_cd_only = 1;
    else {
        QAction* cd_only_action = new QAction("Show current directory only", this);
        connect(cd_only_action,SIGNAL(toggled(bool)),this,SLOT(toggle_show_cd_only(bool)));
        cd_only_action->setCheckable(true);
        cd_only_action->setChecked(show_cd_only);
        cd_only_action->setShortcutContext(Qt::WindowShortcut);

        if (show_cd_only == 1)
            this->toggle_show_cd_only(true);
        else if (show_cd_only == 0)
            this->toggle_show_cd_only(false);
        actions.append(cd_only_action);
    }
    return actions;
}

//@Slot(bool)
void ExplorerTreeWidget::toggle_show_cd_only(bool checked)
{
    ExplorerWidget* widget1 = qobject_cast<ExplorerWidget*>(parent_widget);
    ProjectExplorerWidget* widget2 = qobject_cast<ProjectExplorerWidget*>(parent_widget);
    if (widget1)
        emit widget1->sig_option_changed("show_cd_only", checked);
    else if (widget2)
        emit widget2->sig_option_changed("show_cd_only", checked);
    if (checked)
        show_cd_only = 1;
    else
        show_cd_only = 0;
    if (checked) {
        if (!__last_folder.isEmpty())
            this->set_current_folder(__last_folder);
    }
    else if (__original_root_index.isValid())
        this->setRootIndex(__original_root_index);
}

QModelIndex ExplorerTreeWidget::set_current_folder(const QString &folder)
{
    QModelIndex index = fsmodel->setRootPath(folder);
    this->__last_folder = folder;
    if (show_cd_only == 1) {
        if (!__original_root_index.isValid())
            __original_root_index = this->rootIndex();
        this->setRootIndex(index);
    }
    return index;
}

QString ExplorerTreeWidget::get_current_folder() const
{
    return this->__last_folder;
}

void ExplorerTreeWidget::refresh(QString new_path, bool force_current)
{
    if (new_path.isEmpty())
        new_path = misc::getcwd_or_home();
    if (force_current) {
        QModelIndex index = this->set_current_folder(new_path);
        this->expand(index);
        this->setCurrentIndex(index);
    }
    emit set_previous_enabled(histindex > 0);
    emit set_next_enabled(histindex > -1 && histindex < history.size()-1);
    this->filter_directories();
}

void ExplorerTreeWidget::directory_clicked(const QString &dirname)
{
    this->chdir(dirname);
}

void ExplorerTreeWidget::go_to_parent_directory()
{
    QString path = misc::getcwd_or_home() + "/" + os::pardir;//上一层目录
    QFileInfo info(path);
    this->chdir(info.absoluteFilePath());
}

void ExplorerTreeWidget::go_to_previous_directory()
{
    if (this->histindex >= 0)
        this->histindex -= 1;
    this->chdir(QString(), true);
}

void ExplorerTreeWidget::go_to_next_directory()
{
    this->histindex += 1;
    this->chdir(QString(), true);
}

void ExplorerTreeWidget::update_history(QString directory)
{
    try {
        QFileInfo info(directory);
        directory = info.absoluteFilePath();
        if (history.contains(directory))
            histindex = history.indexOf(directory);
    } catch (...) {
        QString user_directory = get_home_dir();
        this->chdir(user_directory, true);
    }
}

void ExplorerTreeWidget::chdir(QString directory, bool browsing_history)
{
    //这里的传入的directory是相对路径
    if (!directory.isEmpty()) {
        QFileInfo info(directory);
        directory = info.absoluteFilePath();
    }
    if (browsing_history) {
        if (histindex >= 0 && histindex < history.size())
            directory = history[histindex];
    }
    else if (history.contains(directory))
        histindex = history.indexOf(directory);
    else {
        if (histindex == -1)
            history.clear();
        else {
            this->history = this->history.mid(0, this->histindex+1);
        }
        if (history.size() == 0 ||
                (!history.isEmpty() && history.last() != directory))
            history.append(directory);
        histindex = history.size()-1;
    }

    //QDir::setCurrent设置Qt应用程序工作目录，没有意义，应该cmd cd到directory
    //bool ok = QDir::setCurrent(directory);
    bool ok = true;
    if (ok) {
        ExplorerWidget* widget1 = qobject_cast<ExplorerWidget*>(parent_widget);
        //ProjectExplorerWidget* widget2 = qobject_cast<ProjectExplorerWidget*>(parent_widget);
        if (widget1)
            emit widget1->open_dir(directory);
        this->refresh(directory, true);
    }
    else {
        QMessageBox::critical(parent_widget, "Error",
                              "You don't have the right permissions to "
                              "open this directory");
    }
}


/********** ExplorerWidget **********/
ExplorerWidget::ExplorerWidget(QWidget* parnet, const QStringList& name_filters,
               bool show_all, int show_cd_only, bool show_icontext)
    : QWidget (parnet)
{
    this->treewidget = new ExplorerTreeWidget(this, show_cd_only);
    QToolButton* button_previous = new QToolButton(this);
    QToolButton* button_next = new QToolButton(this);
    QToolButton* button_parent = new QToolButton(this);
    this->button_menu = new QToolButton(this);
    QMenu* menu = new QMenu(this);

    this->action_widgets = QList<QToolButton*>();
    action_widgets << button_previous << button_next << button_parent
                   << button_menu;

    QAction* icontext_action = new QAction("Show icons and text", this);
    connect(icontext_action,SIGNAL(toggled(bool)),this,SLOT(toggle_icontext(bool)));
    icontext_action->setCheckable(true);
    icontext_action->setShortcutContext(Qt::WindowShortcut);

    QAction* previous_action = new QAction("Previous", this);
    previous_action->setIcon(ima::icon("ArrowBack"));
    connect(previous_action,SIGNAL(triggered(bool)),treewidget,SLOT(go_to_previous_directory()));
    previous_action->setShortcutContext(Qt::WindowShortcut);

    QAction* next_action = new QAction("Next", this);
    next_action->setIcon(ima::icon("ArrowForward"));
    connect(next_action,SIGNAL(triggered(bool)),treewidget,SLOT(go_to_next_directory()));
    next_action->setShortcutContext(Qt::WindowShortcut);

    QAction* parent_action = new QAction("Parent", this);
    parent_action->setIcon(ima::icon("ArrowUp"));
    connect(parent_action,SIGNAL(triggered(bool)),treewidget,SLOT(go_to_parent_directory()));
    parent_action->setShortcutContext(Qt::WindowShortcut);

    QAction* options_action = new QAction("", this);
    options_action->setToolTip("Options");
    options_action->setShortcutContext(Qt::WindowShortcut);

    treewidget->setup(name_filters, show_all);
    treewidget->chdir(misc::getcwd_or_home());
    treewidget->common_actions.append(nullptr);
    treewidget->common_actions.append(icontext_action);

    button_previous->setDefaultAction(previous_action);
    previous_action->setEnabled(false);

    button_next->setDefaultAction(next_action);
    next_action->setEnabled(false);

    button_parent->setDefaultAction(parent_action);

    button_menu->setIcon(ima::icon("tooloptions"));
    button_menu->setPopupMode(QToolButton::InstantPopup);
    button_menu->setMenu(menu);
    add_actions(menu, treewidget->common_actions);
    options_action->setMenu(menu);

    this->toggle_icontext(show_icontext);
    icontext_action->setChecked(show_icontext);

    foreach (QToolButton* widget, action_widgets) {
        widget->setAutoRaise(true);
        widget->setIconSize(QSize(16, 16));
    }

    QHBoxLayout* blayout = new QHBoxLayout;
    blayout->addWidget(button_previous);
    blayout->addWidget(button_next);
    blayout->addWidget(button_parent);
    blayout->addStretch();
    blayout->addWidget(button_menu);

    QVBoxLayout* layout = create_plugin_layout(blayout, treewidget);
    this->setLayout(layout);

    connect(treewidget,SIGNAL(set_previous_enabled(bool)),previous_action,SLOT(setEnabled(bool)));
    connect(treewidget,SIGNAL(set_next_enabled(bool)),next_action,SLOT(setEnabled(bool)));
}

void ExplorerWidget::toggle_icontext(bool state)
{
    emit sig_option_changed("show_icontext", state);
    foreach (QToolButton* widget, action_widgets) {
        if (widget != button_menu) {
            if (state)
                widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            else
                widget->setToolButtonStyle(Qt::ToolButtonIconOnly);
        }
    }
}


/********** Tests **********/
FileExplorerTest::FileExplorerTest()
    : QWidget ()
{
    QVBoxLayout* vlayout = new QVBoxLayout;
    this->setLayout(vlayout);
    this->explorer = new ExplorerWidget(this,{"*.py", "*.pyw"},
                                  false,-1);
    vlayout->addWidget(explorer);

    QHBoxLayout* hlayout1 = new QHBoxLayout;
    vlayout->addLayout(hlayout1);
    QLabel* label = new QLabel("<b>Open file:</b>");
    label->setAlignment(Qt::AlignRight);
    hlayout1->addWidget(label);
    this->label1 = new QLabel;
    hlayout1->addWidget(label1);
    connect(explorer,SIGNAL(sig_open_file(QString)),label1,SLOT(setText(QString)));

    QHBoxLayout* hlayout2 = new QHBoxLayout;
    vlayout->addLayout(hlayout2);
    label = new QLabel("<b>Open dir:</b>");
    label->setAlignment(Qt::AlignRight);
    hlayout2->addWidget(label);
    this->label2 = new QLabel;
    hlayout2->addWidget(label2);
    connect(explorer,SIGNAL(open_dir(QString)),label2,SLOT(setText(QString)));

    QHBoxLayout* hlayout3 = new QHBoxLayout;
    vlayout->addLayout(hlayout3);
    label = new QLabel("<b>Option changed:</b>");
    label->setAlignment(Qt::AlignRight);
    hlayout3->addWidget(label);
    this->label3 = new QLabel;
    hlayout3->addWidget(label3);

    connect(explorer, &ExplorerWidget::sig_option_changed,
            [this](QString x,QVariant y){y.toBool() ? label3->setText(QString("option_changed: %1, True").arg(x))
                                    : label3->setText(QString("option_changed: %1, False").arg(x));});
    connect(explorer,&ExplorerWidget::open_dir,
            [this](){explorer->treewidget->refresh("..");});
}

ProjectExplorerTest1::ProjectExplorerTest1(QWidget* parent)
    : QWidget (parent)
{
    QVBoxLayout* vlayout = new QVBoxLayout;
    this->setLayout(vlayout);
    this->treewidget = new FilteredDirView(this);
    this->treewidget->setup_view();
    this->treewidget->set_root_path("F:/MyPython/spyder/widgets");
    this->treewidget->set_folder_names(QStringList("variableexplorer"));
    this->treewidget->setup_project_view();
    vlayout->addWidget(this->treewidget);
}

static void test()
{
    //通过测试，未发现bug
    FileExplorerTest test;
    test.resize(640, 480);
    test.show();

    ProjectExplorerTest1 test2;
    test2.resize(640, 480);
    test2.show();
}
