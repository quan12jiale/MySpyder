#include "configdialog.h"
#include "app/mainwindow.h"

static QString HDPI_QT_PAGE = "https://doc.qt.io/qt-5/highdpi.html";

void ConfigPage::set_option(const QString& option,const QVariant& value)
{
    CONF_set(this->CONF_SECTION, option, value);
}

QVariant ConfigPage::get_option(const QString &option, const QVariant &_default)
{
    return CONF_get(this->CONF_SECTION, option, _default);
}


/********** ConfigPage **********/
ConfigPage::ConfigPage(QWidget* parent)
    : QWidget (parent)
{
    this->is_modified = false;
}

void ConfigPage::initialize()
{
    this->setup_page();
    this->load_from_conf();
}

void ConfigPage::set_modified(bool state)
{
    this->is_modified = state;
    emit apply_button_enabled(state);
}

void ConfigPage::apply_changes()
{
    if (this->is_modified) {
        this->save_to_conf();
        // if self.apply_callback is not None:

        if (this->CONF_SECTION == "main")
            this->_save_lang();
        // TODO
    }
}


/********** ConfigDialog **********/
ConfigDialog::ConfigDialog(MainWindow *parent)
    : QDialog (parent)
{
    this->main = parent;

    this->pages_widget = new QStackedWidget;
    this->contents_widget = new QListWidget;
    this->button_reset = new QPushButton("Reset to defaults");

    QDialogButtonBox* bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply
                                                  | QDialogButtonBox::Cancel);
    this->apply_btn = bbox->button(QDialogButtonBox::Apply);

    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowTitle("Preferences");
    this->setWindowIcon(ima::icon("configure"));
    this->contents_widget->setMovement(QListView::Static);
    this->contents_widget->setSpacing(1);
    this->contents_widget->setCurrentRow(0);

    QSplitter* hsplitter = new QSplitter;
    hsplitter->addWidget(this->contents_widget);
    hsplitter->addWidget(this->pages_widget);

    QHBoxLayout* btnlayout = new QHBoxLayout;
    btnlayout->addWidget(this->button_reset);
    btnlayout->addStretch(1);
    btnlayout->addWidget(bbox);

    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->addWidget(hsplitter);
    vlayout->addLayout(btnlayout);

    this->setLayout(vlayout);

    if (this->main)
        connect(button_reset, SIGNAL(clicked()), this->main, SLOT(reset_spyder()));
    connect(pages_widget, SIGNAL(currentChanged(int)), this, SLOT(current_page_changed(int)));
    connect(contents_widget, SIGNAL(currentRowChanged(int)), pages_widget, SLOT(setCurrentIndex(int)));

    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(bbox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(button_clicked(QAbstractButton*)));

    // CONF.set('main', 'interface_language', load_lang_conf())
}

int ConfigDialog::get_current_index() const
{
    return this->contents_widget->currentRow();
}

void ConfigDialog::set_current_index(int index)
{
    this->contents_widget->setCurrentRow(index);
}

QWidget* ConfigDialog::get_page(int index)
{
    QWidget* widget;
    if (index == -1)
        widget = this->pages_widget->currentWidget();
    else
        widget = this->pages_widget->widget(index);
    QScrollArea* area = qobject_cast<QScrollArea*>(widget);
    return area->widget();
}

void ConfigDialog::accept()
{
    for (int index = 0; index < this->pages_widget->count(); ++index) {
        QWidget* tmp = this->get_page(index);
        ConfigPage* configpage = qobject_cast<ConfigPage*>(tmp);
        if (configpage == nullptr) {
            qDebug() << __FILE__ << __LINE__;
            return;
        }
        if (!configpage->is_valid())
            return;
        configpage->apply_changes();
    }
    QDialog::accept();
}

void ConfigDialog::button_clicked(QAbstractButton *button)
{
    qDebug() << __FILE__ << __LINE__;
    if (button == this->apply_btn) {
        QWidget* tmp = this->get_page();
        ConfigPage* configpage = qobject_cast<ConfigPage*>(tmp);
        if (configpage == nullptr) {
            qDebug() << __FILE__ << __LINE__;
            return;
        }
        if (!configpage->is_valid())
            return;
        configpage->apply_changes();
    }
}

void ConfigDialog::current_page_changed(int index)
{
    QWidget* tmp = this->get_page(index);
    ConfigPage* widget = qobject_cast<ConfigPage*>(tmp);
    //self.apply_btn.setVisible(widget.apply_callback is not None)
    this->apply_btn->setVisible(true);
    this->apply_btn->setEnabled(widget->is_modified);
}

void ConfigDialog::add_page(SpyderConfigPage *widget)
{
    connect(this, SIGNAL(check_settings()), widget, SLOT(check_settings()));
    connect(widget, &ConfigPage::show_this_page,
            [this](){int row = this->contents_widget->count();
                    this->contents_widget->setCurrentRow(row);});
    connect(widget, SIGNAL(apply_button_enabled(bool)), this->apply_btn, SLOT(setEnabled(bool)));

    QScrollArea* scrollarea = new QScrollArea(this);
    scrollarea->setWidgetResizable(true);
    scrollarea->setWidget(widget);
    this->pages_widget->addWidget(scrollarea);
    QListWidgetItem* item = new QListWidgetItem(this->contents_widget);
    try {
        item->setIcon(widget->get_icon());
    } catch (...) {
    }
    item->setText(widget->get_name());
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setSizeHint(QSize(0, 25));
}

void ConfigDialog::check_all_settings()
{
    emit check_settings();
}

void ConfigDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    emit size_change(this->size());
}


/********** SpyderConfigPage **********/
SpyderConfigPage::SpyderConfigPage(QWidget* parent)
    : ConfigPage (parent)
{
    this->default_button_group = nullptr;
}

void SpyderConfigPage::set_modified(bool state)
{
    ConfigPage::set_modified(state);
    if (!state)
        this->changed_options.clear();
}

bool SpyderConfigPage::is_valid()
{
    foreach (QLineEdit* lineedit, this->lineedits.keys()) {
        if (this->validate_data.contains(lineedit) && lineedit->isEnabled()) {
            auto pair = this->validate_data[lineedit];
            FUNC_PT validator  = pair.first;
            QString invalid_msg = pair.second;
            QString text = lineedit->text();
            if (!validator(text)) {
                QMessageBox::critical(this, this->get_name(),
                                      QString("%1:<br><b>%2</b>").arg(invalid_msg).arg(text),
                                      QMessageBox::Ok);
                return false;
            }
        }
    }
    return true;
}

