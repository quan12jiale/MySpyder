#include "plugins_editor.h"
#include "app/mainwindow.h"
#include "plugins/projects.h"

// load_breakpoints()函数的返回值是
//QList<QPair<int, QString> >

QHash<QString,QVariant> _load_all_breakpoints()
{
    QHash<QString,QVariant> bp_dict = CONF_get("run", "breakpoints",
                                               QHash<QString,QVariant>()).toHash();
    QList<QString> keys = bp_dict.keys();
    foreach (QString filename, keys) {
        QFileInfo info(filename);
        if (!info.isFile())
            bp_dict.remove(filename);
    }
    return bp_dict;
}

QList<QVariant> load_breakpoints(const QString& filename)
{
    QList<QVariant> breakpoints = _load_all_breakpoints().
            value(filename, QList<QVariant>()).toList();
    return breakpoints;
}

void save_breakpoints(const QString& filename, const QList<QVariant>& breakpoints)
{
    QFileInfo info(filename);
    if (!info.isFile())
        return;
    QHash<QString,QVariant> bp_dict = _load_all_breakpoints();
    bp_dict[filename] = breakpoints;
    CONF_set("run", "breakpoints", bp_dict);
}

void clear_all_breakpoints()
{
    CONF_set("run", "breakpoints", QHash<QString,QVariant>());
}

void clear_breakpoint(const QString& filename, int lineno)
{
    QList<QVariant> breakpoints = load_breakpoints(filename);
    if (!breakpoints.isEmpty()) {
        QList<QVariant> tmp = breakpoints;
        foreach (auto breakpoint, tmp) {
            QList<QVariant> pair = breakpoint.toList();
            if (pair[0].toInt() == lineno)
                breakpoints.removeOne(breakpoint);
        }
        save_breakpoints(filename, breakpoints);
    }
}


/********** Editor **********/
Editor::Editor(MainWindow *parent, bool ignore_last_opened_files)
    : SpyderPluginWidget (parent)
{
    Q_UNUSED(ignore_last_opened_files);
    this->CONF_SECTION = "editor";
    //CONFIGWIDGET_CLASS = EditorConfigPage  plugin.create_configwidget()函数需要使用
    this->TEMPFILE_PATH = get_conf_path("temp.py");
    this->TEMPLATE_PATH = get_conf_path("template.py");
    this->DISABLE_ACTIONS_WHEN_HIDDEN = false;//父类中为true

    this->shortcut = CONF_get("shortcuts", QString("_/switch to %1").arg(CONF_SECTION)).toString();
    this->toggle_view_action = nullptr;

    this->__set_eol_chars = true;

    // Creating template if it doesn't already exist
    QFileInfo info(this->TEMPLATE_PATH);
    if (!info.isFile()) {
        QStringList shebang;
        if (os::name != "nt")
            shebang.append("#!/usr/bin/env python3");
        shebang.append(QStringList({"# -*- coding: utf-8 -*-",
                                   "\"\"\"",
                                   "Created on %1",
                                   "",
                                   "@author: %2",
                                   "\"\"\"",
                                   ""}));
        QStringList header = shebang;
        encoding::write(header.join(os::linesep), this->TEMPLATE_PATH);
    }

    this->projects = nullptr;
    this->outlineexplorer = nullptr;
    //self.help = None

    this->file_dependent_actions.clear();
    this->pythonfile_dependent_actions.clear();
    this->dock_toolbar_actions.clear();
    this->edit_menu_actions.clear();
    this->stack_menu_actions.clear();

    this->__first_open_files_setup = true;
    this->editorstacks.clear();
    this->last_focus_editorstack.clear();
    this->editorwindows.clear();
    this->editorwindows_to_be_created.clear();
    this->toolbar_list.clear();
    this->menu_list.clear();

    this->initialize_plugin();

    this->dialog_size = QSize();

    QStatusBar* statusbar = this->main->statusBar();
    this->readwrite_status = new ReadWriteStatus(this, statusbar);
    this->eol_status = new EOLStatus(this, statusbar);
    this->encoding_status = new EncodingStatus(this, statusbar);
    this->cursorpos_status = new CursorPositionStatus(this, statusbar);

    QVBoxLayout* layout = new QVBoxLayout;
    this->dock_toolbar = new QToolBar(this);
    add_actions(this->dock_toolbar, this->dock_toolbar_actions);
    layout->addWidget(this->dock_toolbar);

    this->last_edit_cursor_pos.first = QString();
    this->cursor_pos_history.clear();
    this->cursor_pos_index = -1;
    this->__ignore_cursor_position = true;

    connect(this->main, SIGNAL(all_actions_defined()), this, SLOT(setup_other_windows()));
    connect(this->main, SIGNAL(sig_pythonpath_changed()), this, SLOT(set_path()));

    this->find_widget = new FindReplace(this, true);
    this->find_widget->hide();
    connect(find_widget, SIGNAL(visibility_changed(bool)), this, SLOT(rehighlight_cells()));
    this->register_widget_shortcuts(this->find_widget);

    QWidget* editor_widgets = new QWidget(this);
    QVBoxLayout* editor_layout = new QVBoxLayout;
    editor_layout->setContentsMargins(0, 0, 0, 0);
    editor_widgets->setLayout(editor_layout);
    this->editorsplitter = new EditorSplitter(this, this,
                                              stack_menu_actions,true);
    editor_layout->addWidget(this->editorsplitter);
    editor_layout->addWidget(this->find_widget);

    this->splitter = new QSplitter(this);
    this->splitter->setContentsMargins(0, 0, 0, 0);
    this->splitter->addWidget(editor_widgets);
    this->splitter->setStretchFactor(0, 5);
    this->splitter->setStretchFactor(1, 1);
    layout->addWidget(this->splitter);
    this->setLayout(layout);
    this->setFocusPolicy(Qt::ClickFocus);

    QString state = this->get_option("splitter_state", QString()).toString();
    if (!state.isEmpty()) {
        this->splitter->restoreState(QByteArray::fromHex(state.toUtf8()));
    }

    this->recent_files = this->get_option("recent_files", QStringList()).toStringList();
    this->untitled_num = 0;

    this->edit_filetypes.clear();
    this->edit_filters.clear();

    this->__ignore_cursor_position = false;
    CodeEditor* current_editor = this->get_current_editor();
    if (current_editor) {
        QString filename = this->get_current_filename();
        int position = current_editor->get_position("cursor");
        this->add_cursor_position_to_history(filename, position);
    }
    this->update_cursorpos_actions();
    this->set_path();
}

void Editor::set_projects(Projects *projects)
{
    this->projects = projects;
}

void Editor::show_hide_projects()
{
    if (this->projects) {
        SpyderDockWidget* dw = this->projects->dockwidget;
        if (dw->isVisible())
            dw->hide();
        else {
            dw->show();
            dw->raise();
        }
        this->switch_to_plugin();
    }
}

void Editor::set_outlineexplorer(OutlineExplorer *outlineexplorer)
{
    this->outlineexplorer = outlineexplorer;
    foreach (EditorStack* editorstack, this->editorstacks) {
        editorstack->set_outlineexplorer(this->outlineexplorer);
    }
    this->editorstacks[0]->initialize_outlineexplorer();
    connect(this->outlineexplorer, &OutlineExplorerWidget::edit_goto,
            [this](QString filenames, int _goto, QString word)
    {this->load(filenames, _goto, word, this);});
    connect(this->outlineexplorer, &OutlineExplorerWidget::edit,
            [this](QString filenames)
    {this->load(filenames, -1, "", this);});
}

//set_help

//------ Private API ------
void Editor::restore_scrollbar_position()
{
    try {
        this->get_current_editor()->centerCursor();
    } catch (...) {
    }
}

//------ SpyderPluginWidget API ------
QString Editor::get_plugin_title() const
{
    QString title = "Editor";
    if (this->dockwidget) {
        QString filename = this->get_current_filename();
        if (this->dockwidget->dock_tabbar) {
            if (!filename.isEmpty() && this->dockwidget->dock_tabbar->count() < 2)
                title += " - " + filename;
        }
        else
            title += " - " + filename;
    }
    return title;
}

QIcon Editor::get_plugin_icon() const
{
    return ima::icon("edit");
}

QWidget* Editor::get_focus_widget() const
{
    return this->get_current_editor();
}

void Editor::visibility_changed(bool enable)
{
    SpyderPluginWidget::visibility_changed(enable);
    if (this->dockwidget->isWindow())
        this->dock_toolbar->show();
    else
        this->dock_toolbar->hide();
    if (enable)
        this->refresh_plugin();
    emit update_plugin_title();
}

void Editor::refresh_plugin()
{
    EditorStack* editorstack  = this->get_current_editorstack();
    editorstack->refresh();
    this->refresh_save_all_action();
}

bool Editor::closing_plugin(bool cancelable)
{
    QByteArray state = this->splitter->saveState();
    this->set_option("splitter_state", qbytearray_to_str(state));
    QStringList filenames;
    EditorStack* editorstack = this->editorstacks[0];

    QString active_project_path;
    if (this->projects)
        active_project_path = this->projects->get_active_project_path();
    if (active_project_path.isEmpty())
        this->set_open_filenames();
    else {
        foreach (FileInfo* finfo, editorstack->data) {
            filenames.append(finfo->filename);
        }
        this->projects->set_project_filenames(filenames);
    }

    this->set_option("layout_settings", QVariant::fromValue(this->editorsplitter->get_layout_settings()));
    QList<QVariant> list;
    foreach (EditorMainWindow* win, this->editorwindows) {
        list.append(QVariant::fromValue(win->get_layout_settings()));
    }
    this->set_option("windows_layout_settings", list);
    this->set_option("recent_files", this->recent_files);
    try {
        if (!editorstack->save_if_changed(cancelable) && cancelable)
            return false;
        else {
            QList<EditorMainWindow*> tmp = this->editorwindows;
            foreach (EditorMainWindow* win, tmp) {
                win->close();
            }
            return true;
        }
    } catch (...) {
        return true;
    }
}

