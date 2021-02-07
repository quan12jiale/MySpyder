#include "history.h"
#include "app/mainwindow.h"

HistoryLog::HistoryLog(MainWindow* parent)
    : SpyderPluginWidget (parent)
{
    this->CONF_SECTION = "historylog";

    this->shortcut = CONF_get("shortcuts", QString("_/switch to %1").arg(CONF_SECTION)).toString();
    this->toggle_view_action = nullptr;

    this->tabwidget = nullptr;
    this->menu_actions = QList<QAction*>();
    //self.dockviewer = None 源码所有地方都没有用到dockviewer
    this->wrap_action = nullptr;

    this->editors = QList<CodeEditor*>();
    this->filenames = QStringList();

    this->initialize_plugin();

    QVBoxLayout* layout = new QVBoxLayout;
    this->tabwidget = new Tabs(this, this->menu_actions);
    connect(tabwidget, SIGNAL(currentChanged(int)), SLOT(refresh_plugin()));
    connect(tabwidget, SIGNAL(move_data(int,int)), SLOT(move_tab(int,int)));

    layout->addWidget(this->tabwidget);

    QToolButton* options_button = new QToolButton(this);
    options_button->setText("Options");
    options_button->setIcon(ima::icon("tooloptions"));
    options_button->setPopupMode(QToolButton::InstantPopup);
    QMenu* menu = new QMenu(this);
    add_actions(menu, this->menu_actions);
    options_button->setMenu(menu);
    this->tabwidget->setCornerWidget(options_button);

    this->find_widget = new FindReplace(this);
    find_widget->hide();
    this->register_widget_shortcuts(this->find_widget);

    layout->addWidget(this->find_widget);

    this->setLayout(layout);
}

QString HistoryLog::get_plugin_title() const
{
    return "HistoryLog log";
}

QIcon HistoryLog::get_plugin_icon() const
{
    return ima::icon("HistoryLog");
}

QWidget* HistoryLog::get_focus_widget() const
{
    return this->tabwidget->currentWidget();
}

bool HistoryLog::closing_plugin(bool cancelable)
{
    Q_UNUSED(cancelable);
    return true;
}

void HistoryLog::refresh_plugin()
{
    CodeEditor* editor;
    if (this->tabwidget->count()) {
        editor = qobject_cast<CodeEditor*>(this->tabwidget->currentWidget());
    }
    else
        editor = nullptr;
    this->find_widget->set_editor(editor);
}

QList<QAction*> HistoryLog::get_plugin_actions()
{
    QAction* HistoryLog_action = new QAction(ima::icon("HistoryLog"), "HistoryLog...", this);
    HistoryLog_action->setToolTip("Set HistoryLog maximum entries");
    connect(HistoryLog_action, SIGNAL(triggered(bool)), SLOT(change_history_depth()));

    this->wrap_action = new QAction("Wrap lines", this);
    connect(wrap_action, SIGNAL(toggled(bool)), SLOT(toggle_wrap_mode(bool)));
    this->wrap_action->setCheckable(true);
    this->wrap_action->setChecked(this->get_option("wrap").toBool());

    this->menu_actions.clear();
    menu_actions << HistoryLog_action << wrap_action;
    return this->menu_actions;
}

void HistoryLog::on_first_registration()
{
    // TODO
    //this->main->tabify_plugins(this->main->ipyconsole, this);
}

void HistoryLog::initialize_plugin_in_mainwindow_layout()
{
    if (this->get_option("first_time").toBool()) {
        this->on_first_registration();
        this->set_option("first_time", false);
    }
}

void HistoryLog::register_plugin()
{
    connect(this, SIGNAL(focus_changed()), this->main, SLOT(plugin_focus_changed()));
    this->main->add_dockwidget(this);
    // TODO
    //self.main.console.shell.refresh.connect(self.refresh_plugin)
}

