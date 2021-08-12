#include "runconfig.h"

static QString CURRENT_INTERPRETER = "Execute in current console";
static QString DEDICATED_INTERPRETER = "Execute in a dedicated console";
static QString SYSTERM_INTERPRETER = "Execute in an external system terminal";

static QString CURRENT_INTERPRETER_OPTION = "default/interpreter/current";
static QString DEDICATED_INTERPRETER_OPTION = "default/interpreter/dedicated";
static QString SYSTERM_INTERPRETER_OPTION = "default/interpreter/systerm";

static QString WDIR_USE_SCRIPT_DIR_OPTION = "default/wdir/use_script_directory";
static QString WDIR_USE_CWD_DIR_OPTION = "default/wdir/use_cwd_directory";
static QString WDIR_USE_FIXED_DIR_OPTION = "default/wdir/use_fixed_directory";
static QString WDIR_FIXED_DIR_OPTION = "default/wdir/fixed_directory";

static QString ALWAYS_OPEN_FIRST_RUN = "Always show %1 on a first file run";
QString ALWAYS_OPEN_FIRST_RUN_OPTION = "open_on_firstrun";

static QString CLEAR_ALL_VARIABLES = "Remove all variables before execution";
static QString POST_MORTEM = "Directly enter debugging when errors appear";
static QString INTERACT = "Interact with the Python console after execution";

static QString FILE_DIR = "The directory of the file being executed";
static QString CW_DIR = "The current working directory";
static QString FIXED_DIR = "The following directory:";

RunConfiguration::RunConfiguration(const QString& fname)
{
    Q_UNUSED(fname);
    this->set(CONF_get("run", "defaultconfiguration",
                       QHash<QString,QVariant>()).toHash());
}

void RunConfiguration::set(const QHash<QString, QVariant> &options)
{
    this->args = options.value("args", QString()).toString();
    this->args_enabled = options.value("args/enabled", false).toBool();
    this->current = options.value("current", CONF_get("run", CURRENT_INTERPRETER_OPTION,
                                                      true)).toBool();
    this->systerm = options.value("systerm", CONF_get("run", SYSTERM_INTERPRETER_OPTION,
                                                      false)).toBool();

    this->interact = options.value("interact", CONF_get("run", "interact",
                                                      false)).toBool();
    this->post_mortem = options.value("post_mortem", CONF_get("run", "post_mortem",
                                                      false)).toBool();
    this->python_args = options.value("python_args", QString()).toString();
    this->python_args_enabled = options.value("python_args/enabled", false).toBool();

    this->clear_namespace = options.value("clear_namespace", CONF_get("run", "clear_namespace",
                                                      false)).toBool();
    this->file_dir = options.value("file_dir", CONF_get("run", WDIR_USE_SCRIPT_DIR_OPTION,
                                                      true)).toBool();
    this->cw_dir = options.value("cw_dir", CONF_get("run", WDIR_USE_CWD_DIR_OPTION,
                                                      false)).toBool();
    this->fixed_dir = options.value("fixed_dir", CONF_get("run", WDIR_USE_FIXED_DIR_OPTION,
                                                      false)).toBool();
    this->dir = options.value("dir", QString()).toString();

}

QHash<QString,QVariant> RunConfiguration::get() const
{
    QHash<QString,QVariant> dict;
    dict["args/enabled"] = this->args_enabled;
    dict["args"] = this->args;
    dict["workdir/enabled"] = this->wdir_enabled;
    dict["workdir"] = this->wdir;
    dict["current"] = this->current;
    dict["systerm"] = this->systerm;
    dict["interact"] = this->interact;
    dict["post_mortem"] = this->post_mortem;
    dict["python_args/enabled"] = this->python_args_enabled;
    dict["python_args"] = this->python_args;
    dict["clear_namespace"] = this->clear_namespace;
    dict["file_dir"] = this->file_dir;
    dict["cw_dir"] = this->cw_dir;
    dict["fixed_dir"] = this->fixed_dir;
    dict["dir"] = this->dir;
    return dict;
}