QList<QAction*> Editor::get_plugin_actions()
{
    this->new_action = new QAction("&New file...", this);
    this->new_action->setIcon(ima::icon("filenew"));
    this->new_action->setToolTip("New file");
    connect(new_action, &QAction::triggered,[=](){this->_new();});
    this->new_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(new_action, "Editor", "New file", true);

    this->open_last_closed_action = new QAction("O&pen last closed", this);
    this->open_last_closed_action->setToolTip("Open last closed");
    connect(open_last_closed_action, SIGNAL(triggered()), this, SLOT(open_last_closed()));
    this->register_shortcut(open_last_closed_action, "Editor", "Open last closed");

    this->open_action = new QAction("&Open...", this);
    this->open_action->setIcon(ima::icon("fileopen"));
    this->open_action->setToolTip("Open file");
    connect(open_action, SIGNAL(triggered()), this, SLOT(load()));
    this->open_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(open_action, "Editor", "Open file", true);

    this->revert_action = new QAction("&Revert", this);
    this->revert_action->setIcon(ima::icon("revert"));
    this->revert_action->setToolTip("Revert file from disk");
    connect(revert_action, SIGNAL(triggered(bool)), this, SLOT(revert()));

    this->save_action = new QAction("&Save", this);
    this->save_action->setIcon(ima::icon("filesave"));
    this->save_action->setToolTip("Save file");
    connect(save_action, &QAction::triggered,[=](){this->save();});
    this->save_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(save_action, "Editor", "Save file", true);

    this->save_all_action = new QAction("Sav&e all", this);
    this->save_all_action->setIcon(ima::icon("save_all"));
    this->save_all_action->setToolTip("Save all files");
    connect(save_all_action, SIGNAL(triggered()), this, SLOT(save_all()));
    this->save_all_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(save_all_action, "Editor", "Save all", true);

    QAction* save_as_action = new QAction("Save &as...", this);
    save_as_action->setIcon(ima::icon("filesaveas"));
    save_as_action->setToolTip("Save current file as...");
    connect(save_as_action, SIGNAL(triggered()), this, SLOT(save_as()));
    save_as_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(save_as_action, "Editor", "Save As");

    QAction* save_copy_as_action = new QAction("Save copy as...", this);
    save_copy_as_action->setIcon(ima::icon("filesaveas"));
    save_copy_as_action->setToolTip("Save copy of current file as...");
    connect(save_copy_as_action, SIGNAL(triggered(bool)), this, SLOT(save_copy_as()));

    QAction* print_preview_action = new QAction("Print preview...", this);
    print_preview_action->setToolTip("Print preview...");
    connect(print_preview_action, SIGNAL(triggered(bool)), this, SLOT(print_preview()));

    this->print_action = new QAction("&Print...", this);
    this->print_action->setIcon(ima::icon("print"));
    this->print_action->setToolTip("Print current file...");
    connect(print_action, SIGNAL(triggered()), this, SLOT(print_file()));

    this->close_action = new QAction("&Close", this);
    this->close_action->setIcon(ima::icon("fileclose"));
    this->close_action->setToolTip("Close current file");
    connect(close_action, SIGNAL(triggered()), this, SLOT(close_file()));

    this->close_all_action = new QAction("C&lose all", this);
    this->close_all_action->setIcon(ima::icon("filecloseall"));
    this->close_all_action->setToolTip("Close all opened files");
    connect(close_all_action, SIGNAL(triggered()), this, SLOT(close_all_files()));
    this->close_all_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(close_all_action, "Editor", "Close all");

    // ---- Find menu and toolbar ----
    QString _text = "&Find text";
    QAction* find_action = new QAction(_text, this);
    find_action->setIcon(ima::icon("find"));
    find_action->setToolTip(_text);
    connect(find_action, SIGNAL(triggered()), this, SLOT(find()));
    find_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(find_action, "_", "Find text", true);

    QAction* find_next_action = new QAction("Find &next", this);
    find_next_action->setIcon(ima::icon("findnext"));
    connect(find_next_action, SIGNAL(triggered()), this, SLOT(find_next()));
    find_next_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(find_next_action, "_", "Find next");

    QAction* find_previous_action = new QAction("Find &previous", this);
    find_previous_action->setIcon(ima::icon("findprevious"));
    connect(find_previous_action, SIGNAL(triggered()), this, SLOT(find_previous()));
    find_previous_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(find_previous_action, "_", "Find previous");

    _text = "&Replace text";
    QAction* replace_action = new QAction(_text, this);
    replace_action->setIcon(ima::icon("replace"));
    replace_action->setToolTip(_text);
    connect(replace_action, SIGNAL(triggered()), this, SLOT(replace()));
    replace_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(replace_action, "_", "Replace text");

    // ---- Debug menu and toolbar ----
    QAction* set_clear_breakpoint_action = new QAction("Set/Clear breakpoint", this);
    set_clear_breakpoint_action->setIcon(ima::icon("breakpoint_big"));
    connect(set_clear_breakpoint_action, SIGNAL(triggered()), this, SLOT(set_or_clear_breakpoint()));
    set_clear_breakpoint_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(set_clear_breakpoint_action, "Editor", "Breakpoint");

    QAction* set_cond_breakpoint_action = new QAction("Set/Edit conditional breakpoint", this);
    set_cond_breakpoint_action->setIcon(ima::icon("breakpoint_cond_big"));
    connect(set_cond_breakpoint_action, SIGNAL(triggered()), this, SLOT(set_or_edit_conditional_breakpoint()));
    set_cond_breakpoint_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(set_cond_breakpoint_action, "Editor", "Conditional breakpoint");

    QAction* clear_all_breakpoints_action = new QAction("Clear breakpoints in all files", this);
    connect(clear_all_breakpoints_action, SIGNAL(triggered()), this, SLOT(clear_all_breakpoints()));

    this->winpdb_action = new QAction("Debug with winpdb", this);
    connect(winpdb_action, SIGNAL(triggered()), this, SLOT(run_winpdb()));
    this->winpdb_action->setEnabled(false);

    // --- Debug toolbar ---
    QAction* debug_action = new QAction("&Debug", this);
    debug_action->setIcon(ima::icon("debug"));
    debug_action->setToolTip("Debug file");
    connect(debug_action, SIGNAL(triggered()), this, SLOT(debug_file()));
    this->register_shortcut(debug_action, "_", "Debug", true);

    QAction* debug_next_action = new QAction("Step", this);
    debug_next_action->setIcon(ima::icon("arrow-step-over"));
    debug_next_action->setToolTip("Run current line");
    connect(debug_next_action, &QAction::triggered, [=](){this->debug_command("next");});
    this->register_shortcut(debug_next_action, "_", "Debug Step Over", true);

    QAction* debug_continue_action = new QAction("Continue", this);
    debug_continue_action->setIcon(ima::icon("arrow-continue"));
    debug_continue_action->setToolTip("Continue execution until next breakpoint");
    connect(debug_continue_action, &QAction::triggered, [=](){this->debug_command("continue");});
    this->register_shortcut(debug_continue_action, "_", "Debug Continue", true);

    QAction* debug_step_action = new QAction("Step Into", this);
    debug_step_action->setIcon(ima::icon("arrow-step-in"));
    debug_step_action->setToolTip("Step into function or method of current line");
    connect(debug_step_action, &QAction::triggered, [=](){this->debug_command("step");});
    this->register_shortcut(debug_step_action, "_", "Debug Step Into", true);

    QAction* debug_return_action = new QAction("Step Return", this);
    debug_return_action->setIcon(ima::icon("arrow-step-out"));
    debug_return_action->setToolTip("Run until current function or method returns");
    connect(debug_return_action, &QAction::triggered, [=](){this->debug_command("return");});
    this->register_shortcut(debug_return_action, "_", "Debug Step Retuen", true);

    QAction* debug_exit_action = new QAction("Step", this);
    debug_exit_action->setIcon(ima::icon("stop_debug"));
    debug_exit_action->setToolTip("Stop debugging");
    connect(debug_exit_action, &QAction::triggered, [=](){this->debug_command("exit");});
    this->register_shortcut(debug_exit_action, "_", "Debug Exit", true);

    // --- Run toolbar ---
    QAction* run_action = new QAction("&Run", this);
    run_action->setIcon(ima::icon("run"));
    run_action->setToolTip("Run file");
    connect(run_action, &QAction::triggered, [=](){this->run_file();});
    this->register_shortcut(run_action, "_", "Run", true);

    QAction* configure_action = new QAction("&Configuration per file...", this);
    configure_action->setIcon(ima::icon("run_settings"));
    configure_action->setToolTip("Run settings");
    configure_action->setMenuRole(QAction::NoRole);
    connect(configure_action, SIGNAL(triggered()), this, SLOT(edit_run_configurations()));
    this->register_shortcut(configure_action, "_", "Configure", true);

    QAction* re_run_action = new QAction("Re-run &last script", this);
    re_run_action->setToolTip("Run again last file");
    connect(re_run_action, SIGNAL(triggered()), this, SLOT(re_run_file()));
    this->register_shortcut(re_run_action, "_", "Re-run last script", true);

    QAction* run_selected_action = new QAction("Run &selection or current line", this);
    run_selected_action->setIcon(ima::icon("run_selection"));
    run_selected_action->setToolTip("Run selection or current line");
    connect(run_selected_action, SIGNAL(triggered()), this, SLOT(run_selection()));
    this->register_shortcut(re_run_action, "Editor", "Run selection", true);

    QAction* run_cell_action = new QAction("Run cell", this);
    run_cell_action->setIcon(ima::icon("run_cell"));
    run_cell_action->setShortcut(QKeySequence("Ctrl+Return"));
    run_cell_action->setToolTip("Run current cell (Ctrl+Enter)\n"
                                "[Use #%% to create cells]");
    connect(run_cell_action, SIGNAL(triggered()), this, SLOT(run_cell()));
    run_cell_action->setShortcutContext(Qt::WidgetShortcut);

    QAction* run_cell_advance_action = new QAction("Run cell and advance", this);
    run_cell_advance_action->setIcon(ima::icon("run_cell_advance"));
    run_cell_advance_action->setShortcut(QKeySequence("Shift+Return"));
    run_cell_advance_action->setToolTip("Run current cell and go to the next one "
                                "(Shift+Enter)");
    connect(run_cell_advance_action, SIGNAL(triggered()), this, SLOT(run_cell_and_advance()));
    run_cell_advance_action->setShortcutContext(Qt::WidgetShortcut);

    QAction* re_run_last_cell_action = new QAction("Re-run last cell", this);
    re_run_last_cell_action->setToolTip("Re run last cell ");
    connect(re_run_last_cell_action, SIGNAL(triggered()), this, SLOT(re_run_last_cell()));
    re_run_last_cell_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(re_run_last_cell_action, "Editor", "re-run last cell", true);

    // --- Source code Toolbar ---
    this->todo_list_action = new QAction("Show todo list", this);
    this->todo_list_action->setIcon(ima::icon("todo_list"));
    this->todo_list_action->setToolTip("Show comments list (TODO/FIXME/XXX/HINT/TIP/@todo/"
                                       "HACK/BUG/OPTIMIZE/!!!/?? ?)");
    connect(todo_list_action, SIGNAL(triggered()), this, SLOT(go_to_next_todo()));

    this->todo_menu = new QMenu(this);
    this->todo_list_action->setMenu(this->todo_menu);
    connect(todo_menu, SIGNAL(aboutToShow()), this, SLOT(update_todo_menu()));

    this->warning_list_action = new QAction("Show warning/error list", this);
    this->warning_list_action->setIcon(ima::icon("wng_list"));
    this->warning_list_action->setToolTip("Show code analysis warnings/errors");
    connect(warning_list_action, SIGNAL(triggered()), this, SLOT(go_to_next_warning()));

    this->warning_menu = new QMenu(this);
    this->warning_list_action->setMenu(this->warning_menu);
    connect(warning_menu, SIGNAL(aboutToShow()), this, SLOT(update_warning_menu()));

    this->previous_warning_action = new QAction("Previous warning/error", this);
    this->previous_warning_action->setIcon(ima::icon("prev_wng"));
    this->previous_warning_action->setToolTip("Go to previous code analysis warning/error");
    connect(previous_warning_action, SIGNAL(triggered()), this, SLOT(go_to_previous_warning()));

    this->next_warning_action = new QAction("Next warning/error", this);
    this->next_warning_action->setIcon(ima::icon("next_wng"));
    this->next_warning_action->setToolTip("Go to next code analysis warning/error");
    connect(next_warning_action, SIGNAL(triggered()), this, SLOT(go_to_next_warning()));

    this->previous_edit_cursor_action = new QAction("Last edit location", this);
    this->previous_edit_cursor_action->setIcon(ima::icon("last_edit_location"));
    this->previous_edit_cursor_action->setToolTip("Go to last edit location");
    connect(previous_edit_cursor_action, SIGNAL(triggered()), this, SLOT(go_to_last_edit_location()));
    this->previous_edit_cursor_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(previous_edit_cursor_action, "Editor", "Last edit location", true);

    this->previous_cursor_action = new QAction("Previous cursor position", this);
    this->previous_cursor_action->setIcon(ima::icon("prev_cursor"));
    this->previous_cursor_action->setToolTip("Go to previous cursor position");
    connect(previous_cursor_action, SIGNAL(triggered()), this, SLOT(go_to_previous_cursor_position()));
    this->previous_cursor_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(previous_cursor_action, "Editor", "Previous cursor position", true);

    this->next_cursor_action = new QAction("Next cursor position", this);
    this->next_cursor_action->setIcon(ima::icon("next_cursor"));
    this->next_cursor_action->setToolTip("Go to next cursor position");
    connect(next_cursor_action, SIGNAL(triggered()), this, SLOT(go_to_next_cursor_position()));
    this->next_cursor_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(next_cursor_action, "Editor", "Next cursor position", true);

    // --- Edit Toolbar ---
    this->toggle_comment_action = new QAction("Comment/Uncomment", this);
    this->toggle_comment_action->setIcon(ima::icon("comment"));
    this->toggle_comment_action->setToolTip("Comment current line or selection");
    connect(toggle_comment_action, SIGNAL(triggered()), this, SLOT(toggle_comment()));
    this->toggle_comment_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(toggle_comment_action, "Editor", "Toggle comment");

    QAction* blockcomment_action = new QAction("Add &block comment", this);
    blockcomment_action->setToolTip("Add block comment around current line or selection");
    connect(blockcomment_action, SIGNAL(triggered()), this, SLOT(blockcomment()));
    blockcomment_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(blockcomment_action, "Editor", "Blockcomment");

    QAction* unblockcomment_action = new QAction("R&emove block comment", this);
    unblockcomment_action->setToolTip("Remove comment block around current line or selection");
    connect(unblockcomment_action, SIGNAL(triggered()), this, SLOT(unblockcomment()));
    unblockcomment_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(unblockcomment_action, "Editor", "UnBlockcomment");

    // The following action shortcuts are hard-coded in CodeEditor
    this->indent_action = new QAction("Indent", this);
    //this->indent_action->setShortcut("Tab");   //注意这里
    this->indent_action->setIcon(ima::icon("indent"));
    this->indent_action->setToolTip("Indent current line or selection");
    connect(indent_action, SIGNAL(triggered()), this, SLOT(indent()));
    this->indent_action->setShortcutContext(Qt::WidgetShortcut);

    this->unindent_action = new QAction("Unindent", this);
    this->unindent_action->setIcon(ima::icon("unindent"));
    this->unindent_action->setToolTip("Unindent current line or selection");
    connect(unindent_action, SIGNAL(triggered()), this, SLOT(unindent()));
    this->unindent_action->setShortcutContext(Qt::WidgetShortcut);

    this->text_uppercase_action = new QAction("Toggle Uppercase", this);
    this->text_uppercase_action->setToolTip("Change to uppercase current line or selection");
    connect(text_uppercase_action, SIGNAL(triggered()), this, SLOT(text_uppercase()));
    this->text_uppercase_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(text_uppercase_action, "Editor", "transform to uppercase");

    this->text_lowercase_action = new QAction("Toggle Lowercase", this);
    this->text_lowercase_action->setToolTip("Change to lowercase current line or selection");
    connect(text_lowercase_action, SIGNAL(triggered()), this, SLOT(text_lowercase()));
    this->text_lowercase_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(text_lowercase_action, "Editor", "transform to lowercase");

    // -----------------------
    this->win_eol_action = new QAction("Carriage return and line feed (Windows)", this);
    connect(win_eol_action, &QAction::toggled,
            [this](bool checked){this->toggle_eol_chars("nt", checked);});
    win_eol_action->setCheckable(true);

    this->linux_eol_action = new QAction("Line feed (UNIX)", this);
    connect(linux_eol_action, &QAction::toggled,
            [this](bool checked){this->toggle_eol_chars("posix", checked);});
    linux_eol_action->setCheckable(true);

    this->mac_eol_action = new QAction("Carriage return (Mac)", this);
    connect(mac_eol_action, &QAction::toggled,
            [this](bool checked){this->toggle_eol_chars("mac", checked);});
    mac_eol_action->setCheckable(true);

    QActionGroup* eol_action_group = new QActionGroup(this);
    QList<QAction*> eol_actions;
    eol_actions << win_eol_action << linux_eol_action << mac_eol_action;
    add_actions(eol_action_group, eol_actions);
    QMenu* eol_menu = new QMenu("Convert end-of-line characters", this);
    add_actions(eol_menu, eol_actions);

    QAction* trailingspaces_action = new QAction("Remove trailing spaces", this);
    connect(trailingspaces_action, SIGNAL(triggered()), this, SLOT(remove_trailing_spaces()));

    this->showblanks_action = new QAction("Show blank spaces", this);
    // 下面这个连接好像会产生bug
    connect(showblanks_action, SIGNAL(toggled(bool)), this, SLOT(toggle_show_blanks(bool)));
    showblanks_action->setCheckable(true);

    QAction* fixindentation_action = new QAction("Fix indentation", this);
    fixindentation_action->setToolTip("Replace tab characters by space characters");
    connect(fixindentation_action, SIGNAL(triggered()), this, SLOT(fix_indentation()));

    QAction* gotoline_action = new QAction("Go to line...", this);
    gotoline_action->setIcon(ima::icon("gotoline"));
    connect(gotoline_action, &QAction::triggered, [this](){this->go_to_line();});
    gotoline_action->setShortcutContext(Qt::WidgetShortcut);
    this->register_shortcut(gotoline_action, "Editor", "Go to line");

    QAction* workdir_action = new QAction("Set console working directory", this);
    workdir_action->setIcon(ima::icon("DirOpenIcon"));
    workdir_action->setToolTip("Set current console (and file explorer) working "
                               "directory to current script directory");
    connect(workdir_action, SIGNAL(triggered()), this, SLOT(__set_workdir()));

    this->max_recent_action = new QAction("Maximum number of recent files...", this);
    connect(max_recent_action, SIGNAL(triggered()), this, SLOT(change_max_recent_files()));

    this->clear_recent_action = new QAction("Clear this list", this);
    this->clear_recent_action->setToolTip("Clear recent files list");
    connect(clear_recent_action, SIGNAL(triggered()), this, SLOT(clear_recent_files()));

    // ---- File menu/toolbar construction ----
    this->recent_file_menu = new QMenu("Open &recent", this);
    connect(recent_file_menu, SIGNAL(aboutToShow()), this, SLOT(update_recent_file_menu()));

    QList<QObject*> file_menu_actions;
    file_menu_actions << new_action << nullptr
                      << open_action << open_last_closed_action << recent_file_menu
                      << nullptr << nullptr << save_action << save_all_action
                      << save_as_action << save_copy_as_action << revert_action
                      << nullptr << print_preview_action << print_action << nullptr
                      << close_action << close_all_action << nullptr;
    this->main->file_menu_actions.append(file_menu_actions);
    QList<QAction*> file_toolbar_actions;
    file_toolbar_actions << new_action << open_action
                         << save_action << save_all_action;
    file_toolbar_actions.append(this->main->file_toolbar_actions);
    this->main->file_toolbar_actions = file_toolbar_actions;

    this->main->search_menu_actions.clear();
    this->main->search_menu_actions << find_action << find_next_action
                                    << find_previous_action << replace_action;
    this->main->search_toolbar_actions.clear();
    this->main->search_toolbar_actions << find_action << find_next_action
                                       << replace_action;

    this->edit_menu_actions.clear();
    this->edit_menu_actions << toggle_comment_action << blockcomment_action
                            << unblockcomment_action
                            << indent_action << unindent_action
                            << text_uppercase_action
                            << text_lowercase_action;
    this->main->edit_menu_actions << nullptr;
    this->main->edit_menu_actions.append(this->edit_menu_actions);
    QList<QAction*> edit_toolbar_actions;
    edit_toolbar_actions << toggle_comment_action
                         << unindent_action << indent_action;
    this->main->edit_toolbar_actions.append(edit_toolbar_actions);

    this->main->search_menu_actions.append(gotoline_action);
    this->main->search_toolbar_actions.append(gotoline_action);

    // ---- Run menu/toolbar construction ----
    QList<QObject*> run_menu_actions;
    run_menu_actions << run_action << run_cell_action
                     << run_cell_advance_action
                     << re_run_last_cell_action << nullptr
                     << run_selected_action << re_run_action
                     << configure_action << nullptr;
    this->main->run_menu_actions.append(run_menu_actions);

    QList<QAction*> run_toolbar_actions;
    run_toolbar_actions << run_action << run_cell_action
                        << run_cell_advance_action << run_selected_action
                        << re_run_action;
    this->main->run_toolbar_actions.append(run_toolbar_actions);

    // 'list_breakpoints' is used by the breakpoints
    // plugin to add its "List breakpoints" action to this menu
    QList<QObject*> debug_menu_actions; //中间有一个list_breakpoints
    debug_menu_actions << debug_action
                       << debug_next_action
                       << debug_step_action
                       << debug_return_action
                       << debug_continue_action
                       << debug_exit_action
                       << nullptr
                       << set_clear_breakpoint_action
                       << set_cond_breakpoint_action
                       << clear_all_breakpoints_action
                       << nullptr
                       << this->winpdb_action;
    this->main->debug_menu_actions.append(debug_menu_actions);
    QList<QAction*> debug_toolbar_actions;
    debug_toolbar_actions << debug_action << debug_next_action
                          << debug_step_action << debug_return_action
                          << debug_continue_action << debug_exit_action;
    this->main->debug_toolbar_actions.append(debug_toolbar_actions);

    // ---- Source menu/toolbar construction ----
    QList<QObject*> source_menu_actions;
    source_menu_actions << eol_menu << showblanks_action
                        << trailingspaces_action << fixindentation_action
                        << nullptr << todo_list_action << warning_list_action
                        << previous_warning_action << next_warning_action
                        << nullptr << previous_edit_cursor_action
                        << previous_cursor_action << next_cursor_action;
    this->main->source_menu_actions .append(source_menu_actions);

    QList<QAction*> source_toolbar_actions;
    source_toolbar_actions << todo_list_action
                           << warning_list_action
                           << previous_warning_action
                           << next_warning_action
                           << nullptr
                           << previous_edit_cursor_action
                           << previous_cursor_action
                           << next_cursor_action;
    this->main->source_toolbar_actions.append(source_toolbar_actions);

    // ---- Dock widget and file dependent actions ----
    this->dock_toolbar_actions.clear();
    this->dock_toolbar_actions.append(file_toolbar_actions);
    this->dock_toolbar_actions.append(nullptr);
    this->dock_toolbar_actions.append(source_toolbar_actions);
    this->dock_toolbar_actions.append(nullptr);
    this->dock_toolbar_actions.append(run_toolbar_actions);
    this->dock_toolbar_actions.append(nullptr);
    this->dock_toolbar_actions.append(debug_toolbar_actions);
    this->dock_toolbar_actions.append(nullptr);
    this->dock_toolbar_actions.append(edit_toolbar_actions);

    this->pythonfile_dependent_actions.clear();
    this->pythonfile_dependent_actions << run_action << configure_action
                                       << set_clear_breakpoint_action
                                       << set_cond_breakpoint_action
                                       << debug_action << run_selected_action
                                       << run_cell_action
                                       << run_cell_advance_action
                                       << re_run_last_cell_action
                                       << blockcomment_action
                                       << unblockcomment_action
                                       << this->winpdb_action;

    this->cythonfile_compatible_actions.clear();
    this->cythonfile_compatible_actions << run_action << configure_action;

    this->file_dependent_actions.clear();
    this->file_dependent_actions.append(this->pythonfile_dependent_actions);
    this->file_dependent_actions << save_action << save_as_action << save_copy_as_action
                                 << print_preview_action << print_action
                                 << save_all_action << gotoline_action << workdir_action
                                 << close_action << close_all_action
                                 << toggle_comment_action << revert_action
                                 << indent_action << unindent_action;

    this->stack_menu_actions.clear();
    this->stack_menu_actions << gotoline_action << workdir_action;

    return this->file_dependent_actions;
}

