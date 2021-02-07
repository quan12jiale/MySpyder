#include "maininterpreter.h"
#include "app/mainwindow.h"

MainInterpreterConfigPage::MainInterpreterConfigPage(QWidget *parent, MainWindow *main)
    : GeneralConfigPage (parent, main)
{
    this->CONF_SECTION = "main_interpreter";
    this->NAME = "Python interpreter";
    this->ICON = ima::icon("python");

    //构造函数
    this->cus_exec_radio = nullptr;
    this->pyexec_edit = nullptr;
    this->cus_exec_combo = nullptr;

    QString executable = this->get_option("executable", misc::get_python_executable()).toString();
    if (this->get_option("default").toBool())
        executable = misc::get_python_executable();

    QString tmp(executable);
    QFileInfo info(executable);
    if (!info.isFile())
        this->set_option("executable", misc::get_python_executable());
    else if (executable.endsWith("pythonw.exe"))
        this->set_option("executable", tmp.replace("pythonw.exe", "python.exe"));

    if (!this->get_option("default").toBool()) {
        if (this->get_option("custom_interpreter").toString().isEmpty())
            this->set_option("custom_interpreter", " ");
        this->set_custom_interpreters_list(executable);
        this->validate_custom_interpreters_list();
    }
}

void MainInterpreterConfigPage::setup_page()
{
    QGroupBox* pyexec_group = new QGroupBox("Python interpreter");
    QButtonGroup* pyexec_bg = new QButtonGroup(pyexec_group);
    QLabel* pyexec_label = new QLabel("Select the Python interpreter"
                                      " for all Spyder consoles");
    this->def_exec_radio = create_radiobutton("Default (i.e. the same as Spyder's)",
                                              "default", QVariant(), QString(),
                                              QString(), QString(), false,
                                              pyexec_bg);
    this->cus_exec_radio = create_radiobutton("Use the following Python interpreter:",
                                              "custom", QVariant(), QString(),
                                              QString(), QString(), false,
                                              pyexec_bg);
    QString filters = "Executables (*.exe)";

    QVBoxLayout* pyexec_layout = new QVBoxLayout;
    pyexec_layout->addWidget(pyexec_label);
    pyexec_layout->addWidget(this->def_exec_radio);
    pyexec_layout->addWidget(this->cus_exec_radio);
    this->validate_custom_interpreters_list();

    this->cus_exec_combo = create_file_combobox("Recent custom interpreters",
                                                this->get_option("custom_interpreters_list")
                                                .toStringList(), "custom_interpreter",
                                                QVariant(), QString(), false,
                                                filters, true, true);
    connect(def_exec_radio, SIGNAL(toggled(bool)),
            cus_exec_combo, SLOT(setDisabled(bool)));
    connect(cus_exec_radio, SIGNAL(toggled(bool)),
            cus_exec_combo, SLOT(setEnabled(bool)));
    pyexec_layout->addWidget(this->cus_exec_combo);
    pyexec_group->setLayout(pyexec_layout);

    FileComboBox* combobox = reinterpret_cast<FileComboBox*>
            (cus_exec_combo->property("combobox").toULongLong());
    this->pyexec_edit = combobox->lineEdit();

    QGroupBox* umr_group = new QGroupBox("User Module Reloader (UMR)");
    QLabel* umr_label = new QLabel("UMR forces Python to reload modules which were "
                                   "imported when executing a file in a Python or "
                                   "IPython console with the <i>runfile</i> "
                                   "function.");
    umr_label->setWordWrap(true);

    QCheckBox* umr_enabled_box = create_checkbox("Enable UMR", "umr/enabled",
                                                 QVariant(), QString(),
                                                 "This option will enable the User Module Reloader (UMR) "
                                                 "in Python/IPython consoles. UMR forces Python to "
                                                 "reload deeply modules during import when running a "
                                                 "Python script using the Spyder's builtin function "
                                                 "<b>runfile</b>."
                                                 "<br><br><b>1.</b> UMR may require to restart the "
                                                 "console in which it will be called "
                                                 "(otherwise only newly imported modules will be "
                                                 "reloaded when executing files)."
                                                 "<br><br><b>2.</b> If errors occur when re-running a "
                                                 "PyQt-based program, please check that the Qt objects "
                                                 "are properly destroyed (e.g. you may have to use the "
                                                 "attribute <b>Qt.WA_DeleteOnClose</b> on your main "
                                                 "window, using the <b>setAttribute</b> method)",
                                                 QString(), true);
    QCheckBox* umr_verbose_box = create_checkbox("Show reloaded modules list", "umr/verbose",
                                                 QVariant(), QString(), QString(),
                                                 "Please note that these changes will "
                                                 "be applied only to new consoles");
    QPushButton* umr_namelist_btn = new QPushButton("Set UMR excluded (not reloaded) modules");
    connect(umr_namelist_btn, SIGNAL(clicked()), SLOT(set_umr_namelist()));

    QVBoxLayout* umr_layout = new QVBoxLayout;
    umr_layout->addWidget(umr_label);
    umr_layout->addWidget(umr_enabled_box);
    umr_layout->addWidget(umr_verbose_box);
    umr_layout->addWidget(umr_namelist_btn);
    umr_group->setLayout(umr_layout);

    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->addWidget(pyexec_group);
    vlayout->addWidget(umr_group);
    vlayout->addStretch(1);
    this->setLayout(vlayout);
}