void SpyderConfigPage::load_from_conf()
{
    foreach (QCheckBox* checkbox, this->checkboxes.keys()) {
        auto pair = this->checkboxes[checkbox];
        QString option = pair.first;
        QVariant _default = pair.second;

        checkbox->setChecked(this->get_option(option, _default).toBool());
        connect(checkbox,&QAbstractButton::clicked,
                [this, option](){this->has_been_modified(option);});
    }
    foreach (QRadioButton* radiobutton, this->radiobuttons.keys()) {
        auto pair = this->radiobuttons[radiobutton];
        QString option = pair.first;
        QVariant _default = pair.second;

        radiobutton->setChecked(this->get_option(option, _default).toBool());
        connect(radiobutton,&QAbstractButton::toggled,
                [this, option](){this->has_been_modified(option);});
        if (!radiobutton->property("restart_required").isNull()) {
            bool restart_required = radiobutton->property("restart_required").toBool();
            if (restart_required)
                this->restart_options[option] = radiobutton->property("label_text").toString();
        }
    }
    foreach (QLineEdit* lineedit, this->lineedits.keys()) {
        auto pair = this->lineedits[lineedit];
        QString option = pair.first;
        QVariant _default = pair.second;

        lineedit->setText(this->get_option(option, _default).toString());
        connect(lineedit,&QLineEdit::textChanged,
                [this, option](){this->has_been_modified(option);});
        if (!lineedit->property("restart_required").isNull()) {
            bool restart_required = lineedit->property("restart_required").toBool();
            if (restart_required)
                this->restart_options[option] = lineedit->property("label_text").toString();
        }
    }
    foreach (QSpinBox* spinbox, this->spinboxes.keys()) {
        auto pair = this->spinboxes[spinbox];
        QString option = pair.first;
        QVariant _default = pair.second;

        spinbox->setValue(this->get_option(option, _default).toInt());
        connect(spinbox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), //对信号重载
                [this, option](){this->has_been_modified(option);});
    }
    foreach (QComboBox* combobox, this->comboboxes.keys()) {
        bool break_flag = true;
        auto pair = this->comboboxes[combobox];
        QString option = pair.first;
        QVariant _default = pair.second;
        QVariant value = this->get_option(option, _default);
        int index = 0;
        for (index = 0; index < combobox->count(); ++index) {
            QString data = combobox->itemData(index).toString();
            if (data == value.toString()) {
                break_flag = false;
                break;
            }
        }
        if (break_flag) {
            if (combobox->count() == 0)
                index = -1;
        }
        if (index > 0)
            combobox->setCurrentIndex(index);
        connect(combobox,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                [this, option](){this->has_been_modified(option);});
        if (!combobox->property("restart_required").isNull()) {
            bool restart_required = combobox->property("restart_required").toBool();
            if (restart_required)
                this->restart_options[option] = combobox->property("label_text").toString();
        }
    }
    foreach (auto pair, this->fontboxes.keys()) {
        QFontComboBox* fontbox = pair.first;
        QSpinBox* sizebox = pair.second;
        QString option = this->fontboxes[pair];

        QFont font  = this->get_font(option);
        fontbox->setCurrentFont(font);
        sizebox->setValue(font.pointSize());
        QString property;
        if (option.isEmpty())
            property = "plugin_font";
        else
            property = option;
        connect(fontbox,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                [this, property](){this->has_been_modified(property);});
        connect(sizebox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), //对信号重载
                [this, property](){this->has_been_modified(property);});
    }
    foreach (ColorLayout* clayout, this->coloredits.keys()) {
        auto pair = this->coloredits[clayout];
        QString option = pair.first;
        QVariant _default = pair.second;

        QLineEdit* edit = clayout->lineedit;
        ColorButton* btn = clayout->colorbtn;
        edit->setText(this->get_option(option, _default).toString());
        connect(btn,&QAbstractButton::clicked,
                [this, option](){this->has_been_modified(option);});
        connect(edit,&QLineEdit::textChanged,
                [this, option](){this->has_been_modified(option);});
    }
    foreach (SceditKey key, this->scedits.keys()) {
        ColorLayout* clayout = key.clayout;
        QCheckBox* cb_bold = key.cb_bold;
        QCheckBox* cb_italic = key.cb_italic;
        auto pair = this->scedits[key];
        QString option = pair.first;
        QVariant _default = pair.second;

        QLineEdit* edit = clayout->lineedit;
        ColorButton* btn = clayout->colorbtn;
        ColorBoolBool res = this->get_option(option, _default).value<ColorBoolBool>();
        QString color = res.color;
        bool bold = res.bold;
        bool italic  = res.italic;
        edit->setText(color);
        cb_bold->setChecked(bold);
        cb_italic->setChecked(italic);
        connect(edit,&QLineEdit::textChanged,
                [this, option](){this->has_been_modified(option);});

        connect(btn,&QAbstractButton::clicked,
                [this, option](){this->has_been_modified(option);});
        connect(cb_bold,&QAbstractButton::clicked,
                [this, option](){this->has_been_modified(option);});
        connect(cb_italic,&QAbstractButton::clicked,
                [this, option](){this->has_been_modified(option);});
    }
}

void SpyderConfigPage::save_to_conf()
{
    foreach (QCheckBox* checkbox, this->checkboxes.keys()) {
        auto pair = this->checkboxes[checkbox];
        QString option = pair.first;
        this->set_option(option, checkbox->isChecked());
    }

    foreach (QRadioButton* radiobutton, this->radiobuttons.keys()) {
        auto pair = this->radiobuttons[radiobutton];
        QString option = pair.first;
        this->set_option(option, radiobutton->isChecked());
    }

    foreach (QLineEdit* lineedit, this->lineedits.keys()) {
        auto pair = this->lineedits[lineedit];
        QString option = pair.first;
        this->set_option(option, lineedit->text());
    }

    foreach (QSpinBox* spinbox, this->spinboxes.keys()) {
        auto pair = this->spinboxes[spinbox];
        QString option = pair.first;
        this->set_option(option, spinbox->value());
    }

    foreach (QComboBox* combobox, this->comboboxes.keys()) {
        auto pair = this->comboboxes[combobox];
        QString option = pair.first;
        QVariant data = combobox->itemData(combobox->currentIndex());
        this->set_option(option, data.toString());
    }

    foreach (auto key, this->fontboxes.keys()) {
        QFontComboBox* fontbox = key.first;
        QSpinBox* sizebox = key.second;
        QString option = this->fontboxes[key];
        QFont font = fontbox->currentFont();
        font.setPointSize(sizebox->value());
        this->set_font(font, option);
    }

    foreach (ColorLayout* clayout, this->coloredits.keys()) {
        auto pair = this->coloredits[clayout];
        QString option = pair.first;
        this->set_option(option, clayout->lineedit->text());
    }

    foreach (SceditKey key, this->scedits.keys()) {
        ColorLayout* clayout = key.clayout;
        QCheckBox* cb_bold = key.cb_bold;
        QCheckBox* cb_italic = key.cb_italic;
        auto pair = this->scedits[key];
        QString option = pair.first;

        QString color = clayout->lineedit->text();
        bool bold = cb_bold->isChecked();
        bool italic = cb_italic->isChecked();
        QVariant value = QVariant::fromValue(ColorBoolBool(color, bold, italic));
        this->set_option(option, value);
    }
}

void SpyderConfigPage::has_been_modified(const QString& option)
{
    this->set_modified(true);
    this->changed_options.insert(option);
}

void SpyderConfigPage::show_message(QString msg_warning, QString msg_info,
                                    bool msg_if_enabled, bool is_checked)
{
    if (is_checked || !msg_if_enabled) {
        if (!msg_warning.isEmpty())
            QMessageBox::warning(this, this->get_name(), msg_warning,
                                 QMessageBox::Ok);
        if (!msg_info.isEmpty())
            QMessageBox::information(this, this->get_name(), msg_info,
                                     QMessageBox::Ok);
    }
}

QCheckBox* SpyderConfigPage::create_checkbox(QString text, QString option, QVariant _default,
                                             QString tip, QString msg_warning,
                                             QString msg_info, bool msg_if_enabled)
{
    QCheckBox* checkbox = new QCheckBox(text);
    if (!tip.isEmpty())
        checkbox->setToolTip(tip);
    this->checkboxes[checkbox] = QPair<QString,QVariant>(option, _default);
    if (!msg_warning.isEmpty() || !msg_info.isEmpty()) {
        connect(checkbox, &QAbstractButton::clicked,
                [=](bool is_checked){this->show_message(msg_warning, msg_info,
                                                        msg_if_enabled, is_checked);});
    }
    return checkbox;
}

QRadioButton* SpyderConfigPage::create_radiobutton(QString text, QString option, QVariant _default,
                                                   QString tip, QString msg_warning,
                                                   QString msg_info, bool msg_if_enabled,
                                                   QButtonGroup *button_group, bool restart)
{
    QRadioButton* radiobutton = new QRadioButton(text);
    if (button_group == nullptr) {
        if (this->default_button_group == nullptr)
            default_button_group = new QButtonGroup(this);
        button_group = default_button_group;
    }
    button_group->addButton(radiobutton);
    if (!tip.isEmpty())
        radiobutton->setToolTip(tip);
    this->radiobuttons[radiobutton] = QPair<QString,QVariant>(option, _default);
    if (!msg_warning.isEmpty() || !msg_info.isEmpty()) {
        connect(radiobutton, &QAbstractButton::toggled,
                [=](bool is_checked){this->show_message(msg_warning, msg_info,
                                                        msg_if_enabled, is_checked);});
    }
    radiobutton->setProperty("restart_required", restart); //动态属性
    radiobutton->setProperty("label_text", text);
    return radiobutton;
}