void Editor::register_plugin()
{
    connect(this->main, SIGNAL(restore_scrollbar_position()),
            this, SLOT(restore_scrollbar_position()));
    // main.console.edit_goto.connect(self.load)
    connect(this, SIGNAL(exec_in_extconsole(const QString&, bool)),
            this->main, SLOT(execute_in_external_console(const QString&, bool)));
    connect(this, SIGNAL(redirect_stdio(bool)),
            this->main, SLOT(redirect_internalshell_stdio(bool)));
    connect(this, &Editor::open_dir,
            [this](QString dir){this->main->workingdirectory->chdir(dir);});

    //set_help(self.main.help)
    if (this->main->outlineexplorer)
        this->set_outlineexplorer(this->main->outlineexplorer);
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack->data.isEmpty())
        this->__load_temp_file();//如果data为空，加载临时文件
    this->main->add_dockwidget(this);
    this->main->add_to_fileswitcher(this, editorstack->tabs,
                                    editorstack->data, ima::icon("TextFileIcon"));
}

void Editor::update_font()
{
    QFont font = this->get_plugin_font();
    QHash<QString, ColorBoolBool> color_scheme = this->get_color_scheme();
    foreach (EditorStack* editorstack, this->editorstacks) {
        editorstack->set_default_font(font, color_scheme);
        QSize completion_size = CONF_get("main", "completion/size").toSize();
        foreach (FileInfo* finfo, editorstack->data) {
            CompletionWidget* comp_widget = finfo->editor->completion_widget;
            comp_widget->setup_appearance(completion_size, font);
        }
    }
}