QString RunConfiguration::get_working_directory() const
{
    return this->dir;
}

QString RunConfiguration::get_arguments() const
{
    if (this->args_enabled)
        return this->args;
    else
        return QString("");
}

QString RunConfiguration::get_python_arguments() const
{
    if (this->python_args_enabled)
        return this->python_args;
    else
        return QString("");
}


/********** 辅助函数 **********/
QList<QPair<QString,QHash<QString,QVariant>>> _get_run_configurations()
{
    int history_count = CONF_get("run", "history", 20).toInt();
    Q_ASSERT(history_count >= 10);
    QList<QVariant> list = CONF_get("run", "configurations", QList<QVariant>()).toList();
    QList<QPair<QString,QHash<QString,QVariant>>> res;
    foreach (QVariant tmp, list) {
        QList<QVariant> pair = tmp.toList();
        Q_ASSERT(pair.size() == 2);
        QString filename = pair[0].toString();
        QHash<QString,QVariant> options = pair[1].toHash();

        QFileInfo info(filename);
        if (info.isFile()) {
            res.append(QPair<QString,QHash<QString,QVariant>>(filename, options));
        }
    }
    return res.mid(0, history_count);
}

void _set_run_configurations(const QList<QPair<QString,QHash<QString,QVariant>>>& configurations)
{
    int history_count = CONF_get("run", "history", 20).toInt();
    Q_ASSERT(history_count >= 10);
    QList<QVariant> list;
    int len = configurations.size();
    for (int i = 0; i < len; ++i)
        list.append(QVariant());

    for (int i = 0; i < len; ++i) {
        auto pair = configurations[i];
        QString filename = pair.first;
        QHash<QString,QVariant> options = pair.second;
        QList<QVariant> tmp;
        tmp.append(filename);
        tmp.append(options);
        list[i] = tmp;
    }
    CONF_set("run", "configurations", list.mid(0, history_count));
}

// 外部负责释放指针
RunConfiguration* get_run_configuration(const QString& fname)
{
    auto configurations = _get_run_configurations();
    foreach (auto pair, configurations) {
        QString filename = pair.first;
        QHash<QString,QVariant> options = pair.second;
        if (fname == filename) {
            RunConfiguration* runconf = new RunConfiguration;
            runconf->set(options);
            return runconf;
        }
    }
    return nullptr;
}