QWidget* SpyderConfigPage::create_lineedit(QString text, QString option, QVariant _default,
                                           QString tip, Qt::Orientation alignment,
                                           QString regex, bool restart)
{
    QLabel* label = new QLabel(text);
    label->setWordWrap(true);
    QLineEdit* edit = new QLineEdit;
    QBoxLayout* layout;
    if (alignment == Qt::Vertical)
        layout = new QVBoxLayout;
    else
        layout = new QHBoxLayout;
    layout->addWidget(label);
    layout->addWidget(edit);
    layout->setContentsMargins(0, 0, 0, 0);
    if (!tip.isEmpty())
        edit->setToolTip(tip);
    if (!regex.isEmpty())
        edit->setValidator(new QRegExpValidator(QRegExp(regex)));
    this->lineedits[edit] = QPair<QString,QVariant>(option, _default);
    QWidget* widget = new QWidget(this);
    widget->setProperty("label", reinterpret_cast<qulonglong>(label)); //动态属性
    widget->setProperty("textbox", reinterpret_cast<qulonglong>(edit));
    widget->setLayout(layout);
    edit->setProperty("restart_required", restart);
    edit->setProperty("label_text", text);
    return widget;
}

bool isdir(QString path)
{
    QFileInfo info(path);
    return info.isDir();
}

bool isfile(QString path)
{
    QFileInfo info(path);
    return info.isFile();
}

QWidget* SpyderConfigPage::create_browsedir(QString text, QString option,
                                            QVariant _default, QString tip)
{
    Q_UNUSED(tip);
    QWidget* widget = this->create_lineedit(text, option, _default, QString(), Qt::Horizontal);
    QLineEdit* edit = nullptr;
    foreach (edit, this->lineedits.keys()) {
        if (widget->isAncestorOf(edit))
            break;
    }
    QString msg = "Invalid directory path";
    this->validate_data[edit] = QPair<FUNC_PT,QString>(isdir, msg);
    QPushButton* browse_btn = new QPushButton(ima::icon("DirOpenIcon"),"",this);
    browse_btn->setToolTip("Select directory");
    connect(browse_btn, &QAbstractButton::clicked,
            [this, edit](){this->select_directory(edit);});
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(widget);
    layout->addWidget(browse_btn);
    layout->setContentsMargins(0, 0, 0, 0);
    QWidget* browsedir = new QWidget(this);
    browsedir->setLayout(layout);
    return browsedir;
}

void SpyderConfigPage::select_directory(QLineEdit *edit)
{
    QString basedir = edit->text();
    if (!isdir(basedir))
        basedir = misc::getcwd_or_home();
    QString title = "Select directory";
    QString directory = QFileDialog::getExistingDirectory(this, title, basedir);
    if (!directory.isEmpty())
        edit->setText(directory);
}

QWidget* SpyderConfigPage::create_browsefile(QString text, QString option, QVariant _default,
                                             QString tip, QString filters)
{
    Q_UNUSED(tip);
    QWidget* widget = this->create_lineedit(text, option, _default, QString(), Qt::Horizontal);
    QLineEdit* edit = nullptr;
    foreach (edit, this->lineedits.keys()) {
        if (widget->isAncestorOf(edit))
            break;
    }
    QString msg = "Invalid file path";
    this->validate_data[edit] = QPair<FUNC_PT,QString>(isfile, msg);
    QPushButton* browse_btn = new QPushButton(ima::icon("FileIcon"),"",this);
    browse_btn->setToolTip("Select file");
    connect(browse_btn, &QAbstractButton::clicked,
            [this, edit, filters](){this->select_file(edit, filters);});
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(widget);
    layout->addWidget(browse_btn);
    layout->setContentsMargins(0, 0, 0, 0);
    QWidget* browsedir = new QWidget(this);
    browsedir->setLayout(layout);
    return browsedir;
}

void SpyderConfigPage::select_file(QLineEdit *edit, QString filters)
{
    QFileInfo info(edit->text());
    QString basedir = info.absolutePath();
    if (!isdir(basedir))
        basedir = misc::getcwd_or_home();
    if (filters.isEmpty())
        filters = "All files (*)";
    QString title = "Select file";
    QString filename = QFileDialog::getOpenFileName(this, title, basedir, filters);
    if (!filename.isEmpty())
        edit->setText(filename);
}

QWidget* SpyderConfigPage::create_spinbox(QString prefix, QString suffix,
                                          QString option, QVariant _default,
                                          int min_, int max_, int step, QString tip)
{
    QWidget* widget = new QWidget(this);
    QLabel *plabel=nullptr, *slabel=nullptr;
    if (!prefix.isEmpty()) {
        plabel = new QLabel(prefix);
        widget->setProperty("plabel", reinterpret_cast<qulonglong>(plabel));
    }
    if (!suffix.isEmpty()) {
        slabel = new QLabel(suffix);
        widget->setProperty("slabel", reinterpret_cast<qulonglong>(slabel));
    }
    QSpinBox* spinbox = nullptr;
    if (step != -1) {
        spinbox = new QSpinBox;
        spinbox->setSingleStep(step);
    }
    else
        spinbox = new QSpinBox;

    if (min_ != -1)
        spinbox->setMinimum(min_);
    if (max_ != -1)
        spinbox->setMaximum(max_);
    if (!tip.isEmpty())
        spinbox->setToolTip(tip);
    this->spinboxes[spinbox] = QPair<QString,QVariant>(option, _default);
    QHBoxLayout* layout = new QHBoxLayout;
    QList<QWidget*> list;
    list << plabel << spinbox << slabel;
    foreach (QWidget* subwidget, list) {
        if (subwidget)
            layout->addWidget(subwidget);;
    }
    layout->addStretch(1);
    layout->setContentsMargins(0, 0, 0, 0);
    widget->setProperty("spinbox", reinterpret_cast<qulonglong>(spinbox));//动态属性
    widget->setLayout(layout);
    return widget;
}

QPair<QLabel*,ColorLayout*> SpyderConfigPage::create_coloredit(QString text, QString option,
                                                                              QVariant _default, QString tip)
{
    QLabel* label = new QLabel(text);
    ColorLayout* clayout = new ColorLayout(QColor(Qt::black), this);
    clayout->lineedit->setMaximumWidth(80);
    // 源码是clayout.setToolTip(tip)
    if (!tip.isEmpty())
        clayout->lineedit->setToolTip(tip);
    this->coloredits[clayout] = QPair<QString,QVariant>(option, _default);
    return QPair<QLabel*,ColorLayout*>(label, clayout);
}

QWidget* SpyderConfigPage::create_coloredit_with_layout(QString text, QString option,
                                                        QVariant _default, QString tip)
{
    QPair<QLabel*,ColorLayout*> pair = create_coloredit(text, option, _default, tip);
    QLabel* label = pair.first;
    ColorLayout* clayout = pair.second;

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(label);
    layout->addLayout(clayout);
    layout->addStretch(1);
    layout->setContentsMargins(0, 0, 0, 0);
    QWidget* widget = new QWidget(this);
    widget->setLayout(layout);
    return widget;
}

Scedit_Without_Layout SpyderConfigPage::create_scedit(QString text, QString option,
                                                                     QVariant _default, QString tip)
{
    QLabel* label = new QLabel(text);
    ColorLayout* clayout = new ColorLayout(QColor(Qt::black), this);
    clayout->lineedit->setMaximumWidth(80);
    if (!tip.isEmpty())
        clayout->lineedit->setToolTip(tip);
    QCheckBox* cb_bold = new QCheckBox;
    cb_bold->setIcon(ima::icon("bold"));
    cb_bold->setToolTip("Bold");

    QCheckBox* cb_italic = new QCheckBox;
    cb_italic->setIcon(ima::icon("italic"));
    cb_italic->setToolTip("Italic");
    scedits[SceditKey(clayout, cb_bold, cb_italic)] = QPair<QString,QVariant>(option, _default);
    return Scedit_Without_Layout(label, clayout, cb_bold, cb_italic);
}