//------ Focus tabwidget
EditorStack* Editor::__get_focus_editorstack() const
{
    QWidget* fwidget = QApplication::focusWidget();
    EditorStack* editorstack = qobject_cast<EditorStack*>(fwidget);
    if (editorstack) {

        return editorstack;
    }
    else {
        foreach (EditorStack* editorstack, this->editorstacks) {
            if (editorstack->isAncestorOf(fwidget))
                return editorstack;
        }
    }
    return nullptr;
}

void Editor::set_last_focus_editorstack(QWidget *editorwindow, EditorStack *editorstack)
{
    this->last_focus_editorstack[editorwindow] = editorstack;
    this->last_focus_editorstack[nullptr] = editorstack;
}

EditorStack* Editor::get_last_focus_editorstack(EditorMainWindow *editorwindow) const
{
    return this->last_focus_editorstack[editorwindow];
}

void Editor::remove_last_focus_editorstack(EditorStack *editorstack)
{
    for (auto it=this->last_focus_editorstack.begin();it!=this->last_focus_editorstack.end();it++) {
        QWidget* editorwindow = it.key();
        EditorStack* widget = it.value();
        if (widget == editorstack)
            this->last_focus_editorstack[editorwindow] = nullptr;
    }
}

void Editor::save_focus_editorstack()
{
    EditorStack* editorstack = this->__get_focus_editorstack();
    if (editorstack != nullptr) {
        if (this->isAncestorOf(editorstack))
            this->set_last_focus_editorstack(this, editorstack);
        foreach (EditorMainWindow* win, this->editorwindows) {
            if (win->isAncestorOf(editorstack))
                this->set_last_focus_editorstack(win, editorstack);
        }
    }
}

// ------ Handling editorstacks
void Editor::register_editorstack(EditorStack *editorstack)
{
    this->editorstacks.append(editorstack);
    this->register_widget_shortcuts(editorstack);
    if (this->editorstacks.size() > 1 && this->main != nullptr) {
        //self.main.fileswitcher.sig_goto_file.connect(
    }

    if (this->isAncestorOf(editorstack)) {
        // editorstack is a child of the Editor plugin
        this->set_last_focus_editorstack(this, editorstack);
        editorstack->set_closable(this->editorstacks.size() > 1);
        if (this->outlineexplorer != nullptr)
            editorstack->set_outlineexplorer(this->outlineexplorer);
        editorstack->set_find_widget(this->find_widget);
        connect(editorstack, SIGNAL(reset_statusbar()), readwrite_status, SLOT(hide()));
        connect(editorstack, SIGNAL(reset_statusbar()), encoding_status, SLOT(hide()));
        connect(editorstack, SIGNAL(reset_statusbar()), cursorpos_status, SLOT(hide()));

        connect(editorstack, SIGNAL(readonly_changed(bool)), readwrite_status, SLOT(readonly_changed(bool)));
        connect(editorstack, SIGNAL(encoding_changed(const QString&)), encoding_status, SLOT(encoding_changed(const QString&)));
        connect(editorstack, SIGNAL(sig_editor_cursor_position_changed(int,int)),
                cursorpos_status, SLOT(cursor_position_changed(int,int)));
        connect(editorstack, SIGNAL(sig_refresh_eol_chars(const QString&)), eol_status, SLOT(eol_changed(const QString&)));
    }
    //editorstack.set_help(self.help)
    editorstack->set_io_actions(new_action, open_action, save_action, revert_action);
    editorstack->set_tempfile_path(this->TEMPFILE_PATH);
    //editorstack.set_introspector(self.introspector)

    editorstack->set_pyflakes_enabled(this->get_option("code_analysis/pyflakes").toBool());
    editorstack->set_pep8_enabled(this->get_option("code_analysis/pep8").toBool());
    editorstack->set_todolist_enabled(this->get_option("todo_list").toBool());
    editorstack->set_realtime_analysis_enabled(this->get_option("realtime_analysis").toBool());
    editorstack->set_realtime_analysis_timeout(this->get_option("realtime_analysis/timeout").toInt());
    editorstack->set_blanks_enabled(this->get_option("blank_spaces").toBool());
    editorstack->set_linenumbers_enabled(this->get_option("line_numbers").toBool());
    editorstack->set_edgeline_enabled(this->get_option("edge_line").toBool());
    editorstack->set_edgeline_column(this->get_option("edge_line_column").toInt());
    editorstack->set_codecompletion_auto_enabled(this->get_option("codecompletion/auto").toBool());
    editorstack->set_codecompletion_case_enabled(this->get_option("codecompletion/case_sensitive").toBool());
    editorstack->set_codecompletion_enter_enabled(this->get_option("codecompletion/enter_key").toBool());
    editorstack->set_calltips_enabled(this->get_option("calltips").toBool());
    editorstack->set_go_to_definition_enabled(this->get_option("go_to_definition").toBool());
    editorstack->set_focus_to_editor(this->get_option("focus_to_editor").toBool());
    editorstack->set_close_parentheses_enabled(this->get_option("close_parentheses").toBool());
    editorstack->set_close_quotes_enabled(this->get_option("close_quotes").toBool());
    editorstack->set_add_colons_enabled(this->get_option("add_colons").toBool());
    editorstack->set_auto_unindent_enabled(this->get_option("auto_unindent").toBool());
    editorstack->set_indent_chars(this->get_option("indent_chars").toString());
    editorstack->set_tab_stop_width_spaces(this->get_option("tab_stop_width_spaces").toInt());
    editorstack->set_wrap_enabled(this->get_option("wrap").toBool());
    editorstack->set_tabmode_enabled(this->get_option("tab_always_indent").toBool());
    editorstack->set_intelligent_backspace_enabled(this->get_option("intelligent_backspace").toBool());
    editorstack->set_highlight_current_line_enabled(this->get_option("highlight_current_line").toBool());
    editorstack->set_highlight_current_cell_enabled(this->get_option("highlight_current_cell").toBool());
    editorstack->set_occurrence_highlighting_enabled(this->get_option("occurrence_highlighting").toBool());
    editorstack->set_occurrence_highlighting_timeout(this->get_option("occurrence_highlighting/timeout").toInt());
    editorstack->set_checkeolchars_enabled(this->get_option("check_eol_chars").toBool());
    editorstack->set_tabbar_visible(this->get_option("show_tab_bar").toBool());
    editorstack->set_always_remove_trailing_spaces(this->get_option("always_remove_trailing_spaces").toBool());

    editorstack->set_help_enabled(CONF_get("help", "connect/editor").toBool());
    QHash<QString,ColorBoolBool> color_scheme = this->get_color_scheme();
    editorstack->set_default_font(this->get_plugin_font(), color_scheme);

    connect(editorstack, SIGNAL(starting_long_process(const QString&)),
            this, SLOT(starting_long_process(const QString&)));
    connect(editorstack, SIGNAL(ending_long_process(const QString&)),
            this, SLOT(ending_long_process(const QString&)));

    connect(editorstack, SIGNAL(redirect_stdio(bool)), this, SIGNAL(redirect_stdio(bool)));
    connect(editorstack, SIGNAL(exec_in_extconsole(const QString&, bool)),
            this, SIGNAL(exec_in_extconsole(const QString&, bool)));
    connect(editorstack, SIGNAL(update_plugin_title()), this, SIGNAL(update_plugin_title()));

    connect(editorstack, SIGNAL(editor_focus_changed()), this, SLOT(save_focus_editorstack()));
    connect(editorstack, SIGNAL(editor_focus_changed()), this, SLOT(set_editorstack_for_introspection()));
    connect(editorstack, SIGNAL(editor_focus_changed()), this->main, SLOT(plugin_focus_changed()));

    connect(editorstack, &EditorStack::zoom_in, [this](){this->zoom(1);});
    connect(editorstack, &EditorStack::zoom_out, [this](){this->zoom(-1);});
    connect(editorstack, &EditorStack::zoom_reset, [this](){this->zoom(0);});

    connect(editorstack, static_cast<void (EditorStack::*)(QString)>(&EditorStack::sig_new_file),
            [this](QString s){this->_new(QString(), nullptr, s);});
    connect(editorstack, static_cast<void (EditorStack::*)()>(&EditorStack::sig_new_file),
            [this](){this->_new();});
    connect(editorstack, SIGNAL(sig_close_file(const QString&, const QString&)),
            this, SLOT(close_file_in_all_editorstacks(const QString&, const QString&)));
    connect(editorstack, SIGNAL(file_saved(QString, QString, QString)),
            this, SLOT(file_saved_in_editorstack(QString, QString, QString)));
    connect(editorstack, SIGNAL(file_renamed_in_data(QString, QString, QString)),
            this, SLOT(file_renamed_in_data_in_editorstack(QString, QString, QString)));

    connect(editorstack, SIGNAL(create_new_window()), this,SLOT(create_new_window()));
    connect(editorstack, SIGNAL(opened_files_list_changed()), this, SLOT(opened_files_list_changed()));
    connect(editorstack, SIGNAL(analysis_results_changed()), this, SLOT(analysis_results_changed()));
    connect(editorstack, SIGNAL(todo_results_changed()), this, SLOT(todo_results_changed()));

    connect(editorstack, SIGNAL(update_code_analysis_actions()), this, SLOT(update_code_analysis_actions()));
    connect(editorstack, SIGNAL(update_code_analysis_actions()), this, SLOT(update_todo_actions()));
    connect(editorstack, SIGNAL(refresh_file_dependent_actions()), this, SLOT(refresh_file_dependent_actions()));
    connect(editorstack, SIGNAL(refresh_save_all_action()), this, SLOT(refresh_save_all_action()));

    connect(editorstack, SIGNAL(sig_refresh_eol_chars(QString)),
            this, SLOT(refresh_eol_chars(QString)));
    connect(editorstack, SIGNAL(save_breakpoints(const QString&, const QString&)),
            this, SLOT(save_breakpoints(const QString&, const QString&)));
    connect(editorstack, SIGNAL(text_changed_at(const QString&, int)),
            this, SLOT(text_changed_at(const QString&, int)));
    connect(editorstack, SIGNAL(current_file_changed(const QString&, int)),
            this, SLOT(current_file_changed(const QString&, int)));

    connect(editorstack, static_cast<void (EditorStack::*)(QString)>(&EditorStack::plugin_load),
            [this](QString fname){this->load(fname);});
    connect(editorstack, static_cast<void (EditorStack::*)()>(&EditorStack::plugin_load),
            [this](){this->load();});
    connect(editorstack, &EditorStack::edit_goto,
            [this](QString fname, int _goto, QString word){this->load(fname, _goto, word);});

    connect(editorstack, SIGNAL(sig_save_as()), this, SLOT(save_as()));
    connect(editorstack, SIGNAL(sig_prev_edit_pos()), this, SLOT(go_to_last_edit_location()));
    connect(editorstack, SIGNAL(sig_prev_cursor()), this, SLOT(go_to_previous_cursor_position()));
    connect(editorstack, SIGNAL(sig_next_cursor()), this, SLOT(go_to_next_cursor_position()));
}

