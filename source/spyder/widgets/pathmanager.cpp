#include "pathmanager.h"

PathManager::PathManager(QWidget* parent, QStringList pathlist, QStringList ro_pathlist,
                         QStringList not_active_pathlist, bool sync)
    : QDialog (parent)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->pathlist = pathlist;
    this->ro_pathlist = ro_pathlist;
    this->not_active_pathlist = not_active_pathlist;

    this->last_path = misc::getcwd_or_home();

    this->setWindowTitle("PYTHONPATH manager");
    this->setWindowIcon(ima::icon("pythonpath"));
    this->resize(500, 300);

    QVBoxLayout* layout = new QVBoxLayout;
    this->setLayout(layout);

    QHBoxLayout* top_layout = new QHBoxLayout;
    layout->addLayout(top_layout);
    this->toolbar_widgets1 = this->setup_top_toolbar(top_layout);

    this->listwidget = new QListWidget(this);
    connect(listwidget, SIGNAL(currentRowChanged(int )), this, SLOT(refresh(int)));
    connect(listwidget, SIGNAL(itemChanged(QListWidgetItem *)),
            this, SLOT(update_not_active_pathlist(QListWidgetItem *)));
    layout->addWidget(this->listwidget);

    QHBoxLayout* bottom_layout = new QHBoxLayout;
    layout->addLayout(bottom_layout);
    this->sync_button = nullptr;
    this->toolbar_widgets2 = this->setup_bottom_toolbar(bottom_layout, sync);

    QDialogButtonBox* bbox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
    bottom_layout->addWidget(bbox);

    this->update_list();
    this->refresh();
}

QStringList PathManager::active_pathlist() const
{
    QStringList list;
    foreach (QString path, this->pathlist) {
        if (! this->not_active_pathlist.contains(path))
            list.append(path);
    }
    return list;
}

void PathManager::_add_widgets_to_layout(QBoxLayout *layout, const QList<QToolButton *> &widgets)
{
    layout->setAlignment(Qt::AlignLeft);
    foreach (QToolButton* widget, widgets) {
        layout->addWidget(widget);
    }
}

QList<QToolButton*> PathManager::setup_top_toolbar(QBoxLayout* layout)
{
    QList<QToolButton*> toolbar;

    QToolButton* movetop_button = new QToolButton(this);
    movetop_button->setText("Move to top");
    movetop_button->setIcon(ima::icon("2uparrow"));
    connect(movetop_button, &QAbstractButton::clicked,
            [=](){this->move_to(0);});
    movetop_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QToolButton* moveup_button = new QToolButton(this);
    moveup_button->setText("Move up");
    moveup_button->setIcon(ima::icon("1uparrow"));
    connect(moveup_button, &QAbstractButton::clicked,
            [=](){this->move_to();});
    moveup_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QToolButton* movedown_button = new QToolButton(this);
    movedown_button->setText("Move down");
    movedown_button->setIcon(ima::icon("1downarrow"));
    connect(movedown_button, &QAbstractButton::clicked,
            [=](){this->move_to(-1, 1);});
    movedown_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QToolButton* movebottom_button = new QToolButton(this);
    movebottom_button->setText("Move to bottom");
    movebottom_button->setIcon(ima::icon("2downarrow"));
    connect(movebottom_button, &QAbstractButton::clicked,
            [=](){this->move_to(1);});
    movebottom_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolbar << movetop_button << moveup_button << movedown_button << movebottom_button;
    this->selection_widgets.append(toolbar);
    this->_add_widgets_to_layout(layout, toolbar);
    return toolbar;
}

QList<QToolButton*> PathManager::setup_bottom_toolbar(QBoxLayout* layout, bool sync)
{
    QList<QToolButton*> toolbar;

    QToolButton* add_button = new QToolButton(this);
    add_button->setText("Add path");
    add_button->setIcon(ima::icon("edit_add"));
    connect(add_button, &QAbstractButton::clicked,
            this, &PathManager::add_path);
    add_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QToolButton* remove_button = new QToolButton(this);
    remove_button->setText("Remove path");
    remove_button->setIcon(ima::icon("edit_remove"));
    connect(remove_button, &QAbstractButton::clicked,
            this, &PathManager::remove_path);
    remove_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolbar << add_button << remove_button;
    this->selection_widgets.append(remove_button);
    this->_add_widgets_to_layout(layout, toolbar);
    layout->addStretch(1);
    if (os::name == "nt" && sync) {
        this->sync_button = new QToolButton(this);
        this->sync_button->setText("Synchronize...");
        this->sync_button->setIcon(ima::icon("fileimport"));
        connect(sync_button, &QAbstractButton::clicked,
                this, &PathManager::synchronize);
        this->sync_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        layout->addWidget(this->sync_button);
    }
    return toolbar;
}