/********** RunConfigOptions **********/
RunConfigOptions::RunConfigOptions(QWidget* parent)
    : QWidget (parent)
{
    this->dir = QString();
    this->runconf = RunConfiguration();
    bool firstrun_o = CONF_get("run", ALWAYS_OPEN_FIRST_RUN_OPTION,
                               false).toBool();

    QGroupBox* interpreter_group = new QGroupBox("Console", this);
    QVBoxLayout* interpreter_layout = new QVBoxLayout;
    interpreter_group->setLayout(interpreter_layout);

    this->current_radio = new QRadioButton(CURRENT_INTERPRETER, this);
    interpreter_layout->addWidget(this->current_radio);

    this->dedicated_radio = new QRadioButton(DEDICATED_INTERPRETER, this);
    interpreter_layout->addWidget(this->dedicated_radio);

    this->systerm_radio = new QRadioButton(SYSTERM_INTERPRETER, this);
    interpreter_layout->addWidget(this->systerm_radio);

    QGroupBox* common_group = new QGroupBox("General settings", this);
    QGridLayout* common_layout = new QGridLayout;
    common_group->setLayout(common_layout);

    this->clear_var_cb = new QCheckBox(CLEAR_ALL_VARIABLES, this);
    common_layout->addWidget(this->clear_var_cb, 0, 0);

    this->post_mortem_cb = new QCheckBox(POST_MORTEM, this);
    common_layout->addWidget(this->post_mortem_cb, 1, 0);

    this->clo_cb = new QCheckBox("Command line options:", this);
    common_layout->addWidget(this->clo_cb, 2, 0);
    this->clo_edit = new QLineEdit(this);
    connect(clo_cb, SIGNAL(toggled(bool)), clo_edit, SLOT(setEnabled(bool)));
    this->clo_edit->setEnabled(false);
    common_layout->addWidget(this->clo_edit, 2, 1);

    QGroupBox* wdir_group = new QGroupBox("Working Directory settings", this);
    QVBoxLayout* wdir_layout = new QVBoxLayout();
    wdir_group->setLayout(wdir_layout);

    this->file_dir_radio = new QRadioButton(FILE_DIR, this);
    wdir_layout->addWidget(this->file_dir_radio);

    this->cwd_radio = new QRadioButton(CW_DIR, this);
    wdir_layout->addWidget(this->cwd_radio);

    QHBoxLayout* fixed_dir_layout = new QHBoxLayout();
    this->fixed_dir_radio = new QRadioButton(FIXED_DIR, this);
    fixed_dir_layout->addWidget(this->fixed_dir_radio);
    this->wd_edit = new QLineEdit(this);
    connect(fixed_dir_radio, SIGNAL(toggled(bool)), wd_edit, SLOT(setEnabled(bool)));
    this->wd_edit->setEnabled(false);
    fixed_dir_layout->addWidget(this->wd_edit);
    QPushButton* browse_btn = new QPushButton(ima::icon("DirOpenIcon"), "", this);
    browse_btn->setToolTip("Select directory");
    connect(browse_btn, SIGNAL(clicked()), SLOT(select_directory()));
    fixed_dir_layout->addWidget(browse_btn);
    wdir_layout->addLayout(fixed_dir_layout);

    QGroupBox* external_group = new QGroupBox("External system terminal", this);
    external_group->setDisabled(true);

    connect(systerm_radio, SIGNAL(toggled(bool)), external_group, SLOT(setEnabled(bool)));

    QGridLayout* external_layout = new QGridLayout();
    external_group->setLayout(external_layout);
    this->interact_cb = new QCheckBox(INTERACT, this);
    external_layout->addWidget(this->interact_cb, 1, 0, 1, -1);

    this->pclo_cb = new QCheckBox("Command line options:", this);
    external_layout->addWidget(this->pclo_cb, 3, 0);
    this->pclo_edit = new QLineEdit(this);
    connect(pclo_cb, SIGNAL(toggled(bool)), pclo_edit, SLOT(setEnabled(bool)));
    this->pclo_edit->setEnabled(false);
    this->pclo_edit->setToolTip("<b>-u</b> is added to the "
                                "other options you set here");
    external_layout->addWidget(this->pclo_edit, 3, 1);

    QFrame* hline = new QFrame(this);
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);
    this->firstrun_cb = new QCheckBox(ALWAYS_OPEN_FIRST_RUN.arg("this dialog"), this);
    connect(firstrun_cb, SIGNAL(clicked()), SLOT(set_firstrun_o()));
    this->firstrun_cb->setChecked(firstrun_o);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(interpreter_group);
    layout->addWidget(common_group);
    layout->addWidget(wdir_group);
    layout->addWidget(external_group);
    layout->addWidget(hline);
    layout->addWidget(this->firstrun_cb);
    this->setLayout(layout);
}

void RunConfigOptions::select_directory()
{
    QString basedir = this->wd_edit->text();
    QFileInfo info(basedir);
    if (!info.isDir())
        basedir = misc::getcwd_or_home();
    QString directory = QFileDialog::getExistingDirectory(this, "Select directory",
                                                          basedir);
    if (!directory.isEmpty()) {
        this->wd_edit->setText(directory);
        this->dir = directory;
    }
}