QWidget* SpyderConfigPage::create_scedit_with_layout(QString text, QString option,
                                                     QVariant _default, QString tip)
{
    Scedit_Without_Layout tmp = create_scedit(text, option, _default, tip);
    QLabel* label = tmp.label;
    ColorLayout* clayout = tmp.clayout;
    QCheckBox* cb_bold = tmp.cb_bold;
    QCheckBox* cb_italic = tmp.cb_italic;

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(label);
    layout->addLayout(clayout);
    layout->addSpacing(10);
    layout->addWidget(cb_bold);
    layout->addWidget(cb_italic);
    layout->addStretch(1);
    layout->setContentsMargins(0, 0, 0, 0);
    QWidget* widget = new QWidget(this);
    widget->setLayout(layout);
    return widget;
}

QWidget* SpyderConfigPage::create_combobox(QString text, QList<QPair<QString, QString> > choices,
                                           QString option, QVariant _default, QString tip, bool restart)
{
    QLabel* label = new QLabel(text);
    QComboBox* combobox = new QComboBox;
    if (!tip.isEmpty())
        combobox->setToolTip(tip);
    foreach (auto pair, choices) {
        QString name = pair.first;
        QString key = pair.second;
        if (!(name.isEmpty() && key.isEmpty()))
            combobox->addItem(name, key);
    }

    int count = 0;
    for (int index = 0; index < choices.size(); ++index) {
        auto pair = choices[index];
        QString name = pair.first;
        QString key = pair.second;
        if (name.isEmpty() && key.isEmpty()) {
            combobox->insertSeparator(index + count);
            count++;
        }
    }
    this->comboboxes[combobox] = QPair<QString,QVariant>(option, _default);
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(label);
    layout->addWidget(combobox);
    layout->addStretch(1);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget* widget = new QWidget(this);
    widget->setProperty("label", reinterpret_cast<qulonglong>(label));//动态属性
    widget->setProperty("combobox", reinterpret_cast<qulonglong>(combobox));
    widget->setLayout(layout);
    combobox->setProperty("restart_required", restart);
    combobox->setProperty("label_text", text);
    return widget;
}

QWidget* SpyderConfigPage::create_file_combobox(QString text, QStringList choices,
                                                QString option, QVariant _default, QString tip,
                                                bool restart, QString filters,
                                                bool adjust_to_contents, bool default_line_edit)
{
    FileComboBox* combobox = new FileComboBox(this, adjust_to_contents, default_line_edit);
    combobox->setProperty("restart_required", restart);// 动态属性
    combobox->setProperty("label_text", text);
    QLineEdit* edit = combobox->lineEdit();
    edit->setProperty("restart_required", restart);
    edit->setProperty("label_text", text);
    this->lineedits[edit] = QPair<QString,QVariant>(option, _default);

    if (!tip.isEmpty())
        combobox->setToolTip(tip);
    combobox->addItems(choices);

    QString msg = "Invalid file path";
    this->validate_data[edit] = QPair<FUNC_PT,QString>(isfile, msg);
    QPushButton* browse_btn = new QPushButton(ima::icon("FileIcon"), "", this);
    browse_btn->setToolTip("Select file");
    connect(browse_btn, &QAbstractButton::clicked,
            [this, edit, filters](){this->select_file(edit, filters);});

    QGridLayout* layout = new QGridLayout;
    layout->addWidget(combobox, 0, 0, 0, 9);
    layout->addWidget(browse_btn, 0, 10);
    layout->setContentsMargins(0, 0, 0, 0);
    QWidget* widget = new QWidget(this);
    widget->setProperty("combobox", reinterpret_cast<qulonglong>(combobox));// 动态属性
    widget->setProperty("browse_btn", reinterpret_cast<qulonglong>(browse_btn));
    widget->setLayout(layout);
    return widget;
}

QWidget* SpyderConfigPage::create_fontgroup(QString option, QString title,
                                            QFontComboBox::FontFilters fontfilters)
{
    QLabel* fontlabel;
    if (!title.isEmpty())
        fontlabel = new QLabel(title);
    else
        fontlabel = new QLabel("Font: ");
    QFontComboBox* fontbox = new QFontComboBox;
    fontbox->setFontFilters(fontfilters);

    QLabel* sizelabel = new QLabel("  Size: ");
    QSpinBox* sizebox = new QSpinBox;
    sizebox->setRange(7, 100);
    this->fontboxes[QPair<QFontComboBox*,QSpinBox*>(fontbox, sizebox)] = option;
    QHBoxLayout* layout = new QHBoxLayout;

    layout->addWidget(fontlabel);
    layout->addWidget(fontbox);
    layout->addWidget(sizelabel);
    layout->addWidget(sizebox);
    layout->addStretch(1);

    QWidget* widget = new QWidget(this);
    widget->setProperty("fontlabel", reinterpret_cast<qulonglong>(fontlabel));
    widget->setProperty("sizelabel", reinterpret_cast<qulonglong>(sizelabel));
    widget->setProperty("fontbox", reinterpret_cast<qulonglong>(fontbox));
    widget->setProperty("sizebox", reinterpret_cast<qulonglong>(sizebox));
    widget->setLayout(layout);
    return widget;
}

QPushButton* SpyderConfigPage::create_button(const QString &text)
{
    QPushButton* btn = new QPushButton(text);
    // btn.clicked.connect(callback)
    connect(btn, &QAbstractButton::clicked,
            [this](){this->has_been_modified("");});
    return btn;
}

QWidget* SpyderConfigPage::create_tab(const QList<QWidget *> &widgets)
{
    QWidget* widget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    foreach (QWidget* widg, widgets) {
        layout->addWidget(widg);
    }
    layout->addStretch(1);
    widget->setLayout(layout);
    return widget;
}


/********** PluginConfigPage **********/
PluginConfigPage::PluginConfigPage(SpyderPluginMixin* plugin, QWidget* parent)
    : SpyderConfigPage (parent)
{
    this->plugin = plugin;
}

QString PluginConfigPage::get_name() const
{
    return this->plugin->get_plugin_title();
}

QIcon PluginConfigPage::get_icon() const
{
    return this->plugin->get_plugin_icon();
}


/********** GeneralConfigPage **********/
GeneralConfigPage::GeneralConfigPage(QWidget* parent, MainWindow* main)
    : SpyderConfigPage (parent)
{
    this->main = main;
}

QString GeneralConfigPage::get_name() const
{
    return this->NAME;
}

QIcon GeneralConfigPage::get_icon() const
{
    return this->ICON;
}

void GeneralConfigPage::prompt_restart_required()
{
    QSet<QString> changed_opts = this->changed_options;
    QHash<QString, QString> restart_opts = this->restart_options;
    QStringList options;
    for (auto it=changed_opts.begin();it!=changed_opts.end();it++){
        if (restart_opts.contains(*it))
            options.append(*it);
    }

    QString msg_start;
    if (options.size() == 1)
        msg_start = "Spyder needs to restart to change the "
                    "following setting:";
    else
        msg_start = "Spyder needs to restart to change the "
                    "following settings:";
    QString msg_end = "Do you wish to restart now?";

    QString msg_options;
    foreach (QString option, options) {
        msg_options += QString("<li>%1</li>").arg(option);
    }
    QString msg_title = "Information";
    QString msg = QString("%1<ul>%2</ul><br>%3").arg(msg_start).arg(msg_options).arg(msg_end);
    QMessageBox::StandardButton answer = QMessageBox::information(this, msg_title, msg,
                                                                  QMessageBox::Yes | QMessageBox::No);
    if (answer == QMessageBox::Yes)
        this->restart();
}

void GeneralConfigPage::restart()
{
    this->main->restart();
}


