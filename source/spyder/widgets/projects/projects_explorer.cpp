#include "projects_explorer.h"

ProjectsExplorerTreeWidget::ProjectsExplorerTreeWidget(QWidget* parent, bool show_hscrollbar)
    : FilteredDirView (parent)
{
    this->last_folder = QString();
    this->setSelectionMode(FilteredDirView::ExtendedSelection);
    this->show_hscrollbar = show_hscrollbar;

    this->setDragEnabled(true);
    this->setDragDropMode(FilteredDirView::DragDrop);
}

QList<QObject*> ProjectsExplorerTreeWidget::setup_common_actions()
{
    QList<QObject*> actions = FilteredDirView::setup_common_actions();

    QAction* hscrollbar_action = new QAction("Show horizontal scrollbar", this);
    connect(hscrollbar_action, SIGNAL(toggled(bool)), this, SLOT(toggle_hscrollbar(bool)));
    hscrollbar_action->setChecked(this->show_hscrollbar);
    this->toggle_hscrollbar(this->show_hscrollbar);
    actions.append(hscrollbar_action);

    return actions;
}

void ProjectsExplorerTreeWidget::toggle_hscrollbar(bool checked)
{
    ProjectExplorerWidget* widget = qobject_cast<ProjectExplorerWidget*>(parent_widget);
    if (widget)
        emit widget->sig_option_changed("show_hscrollbar", checked);
    this->show_hscrollbar = checked;
    this->header()->setStretchLastSection(!checked);
    this->header()->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void ProjectsExplorerTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QModelIndex index = this->indexAt(event->pos());
    if (index.isValid()) {
        QString dst = this->get_filename(index);
        QFileInfo info(dst);
        if (info.isDir())
            event->acceptProposedAction();
        else
            event->ignore();
    }
    else
        event->ignore();
}

void ProjectsExplorerTreeWidget::dropEvent(QDropEvent *event)
{
    event->ignore();
    Qt::DropAction action = event->dropAction();
    if (action != Qt::MoveAction && action != Qt::CopyAction)
        return;

    QString dst = this->get_filename(this->indexAt(event->pos()));
    int yes_to_all=-1, no_to_all=-1;
    QStringList src_list;
    foreach (QUrl url, event->mimeData()->urls())
        src_list.append(url.toString());
    QMessageBox::StandardButtons buttons;
    if (src_list.size() > 1)
        buttons = QMessageBox::Yes | QMessageBox::YesAll |
                  QMessageBox::No | QMessageBox::NoAll | QMessageBox::Cancel;
    else
        buttons = QMessageBox::Yes | QMessageBox::No;
    foreach (QString src, src_list) {
        if (src == dst)
            continue;
        QFileInfo info(src);
        QString dst_fname = dst + '/' + info.fileName();
        if (QFileInfo::exists(dst_fname)) {
            if (yes_to_all != -1 || no_to_all != -1) {
                if (no_to_all == 1)
                    continue;
            }
            else if (info.isFile()) {
                QMessageBox::StandardButton answer = QMessageBox::warning(this, "Project explorer",
                                     QString("File <b>%1</b> already exists.<br>"
                                             "Do you want to overwrite it?").arg(dst_fname),
                                     buttons);
                if (answer == QMessageBox::No)
                    continue;
                else if (answer == QMessageBox::Cancel)
                    break;
                else if (answer == QMessageBox::YesAll)
                    yes_to_all = 1;
                else if (answer == QMessageBox::NoAll) {
                    no_to_all = 1;
                    continue;
                }
            }
            else {
                QMessageBox::critical(this, "Project explorer",
                                      QString("Folder <b>%1</b> already exists.")
                                      .arg(dst_fname));
                event->setDropAction(Qt::CopyAction);
                return;
            }
        }
        try {
            if (action == Qt::CopyAction) {
                if (info.isFile())
                    QFile::copy(src, dst);
                else
                    QFile::copy(src, dst);//shutil.copytree(src, dst) TODO
            }
            else {
                if (info.isFile())
                    misc::move_file(src, dst);
                else {
                    QFile::copy(src, dst);
                    QFile::remove(src);
                }
                //    TODO
                //emit this->parent_widget->removed(src);
            }
        } catch (const std::exception& error) {
            QString action_str;
            if (action == Qt::CopyAction)
                action_str = "copy";
            else
                action_str = "move";
            QMessageBox::critical(this, "Project Explorer",
                                  QString("<b>Unable to %1 <i>%2</i></b>"
                                          "<br><br>Error message:<br>%3")
                                  .arg(action_str).arg(src).arg(error.what()));
        }
    }
}