void RunConfigOptions::set(const QHash<QString, QVariant> &options)
{
    this->runconf.set(options);
    this->clo_cb->setChecked(runconf.args_enabled);
    this->clo_edit->setText(runconf.args);
    if (runconf.current)
        this->current_radio->setChecked(true);
    else if (runconf.systerm)
        this->systerm_radio->setChecked(true);
    else
        this->dedicated_radio->setChecked(true);
    this->interact_cb->setChecked(runconf.interact);
    this->post_mortem_cb->setChecked(runconf.post_mortem);
    this->pclo_cb->setChecked(runconf.python_args_enabled);
    this->pclo_edit->setText(runconf.python_args);
    this->clear_var_cb->setChecked(runconf.clear_namespace);
    this->file_dir_radio->setChecked(runconf.file_dir);
    this->cwd_radio->setChecked(runconf.cw_dir);
    this->fixed_dir_radio->setChecked(runconf.fixed_dir);
    this->dir = runconf.dir;
    this->wd_edit->setText(this->dir);
}

QHash<QString,QVariant> RunConfigOptions::get()
{
    this->runconf.args_enabled = clo_cb->isChecked();
    this->runconf.args = clo_edit->text();
    this->runconf.current = current_radio->isChecked();
    this->runconf.systerm = systerm_radio->isChecked();
    this->runconf.interact = interact_cb->isChecked();
    this->runconf.post_mortem = post_mortem_cb->isChecked();
    this->runconf.python_args_enabled = pclo_cb->isChecked();
    this->runconf.python_args = pclo_edit->text();
    this->runconf.clear_namespace = clear_var_cb->isChecked();
    this->runconf.file_dir = file_dir_radio->isChecked();
    this->runconf.cw_dir = cwd_radio->isChecked();
    this->runconf.fixed_dir = fixed_dir_radio->isChecked();
    this->runconf.dir = wd_edit->text();
    return runconf.get();
}

bool RunConfigOptions::is_valid()
{
    QString wdir = this->wd_edit->text();
    QFileInfo info(wdir);
    if (!fixed_dir_radio->isChecked() || info.isDir())
        return true;
    else {
        QMessageBox::critical(this, "Run configuration",
                              QString("The following working directory is "
                                      "not valid:<br><b>%1</b>").arg(wdir));
        return false;
    }
}

void RunConfigOptions::set_firstrun_o()
{
    CONF_set("run", ALWAYS_OPEN_FIRST_RUN_OPTION,
             firstrun_cb->isChecked());
}


/********** BaseRunConfigDialog **********/
BaseRunConfigDialog::BaseRunConfigDialog(QWidget* parent)
    : QDialog (parent)
{
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowIcon(ima::icon("run_settings"));
    QVBoxLayout* layout = new QVBoxLayout;
    this->setLayout(layout);
}

void BaseRunConfigDialog::add_widgets(const QList<QPair<int, QWidget *> > &widgets_or_spacings)
{
    QBoxLayout* layout = dynamic_cast<QBoxLayout*>(this->layout());
    Q_ASSERT(layout);
    foreach (auto pair, widgets_or_spacings) {
        if (pair.second == nullptr)
            layout->addSpacing(pair.first);
        else
            layout->addWidget(pair.second);
    }
}

void BaseRunConfigDialog::add_button_box(QDialogButtonBox::StandardButtons stdbtns)
{
    QDialogButtonBox* bbox = new QDialogButtonBox(stdbtns);
    QPushButton* run_btn = bbox->addButton("Run", QDialogButtonBox::AcceptRole);
    connect(run_btn, SIGNAL(clicked()), SLOT(run_btn_clicked()));
    connect(bbox, SIGNAL(accepted()), SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), SLOT(reject()));
    QHBoxLayout* btnlayout = new QHBoxLayout;
    btnlayout->addStretch(1);
    btnlayout->addWidget(bbox);
    QBoxLayout* layout = dynamic_cast<QBoxLayout*>(this->layout());
    Q_ASSERT(layout);
    layout->addLayout(btnlayout);
}

void BaseRunConfigDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    emit size_change(this->size());
}


/********** RunConfigOneDialog **********/
RunConfigOneDialog::RunConfigOneDialog(QWidget* parent)
    : BaseRunConfigDialog (parent)
{
    this->filename = QString();
    this->runconfigoptions = nullptr;
}