bool Editor::unregister_editorstack(EditorStack* editorstack)
{
    this->remove_last_focus_editorstack(editorstack);
    if (this->editorstacks.size() > 1) {
        this->editorstacks.removeOne(editorstack);
        return true;
    }
    else
        return false;
}

void Editor::clone_editorstack(EditorStack *editorstack)
{
    editorstack->clone_from(this->editorstacks[0]);
    foreach (FileInfo* finfo, editorstack->data) {
        this->register_widget_shortcuts(finfo->editor);
    }
}

void Editor::close_file_in_all_editorstacks(const QString &editorstack_id_str, const QString &filename)
{
    foreach (EditorStack* editorstack, this->editorstacks) {
        size_t id = reinterpret_cast<size_t>(editorstack);
        if (QString::number(id) != editorstack_id_str) {
            editorstack->blockSignals(true);
            int index = editorstack->get_index_from_filename(filename);
            editorstack->close_file(index, true);
            editorstack->blockSignals(false);
        }
    }
}

void Editor::file_saved_in_editorstack(const QString &editorstack_id_str,
                                       const QString &original_filename, const QString &filename)
{
    foreach (EditorStack* editorstack, this->editorstacks) {
        size_t id = reinterpret_cast<size_t>(editorstack);
        if (QString::number(id) != editorstack_id_str) {
            editorstack->file_saved_in_other_editorstack(original_filename, filename);
        }
    }
}

void Editor::file_renamed_in_data_in_editorstack(const QString &editorstack_id_str,
                                                 const QString &original_filename, const QString &filename)
{
    foreach (EditorStack* editorstack, this->editorstacks) {
        size_t id = reinterpret_cast<size_t>(editorstack);
        if (QString::number(id) != editorstack_id_str) {
            editorstack->rename_in_data(original_filename, filename);
        }
    }
}

void Editor::set_editorstack_for_introspection()
{
    EditorStack* editorstack = this->__get_focus_editorstack();
    if (editorstack != nullptr) {
        // introspector.set_editor_widget(editorstack)
        //introspector是utils/introspection/manager.py中IntrospectionManager类

        // TODO
    }
}

void Editor::setup_other_windows()
{
    this->toolbar_list.clear();
    toolbar_list.append(StrStrActions("File toolbar", "file_toolbar", this->main->file_toolbar_actions));
    QList<QAction*> actions;
    foreach (QObject* object, this->main->search_menu_actions) {
        actions.append(qobject_cast<QAction*>(object));
    }
    toolbar_list.append(StrStrActions("Search toolbar", "search_toolbar", actions));
    toolbar_list.append(StrStrActions("Source toolbar", "source_toolbar", this->main->source_toolbar_actions));
    toolbar_list.append(StrStrActions("Run toolbar", "run_toolbar", this->main->run_toolbar_actions));
    toolbar_list.append(StrStrActions("Debug toolbar", "debug_toolbar", this->main->debug_toolbar_actions));
    toolbar_list.append(StrStrActions("Edit toolbar", "edit_toolbar", this->main->edit_toolbar_actions));

    this->menu_list.clear();
    menu_list.append(QPair<QString,QList<QObject*>>("&File", this->main->file_menu_actions));
    menu_list.append(QPair<QString,QList<QObject*>>("&Edit", this->main->edit_menu_actions));
    menu_list.append(QPair<QString,QList<QObject*>>("&Search", this->main->search_menu_actions));
    menu_list.append(QPair<QString,QList<QObject*>>("Sour&ce", this->main->source_menu_actions));
    menu_list.append(QPair<QString,QList<QObject*>>("&Run", this->main->run_menu_actions));
    menu_list.append(QPair<QString,QList<QObject*>>("&Tools", this->main->tools_menu_actions));
    menu_list.append(QPair<QString,QList<QObject*>>("&View", QList<QObject*>()));
    menu_list.append(QPair<QString,QList<QObject*>>("&Help", this->main->help_menu_actions));


    foreach (LayoutSettings layout_settings, this->editorwindows_to_be_created) {
        EditorMainWindow* win = this->create_new_window();
        win->set_layout_settings(layout_settings);
    }
}

EditorMainWindow* Editor::create_new_window()
{
    QHash<QString, QVariant> oe_options = this->outlineexplorer->get_options();
    bool show_fullpath = oe_options["show_fullpath"].toBool();
    bool show_all_files = oe_options["show_all_files"].toBool();
    bool show_comments = oe_options["show_comments"].toBool();
    EditorMainWindow* window = new EditorMainWindow(this, stack_menu_actions,
                                                    toolbar_list, menu_list,
                                                    show_fullpath, show_all_files,
                                                    show_comments);
    window->add_toolbars_to_menu("&View", window->get_toolbars());
    window->load_toolbars();
    window->resize(this->size());
    window->show();
    this->register_editorwindow(window);
    connect(window, &QObject::destroyed,
            [this, window](){this->unregister_editorwindow(window);});
    return window;
}

void Editor::register_editorwindow(EditorMainWindow *window)
{
    this->editorwindows.append(window);
}

void Editor::unregister_editorwindow(EditorMainWindow *window)
{
    this->editorwindows.removeOne(window);
}

//------ Accessors
QStringList Editor::get_filenames() const
{
    QStringList list;
    foreach (FileInfo* finfo, this->editorstacks[0]->data) {
        list.append(finfo->filename);
    }
    return list;
}

int Editor::get_filename_index(const QString &filename) const
{
    return this->editorstacks[0]->has_filename(filename);
}

EditorStack* Editor::get_current_editorstack(EditorMainWindow *editorwindow) const
{
    if (!this->editorstacks.isEmpty()) {
        EditorStack* editorstack = nullptr;
        if (this->editorstacks.size() == 1)
            editorstack = this->editorstacks[0];
        else {
            editorstack = this->__get_focus_editorstack();
            if (editorstack == nullptr || editorwindow != nullptr) {
                editorstack = this->get_last_focus_editorstack(editorwindow);
                if (editorstack == nullptr)
                    editorstack = this->editorstacks[0];
            }
        }
        return editorstack;
    }
    return nullptr;
}

CodeEditor* Editor::get_current_editor() const
{
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack != nullptr)
        return editorstack->get_current_editor();
    return nullptr;
}

FileInfo* Editor::get_current_finfo() const
{
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack != nullptr)
        return editorstack->get_current_finfo();
    return nullptr;
}

QString Editor::get_current_filename() const
{
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack != nullptr)
        return editorstack->get_current_filename();
    return QString();
}

bool Editor::is_file_opened() const
{
    return this->editorstacks[0]->is_file_opened();
}

int Editor::is_file_opened(const QString &filename) const
{
    return this->editorstacks[0]->is_file_opened(filename);
}

CodeEditor* Editor::set_current_filename(const QString &filename, EditorMainWindow *editorwindow)
{
    EditorStack* editorstack = this->get_current_editorstack(editorwindow);
    return editorstack->set_current_filename(filename);
}

void Editor::set_path()
{
    foreach (FileInfo* finfo, this->editorstacks[0]->data) {
        finfo->path = this->main->get_spyder_pythonpath();
    }
    //if self.introspector:
}

//------ FileSwitcher API
EditorStack* Editor::get_current_tab_manager()
{
    return this->get_current_editorstack();
}

//------ Refresh methods
void Editor::refresh_file_dependent_actions()
{
    if (this->dockwidget && this->dockwidget->isVisible()) {
        bool enable = this->get_current_editor() != nullptr;
        foreach (QAction* action, this->file_dependent_actions) {
            action->setEnabled(enable);
        }
    }
}
void Editor::refresh_save_all_action()
{
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack) {
        bool state = false;
        foreach (FileInfo* finfo, editorstack->data) {
            state = finfo->editor->document()->isModified() || finfo->newly_created;
            if (state)
                break;
        }
        this->save_all_action->setEnabled(state);
    }
}

void Editor::update_warning_menu()
{
    EditorStack* editorstack = this->get_current_editorstack();
    QList<QList<QVariant>> check_results = editorstack->get_analysis_results();
    this->warning_menu->clear();
    QString filename = this->get_current_filename();
    foreach (auto pair, check_results) {
        QString message = pair[0].toString();
        int line_number = pair[1].toInt();
        bool error = message.contains("syntax");
        QString text = message.left(1).toUpper() + message.mid(1);
        QIcon icon = error ? ima::icon("error") : ima::icon("warning");

        auto slot_fun = [=](){this->switch_to_plugin();this->load(filename, line_number);};
        QAction* action = new QAction(icon, text, this);
        connect(action, &QAction::triggered, slot_fun);
        this->warning_menu->addAction(action);
    }
}

void Editor::analysis_results_changed()
{
    EditorStack* editorstack = this->get_current_editorstack();
    QList<QList<QVariant>> results = editorstack->get_analysis_results();
    int index = editorstack->get_stack_index();
    if (index != -1) {
        QString filename = editorstack->data[index]->filename;
        foreach (EditorStack* other_editorstack, this->editorstacks) {
            if (other_editorstack != editorstack)
                other_editorstack->set_analysis_results(filename, results);
        }
    }
    this->update_code_analysis_actions();
}

void Editor::update_todo_menu()
{
    EditorStack* editorstack = this->get_current_editorstack();
    QList<QList<QVariant>> results = editorstack->get_todo_results();
    this->todo_menu->clear();
    QString filename = this->get_current_filename();
    foreach (auto pair, results) {
        QString text = pair[0].toString();
        int line0 = pair[1].toInt();
        QIcon icon = ima::icon("todo");

        auto slot_fun = [=](){this->switch_to_plugin();this->load(filename, line0);};
        QAction* action = new QAction(icon, text, this);
        connect(action, &QAction::triggered, slot_fun);
        this->todo_menu->addAction(action);
    }
    this->update_todo_actions();
}

void Editor::todo_results_changed()
{
    EditorStack* editorstack = this->get_current_editorstack();
    QList<QList<QVariant>> results = editorstack->get_todo_results();
    int index = editorstack->get_stack_index();
    if (index != -1) {
        QString filename = editorstack->data[index]->filename;
        foreach (EditorStack* other_editorstack, this->editorstacks) {
            if (other_editorstack != editorstack)
                other_editorstack->set_todo_results(filename, results);
        }
    }
    this->update_todo_actions();
}

void Editor::refresh_eol_chars(const QString &os_name)
{
    this->__set_eol_chars = false;
    if (os_name == "nt")
        this->win_eol_action->setChecked(true);
    else if (os_name == "posix")
        this->linux_eol_action->setChecked(true);
    else
        this->mac_eol_action->setChecked(true);
    this->__set_eol_chars = true;
}

//------ Slots
void Editor::opened_files_list_changed()
{
    CodeEditor* editor = this->get_current_editor();
    if (editor) {
        bool python_enable = editor->is_python();
        // programs.is_module_installed('Cython')
        bool cython_enable = python_enable || (true && editor->is_cython());
        foreach (QAction* action, this->pythonfile_dependent_actions) {
            bool enable;
            if (cythonfile_compatible_actions.contains(action))
                enable = cython_enable;
            else
                enable = python_enable;
            if (action == this->winpdb_action) {
                //action.setEnabled(enable and WINPDB_PATH is not None)
                action->setEnabled(enable && false);
            }
            else
                action->setEnabled(enable);
        }
        emit open_file_update(this->get_current_filename());
    }
}