void PathManager::synchronize()
{
    QMessageBox::question(this, "Synchronize",
                          "This will synchronize Spyder's path list with "
                          "<b>PYTHONPATH</b> environment variable for current user, "
                          "allowing you to run your Python modules outside Spyder "
                          "without having to configure sys.path. "
                          "<br>Do you want to clear contents of PYTHONPATH before "
                          "adding Spyder's path list?",
                          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    return;
    //该槽函数并未实现真正功能，需要对winreg.h(Windows注册表)头文件中的函数比较熟悉
}

QStringList PathManager::get_path_list() const
{
    return this->pathlist;
}

void PathManager::update_not_active_pathlist(QListWidgetItem *item)
{
    QString path = item->text();
    if (item->checkState() != Qt::Unchecked)
        this->remove_from_not_active_pathlist(path);
    else
        this->add_to_not_active_pathlist(path);
}

void PathManager::add_to_not_active_pathlist(const QString &path)
{
    if (! this->not_active_pathlist.contains(path))
        this->not_active_pathlist.append(path);
}

void PathManager::remove_from_not_active_pathlist(const QString &path)
{
    if (this->not_active_pathlist.contains(path))
        this->not_active_pathlist.removeAll(path);
}

void PathManager::update_list()
{
    this->listwidget->clear();
    QStringList tmp = this->pathlist + this->ro_pathlist;
    foreach (QString name, tmp) {
        QListWidgetItem* item = new QListWidgetItem(name);
        item->setIcon(ima::icon("DirClosedIcon"));
        if (this->ro_pathlist.contains(name)) {
            item->setFlags(Qt::NoItemFlags | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        }
        else if (this->not_active_pathlist.contains(name)) {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
        }
        else {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        }
        this->listwidget->addItem(item);
    }
    this->refresh();
}

void PathManager::refresh(int row)
{
    Q_UNUSED(row);
    foreach (QToolButton* widget, this->selection_widgets) {
        widget->setEnabled(this->listwidget->currentItem() != nullptr);
    }
    bool not_empty = this->listwidget->count() > 0;
    if (this->sync_button != nullptr)
        this->sync_button->setEnabled(not_empty);
}

void PathManager::move_to(int absolute, int relative)
{
    int index = this->listwidget->currentRow();
    int new_index;
    if (absolute != -1) {
        if (absolute)
            new_index = this->pathlist.size()-1;
        else
            new_index = 0;
    }
    else
        new_index = index + relative;
    new_index = std::max(0, std::min(this->pathlist.size()-1, new_index));
    QString path = this->pathlist.takeAt(index);
    this->pathlist.insert(new_index, path);
    this->update_list();
    this->listwidget->setCurrentRow(new_index);
}

void PathManager::remove_path()
{
    QMessageBox::StandardButton answer = QMessageBox::warning(this, "Remove path",
                                                              "Do you really want to remove selected path?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::Yes) {
        this->pathlist.removeAt(this->listwidget->currentRow());
        this->remove_from_not_active_pathlist(this->listwidget->currentItem()->text());
        this->update_list();
    }
}

void PathManager::add_path()
{
    emit redirect_stdio(false);
    QString directory = QFileDialog::getExistingDirectory(this, "Select directory",
                                                          this->last_path);
    emit redirect_stdio(true);
    if (!directory.isEmpty()) {
        QFileInfo info(directory);
        directory = info.absoluteFilePath();
        this->last_path = directory;
        if (this->pathlist.contains(directory)) {
            QListWidgetItem* item = this->listwidget->findItems(directory, Qt::MatchExactly)[0];
            item->setCheckState(Qt::Checked);
            QMessageBox::StandardButton answer = QMessageBox::question(this, "Add path",
                                                                       "This directory is already included in Spyder "
                                                                       "path list.<br>Do you want to move it to the "
                                                                       "top of the list?",
                                                                       QMessageBox::Yes | QMessageBox::No);
            if (answer == QMessageBox::Yes)
                this->pathlist.removeAll(directory);
            else
                return;
        }
        this->pathlist.insert(0, directory);
        this->update_list();
    }
}


static void test()
{
    //无bug，只有synchronize()槽函数未真正实现，需要对winreg.h(Windows注册表)头文件中的函数比较熟悉
    QStringList pathlist = {"F:/python/spyder/widgets", "D:/Anaconda3/python37.zip"};
    QStringList ro_pathlist = {"D:/Anaconda3/DLLs", "D:/Anaconda3/lib", "D:/Anaconda3", "", "D:/Anaconda3/lib/site-packages",
                              "D:/Anaconda3/lib/site-packages/win32", "D:/Anaconda3/lib/site-packages/win32/lib",
                              "D:/Anaconda3/lib/site-packages/Pythonwin", "D:/Anaconda3/lib/site-packages/TPython/extensions",
                              "C:/Users/quan1/.ipython"};
    PathManager* test = new PathManager(nullptr, pathlist, ro_pathlist);
    test->exec();
    qDebug() << test->get_path_list();
}