void RunConfigOneDialog::setup(const QString &fname)
{
    this->filename = fname;
    this->runconfigoptions = new RunConfigOptions(this);
    this->runconfigoptions->set(RunConfiguration(fname).get());
    QList<QPair<int,QWidget*>> widgets;
    widgets.append(QPair<int,QWidget*>(0, this->runconfigoptions));
	this->add_widgets(widgets);
    this->add_button_box(QDialogButtonBox::Cancel);
    QFileInfo info(fname);
    this->setWindowTitle(QString("Run settings for %1")
                         .arg(info.fileName()));
}

void RunConfigOneDialog::accept()
{
    Q_ASSERT(this->runconfigoptions);
    if (!this->runconfigoptions->is_valid())
        return;
    QList<QPair<QString,QHash<QString,QVariant>>> configurations = _get_run_configurations();
    configurations.insert(0, QPair<QString,QHash<QString,QVariant>>
                          (filename, runconfigoptions->get()));
    _set_run_configurations(configurations);
    QDialog::accept();
}

// 外部负责释放指针
RunConfiguration* RunConfigOneDialog::get_configuration() const
{
    return new RunConfiguration(this->runconfigoptions->runconf);
}


/********** RunConfigDialog **********/
RunConfigDialog::RunConfigDialog(QWidget* parent)
    : BaseRunConfigDialog (parent)
{
    this->file_to_run = QString();
    this->combo = nullptr;
    this->stack = nullptr;
}

void RunConfigDialog::run_btn_clicked()
{
    this->file_to_run = this->combo->currentText();
}

void RunConfigDialog::setup(const QString &fname)
{
    QLabel* combo_label = new QLabel("Select a run configuration:", this);
    this->combo = new QComboBox(this);
    this->combo->setMaxVisibleItems(20);
    this->combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    this->combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    this->stack = new QStackedWidget(this);

    QList<QPair<QString,QHash<QString,QVariant>>> configurations = _get_run_configurations();
    bool break_flag = true;
    // Python for else语法是不发生break，就会进行else
    int index = 0;
    for (index = 0; index < configurations.size(); ++index) {
        auto pair = configurations[index];
        QString filename = pair.first;
        if (fname == filename) {
            break_flag = false;
            break;
        }
    }
    if (break_flag) {
        configurations.insert(0, QPair<QString,QHash<QString,QVariant>>
                              (fname, RunConfiguration(fname).get()));
        index = 0;
    }
    foreach (auto pair, configurations) {
        QString filename = pair.first;
        QHash<QString,QVariant> options = pair.second;
        RunConfigOptions* widget = new RunConfigOptions(this);
        widget->set(options);
        this->combo->addItem(filename);
        this->stack->addWidget(widget);
    }
    connect(combo, SIGNAL(currentIndexChanged(int)),
            stack, SLOT(setCurrentIndex(int)));
    this->combo->setCurrentIndex(index);

    QList<QPair<int,QWidget*>> widgets;
    widgets.append(QPair<int,QWidget*>(0, combo_label));
    widgets.append(QPair<int,QWidget*>(0, this->combo));
    widgets.append(QPair<int,QWidget*>(10, nullptr));
    widgets.append(QPair<int,QWidget*>(0, this->stack));
    this->add_widgets(widgets);
    this->add_button_box(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    this->setWindowTitle("Run configuration per file");
}

void RunConfigDialog::accept()
{
    QList<QPair<QString,QHash<QString,QVariant>>> configurations;
    for (int index = 0; index < this->stack->count(); ++index) {
        QString filename = this->combo->itemText(index);
        QWidget* widget = this->stack->widget(index);
        RunConfigOptions* runconfigoptions = dynamic_cast<RunConfigOptions*>(widget);
        Q_ASSERT(runconfigoptions);
        if (index == this->stack->currentIndex() &&
                !runconfigoptions->is_valid())
            return;
        QHash<QString,QVariant> options = runconfigoptions->get();
        configurations.append(QPair<QString,QHash<QString,QVariant>>(filename, options));
    }
    _set_run_configurations(configurations);
    QDialog::accept();
}


/********** RunConfigPage **********/