void Editor::update_code_analysis_actions()
{
    EditorStack* editorstack = this->get_current_editorstack();
    QList<QList<QVariant>> results = editorstack->get_analysis_results();
    bool state = (this->get_option("code_analysis/pyflakes").toBool()
            || this->get_option("code_analysis/pep8").toBool())
            && !results.isEmpty();
    this->warning_list_action->setEnabled(state);
    this->previous_warning_action->setEnabled(state);
    this->next_warning_action->setEnabled(state);
}

void Editor::update_todo_actions()
{
    EditorStack* editorstack = this->get_current_editorstack();
    QList<QList<QVariant>> results = editorstack->get_todo_results();
    bool state = this->get_option("todo_list").toBool()
            && !results.isEmpty();
    this->todo_list_action->setEnabled(state);
}

void Editor::rehighlight_cells()
{
    CodeEditor* editor = this->get_current_editor();
    editor->rehighlight_cells();
    QApplication::processEvents();
}

//------ Breakpoints
void Editor::save_breakpoints(QString filename, const QString &breakpoint_str)
{
    QFileInfo info(filename);
    filename = info.absoluteFilePath();
    QList<QVariant> breakpoints;
    if (!breakpoint_str.isEmpty())
        breakpoints = eval(breakpoint_str);
    ::save_breakpoints(filename, breakpoints);
    emit breakpoints_saved();
}

void Editor::__load_temp_file()
{
    QFileInfo info(this->TEMPFILE_PATH);
    if (!info.isFile()) {
        QStringList _default;
        _default << "# -*- coding: utf-8 -*-"
                 << "\"\"\""
                 << "Spyder Editor"
                 << ""
                 << "This is a temporary script file."
                 << "\"\"\""
                 << ""
                 << "";
        QString text = _default.join(os::linesep);
        bool ok = encoding::write(text, this->TEMPFILE_PATH);
        if (ok == false) {
            this->_new();
            return;
        }
    }
    this->load(this->TEMPFILE_PATH);
}

void Editor::__set_workdir()
{
    QString fname = this->get_current_filename();
    if (!fname.isEmpty()) {
        QFileInfo info(fname);
        QString directory = info.absolutePath();
        emit open_dir(directory);
    }
}

void Editor::__add_recent_file(const QString &fname)
{
    if (fname.isEmpty())
        return;
    if (this->recent_files.contains(fname))
        this->recent_files.removeAll(fname);
    this->recent_files.insert(0, fname);
    if (this->recent_files.size() > this->get_option("max_recent_files").toInt())
        this->recent_files.removeLast();
}

void Editor::_clone_file_everywhere(FileInfo *finfo)
{
    QList<EditorStack*> tmp = this->editorstacks.mid(1);
    foreach (EditorStack* editorstack, tmp) {
        CodeEditor* editor = editorstack->clone_editor_from(finfo, false);
        this->register_widget_shortcuts(editor);
    }
}

void Editor::_new(QString fname, EditorStack *editorstack, QString text)
{

    QString enc;
    bool default_content;
    bool empty = false;
    try {
        if (text.isEmpty()) {
            default_content = true;
            text = encoding::read(this->TEMPLATE_PATH);
            enc = "utf-8";
            QRegularExpression re("-*- coding: ?([a-z0-9A-Z\\-]*) -*-");
            QRegularExpressionMatch enc_match = re.match(text);
            if (enc_match.hasMatch())
                enc = enc_match.captured(1);

            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            QString username = env.value("USERNAME");

            // Linux, Mac OS X
            if (username.isEmpty())
                username = env.value("USER", "-");
            QString date = QDateTime::currentDateTime().toString();
            text = text.arg(date).arg(username);
        }
        else {
            default_content = false;
            enc = "utf-8";
        }
    } catch (...) {
        text = "";
        enc = "utf-8";
        default_content = true;
        empty = true;
    }

    auto create_fname = [](int n){return "untitled" + QString("%1.py").arg(n);};
    EditorStack* current_es;
    if (editorstack == nullptr)
        current_es = this->get_current_editorstack();
    else
        current_es = editorstack;
    bool created_from_here = fname.isEmpty();
    if (created_from_here) {
        while (true) {
            fname = create_fname(this->untitled_num);
            this->untitled_num++;
            QFileInfo info(fname);
            if (!info.isFile())
                break;
        }
        QString basedir = misc::getcwd_or_home();

        if (!this->main->projects->get_active_project_path().isEmpty())
            basedir = this->main->projects->get_active_project_path();
        else {
            QString c_fname = this->get_current_filename();
            if (!c_fname.isEmpty() && c_fname != this->TEMPFILE_PATH) {
                QFileInfo fileinfo(c_fname);
                basedir = fileinfo.absolutePath();
            }
        }

        fname = basedir + '/' + fname;
    }
    else {
        QFileInfo info(fname);
        fname = info.absoluteFilePath();
        int index = current_es->has_filename(fname);
        if (index > -1 && !current_es->close_file(index))
            return;
    }

    FileInfo* finfo = this->editorstacks[0]->_new(fname, enc, text,
                                                  default_content, empty);
    finfo->path = this->main->get_spyder_pythonpath();
    this->_clone_file_everywhere(finfo);
    CodeEditor* current_editor = current_es->set_current_filename(finfo->filename);
    this->register_widget_shortcuts(current_editor);
    if (!created_from_here)
        this->save(-1, true);
}

void Editor::edit_template()
{
    this->load(this->TEMPLATE_PATH);
}

void Editor::update_recent_file_menu()
{
    QStringList recent_files;
    foreach (QString fname, this->recent_files) {
        QFileInfo info(fname);
        if (this->is_file_opened(fname) == -1 && info.isFile())
            recent_files.append(fname);
    }
    this->recent_file_menu->clear();
    if (!recent_files.isEmpty()) {
        foreach (QString fname, recent_files) {
            QAction* action = new QAction(fname, this);
            action->setIcon(ima::icon("FileIcon"));
            connect(action, SIGNAL(triggered()), this, SLOT(load()));
            action->setData(fname);
            this->recent_file_menu->addAction(action);
        }
    }
    this->clear_recent_action->setEnabled(recent_files.size() > 0);
    QList<QAction*> actions;
    actions << nullptr << this->max_recent_action << this->clear_recent_action;
    add_actions(this->recent_file_menu, actions);
}

void Editor::clear_recent_files()
{
    this->recent_files.clear();
}

void Editor::change_max_recent_files()
{
    EditorStack* editorstack = this->get_current_editorstack();
    bool valid;
    int mrf = QInputDialog::getInt(editorstack, "Editor",
                                   "Maximum number of recent files",
                                   this->get_option("max_recent_files").toInt(),
                                   1, 35, 1, &valid);
    if (valid)
        this->set_option("max_recent_files", mrf);
}

QString _convert(QString fname)
{
    QFileInfo info(fname);
    fname = info.absoluteFilePath();
    if (os::name == "nt" && fname.size() == 2 && fname[1] == '.')
        fname = fname[0].toUpper() + fname.mid(1);
    return fname;
}

void Editor::load()
{

    CodeEditor* editor0 = this->get_current_editor();
    int position0 = 0;
    QString filename0;
    if (editor0) {
        position0 = editor0->get_position("cursor");
        filename0 = this->get_current_filename();
    }

    QString filename;
    QStringList filenames;
    QAction* action = qobject_cast<QAction*>(this->sender());
    if (action)
        filename = action->data().toString();
    if (filename.isEmpty()) {
        QString basedir = misc::getcwd_or_home();
        if (this->edit_filetypes.isEmpty())
            this->edit_filetypes = get_edit_filetypes();
        if (this->edit_filters.isEmpty())
            this->edit_filters = get_edit_filters();

        QString c_fname = this->get_current_filename();
        if (!c_fname.isEmpty() && c_fname != this->TEMPFILE_PATH) {
            QFileInfo info(c_fname);
            basedir = info.absolutePath();
        }

        emit redirect_stdio(false);
        EditorStack* parent_widget = this->get_current_editorstack();
        QString selectedfilter = "";

        if (!filename0.isEmpty()) {
            QFileInfo info(filename0);
            selectedfilter = get_filter(this->edit_filetypes,"."+info.suffix());
        }
        if (!running_under_pytest()) {
            filenames = QFileDialog::getOpenFileNames(parent_widget,
                                                      "Open file", basedir,
                                                      edit_filters,
                                                      &selectedfilter,
                                                      QFileDialog::HideNameFilterDetails);
        }
        else {
            QFileDialog* dialog = new QFileDialog(parent_widget, "Open file");
            dialog->setOptions(QFileDialog::DontUseNativeDialog);
            if (dialog->exec())
                filenames = dialog->selectedFiles();
        }
        emit redirect_stdio(true);
        if (filenames.isEmpty())
            return;
    }

    QWidget* focus_widget = QApplication::focusWidget();
    CodeEditor* code_editor = qobject_cast<CodeEditor*>(focus_widget);
    EditorMainWindow* editorwindow = nullptr;
    if (!this->editorwindows.isEmpty() && !this->dockwidget->isVisible()) {
        qDebug() << "editorwindows.isEmpty";
        editorwindow = this->editorwindows[0];
        editorwindow->setFocus();
        editorwindow->raise();
    }
    else if (this->dockwidget && !this->ismaximized &&
             !this->dockwidget->isAncestorOf(focus_widget)
             && !code_editor) {
        qDebug() << "this->dockwidget";
        this->dockwidget->setVisible(true);
        this->dockwidget->setFocus();
        this->dockwidget->raise();
    }

    if (!filename.isEmpty()) {
        filenames = QStringList(_convert(filename));
    }
    else {
        QStringList tmp;
        foreach (QString fname, filenames) {
            tmp.append(_convert(fname));
        }
        filenames = tmp;
    }

    for (int index = 0; index < filenames.size(); ++index) {
        filename = filenames[index];
        CodeEditor* current_editor = this->set_current_filename(filename, editorwindow);
        if (current_editor == nullptr) {
            QFileInfo info(filename);
            if (!info.isFile())
                continue;

            EditorStack* current_es = this->get_current_editorstack(editorwindow);
            FileInfo* finfo = this->editorstacks[0]->load(filename, false);
            finfo->path = this->main->get_spyder_pythonpath();
            this->_clone_file_everywhere(finfo);
            current_editor = current_es->set_current_filename(filename);
            current_editor->set_breakpoints(load_breakpoints(filename));
            this->register_widget_shortcuts(current_editor);

            current_es->analyze_script();
            this->__add_recent_file(filename);
        }
        current_editor->clearFocus();
        current_editor->setFocus();
        current_editor->window()->raise();
    }
}

void Editor::load(QString filename, int _goto, QString word, QWidget *editorwin, bool processevents)
{
    CodeEditor* editor0 = this->get_current_editor();
    int position0 = 0;
    QString filename0;
    if (editor0) {
        position0 = editor0->get_position("cursor");
        filename0 = this->get_current_filename();
    }

    QStringList filenames;

    QWidget* focus_widget = QApplication::focusWidget();
    CodeEditor* code_editor = qobject_cast<CodeEditor*>(focus_widget);
    EditorMainWindow* editorwindow = nullptr;
    if (!this->editorwindows.isEmpty() && !this->dockwidget->isVisible()) {
        editorwindow = qobject_cast<EditorMainWindow*>(editorwin);
        if ((!editorwindow) || (!this->editorwindows.contains(editorwindow)))
            editorwindow = this->editorwindows[0];
        editorwindow->setFocus();
        editorwindow->raise();
    }
    else if (this->dockwidget && !this->ismaximized &&
             !this->dockwidget->isAncestorOf(focus_widget)
             && !code_editor) {
        this->dockwidget->setVisible(true);
        this->dockwidget->setFocus();
        this->dockwidget->raise();
    }


    filenames = QStringList(_convert(filename));

    QList<int> goto_list;
    goto_list.append(_goto);

    for (int index = 0; index < filenames.size(); ++index) {
        filename = filenames[index];
        CodeEditor* current_editor = this->set_current_filename(filename, editorwindow);
        if (current_editor == nullptr) {
            QFileInfo info(filename);
            if (!info.isFile())
                continue;

            EditorStack* current_es = this->get_current_editorstack(editorwindow);
            Q_ASSERT(this->editorstacks.size() >= 1);
            FileInfo* finfo = this->editorstacks[0]->load(filename, false);
            finfo->path = this->main->get_spyder_pythonpath();
            this->_clone_file_everywhere(finfo);
            current_editor = current_es->set_current_filename(filename);
            current_editor->set_breakpoints(load_breakpoints(filename));
            this->register_widget_shortcuts(current_editor);

            current_es->analyze_script();
            this->__add_recent_file(filename);
        }
        if (goto_list[index] != -1) {
            current_editor->go_to_line(goto_list[index], word);
            int position = current_editor->get_position("cursor");
            this->cursor_moved(filename0, position0, filename, position);
        }
        current_editor->clearFocus();
        current_editor->setFocus();
        current_editor->window()->raise();
        if (processevents)
            QApplication::processEvents();
    }
}