void ProjectsExplorerTreeWidget::_delete(QStringList fnames)
{
    if (fnames.isEmpty())
        fnames = this->get_selected_filenames();
    bool multiple = fnames.size() > 1;
    int yes_to_all = -1;
    foreach (QString fname, fnames) {
        Q_ASSERT(this->proxymodel->path_list.size() > 0);
        if (fname == this->proxymodel->path_list[0])
            emit this->sig_delete_project();
        else {
            yes_to_all = this->delete_file(fname, multiple, yes_to_all);
            if (yes_to_all == 0)
                break;
        }
    }
}


/********** ProjectExplorerWidget **********/
ProjectExplorerWidget::ProjectExplorerWidget(QWidget* parent, const QStringList& name_filters,
                                             bool show_all, bool show_hscrollbar)
    : QWidget (parent)
{
    this->treewidget = nullptr;
    this->emptywidget = nullptr;
    this->name_filters = name_filters;
    this->show_all = show_all;
    this->show_hscrollbar = show_hscrollbar;
    this->setup_layout();
}

void ProjectExplorerWidget::setup_layout()
{
    this->emptywidget = new ProjectsExplorerTreeWidget(this);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(this->emptywidget);
    this->setLayout(layout);
}

void ProjectExplorerWidget::set_project_dir(const QString &directory)
{
    if (!directory.isEmpty()) {
        //sep = "\\";altsep = "/";
        QString project = directory.split(os::altsep).last();
        QFileInfo info(directory);
        this->treewidget->set_root_path(info.absolutePath());
        this->treewidget->set_folder_names(QStringList(project));
    }
    this->treewidget->setup_project_view();
    try {
        this->treewidget->setExpanded(this->treewidget->get_index(directory), true);
    } catch (...) {
    }
}

void ProjectExplorerWidget::clear()
{
    this->treewidget->hide();
    this->emptywidget->show();
}

void ProjectExplorerWidget::setup_project(const QString &directory)
{
    if (this->treewidget != nullptr)
        this->treewidget->hide();

    this->treewidget = new ProjectsExplorerTreeWidget(this, this->show_hscrollbar);
    this->treewidget->setup(this->name_filters, this->show_all);
    this->treewidget->setup_view();

    this->emptywidget->hide();
    this->treewidget->show();
    this->layout()->addWidget(this->treewidget);//动态添加控件

    this->set_project_dir(directory);
    connect(this->treewidget, SIGNAL(sig_delete_project()), this, SLOT(delete_project()));
}

void ProjectExplorerWidget::delete_project()
{
    // current_active_project属性在plugins/projects
    //path = self.current_active_project.root_path
}


/********** ProjectExplorerTest **********/
ProjectExplorerTest::ProjectExplorerTest(const QString& directory)
    : QWidget ()
{
    QVBoxLayout* vlayout = new QVBoxLayout;
    this->setLayout(vlayout);

    this->explorer = new ProjectExplorerWidget(this);
    if (!directory.isEmpty())
        this->directory = directory;
    else {
        QFileInfo info("F:/MyPython/spyder/widgets/projects/explorer.py");
        this->directory = info.absolutePath();
    }
    this->explorer->setup_project(this->directory);
    vlayout->addWidget(this->explorer);

    QHBoxLayout* hlayout1 = new QHBoxLayout;
    vlayout->addLayout(hlayout1);
    QLabel* label = new QLabel("<b>Open file:</b>");
    label->setAlignment(Qt::AlignRight);
    hlayout1->addWidget(label);
    QLabel* label1 = new QLabel;
    hlayout1->addWidget(label1);
    connect(explorer, SIGNAL(sig_open_file(QString)), label1, SLOT(setText(QString)));

    QHBoxLayout* hlayout3 = new QHBoxLayout;
    vlayout->addLayout(hlayout3);
    label = new QLabel("<b>Option changed:</b>");
    label->setAlignment(Qt::AlignRight);
    hlayout3->addWidget(label);
    QLabel* label3= new QLabel;
    hlayout3->addWidget(label3);
    connect(explorer, &ProjectExplorerWidget::sig_option_changed,
            [=](QString x,QVariant y){y.toBool() ? label3->setText(QString("option_changed: %1, True").arg(x))
                                    : label3->setText(QString("option_changed: %1, False").arg(x));});
}

static void test()
{
    //通过测试，未发现bug
    ProjectExplorerTest test;
    test.resize(250, 480);
    test.show();
}