bool MainInterpreterConfigPage::python_executable_changed(const QString &pyexec)
{
    if (!this->cus_exec_radio->isChecked())
        return false;
    QString def_pyexec = misc::get_python_executable();
    if (pyexec == def_pyexec)
        return false;
    if (!programs::is_python_interpreter(pyexec) ||
            !this->warn_python_compatibility(pyexec)) {
        QMessageBox::warning(this, "Warning",
                             "You selected an invalid Python interpreter for the "
                             "console so the previous interpreter will stay. Please "
                             "make sure to select a valid one.");
        this->def_exec_radio->setChecked(true);
        return false;
    }
    return true;
}

bool MainInterpreterConfigPage::warn_python_compatibility(const QString &pyexec)
{
    QFileInfo info(pyexec);
    if (!info.isFile())
        return false;
    // TODO检查选择的python是不是3.几版本
    return true;
}

void MainInterpreterConfigPage::set_umr_namelist()
{
    bool valid;
    QString tmp = this->get_option("umr/namelist").toStringList().join(", ");
    QString arguments = QInputDialog::getText(this, "UMR",
                                              "Set the list of excluded modules as "
                                              "this: <i>numpy, scipy</i>",
                                              QLineEdit::Normal, tmp, &valid);
    if (valid) {
        QStringList fixed_namelist;
        if (!arguments.isEmpty()) {
            fixed_namelist = arguments.replace(" ", "").split(',');
            QString invalid;
            if (!invalid.isEmpty())
                QMessageBox::warning(this, "UMR",
                                     QString("The following modules are not "
                                     "installed on your machine:\n%1")
                                     .arg(invalid));
            QMessageBox::information(this, "UMR",
                                     "Please note that these changes will "
                                     "be applied only to new Python/IPython "
                                     "consoles");
        }

        this->set_option("umr/namelist", fixed_namelist);
    }
}

void MainInterpreterConfigPage::set_custom_interpreters_list(const QString &value)
{
    QStringList custom_list = this->get_option("custom_interpreters_list").toStringList();
    if (!custom_list.contains(value) &&
            value != misc::get_python_executable()) {
        custom_list.append(value);
        this->set_option("custom_interpreters_list", custom_list);
    }
}

void MainInterpreterConfigPage::validate_custom_interpreters_list()
{
    QStringList custom_list = this->get_option("custom_interpreters_list").toStringList();
    QStringList valid_custom_list;
    foreach (QString value, custom_list) {
        QFileInfo info(value);
        if (info.isFile() && programs::is_python_interpreter(value)
                && value != misc::get_python_executable())
            valid_custom_list.append(value);
    }
    this->set_option("custom_interpreters_list", valid_custom_list);
}

void MainInterpreterConfigPage::apply_settings(const QStringList &options)
{
    if (!this->def_exec_radio->isChecked()) {
        QString executable = this->pyexec_edit->text();
        QFileInfo info(executable);
        executable = info.canonicalFilePath();
        if (executable.endsWith("pythonw.exe"))
            executable.replace("pythonw.exe", "python.exe");
        bool change = this->python_executable_changed(executable);
        if (change) {
            this->set_custom_interpreters_list(executable);
            this->set_option("executable", executable);
            this->set_option("custom_interpreter", executable);
        }
    }
    if (this->pyexec_edit->text().isEmpty())
        this->set_option("custom_interpreter", "");
    this->main->apply_settings();
}