void Editor::load(QStringList filenames, int _goto, QString word, QWidget *editorwin, bool processevents)
{
    CodeEditor* editor0 = this->get_current_editor();
    int position0 = 0;
    QString filename0;
    if (editor0) {
        position0 = editor0->get_position("cursor");
        filename0 = this->get_current_filename();
    }
    if (filenames.isEmpty()) {
        QString basedir = misc::getcwd_or_home();
        if (this->edit_filetypes.isEmpty())
            this->edit_filetypes = get_edit_filetypes();
        if (this->edit_filters.isEmpty())
            this->edit_filters = get_edit_filters();

        QString c_fname = this->get_current_filename();
        if (!c_fname.isEmpty() && c_fname != this->TEMPFILE_PATH) {
            QFileInfo info(c_fname);
            basedir = info.absolutePath();
        }

        emit redirect_stdio(false);
        EditorStack* parent_widget = this->get_current_editorstack();
        QString selectedfilter = "";

        if (!filename0.isEmpty()) {
            QFileInfo info(filename0);
            selectedfilter = get_filter(this->edit_filetypes,"."+info.suffix());
        }
        if (!running_under_pytest()) {
            filenames = QFileDialog::getOpenFileNames(parent_widget,
                                                      "Open file", basedir,
                                                      edit_filters,
                                                      &selectedfilter,
                                                      QFileDialog::HideNameFilterDetails);
        }
        else {
            QFileDialog* dialog = new QFileDialog(parent_widget, "Open file");
            dialog->setOptions(QFileDialog::DontUseNativeDialog);
            if (dialog->exec())
                filenames = dialog->selectedFiles();
        }
        emit redirect_stdio(true);
        if (filenames.isEmpty())
            return;
    }

    QWidget* focus_widget = QApplication::focusWidget();
    CodeEditor* code_editor = qobject_cast<CodeEditor*>(focus_widget);
    EditorMainWindow* editorwindow = nullptr;
    if (!this->editorwindows.isEmpty() && !this->dockwidget->isVisible()) {
        editorwindow = qobject_cast<EditorMainWindow*>(editorwin);
        if ((!editorwindow) || (!this->editorwindows.contains(editorwindow)))
            editorwindow = this->editorwindows[0];
        editorwindow->setFocus();
        editorwindow->raise();
    }
    else if (this->dockwidget && !this->ismaximized &&
             !this->dockwidget->isAncestorOf(focus_widget)
             && !code_editor) {
        this->dockwidget->setVisible(true);
        this->dockwidget->setFocus();
        this->dockwidget->raise();
    }

    QStringList tmp;
    foreach (QString fname, filenames) {
        tmp.append(_convert(fname));
    }
    filenames = tmp;

    for (int index = 0; index < filenames.size(); ++index) {
        QString filename = filenames[index];
        CodeEditor* current_editor = this->set_current_filename(filename, editorwindow);
        if (current_editor == nullptr) {
            QFileInfo info(filename);
            if (!info.isFile())
                continue;

            EditorStack* current_es = this->get_current_editorstack(editorwindow);
            FileInfo* finfo = this->editorstacks[0]->load(filename, false);
            finfo->path = this->main->get_spyder_pythonpath();
            this->_clone_file_everywhere(finfo);
            current_editor = current_es->set_current_filename(filename);
            current_editor->set_breakpoints(load_breakpoints(filename));
            this->register_widget_shortcuts(current_editor);

            current_es->analyze_script();
            this->__add_recent_file(filename);
        }

        current_editor->clearFocus();
        current_editor->setFocus();
        current_editor->window()->raise();
        if (processevents)
            QApplication::processEvents();
    }
}

void Editor::print_file()
{
    CodeEditor* editor = this->get_current_editor();
    QString filename = this->get_current_filename();
    Printer* printer = new Printer(QPrinter::HighResolution,
                                   this->get_plugin_font2("printer_header"));
    QPrintDialog* printDialog = new QPrintDialog(printer, editor);
    if (editor->has_selected_text())
        printDialog->setOption(QAbstractPrintDialog::PrintSelection, true);
    emit redirect_stdio(false);
    int answer = printDialog->exec();
    emit redirect_stdio(true);
    if (answer == QDialog::Accepted) {
        this->starting_long_process("Printing...");
        printer->setDocName(filename);
        editor->print(printer);
        this->ending_long_process();
    }
}

void Editor::print_preview()
{
    CodeEditor* editor = this->get_current_editor();
    Printer* printer = new Printer(QPrinter::HighResolution,
                                   this->get_plugin_font2("printer_header"));
    QPrintPreviewDialog* preview = new QPrintPreviewDialog(printer, this);
    preview->setWindowFlags(Qt::Window);
    connect(preview, &QPrintPreviewDialog::paintRequested,
            [editor](QPrinter *printer){editor->print(printer);});
    emit redirect_stdio(false);
    preview->exec();
    emit redirect_stdio(true);
}

void Editor::close_file()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->close_file();
}

void Editor::close_all_files()
{
    this->editorstacks[0]->close_all_files();
}

bool Editor::save(int index, bool force)
{
    EditorStack* editorstack = this->get_current_editorstack();
    return editorstack->save(index, force);
}

void Editor::save_as()
{
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack->save_as()) {
        QString fname = editorstack->get_current_filename();
        this->__add_recent_file(fname);
    }
}

void Editor::save_copy_as()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->save_copy_as();
}

void Editor::save_all()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->save_all();
}

void Editor::revert()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->revert();
}

void Editor::find()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->find_widget->show();
    editorstack->find_widget->search_text->setFocus();
}

void Editor::find_next()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->find_widget->find_next();
}

void Editor::find_previous()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->find_widget->find_previous();
}

void Editor::replace()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->find_widget->show_replace();
}

void Editor::open_last_closed()
{
    EditorStack* editorstack = this->get_current_editorstack();
    QStringList last_closed_files = editorstack->get_last_closed_files();
    if (last_closed_files.size() > 0) {
        QString file_to_open = last_closed_files[0];
        last_closed_files.removeAt(0);
        editorstack->set_last_closed_files(last_closed_files);
        this->load(file_to_open);
    }
}

void Editor::close_file_from_name(QString filename)
{
    QFileInfo info(filename);
    filename = info.absoluteFilePath();
    int index = this->editorstacks[0]->has_filename(filename);
    if (index > -1)
        this->editorstacks[0]->close_file(index);
}

void Editor::removed(const QString &filename)
{
    this->close_file_from_name(filename);
}

void Editor::removed_tree(QString dirname)
{
    QFileInfo info(dirname);
    dirname = info.absoluteFilePath();
    foreach (QString fname, this->get_filenames()) {
        QFileInfo fileinfo(fname);
        if (fileinfo.absoluteFilePath().startsWith(dirname))
            this->close_file_from_name(fname);
    }
}

void Editor::renamed(const QString &source, const QString &dest)
{
    QFileInfo info(source);
    QString filename = info.absoluteFilePath();
    int index = this->editorstacks[0]->has_filename(filename);
    if (index > -1) {
        foreach (EditorStack* editorstack, this->editorstacks) {
            editorstack->rename_in_data(filename, dest);
        }
    }
}
void Editor::renamed_tree(const QString &source, const QString &dest)
{
    QFileInfo info(source);
    QString dirname = info.absoluteFilePath();
    foreach (QString fname, this->get_filenames()) {
        QFileInfo fileinfo(fname);
        if (fileinfo.absoluteFilePath().startsWith(dirname)) {
            QString new_filename = fname;
            new_filename.replace(dirname, dest);
            this->renamed(fname, new_filename);
        }
    }
}

void Editor::indent()
{
    CodeEditor* editor = this->get_current_editor();
    if (editor)
        editor->indent();
}

void Editor::unindent()
{
    CodeEditor* editor = this->get_current_editor();
    if (editor)
        editor->unindent();
}

void Editor::text_uppercase()
{
    CodeEditor* editor = this->get_current_editor();
    if (editor)
        editor->transform_to_uppercase();
}

void Editor::text_lowercase()
{
    CodeEditor* editor = this->get_current_editor();
    if (editor)
        editor->transform_to_lowercase();
}

void Editor::toggle_comment()
{
    CodeEditor* editor = this->get_current_editor();
    if (editor)
        editor->toggle_comment();
}

void Editor::blockcomment()
{
    CodeEditor* editor = this->get_current_editor();
    if (editor)
        editor->blockcomment();
}

void Editor::unblockcomment()
{
    CodeEditor* editor = this->get_current_editor();
    if (editor)
        editor->unblockcomment();
}

void Editor::go_to_next_todo()
{
    this->switch_to_plugin();
    CodeEditor* editor = this->get_current_editor();
    int positin = editor->go_to_next_todo();
    QString filename = this->get_current_filename();
    this->add_cursor_position_to_history(filename, positin);
}

void Editor::go_to_next_warning()
{
    this->switch_to_plugin();
    CodeEditor* editor = this->get_current_editor();
    int positin = editor->go_to_next_warning();
    QString filename = this->get_current_filename();
    this->add_cursor_position_to_history(filename, positin);
}

void Editor::go_to_previous_warning()
{
    this->switch_to_plugin();
    CodeEditor* editor = this->get_current_editor();
    int positin = editor->go_to_previous_warning();
    QString filename = this->get_current_filename();
    this->add_cursor_position_to_history(filename, positin);
}

void Editor::run_winpdb()
{}

void Editor::toggle_eol_chars(const QString &os_name, bool checked)
{
    if (checked) {
        CodeEditor* editor = this->get_current_editor();
        if (this->__set_eol_chars) {
            this->switch_to_plugin();
            editor->set_eol_chars(sourcecode::get_eol_chars_from_os_name(os_name));
        }
    }
}

void Editor::toggle_show_blanks(bool checked)
{
    this->switch_to_plugin();
    CodeEditor* editor = this->get_current_editor();
    editor->set_blanks_enabled(checked);
}

void Editor::remove_trailing_spaces()
{
    this->switch_to_plugin();
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->remove_trailing_spaces();
}

void Editor::fix_indentation()
{
    this->switch_to_plugin();
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->fix_indentation();
}

void Editor::update_cursorpos_actions()
{
    previous_edit_cursor_action->setEnabled(!last_edit_cursor_pos.first.isEmpty());
    previous_cursor_action->setEnabled(this->cursor_pos_index > 0);
    next_cursor_action->setEnabled(cursor_pos_index>=0 &&
                                   cursor_pos_index < cursor_pos_history.size()-1);
}