/********** MainConfigPage **********/
MainConfigPage::MainConfigPage(QWidget* parent, MainWindow* main)
    : GeneralConfigPage (parent, main)
{
    this->CONF_SECTION = "main";
    this->NAME = "General";
}

void MainConfigPage::setup_page()
{
    this->ICON = ima::icon("genprefs");

    QGroupBox* general_group = new QGroupBox("General");
    QList<QPair<QString,QString>> language_choices;
    for (auto it=LANGUAGE_CODES.begin();it!=LANGUAGE_CODES.end();it++) {
        QString key = it.key();
        QString val = it.value();
        language_choices.append(QPair<QString,QString>(val,key));
    }
    std::sort(language_choices.begin(), language_choices.end());
    QWidget* language_combo = create_combobox("Language:",
                                              language_choices,
                                              "interface_language",
                                              QVariant(), QString(),
                                              true);
    QStringList opengl_options;
    opengl_options << "Automatic" << "Desktop" << "Software" << "GLES";
    QList<QPair<QString,QString>> opengl_choices;
    foreach (QString c, opengl_options) {
        opengl_choices.append(QPair<QString,QString>(c, c.toLower()));
    }
    QWidget* opengl_combo = create_combobox("Rendering engine:",
                                            opengl_choices,
                                            "opengl",
                                            QVariant(), QString(),
                                            true);

    QCheckBox* single_instance_box = create_checkbox("Use a single instance",
                                                     "single_instance", QVariant(),
                                                     "Set this to open external<br> "
                                                     "Python files in an already running "
                                                     "instance (Requires a restart)");

    QCheckBox* prompt_box = create_checkbox("Prompt when exiting", "prompt_on_exit");
    QCheckBox* popup_console_box = create_checkbox("Show internal Spyder errors to report "
                                                   "them to Github", "show_internal_errors");
    QCheckBox* check_updates = create_checkbox("Check for updates on startup",
                                               "check_updates_on_startup");


    QHBoxLayout* comboboxes_advanced_layout = new QHBoxLayout;
    QGridLayout* cbs_adv_grid = new QGridLayout;
    cbs_adv_grid->addWidget(reinterpret_cast<QLabel*>(language_combo->property("label").toULongLong()), 0, 0);
    cbs_adv_grid->addWidget(reinterpret_cast<QComboBox*>(language_combo->property("combobox").toULongLong()), 0, 1);
    cbs_adv_grid->addWidget(reinterpret_cast<QLabel*>(opengl_combo->property("label").toULongLong()), 1, 0);
    cbs_adv_grid->addWidget(reinterpret_cast<QComboBox*>(opengl_combo->property("combobox").toULongLong()), 1, 1);
    comboboxes_advanced_layout->addLayout(cbs_adv_grid);
    comboboxes_advanced_layout->addStretch(1);

    QVBoxLayout* general_layout = new QVBoxLayout;
    general_layout->addLayout(comboboxes_advanced_layout);
    general_layout->addWidget(single_instance_box);
    general_layout->addWidget(prompt_box);
    general_layout->addWidget(popup_console_box);
    general_layout->addWidget(check_updates);
    general_group->setLayout(general_layout);

    QGroupBox* interface_group = new QGroupBox("Interface");
    QStringList styles = QStyleFactory::keys();
    QList<QPair<QString,QString>> choices;
    foreach (QString style, styles) {
        choices.append(QPair<QString,QString>(style, style.toLower()));
    }
    QWidget* style_combo = create_combobox("Qt windows style",
                                           choices, "windows_style",
                                           this->main->default_style);
    QStringList themes;
    themes << "Spyder 2" << "Spyder 3";
    QList<QPair<QString,QString>> icon_choices;
    foreach (QString theme, themes) {
        icon_choices.append(QPair<QString,QString>(theme, theme.toLower()));
    }
    QWidget* icons_combo = create_combobox("Icon theme",
                                           icon_choices,
                                           "icon_theme",
                                           QVariant(), QString(),
                                           true);

    QCheckBox* vertdock_box = create_checkbox("Vertical title bars in panes",
                                              "vertical_dockwidget_titlebars");
    QCheckBox* verttabs_box = create_checkbox("Vertical tabs in panes",
                                              "vertical_tabs");
    QCheckBox* animated_box = create_checkbox("Animated toolbars and panes",
                                              "animated_docks");
    QCheckBox* tear_off_box = create_checkbox("Tear off menus", "tear_off_menus",
                                              QVariant(),
                                              "Set this to detach any<br> "
                                              "menu from the main window");
    QCheckBox* margin_box = create_checkbox("Custom margin for panes:",
                                            "use_custom_margin");
    QWidget* margin_spin = create_spinbox("", "pixels", "custom_margin",
                                          0, 0, 30);
    connect(margin_box, SIGNAL(toggled(bool)),
            reinterpret_cast<QSpinBox*>(margin_spin->property("spinbox").toULongLong()),
            SLOT(setEnabled(bool)));
    connect(margin_box, SIGNAL(toggled(bool)),
            reinterpret_cast<QLabel*>(margin_spin->property("slabel").toULongLong()),
            SLOT(setEnabled(bool)));
    reinterpret_cast<QSpinBox*>(margin_spin->property("spinbox").toULongLong())
            ->setEnabled(this->get_option("use_custom_margin").toBool());
    reinterpret_cast<QLabel*>(margin_spin->property("slabel").toULongLong())
            ->setEnabled(this->get_option("use_custom_margin").toBool());

    QCheckBox* cursor_box = create_checkbox("Cursor blinking:",
                                            "use_custom_cursor_blinking");
    QWidget* cursor_spin = create_spinbox("", "ms", "custom_cursor_blinking",
                                          QApplication::cursorFlashTime(),
                                          0, 5000, 100);
    connect(cursor_box, SIGNAL(toggled(bool)),
            reinterpret_cast<QSpinBox*>(cursor_spin->property("spinbox").toULongLong()),
            SLOT(setEnabled(bool)));
    connect(cursor_box, SIGNAL(toggled(bool)),
            reinterpret_cast<QLabel*>(cursor_spin->property("slabel").toULongLong()),
            SLOT(setEnabled(bool)));
    reinterpret_cast<QSpinBox*>(cursor_spin->property("spinbox").toULongLong())
            ->setEnabled(this->get_option("use_custom_margin").toBool());
    reinterpret_cast<QLabel*>(cursor_spin->property("slabel").toULongLong())
            ->setEnabled(this->get_option("use_custom_margin").toBool());

    QGridLayout* margins_cursor_layout = new QGridLayout;
    margins_cursor_layout->addWidget(margin_box, 0, 0);
    margins_cursor_layout->addWidget(reinterpret_cast<QSpinBox*>(margin_spin->property("spinbox").toULongLong()), 0, 1);
    margins_cursor_layout->addWidget(reinterpret_cast<QLabel*>(margin_spin->property("slabel").toULongLong()), 0, 2);
    margins_cursor_layout->addWidget(cursor_box, 1, 0);
    margins_cursor_layout->addWidget(reinterpret_cast<QSpinBox*>(cursor_spin->property("spinbox").toULongLong()), 1, 1);
    margins_cursor_layout->addWidget(reinterpret_cast<QLabel*>(cursor_spin->property("slabel").toULongLong()), 1, 2);
    margins_cursor_layout->setColumnStretch(2, 100);

    QHBoxLayout* comboboxes_layout = new QHBoxLayout;
    QGridLayout* cbs_layout = new QGridLayout;
    cbs_layout->addWidget(reinterpret_cast<QLabel*>(style_combo->property("label").toULongLong()), 0, 0);
    cbs_layout->addWidget(reinterpret_cast<QComboBox*>(style_combo->property("combobox").toULongLong()), 0, 1);
    cbs_layout->addWidget(reinterpret_cast<QLabel*>(icons_combo->property("label").toULongLong()), 1, 0);
    cbs_layout->addWidget(reinterpret_cast<QComboBox*>(icons_combo->property("combobox").toULongLong()), 1, 1);
    comboboxes_layout->addLayout(cbs_layout);
    comboboxes_layout->addStretch(1);

    QVBoxLayout* interface_layout = new QVBoxLayout;
    interface_layout->addLayout(comboboxes_layout);
    interface_layout->addWidget(vertdock_box);
    interface_layout->addWidget(verttabs_box);
    interface_layout->addWidget(animated_box);
    interface_layout->addWidget(tear_off_box);
    interface_layout->addLayout(margins_cursor_layout);
    interface_group->setLayout(interface_layout);

    QGroupBox* sbar_group = new QGroupBox("Status bar");
    QCheckBox* show_status_bar = create_checkbox("Show status bar", "show_status_bar");

    QCheckBox* memory_box = create_checkbox("Show memory usage every", "memory_usage/enable",
                                            QVariant(), this->main->mem_status->toolTip());
    QWidget* memory_spin = create_spinbox("", "ms", "memory_usage/timeout",
                                          QVariant(), 100, 1000000, 100);
    connect(memory_box, SIGNAL(toggled(bool)), memory_spin, SLOT(setEnabled(bool)));
    memory_spin->setEnabled(this->get_option("memory_usage/enable").toBool());
    memory_box->setEnabled(true);
    memory_spin->setEnabled(true);

   bool status_bar_o = this->get_option("show_status_bar").toBool();
   connect(show_status_bar, SIGNAL(toggled(bool)), memory_box, SLOT(setEnabled(bool)));
   connect(show_status_bar, SIGNAL(toggled(bool)), memory_spin, SLOT(setEnabled(bool)));
   memory_box->setEnabled(status_bar_o);
   memory_spin->setEnabled(status_bar_o);

   QGridLayout* cpu_memory_layout = new QGridLayout;
   cpu_memory_layout->addWidget(memory_box, 0, 0);
   cpu_memory_layout->addWidget(memory_spin, 0, 1);

   QVBoxLayout* sbar_layout = new QVBoxLayout;
   sbar_layout->addWidget(show_status_bar);
   sbar_layout->addLayout(cpu_memory_layout);
   sbar_group->setLayout(sbar_layout);

   QGroupBox* screen_resolution_group = new QGroupBox("Screen resolution");
   QButtonGroup* screen_resolution_bg = new QButtonGroup(screen_resolution_group);
   QLabel* screen_resolution_label = new QLabel(QString("Configuration for high DPI "
                                                        "screens<br><br>"
                                                        "Please see "
                                                        "<a href=\"%1\">%1</a><> "
                                                        "for more information about "
                                                        "these options (in "
                                                        "English).").arg(HDPI_QT_PAGE));
   screen_resolution_label->setWordWrap(true);
   screen_resolution_label->setOpenExternalLinks(true);

   QRadioButton* normal_radio = create_radiobutton("Normal", "normal_screen_resolution",
                                                   QVariant(), QString(), QString(), QString(),
                                                   false, screen_resolution_bg);
   QRadioButton* auto_scale_radio = create_radiobutton("Enable auto high DPI scaling", "high_dpi_scaling",
                                                       QVariant(), "Set this for high DPI displays",
                                                       QString(), QString(), false,
                                                       screen_resolution_bg, true);
   QRadioButton* custom_scaling_radio = create_radiobutton("Set a custom high DPI scaling", "high_dpi_custom_scale_factor",
                                                           QVariant(), "Set this for high DPI displays when "
                                                                       "auto scaling does not work",
                                                           QString(), QString(), false,
                                                           screen_resolution_bg, true);
   QWidget* custom_scaling_edit = create_lineedit("", "high_dpi_custom_scale_factors",
                                                  QVariant(), "Enter values for different screens "
                                                              "separated by semicolons ';', "
                                                              "float values are supported",
                                                  Qt::Horizontal, "[0-9]+(?:\\.[0-9]*)(;[0-9]+(?:\\.[0-9]*))*",
                                                  true);

   connect(normal_radio, SIGNAL(toggled(bool)), custom_scaling_edit, SLOT(setDisabled(bool)));
   connect(auto_scale_radio, SIGNAL(toggled(bool)), custom_scaling_edit, SLOT(setDisabled(bool)));
   connect(custom_scaling_radio, SIGNAL(toggled(bool)), custom_scaling_edit, SLOT(setEnabled(bool)));

   QVBoxLayout* screen_resolution_layout = new QVBoxLayout;
   screen_resolution_layout->addWidget(screen_resolution_label);

   QGridLayout* screen_resolution_inner_layout = new QGridLayout;
   screen_resolution_inner_layout->addWidget(normal_radio, 0, 0);
   screen_resolution_inner_layout->addWidget(auto_scale_radio, 1, 0);
   screen_resolution_inner_layout->addWidget(custom_scaling_radio, 2, 0);
   screen_resolution_inner_layout->addWidget(custom_scaling_edit, 2, 1);

   screen_resolution_layout->addLayout(screen_resolution_inner_layout);
   screen_resolution_group->setLayout(screen_resolution_layout);

   QWidget* plain_text_font = create_fontgroup("font", "Plain text font",
                                               QFontComboBox::MonospacedFonts);
   QWidget* rich_text_font = create_fontgroup("rich_font", "Rich text font");

   QGroupBox* fonts_group = new QGroupBox("Fonts");
   QGridLayout* fonts_layout = new QGridLayout;
   fonts_layout->addWidget(reinterpret_cast<QLabel*>(plain_text_font->property("fontlabel").toULongLong()), 0, 0);
   fonts_layout->addWidget(reinterpret_cast<QFontComboBox*>(plain_text_font->property("fontbox").toULongLong()), 0, 1);
   fonts_layout->addWidget(reinterpret_cast<QLabel*>(plain_text_font->property("sizelabel").toULongLong()), 0, 2);
   fonts_layout->addWidget(reinterpret_cast<QSpinBox*>(plain_text_font->property("sizebox").toULongLong()), 0, 3);

   fonts_layout->addWidget(reinterpret_cast<QLabel*>(rich_text_font->property("fontlabel").toULongLong()), 1, 0);
   fonts_layout->addWidget(reinterpret_cast<QFontComboBox*>(rich_text_font->property("fontbox").toULongLong()), 1, 1);
   fonts_layout->addWidget(reinterpret_cast<QLabel*>(rich_text_font->property("sizelabel").toULongLong()), 1, 2);
   fonts_layout->addWidget(reinterpret_cast<QSpinBox*>(rich_text_font->property("sizebox").toULongLong()), 1, 3);
   fonts_group->setLayout(fonts_layout);

   QTabWidget* tabs = new QTabWidget;
   QList<QWidget*> widgets;
   widgets << fonts_group << screen_resolution_group << interface_group;
   tabs->addTab(this->create_tab(widgets), "Appearance");
   widgets.clear();
   widgets << general_group << sbar_group;
   tabs->addTab(this->create_tab(widgets), "Advanced Settings");

   QVBoxLayout* vlayout = new QVBoxLayout;
   vlayout->addWidget(tabs);
   this->setLayout(vlayout);
}