void HistoryLog::update_font()
{
    QHash<QString, ColorBoolBool> color_scheme = this->get_color_scheme();
    QFont font = this->get_plugin_font();
    foreach (CodeEditor* editor, this->editors) {
        editor->set_font(font, color_scheme);
    }
}
void HistoryLog::apply_plugin_settings(const QStringList &options)
{
    QString color_scheme_n = "color_scheme_name";
    QHash<QString, ColorBoolBool> color_scheme_o = this->get_color_scheme();
    QString font_n = "plugin_font";
    QFont font_o = this->get_plugin_font();
    QString wrap_n = "wrap";
    bool wrap_o = this->get_option(wrap_n).toBool();
    this->wrap_action->setChecked(wrap_o);
    foreach (CodeEditor* editor, this->editors) {
        if (options.contains(font_n)) {
            QHash<QString, ColorBoolBool> scs;
            if (options.contains(color_scheme_n))
                scs = color_scheme_o;
            editor->set_font(font_o, scs);
        }
        else if (options.contains(color_scheme_n))
            editor->set_color_scheme(color_scheme_o);
        if (options.contains(wrap_n))
            editor->toggle_wrap_mode(wrap_o);
    }
}

void HistoryLog::move_tab(int index_from, int index_to)
{
    QString filename = this->filenames.takeAt(index_from);
    CodeEditor* editor = this->editors.takeAt(index_from);

    this->filenames.insert(index_to, filename);
    this->editors.insert(index_to, editor);
}

void HistoryLog::add_history(const QString &filename)
{
    if (this->filenames.contains(filename))
        return;
    CodeEditor* editor = new CodeEditor(this);
    QString language;
    QFileInfo info(filename);
    if (info.suffix() == "py")
        language = "py";
    else
        language = "bat";

    QHash<QString, QVariant> kwargs;
    kwargs["linenumbers"] = false;
    kwargs["language"] = language;
    kwargs["scrollflagarea"] = false;
    editor->setup_editor(kwargs);

    connect(editor, SIGNAL(focus_changed()), this, SIGNAL(focus_changed()));
    editor->setReadOnly(true);
    QHash<QString, ColorBoolBool> color_scheme = this->get_color_scheme();
    editor->set_font(this->get_plugin_font(), color_scheme);
    editor->toggle_wrap_mode(this->get_option("wrap").toBool());

    QString text = encoding::read(filename);
    text = sourcecode::normalize_eols(text);
    QList<int> linebreaks;
    QRegularExpression re("\n");
    QRegularExpressionMatchIterator it = re.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        linebreaks.append(match.capturedStart());
    }
    int maxNline = this->get_option("max_entries").toInt();
    if (linebreaks.size() >maxNline) {
        int idx = linebreaks.size() - maxNline - 1;
        text = text.mid(linebreaks[idx]+1);
        encoding::write(text, filename);
    }
    editor->set_text(text);
    editor->set_cursor_position("eof");

    this->editors.append(editor);
    this->filenames.append(filename);
    int index = this->tabwidget->addTab(editor, info.fileName());
    this->find_widget->set_editor(editor);
    this->tabwidget->setTabToolTip(index, filename);
    this->tabwidget->setCurrentIndex(index);
}

void HistoryLog::append_to_history(const QString &filename, const QString &command)
{
    int index = this->filenames.indexOf(filename);
    Q_ASSERT(index >= 0);
    this->editors[index]->append(command);
    if (this->get_option("go_to_eof").toBool())
        this->editors[index]->set_cursor_position("eof");
    this->tabwidget->setCurrentIndex(index);
}

void HistoryLog::change_history_depth()
{
    bool valid;
    int depth = QInputDialog::getInt(this, "HistoryLog",
                                     "Maximum entries",
                                     this->get_option("max_entries").toInt(),
                                     10, 10000, 1, &valid);
    if (valid)
        this->set_option("max_entries", depth);
}

void HistoryLog::toggle_wrap_mode(bool checked)
{
    if (this->tabwidget == nullptr)
        return;
    foreach (CodeEditor* editor, this->editors) {
        editor->toggle_wrap_mode(checked);
    }
    this->set_option("wrap", checked);
}