void Editor::add_cursor_position_to_history(const QString &filename, int position, bool fc)
{
    //变量fc代表file_changed
    if (this->__ignore_cursor_position)
        return;
    QList<QPair<QString, int>> tmp = this->cursor_pos_history;
    for (int index = 0; index < tmp.size(); ++index) {
        QString fname = tmp[index].first;
        int pos = tmp[index].second;

        if (fname == filename) {
            if (pos == position || pos == 0) {
                if (fc) {
                    cursor_pos_history[index] = QPair<QString,int>(filename, position);
                    cursor_pos_index = index;
                    this->update_cursorpos_actions();
                    return;
                }
                else {
                    if (this->cursor_pos_index >= index)
                        this->cursor_pos_index--;
                    this->cursor_pos_history.removeAt(index);
                    break;
                }
            }
        }
    }
    if (cursor_pos_index > -1)
        cursor_pos_history = cursor_pos_history.mid(0, cursor_pos_index+1);
    cursor_pos_history.append(QPair<QString,int>(filename, position));
    cursor_pos_index = cursor_pos_history.size()-1;
    this->update_cursorpos_actions();
}

void Editor::cursor_moved(const QString &filename0, int position0, const QString &filename1, int position1)
{
    // 该函数仅被load()函数所调用
    if (position0 != 0)
        this->add_cursor_position_to_history(filename0, position0);
    this->add_cursor_position_to_history(filename1, position1);
}

void Editor::text_changed_at(const QString &filename, int position)
{
    last_edit_cursor_pos = QPair<QString,int>(filename, position);
}

void Editor::current_file_changed(const QString &filename, int position)
{
    this->add_cursor_position_to_history(filename, position, true);
}

void Editor::go_to_last_edit_location()
{
    if (!this->last_edit_cursor_pos.first.isEmpty()) {
        QString filename = last_edit_cursor_pos.first;
        int position = last_edit_cursor_pos.second;
        QFileInfo info(filename);
        if (!info.isFile()) {
            last_edit_cursor_pos.first = QString();
            return;
        }
        else {
            this->load(filename);
            CodeEditor* editor = this->get_current_editor();
            if (position < editor->document()->characterCount())
                editor->set_cursor_position(position);
        }
    }
}

void Editor::__move_cursor_position(int index_move)
{
    if (this->cursor_pos_index == -1)
        return;
    cursor_pos_history[cursor_pos_index].second = this->get_current_editor()->get_position("cursor");
    this->__ignore_cursor_position = true;
    int old_index = cursor_pos_index;
    cursor_pos_index = std::min(cursor_pos_history.size()-1,
                                std::max(0, cursor_pos_index+index_move));

    QString filename = cursor_pos_history[cursor_pos_index].first;
    int position = cursor_pos_history[cursor_pos_index].second;
    QFileInfo info(filename);
    if (!info.isFile()) {
        cursor_pos_history.removeAt(cursor_pos_index);
        if (cursor_pos_index < old_index)
            old_index--;
        cursor_pos_index = old_index;
    }
    else {
        this->load(filename);
        CodeEditor* editor = this->get_current_editor();
        if (position < editor->document()->characterCount())
            editor->set_cursor_position(position);
    }
    this->__ignore_cursor_position = false;
    this->update_cursorpos_actions();
}

void Editor::go_to_previous_cursor_position()
{
    this->switch_to_plugin();
    this->__move_cursor_position(-1);
}

void Editor::go_to_next_cursor_position()
{
    this->switch_to_plugin();
    this->__move_cursor_position(1);
}

void Editor::go_to_line(int line)
{
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack)
        editorstack->go_to_line(line);
}

void Editor::set_or_clear_breakpoint()
{
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack) {
        this->switch_to_plugin();
        editorstack->set_or_clear_breakpoint();
    }
}

void Editor::set_or_edit_conditional_breakpoint()
{
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack) {
        this->switch_to_plugin();
        editorstack->set_or_edit_conditional_breakpoint();
    }
}

void Editor::clear_all_breakpoints()
{
    this->switch_to_plugin();
    ::clear_all_breakpoints();
    emit breakpoints_saved();
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack) {
        foreach (FileInfo* finfo, editorstack->data) {
            finfo->editor->clear_breakpoints();
        }
    }
    this->refresh_plugin();
}

void Editor::clear_breakpoint(const QString &filename, int lineno)
{
    ::clear_breakpoint(filename, lineno);
    emit breakpoints_saved();
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack) {
        int index = this->is_file_opened(filename);
        if (index > -1)
            editorstack->data[index]->editor->add_remove_breakpoint(lineno);
    }
}

void Editor::debug_command(const QString &command)
{
    this->switch_to_plugin();
    // self.main.ipyconsole.write_to_stdin(command)
    // debug_command("next") debug_command("continue")
    // 该函数用于debug
}

void Editor::edit_run_configurations()
{
    RunConfigDialog* dialog = new RunConfigDialog(this);
    connect(dialog, SIGNAL(size_change(const QSize&)),
            SLOT(set_dialog_size(const QSize&)));
    if (this->dialog_size.isValid())
        dialog->resize(this->dialog_size);
    QFileInfo info(this->get_current_filename());
    QString fname = info.absoluteFilePath();
    dialog->setup(fname);
    if (dialog->exec()) {
        fname = dialog->file_to_run;
        if (!fname.isEmpty()) {
            this->load(fname);
            this->run_file();
        }
    }
}

void Editor::run_file(bool debug)
{
    EditorStack* editorstack = this->get_current_editorstack();
    if (editorstack->save()) {
        CodeEditor* editor = this->get_current_editor();
        QFileInfo info(this->get_current_filename());
        QString fname = info.absoluteFilePath();

        QString dirname = info.absolutePath();

        RunConfiguration* runconf = get_run_configuration(fname);
        if (runconf == nullptr) {
            RunConfigOneDialog* dialog = new RunConfigOneDialog(this);
            connect(dialog, SIGNAL(size_change(const QSize&)),
                    SLOT(set_dialog_size(const QSize&)));
            if (this->dialog_size.isValid())
                dialog->resize(this->dialog_size);
            QFileInfo info(this->get_current_filename());
            dialog->setup(fname);

            bool show_dlg;
            if (CONF_get("run", "open_at_least_once",
                         !running_under_pytest()).toBool()) {
                show_dlg = true;
                CONF_set("run", "open_at_least_once", false);
            }
            else
                show_dlg = CONF_get("run", ALWAYS_OPEN_FIRST_RUN_OPTION).toBool();
            if (show_dlg && !dialog->exec())
                return;
            runconf = dialog->get_configuration();
        }

        Q_ASSERT(runconf);
        QString wdir;

        QString args = runconf->get_arguments();
        bool interact = runconf->interact;

        bool python = true;
        QString python_args = runconf->get_python_arguments();
        bool current = runconf->current;
        bool systerm = runconf->systerm;
        bool post_mortem = runconf->post_mortem;
        bool clear_namespace = runconf->clear_namespace;

        QFileInfo fileinfo(runconf->dir);
        if (runconf->file_dir)
            wdir = dirname;
        else if (runconf->cw_dir)
            wdir = "";
        else if (fileinfo.isDir())
            wdir = runconf->dir;
        else
            wdir = "";

        __last_ec_exec = Last_Ec_Exec(fname, wdir, args, interact, debug,
                                      python, python_args, current, systerm,
                                      post_mortem, clear_namespace);
        this->re_run_file();
        if (!interact && !debug)
            editor->setFocus();
    }
}

void Editor::set_dialog_size(const QSize &size)
{
    this->dialog_size = size;
}

void Editor::debug_file()
{
    this->switch_to_plugin();
    this->run_file(true);
}

void Editor::re_run_file()
{
    if (this->get_option("save_all_before_run").toBool())
        this->save_all();
    if (this->__last_ec_exec.fname.isEmpty())
        return;

    QString fname = __last_ec_exec.fname;
    QString wdir = __last_ec_exec.wdir;
    QString args = __last_ec_exec.args;
    bool interact = __last_ec_exec.interact;
    bool debug = __last_ec_exec.debug;
    bool python = __last_ec_exec.python;
    QString python_args = __last_ec_exec.python_args;
    bool current = __last_ec_exec.current;
    bool systerm = __last_ec_exec.systerm;
    bool post_mortem = __last_ec_exec.post_mortem;
    bool clear_namespace = __last_ec_exec.clear_namespace;

    // 该信号在plugins/ipythonconsole.py878行中连接对应的槽函数
    systerm = true;//暂时置为true，来在programs.cpp中运行
    if (!systerm) {
        emit run_in_current_ipyclient(fname, wdir, args,
                                      debug, post_mortem,
                                      current, clear_namespace);
        qDebug() << __func__ << fname << wdir;
    }
    else {
        this->main->open_external_console(fname, wdir, args, interact,
                                          debug, python, python_args,
                                          systerm, post_mortem);
    }
}

void Editor::run_selection()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->run_selection();
}

void Editor::run_cell()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->run_cell();
}

void Editor::run_cell_and_advance()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->run_cell_and_advance();
}

void Editor::re_run_last_cell()
{
    EditorStack* editorstack = this->get_current_editorstack();
    editorstack->re_run_last_cell();
}

void Editor::zoom(int factor)
{
    CodeEditor* editor = this->get_current_editorstack()->get_current_editor();
    if (factor == 0) {
        QFont font = this->get_plugin_font();
        editor->set_font(font);
    }
    else {
        QFont font = editor->font();
        int size = font.pointSize() + factor;
        if (size > 0) {
            font.setPointSize(size);
            editor->set_font(font);
        }
    }
    editor->update_tab_stop_width_spaces();
}

void Editor::apply_plugin_settings(const QStringList &options)
{
    // TODO
}

QStringList Editor::get_open_filenames() const
{
    EditorStack* editorstack = this->editorstacks[0];
    QStringList filenames;
    foreach (FileInfo* finfo, editorstack->data) {
        filenames.append(finfo->filename);
    }
    return filenames;
}

void Editor::set_open_filenames()
{
    if (this->projects) {
        if (!this->projects->get_active_project()) {
            QStringList filenames = this->get_open_filenames();
            this->set_option("filenames", filenames);
            //关闭时调用该函数保存所有打开的文件名列表
        }
    }
}

bool any_isfile(const QStringList& filenames)
{
    foreach (QString f, filenames) {
        QFileInfo info(f);
        if (info.isFile())
            return true;
    }
    return false;
}

void Editor::setup_open_files()
{
    this->set_create_new_file_if_empty(false);
    QString active_project_path;
    if (this->projects)
        active_project_path = this->projects->get_active_project_path();

    QStringList filenames;
    if (!active_project_path.isEmpty())
        filenames = this->projects->get_project_filenames();
    else
        filenames = this->get_option("filenames", QStringList()).toStringList();
    //启动Spyder时调用该函数获得上次关闭之前保存的文件名列表
    this->close_all_files();

    if (!filenames.isEmpty() && any_isfile(filenames)) {
        this->load(filenames);
        if (this->__first_open_files_setup) {
            this->__first_open_files_setup = false;

            QVariant val = this->get_option("layout_settings");
            SplitSettings layout = val.value<SplitSettings>();
            if (!layout.hexstate.isEmpty())
                this->editorsplitter->set_layout_settings(layout);

            QList<QVariant> list = this->get_option("windows_layout_settings", QList<QVariant>()).toList();
            foreach (QVariant tmp, list) {
                LayoutSettings layout_settings = tmp.value<LayoutSettings>();
                this->editorwindows_to_be_created.append(layout_settings);
            }
            this->set_last_focus_editorstack(this, this->editorstacks[0]);
        }
    }
    else
        //如果文件名列表为空，则加载临时文件
        this->__load_temp_file();
    this->set_create_new_file_if_empty(true);
}

void Editor::save_open_files()
{
    this->set_option("filenames", this->get_open_filenames());
}

void Editor::set_create_new_file_if_empty(bool value)
{
    foreach (EditorStack* editorstack, this->editorstacks) {
        editorstack->create_new_file_if_empty = value;
    }
}

