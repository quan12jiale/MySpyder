#include "projectdialog.h"

bool is_writable(const QString& path)
{
    QFileInfo info(path);
    return info.isDir();
}

ProjectDialog::ProjectDialog(QWidget *parent)
    : QDialog (parent)
{
    QString current_python_version = "3.7";
    QStringList python_versions;
    python_versions << "2.7" << "3.4" << "3.5";
    if (!python_versions.contains(current_python_version)) {
        python_versions.append(current_python_version);
        std::sort(python_versions.begin(), python_versions.end());
    }

    this->project_name = QString();//未用到
    this->location = get_home_dir();

    this->groupbox = new QGroupBox();
    this->radio_new_dir = new QRadioButton("New directory");
    this->radio_from_dir = new QRadioButton("Existing directory");

    this->label_project_name = new QLabel("Project name");
    this->label_location = new QLabel("Location");
    this->label_project_type = new QLabel("Project type");
    this->label_python_version = new QLabel("Python version");

    this->text_project_name = new QLineEdit();
    this->text_location = new QLineEdit(get_home_dir());
    this->combo_project_type = new QComboBox();
    this->combo_python_version = new QComboBox();

    this->button_select_location = new QToolButton();
    this->button_cancel = new QPushButton("Cancel");
    this->button_create = new QPushButton("Create");

    this->bbox = new QDialogButtonBox(Qt::Horizontal);
    this->bbox->addButton(button_cancel, QDialogButtonBox::ActionRole);
    this->bbox->addButton(button_create, QDialogButtonBox::ActionRole);

    // Widget setup
    this->combo_python_version->addItems(python_versions);
    this->radio_new_dir->setChecked(true);
    this->text_location->setEnabled(true);
    this->text_location->setReadOnly(true);
    this->button_select_location->setIcon(ima::get_std_icon("DirOpenIcon"));
    this->button_cancel->setDefault(true);
    this->button_cancel->setAutoDefault(true);
    this->button_create->setEnabled(false);
    this->combo_project_type->addItems(this->_get_project_types());
    this->combo_python_version->setCurrentIndex(
        python_versions.indexOf(current_python_version));
    this->setWindowTitle("Create new project");
    this->setFixedWidth(500);
    this->label_python_version->setVisible(false);
    this->combo_python_version->setVisible(false);

    QHBoxLayout* layout_top = new QHBoxLayout;
    layout_top->addWidget(this->radio_new_dir);
    layout_top->addWidget(this->radio_from_dir);
    layout_top->addStretch(1);
    this->groupbox->setLayout(layout_top);

    QGridLayout* layout_grid = new QGridLayout;
    layout_grid->addWidget(this->label_project_name, 0, 0);
    layout_grid->addWidget(this->text_project_name, 0, 1, 1, 2);
    layout_grid->addWidget(this->label_location, 1, 0);
    layout_grid->addWidget(this->text_location, 1, 1);
    layout_grid->addWidget(this->button_select_location, 1, 2);
    layout_grid->addWidget(this->label_project_type, 2, 0);
    layout_grid->addWidget(this->combo_project_type, 2, 1, 1, 2);
    layout_grid->addWidget(this->label_python_version, 3, 0);
    layout_grid->addWidget(this->combo_python_version, 3, 1, 1, 2);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(this->groupbox);
    layout->addSpacing(10);
    layout->addLayout(layout_grid);
    layout->addStretch();
    layout->addSpacing(20);
    layout->addWidget(this->bbox);

    this->setLayout(layout);

    connect(button_select_location, SIGNAL(clicked()), SLOT(select_location()));
    connect(button_create, SIGNAL(clicked()), SLOT(create_project()));
    connect(button_cancel, SIGNAL(clicked()), SLOT(close()));

    connect(radio_from_dir, &QAbstractButton::clicked,
            [this](){this->update_location();});
    connect(radio_new_dir, &QAbstractButton::clicked,
            [this](){this->update_location();});
    connect(text_project_name, SIGNAL(textChanged(const QString &)),
            SLOT(update_location(const QString &)));
}

QStringList ProjectDialog::_get_project_types() const
{
    QStringList projects;
    projects.append("Empty project");
    return projects;
}

void ProjectDialog::select_location()
{
    QString location = QFileDialog::getExistingDirectory(this, "Select directory",
                                                         this->location);
    if (!location.isEmpty()) {
        if (is_writable(location)) {
            this->location = location;
            this->update_location();
        }
    }
}

void ProjectDialog::update_location(const QString& text)
{
    Q_UNUSED(text);
    this->text_project_name->setEnabled(radio_new_dir->isChecked());
    QString name = this->text_project_name->text().trimmed();

    QString path;
    if (!name.isEmpty() && this->radio_new_dir->isChecked()) {
        path = this->location + "/" + name;
        QFileInfo info(path);
        this->button_create->setDisabled(info.isDir());
    }
    else if (this->radio_from_dir->isChecked()) {
        this->button_create->setEnabled(true);
        path = this->location;
    }
    else {
        this->button_create->setEnabled(false);
        path = this->location;
    }

    this->text_location->setText(path);
}

void ProjectDialog::create_project()
{
    QString tmp = QString("python=%1").arg(combo_python_version->currentText());
    QStringList packages(tmp);
    emit sig_project_creation_requested(text_location->text(),
                                        combo_project_type->currentText(),
                                        packages);
    this->accept();
}

static void test()
{
    ProjectDialog* dlg = new ProjectDialog(nullptr);
    dlg->show();
}