QFont MainConfigPage::get_font(const QString &option) const
{
    return gui::get_font("main", option);
}

void MainConfigPage::set_font(const QFont &font, const QString &option)
{
    gui::set_font(font, "main", option);
    QList<SpyderPluginMixin*> plugins = this->main->widgetlist + this->main->thirdparty_plugins;
    foreach (SpyderPluginMixin* plugin, plugins) {
        plugin->update_font();
    }
}

void MainConfigPage::apply_settings(const QStringList& options)
{
    this->main->apply_settings();
}

void MainConfigPage::_save_lang()
{}


/*
ColorSchemeConfigPage::ColorSchemeConfigPage(QWidget* parent, MainWindow* main)
    : GeneralConfigPage (parent, main)
{
    this->CONF_SECTION = "color_schemes";
    this->NAME = "Syntax coloring";
}

void ColorSchemeConfigPage::setup_page()
{
    this->ICON = ima::icon("eyedropper");

    QStringList names = this->get_option("names").toStringList();
    names.removeOne("Custom");
    QStringList custom_names = this->get_option("custom_names", QStringList()).toStringList();

    // Widgets
    QLabel* about_label = new QLabel("Here you can select the color scheme used in "
                                     "the Editor and all other Spyder plugins.<br><br>"
                                     "You can also edit the color schemes provided "
                                     "by Spyder or create your own ones by using "
                                     "the options provided below.<br>");
    QPushButton* edit_button = new QPushButton("Edit selected");
    QPushButton* create_button = new QPushButton("Create new scheme");
    this->delete_button = new QPushButton("Delete");
    this->preview_editor = new CodeEditor(this);
    this->stacked_widget = new QStackedWidget(this);
    this->reset_button = new QPushButton("Reset");
    this->scheme_editor_dialog = new SchemeEditor(this, this->stacked_widget);

    about_label->setWordWrap(true);
    QList<QPair<QString,QString>> choices;
    choices.append(QPair<QString,QString>("", ""));
    QWidget* schemes_combobox_widget = create_combobox("Scheme:",
                                                       choices,
                                                       "selected");
    this->schemes_combobox = reinterpret_cast<QComboBox*>(schemes_combobox_widget->property("combobox").toULongLong());

    // Layouts
    QVBoxLayout* vlayout = new QVBoxLayout;

    QVBoxLayout* manage_layout = new QVBoxLayout;
    manage_layout->addWidget(about_label);

    QHBoxLayout* combo_layout = new QHBoxLayout;
    combo_layout->addWidget(reinterpret_cast<QLabel*>(schemes_combobox_widget->property("label").toULongLong()));
    combo_layout->addWidget(reinterpret_cast<QComboBox*>(schemes_combobox_widget->property("combobox").toULongLong()));

    QVBoxLayout* buttons_layout = new QVBoxLayout;
    buttons_layout->addLayout(combo_layout);
    buttons_layout->addWidget(edit_button);
    buttons_layout->addWidget(this->reset_button);
    buttons_layout->addWidget(this->delete_button);
    buttons_layout->addStretch(1);
    buttons_layout->addWidget(create_button);

    QVBoxLayout* preview_layout = new QVBoxLayout;
    preview_layout->addWidget(this->preview_editor);

    QHBoxLayout* buttons_preview_layout = new QHBoxLayout;
    buttons_preview_layout->addLayout(buttons_layout);
    buttons_preview_layout->addLayout(preview_layout);

    manage_layout->addLayout(buttons_preview_layout);
    QGroupBox* manage_group = new QGroupBox("Manage color schemes");
    manage_group->setLayout(manage_layout);

    vlayout->addWidget(manage_group);
    this->setLayout(vlayout);

    connect(create_button, SIGNAL(clicked(bool)), this, SLOT(create_new_scheme()));
    connect(edit_button, SIGNAL(clicked(bool)), this, SLOT(edit_scheme()));
    connect(this->reset_button, SIGNAL(clicked(bool)), this, SLOT(reset_to_default()));
    connect(this->delete_button, SIGNAL(clicked(bool)), this, SLOT(delete_scheme()));
    connect(this->schemes_combobox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=](){this->update_preview();});
    connect(this->schemes_combobox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ColorSchemeConfigPage::update_buttons);

    foreach (QString name, names)
        this->scheme_editor_dialog->add_color_scheme_stack(name);
    foreach (QString name, custom_names)
        this->scheme_editor_dialog->add_color_scheme_stack(name, true);

    this->update_combobox();
    this->update_preview();
}

void ColorSchemeConfigPage::apply_settings(const QStringList &options)
{
    //self.main.editor.apply_plugin_settings(['color_scheme_name'])
    this->update_combobox();
    this->update_preview();
}

QString ColorSchemeConfigPage::current_scheme_name() const
{
    return this->schemes_combobox->currentText();
}

QString ColorSchemeConfigPage::current_scheme() const
{
    return this->scheme_choices_dict[this->current_scheme_name()];
}

int ColorSchemeConfigPage::current_scheme_index() const
{
    return this->schemes_combobox->currentIndex();
}

void ColorSchemeConfigPage::update_combobox()
{
    int index = this->current_scheme_index();
    this->schemes_combobox->blockSignals(true);
    QStringList names = this->get_option("names").toStringList();
    names.removeOne("Custom");
    QStringList custom_names = this->get_option("custom_names", QStringList()).toStringList();

    QStringList tmp = names + custom_names;
    foreach (QString n, tmp)
        this->scheme_choices_dict[this->get_option(QString("%1/name").arg(n)).toString()] = n;

    QStringList choices;
    if (!custom_names.isEmpty()) {
        choices.append(names);
        choices.append(QString());
        choices.append(custom_names);
    }
    else
        choices = names;

    QComboBox* combobox = this->schemes_combobox;
    combobox->clear();

    foreach (QString name, choices) {
        if (name.isEmpty())
            continue;
        combobox->addItem(this->get_option(QString("%1/name").arg(name)).toString(), name);
    }
    if (!custom_names.isEmpty())
        combobox->insertSeparator(names.size());

    this->schemes_combobox->blockSignals(false);
    this->schemes_combobox->setCurrentIndex(index);
}

void ColorSchemeConfigPage::update_buttons()
{
    QString current_scheme = this->current_scheme();
    QStringList names = this->get_option("names").toStringList();
    names.removeOne("Custom");
    bool delete_enabled = ! names.contains(current_scheme);
    this->delete_button->setEnabled(delete_enabled);
    this->reset_button->setEnabled(! delete_enabled);
}

void ColorSchemeConfigPage::update_preview(int index)
{
    Q_UNUSED(index);
    QString text = "\"\"\"A string\"\"\"\n\n"
                   "# A comment\n\n"
                   "# %% A cell\n\n"
                   "class Foo(object):\n"
                   "    def __init__(self):\n"
                   "        bar = 42\n"
                   "        print(bar)\n";
    QSettings settings;
    bool show_blanks = settings.value("editor/blank_spaces", false).toBool();

    this->preview_editor->setup_editor(true,QString(),true, gui::get_font(),QHash<QString,ColorBoolBool>(),
                                       false,true,true,true,true,true,
                                       true,true,79,false,true,false
                                       ,show_blanks);
    this->preview_editor->set_text(text);
    this->preview_editor->set_language("Python");
}

void ColorSchemeConfigPage::create_new_scheme()
{
    QStringList names = this->get_option("names").toStringList();
    QStringList custom_names = this->get_option("custom_names", QStringList()).toStringList();

    int counter = custom_names.size() - 1;
    QList<int> custom_index;
    foreach (QString n, custom_names) {
        bool ok;
        int num = n.split('-').last().toInt(&ok);
        if (ok)
            custom_index.append(num);
    }
    for (int i=0;i<custom_names.size();i++) {
        if (custom_index[i] != i) {
            counter = i-1;
            break;
        }
    }
    QString custom_name = QString("custom-%1").arg(counter+1);

    custom_names.append(custom_name);
    this->set_option("custom_names", custom_names);
    foreach (QString key, sh::COLOR_SCHEME_KEYS.keys()) {
        QString name = QString("%1/%2").arg(custom_name).arg(key);
        QString default_name = QString("%1/%2").arg(this->current_scheme()).arg(key);
        QVariant option = this->get_option(default_name);
        this->set_option(name, option);
    }
    this->set_option(QString("%1/name").arg(custom_name), custom_name);

    SchemeEditor* dlg = this->scheme_editor_dialog;
    dlg->add_color_scheme_stack(custom_name, true);
    dlg->set_scheme(custom_name);
    this->load_from_conf();

    if (dlg->exec()) {
        QString name = dlg->get_scheme_name();
        this->set_option(QString("%1/name").arg(custom_name), name);

        QStringList tmp = names + custom_names;
        int index = tmp.indexOf(custom_name) + 1;
        this->update_combobox();
        this->schemes_combobox->setCurrentIndex(index);
    }
    else {
        custom_names.removeOne(custom_name);
        this->set_option("custom_names", custom_names);
        dlg->delete_color_scheme_stack(custom_name);
    }
}

void ColorSchemeConfigPage::edit_scheme()
{
    SchemeEditor* dlg = this->scheme_editor_dialog;
    dlg->set_scheme(this->current_scheme());

    if (dlg->exec()) {
        QHash<QString, ColorBoolBool> temporal_color_scheme = dlg->get_edited_color_scheme();
        foreach (QString key, temporal_color_scheme.keys()) {
            QString option = QString("temp/%1").arg(key);
            ColorBoolBool value = temporal_color_scheme[key];
            this->set_option(option, QVariant::fromValue(value));
        }
        this->update_preview(-1);
    }
}

void ColorSchemeConfigPage::delete_scheme()
{
    QString scheme_name = this->current_scheme();

    QMessageBox::StandardButton answer = QMessageBox::warning(this, "Warning",
                         "Are you sure you want to delete "
                         "this scheme?",
                         QMessageBox::Yes | QMessageBox::No);

    if (answer ==QMessageBox::Yes) {
        QStringList names = this->get_option("names").toStringList();
        this->set_scheme("spyder");
        this->schemes_combobox->setCurrentIndex(names.indexOf("spyder"));
        //this->set_option("selected", "spyder"); 先注释掉

        QStringList custom_names = this->get_option("custom_names", QStringList()).toStringList();
        if (custom_names.contains(scheme_name))
            custom_names.removeAll(scheme_name);
        this->set_option("custom_names", custom_names);

        QSettings settings;
        settings.beginGroup(this->CONF_SECTION);
        foreach (QString key, sh::COLOR_SCHEME_KEYS.keys()) {
            QString option = QString("%1/%2").arg(scheme_name).arg(key);
            settings.remove(option);
        }
        settings.remove(QString("%1/name").arg(scheme_name));
        settings.endGroup();

        this->update_combobox();
        this->update_preview();
    }
}

void ColorSchemeConfigPage::set_scheme(const QString &scheme_name)
{
    SchemeEditor* dlg = this->scheme_editor_dialog;
    dlg->set_scheme(scheme_name);
}

void ColorSchemeConfigPage::reset_to_default()
{
    QString scheme = this->current_scheme();
    QStringList names = this->get_option("names").toStringList();
    if (names.contains(scheme)) {
        foreach (QString key, sh::COLOR_SCHEME_KEYS.keys()) {
            QString option = QString("%1/%2").arg(scheme).arg(key);
            QVariant value = get_default(this->CONF_SECTION, option);
            this->set_option(option, value);
        }
        this->load_from_conf();
    }
}


SchemeEditor::SchemeEditor(ColorSchemeConfigPage* parent, QStackedWidget* stack)
    : QDialog (parent)
{
    this->parent = parent;
    this->stack = stack;

    QDialogButtonBox* bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(this->stack);
    layout->addWidget(bbox);
    this->setLayout(layout);

    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(accepted()), this, SLOT(get_edited_color_scheme()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
}

void SchemeEditor::set_scheme(const QString &scheme_name)
{
    int index = this->order.indexOf(scheme_name);
    if (index != -1)
        this->stack->setCurrentIndex(index);
    this->last_used_scheme = scheme_name;
}

QString SchemeEditor::get_scheme_name() const
{
    return this->scheme_name_textbox[this->last_used_scheme]->text();
}

QHash<QString, ColorBoolBool> SchemeEditor::get_edited_color_scheme() const
{
    QHash<QString, ColorBoolBool> color_scheme;
    QString scheme_name = this->last_used_scheme;

    foreach (QString key, this->widgets[scheme_name].keys()) {
        SceditKey items = this->widgets[scheme_name][key];
        ColorBoolBool value;
        if (items.cb_bold == nullptr)
            ColorBoolBool value(items.clayout->text(), false,
                                false);
        else
            ColorBoolBool value(items.clayout->text(), items.cb_bold->isChecked(),
                                items.cb_italic->isChecked());
        color_scheme[key] = value;
    }
    return color_scheme;
}

void SchemeEditor::add_color_scheme_stack(const QString &scheme_name, bool custom)
{
    QList<QPair<QString, QStringList>> color_scheme_groups;
    color_scheme_groups.append(QPair<QString, QStringList>("Text", QStringList({"normal", "comment", "string", "number", "keyword",
                                                                    "builtin", "definition", "instance"})));
    color_scheme_groups.append(QPair<QString, QStringList>("Highlight", QStringList({"currentcell", "currentline", "occurrence",
                                                                                     "matched_p", "unmatched_p", "ctrlclick"})));
    color_scheme_groups.append(QPair<QString, QStringList>("Background", QStringList({"background", "sideareas"})));

    ColorSchemeConfigPage* parent = this->parent;
    QWidget* line_edit = parent->create_lineedit("Scheme name:",
                                                 QString("%1/name").arg(scheme_name));
    this->widgets[scheme_name].clear();

    reinterpret_cast<QLabel*>(line_edit->property("label").toULongLong())->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    this->setWindowTitle("Color scheme editor");

    QHBoxLayout* name_layout = new QHBoxLayout;
    name_layout->addWidget(reinterpret_cast<QLabel*>(line_edit->property("label").toULongLong()));
    name_layout->addWidget(reinterpret_cast<QLineEdit*>(line_edit->property("textbox").toULongLong()));
    this->scheme_name_textbox[scheme_name] = reinterpret_cast<QLineEdit*>(line_edit->property("textbox").toULongLong());

    if (!custom)
        reinterpret_cast<QLineEdit*>(line_edit->property("textbox").toULongLong())->setDisabled(true);

    QVBoxLayout* cs_layout = new QVBoxLayout;
    cs_layout->addLayout(name_layout);

    QHBoxLayout* h_layout = new QHBoxLayout;
    QVBoxLayout* v_layout = new QVBoxLayout;

    for (int index = 0; index < color_scheme_groups.size(); ++index) {
        auto pair = color_scheme_groups[index];
        QString group_name = pair.first;
        QStringList keys = pair.second;
        QGridLayout* group_layout = new QGridLayout;

        for (int row = 0; row < keys.size(); ++row) {
            QString key = keys[row];
            QString option = QString("%1/%2").arg(scheme_name).arg(key);
            QVariant value = this->parent->get_option(option);
            QString name = sh::COLOR_SCHEME_KEYS[key];

            if (index == 0) {
                auto pair = parent->create_coloredit(name, option);
                QLabel* label = pair.first;
                ColorLayout* clayout = pair.second;
                label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                group_layout->addWidget(label, row+1, 0);
                group_layout->addLayout(clayout, row+1, 1);
                this->widgets[scheme_name][key] = SceditKey(clayout, nullptr, nullptr);
            }
            else {
                Scedit_Without_Layout tmp = parent->create_scedit(name,option);
                QLabel* label = tmp.label;
                ColorLayout* clayout = tmp.clayout;
                QCheckBox* cb_bold = tmp.cb_bold;
                QCheckBox* cb_italic = tmp.cb_italic;
                label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                group_layout->addWidget(label, row+1, 0);
                group_layout->addLayout(clayout, row+1, 1);
                group_layout->addWidget(cb_bold, row+1, 2);
                group_layout->addWidget(cb_italic, row+1, 3);

                this->widgets[scheme_name][key] = SceditKey(clayout, cb_bold, cb_italic);
            }
        }

        QGroupBox* group_box = new QGroupBox(group_name);
        group_box->setLayout(group_layout);

        if (index == 0)
            h_layout->addWidget(group_box);
        else
            v_layout->addWidget(group_box);
    }
    h_layout->addLayout(v_layout);
    cs_layout->addLayout(h_layout);

    QWidget* stackitem = new QWidget;
    stackitem->setLayout(cs_layout);
    this->stack->addWidget(stackitem);
    this->order.append(scheme_name);
}

void SchemeEditor::delete_color_scheme_stack(const QString &scheme_name)
{
    this->set_scheme(scheme_name);
    QWidget* widget = this->stack->currentWidget();
    this->stack->removeWidget(widget);
    this->order.removeOne(scheme_name);
}
*/
