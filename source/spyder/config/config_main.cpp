#include "config_main.h"
#include "fonts.h"

QStringList SANS_SERIF = {"Sans Serif", "DejaVu Sans", "Bitstream Vera Sans",
                          "Bitstream Charter", "Lucida Grande", "MS Shell Dlg 2",
                          "Calibri", "Verdana", "Geneva", "Lucid", "Arial",
                          "Helvetica", "Avant Garde", "Times", "sans-serif"};

QStringList MONOSPACE = {"Monospace", "DejaVu Sans Mono", "Consolas",
                         "Bitstream Vera Sans Mono", "Andale Mono", "Liberation Mono",
                         "Courier New", "Courier", "monospace", "Fixed", "Terminal"};


//本来在config/base.cpp中
QStringList EXCLUDED_NAMES = {"nan", "inf", "infty", "little_endian", "colorbar_doc",
                              "typecodes", "__builtins__", "__main__", "__doc__", "NaN",
                              "Inf", "Infinity", "sctypes", "rcParams", "rcParamsDefault",
                              "sctypeNA", "typeNA", "False_", "True_"};

QStringList EXCLUDE_PATTERNS = {"*.csv, *.dat, *.log, *.tmp, *.bak, *.orig"};

static QStringList NAME_FILTERS = {"README", "INSTALL", "LICENSE", "CHANGELOG",
                                  "*.spydata", "*.npy", "*.npz", "*.mat",
                                  "*.csv", "*.txt", "*.jpg", "*.png",
                                  "*.gif", "*.tif", "*.pkl", "*.pickle",
                                  "*.json", "*.py", "*.ipynb", "*.dat",
                                  "*.pdf", "*.svg"};

unsigned short OPEN_FILES_PORT = 21128;

QList<QPair<QString, QHash<QString,QVariant>>> DEFAULTS =
{
    QPair<QString, QHash<QString,QVariant>>
    ("main", QHash<QString,QVariant>(
    {
     {"icon_theme", "spyder 3"},
     {"opengl", "software"},
     {"single_instance", true},
     {"open_files_port", OPEN_FILES_PORT},
     {"tear_off_menus", false},
     {"normal_screen_resolution", true},
     {"high_dpi_scaling", false},
     {"high_dpi_custom_scale_factor", false},
     {"high_dpi_custom_scale_factors", "1.5"},
     {"vertical_dockwidget_titlebars", false},
     {"vertical_tabs", false},
     {"animated_docks", true},
     {"prompt_on_exit", false},
     {"panes_locked", true},
     {"window/size", QSize(1260, 740)},
     {"window/position", QPoint(10, 10)},
     {"window/is_maximized", true},
     {"window/is_fullscreen", false},
     {"window/prefs_dialog_size", QSize(745, 411)},
     {"show_status_bar", true},
     {"memory_usage/enable", true},
     {"memory_usage/timeout", 2000},
     {"cpu_usage/enable", false},
     {"cpu_usage/timeout", 2000},
     {"use_custom_margin", true},
     {"custom_margin", 0},
     {"use_custom_cursor_blinking", false},
     {"show_internal_errors", true},
     {"check_updates_on_startup", true},
     {"toolbars_visible", true},

     {"font/family", MONOSPACE},
     {"font/size", 10},
     {"font/italic", false},
     {"font/bold", false},
     {"rich_font/family", SANS_SERIF},
     {"rich_font/size", 10},
     {"rich_font/italic", false},
     {"rich_font/bold", false},
     {"cursor/width", 2},
     {"completion/size", QSize(300, 180)},
     {"report_error/remember_me", false},
     {"report_error/remember_token", false}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("quick_layouts", QHash<QString,QVariant>(
    {{"place_holder", ""},
     {"names", QStringList({"Matlab layout", "Rstudio layout", "Vertical split", "Horizontal split"})},
     {"order", QStringList({"Matlab layout", "Rstudio layout", "Vertical split", "Horizontal split"})},
     {"active", QStringList({"Matlab layout", "Rstudio layout", "Vertical split", "Horizontal split"})}})),
    QPair<QString, QHash<QString,QVariant>>
    ("internal_console", QHash<QString,QVariant>(
    {{"max_line_count", 300},
     {"working_dir_history", 30},
     {"working_dir_adjusttocontents", false},
     {"wrap", true},
     {"calltips", true},
     {"codecompletion/auto", false},
     {"codecompletion/enter_key", true},
     {"codecompletion/case_sensitive", true},
     {"external_editor/path", "SciTE"},
     {"external_editor/gotoline", "-goto:"},
     {"light_background", true}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("main_interpreter", QHash<QString,QVariant>(
    {{"default", true},
     {"custom", false},
     {"umr/enabled", true},
     {"umr/verbose", true},
     {"umr/namelist", QStringList()},
     {"custom_interpreters_list", QStringList()},
     {"custom_interpreter", ""}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("ipython_console", QHash<QString,QVariant>(
    {{"show_banner", true},
     {"completion_type", 0},
     {"use_pager", false},
     {"show_calltips", true},
     {"ask_before_closing", false},
     {"show_reset_namespace_warning", true},
     {"buffer_size", 500},
     {"pylab", true},
     {"pylab/autoload", false},
     {"pylab/backend", 0},
     {"pylab/inline/figure_format", 0},
     {"pylab/inline/resolution", 72},
     {"pylab/inline/width", 6},
     {"pylab/inline/height", 4},
     {"pylab/inline/bbox_inches", true},
     {"startup/run_lines", ""},
     {"startup/use_run_file", false},
     {"startup/run_file", ""},
     {"greedy_completer", false},
     {"jedi_completer", false},
     {"autocall", 0},
     {"symbolic_math", false},
     {"in_prompt", ""},
     {"out_prompt", ""},
     {"show_elapsed_time", false},
     {"ask_before_restart", true}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("variable_explorer", QHash<QString,QVariant>(
    {{"check_all", false},
     {"dataframe_format", ".6g"},
     {"excluded_names", EXCLUDED_NAMES},
     {"exclude_private", true},
     {"exclude_capitalized", true},
     {"exclude_unsupported", true},
     {"truncate", true},
     {"minmax", false}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("editor", QHash<QString,QVariant>(
    {{"printer_header/font/family", SANS_SERIF},
     {"printer_header/font/size", 10},
     {"printer_header/font/italic", false},
     {"printer_header/font/bold", false},
     {"wrap", false},
     {"wrapflag", true},
     {"code_analysis/pyflakes", true},
     {"code_analysis/pep8", false},
     {"todo_list", true},
     {"realtime_analysis", true},
     {"realtime_analysis/timeout", 2500},
     {"outline_explorer", true},
     {"line_numbers", true},
     {"blank_spaces", false},
     {"edge_line", true},
     {"edge_line_column", 79},
     {"toolbox_panel", true},
     {"calltips", true},
     {"go_to_definition", true},
     {"close_parentheses", true},
     {"close_quotes", false},
     {"add_colons", true},
     {"auto_unindent", true},
     {"indent_chars", "*    *"}, //这是啥
     {"tab_stop_width_spaces", 4},
     {"codecompletion/auto", true},
     {"codecompletion/enter_key", true},
     {"codecompletion/case_sensitive", true},
     {"check_eol_chars", true},
     {"tab_always_indent", false},
     {"intelligent_backspace", true},
     {"highlight_current_line", true},
     {"highlight_current_cell", true},
     {"occurrence_highlighting", true},
     {"occurrence_highlighting/timeout", 1500},
     {"always_remove_trailing_spaces", false},
     {"show_tab_bar", true},
     {"max_recent_files", 20},
     {"save_all_before_run", true},
     {"focus_to_editor", true},
     {"onsave_analysis", false}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("historylog", QHash<QString,QVariant>(
    {{"enable", true},
     {"max_entries", 100},
     {"wrap", true},
     {"go_to_eof", true}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("help", QHash<QString,QVariant>(
    {{"enable", true},
     {"max_history_entries", 20},
     {"wrap", true},
     {"connect/editor", false},
     {"connect/ipython_console", false},
     {"math", true},
     {"automatic_import", true}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("onlinehelp", QHash<QString,QVariant>(
    {{"enable", true},
     {"zoom_factor", 0.8},
     {"max_history_entries", 20}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("outline_explorer", QHash<QString,QVariant>(
    {{"enable", true},
     {"show_fullpath", false},
     {"show_all_files", false},
     {"show_comments", true}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("project_explorer", QHash<QString,QVariant>(
    {{"name_filters", NAME_FILTERS},
     {"show_all", true},
     {"show_hscrollbar", true},
     {"visible_if_project_open", true}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("explorer", QHash<QString,QVariant>(
    {{"enable", true},
     {"wrap", true},
     {"name_filters", NAME_FILTERS},
     {"show_hidden", true},
     {"show_all", true},
     {"show_icontext", false}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("find_in_files", QHash<QString,QVariant>(
    {{"enable", true},
     {"supported_encodings", QStringList({"utf-8", "iso-8859-1", "cp1252"})},
     {"exclude", EXCLUDE_PATTERNS},
     {"exclude_regexp", false},
     {"search_text_regexp", false},
     {"search_text", QStringList({""})},
     {"search_text_samples", QStringList({"(^|#)[ ]*(TODO|FIXME|XXX|HINT|TIP|@todo|HACK|BUG|OPTIMIZE|!!!|\\?\\?\\?)([^#]*)"})},
     {"more_options", true},
     {"case_sensitive", false}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("workingdir", QHash<QString,QVariant>(
    {{"working_dir_adjusttocontents", false},
     {"working_dir_history", 20},
     {"console/use_project_or_home_directory", false},
     {"console/use_cwd", true},
     {"console/use_fixed_directory", false}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("shortcuts", QHash<QString,QVariant>(
    {{"_/close pane", "Shift+Ctrl+F4"},
     {"_/lock unlock panes", "Shift+Ctrl+F5"},
     {"_/use next layout", "Shift+Alt+PgDown"},
     {"_/use previous layout", "Shift+Alt+PgUp"},
     {"_/preferences", "Ctrl+Alt+Shift+P"},
     {"_/maximize pane", "Ctrl+Alt+Shift+M"},
     {"_/fullscreen mode", "F11"},
     {"_/save current layout", "Shift+Alt+S"},
     {"_/layout preferences", "Shift+Alt+P"},
     {"_/show toolbars", "Alt+Shift+T"},
     {"_/spyder documentation", "F1"},
     {"_/restart", "Shift+Alt+R"},
     {"_/quit", "Ctrl+Q"},

     {"_/file switcher", "Ctrl+P"},
     {"_/symbol finder", "Ctrl+Alt+P"},
     {"_/debug", "Ctrl+F5"},
     {"_/debug step over", "Ctrl+F10"},
     {"_/debug continue", "Ctrl+F12"},
     {"_/debug step into", "Ctrl+F11"},
     {"_/debug step return", "Ctrl+Shift+F11"},
     {"_/debug exit", "Ctrl+Shift+F12"},
     {"_/run", "F5"},
     {"_/configure", "Ctrl+F6"},
     {"_/re-run last script", "F6"},

     {"_/switch to help", "Ctrl+Shift+H"},
     {"_/switch to outline_explorer", "Ctrl+Shift+O"},
     {"_/switch to editor", "Ctrl+Shift+E"},
     {"_/switch to historylog", "Ctrl+Shift+L"},
     {"_/switch to onlinehelp", "Ctrl+Shift+D"},
     {"_/switch to project_explorer", "Ctrl+Shift+P"},
     {"_/switch to ipython_console", "Ctrl+Shift+I"},
     {"_/switch to variable_explorer", "Ctrl+Shift+V"},
     {"_/switch to find_in_files", "Ctrl+Shift+F"},
     {"_/switch to explorer", "Ctrl+Shift+X"},

     {"_/find text", "Ctrl+F"},
     {"_/find next", "F3"},
     {"_/find previous", "Shift+F3"},
     {"_/replace text", "Ctrl+R"},
     {"_/hide find and replace", "Escape"},

     {"editor/code completion", "Ctrl+Space"},
     {"editor/duplicate line", "Ctrl+Alt+Up"},
     {"editor/copy line", "Ctrl+Alt+Down"},
     {"editor/delete line", "Ctrl+D"},
     {"editor/transform to uppercase", "Ctrl+Shift+U"},
     {"editor/transform to lowercase", "Ctrl+U"},
     {"editor/indent", "Ctrl+]"},
     {"editor/unindent", "Ctrl+["},
     {"editor/move line up", "Alt+Up"},
     {"editor/move line down", "Alt+Down"},
     {"editor/go to new line", "Ctrl+Shift+Return"},
     {"editor/go to definition", "Ctrl+G"},
     {"editor/toggle comment", "Ctrl+1"},
     {"editor/blockcomment", "Ctrl+4"},
     {"editor/unblockcomment", "Ctrl+5"},
     {"editor/start of line", "Meta+A"},
     {"editor/end of line", "Meta+E"},
     {"editor/previous line", "Meta+P"},
     {"editor/next line", "Meta+N"},
     {"editor/previous char", "Meta+B"},
     {"editor/next char", "Meta+F"},
     {"editor/previous word", "Meta+Left"},
     {"editor/next word", "Meta+Right"},
     {"editor/kill to line end", "Meta+K"},
     {"editor/kill to line start", "Meta+U"},
     {"editor/yank", "Meta+Y"},
     {"editor/rotate kill ring", "Shift+Meta+Y"},
     {"editor/kill previous word", "Meta+Backspace"},
     {"editor/kill next word", "Meta+D"},
     {"editor/start of document", "Ctrl+Home"},
     {"editor/end of document", "Ctrl+End"},
     {"editor/undo", "Ctrl+Z"},
     {"editor/redo", "Ctrl+Shift+Z"},
     {"editor/cut", "Ctrl+X"},
     {"editor/copy", "Ctrl+C"},
     {"editor/paste", "Ctrl+V"},
     {"editor/delete", "Delete"},
     {"editor/select all", "Ctrl+A"},

     {"editor/inspect current object", "Ctrl+I"},
     {"editor/breakpoint", "F12"},
     {"editor/conditional breakpoint", "Shift+F12"},
     {"editor/run selection", "F9"},
     {"editor/go to line", "Ctrl+L"},
     {"editor/go to previous file", "Ctrl+Shift+Tab"},
     {"editor/go to next file", "Ctrl+Tab"},
     {"editor/cycle to previous file", "Ctrl+PgUp"},
     {"editor/cycle to next file", "Ctrl+PgDown"},
     {"editor/new file", "Ctrl+N"},
     {"editor/open last closed", "Ctrl+Shift+T"},
     {"editor/open file", "Ctrl+O"},
     {"editor/save file", "Ctrl+S"},
     {"editor/save all", "Ctrl+Alt+S"},
     {"editor/save as", "Ctrl+Shift+S"},
     {"editor/close all", "Ctrl+Shift+W"},
     {"editor/last edit location", "Ctrl+Alt+Shift+Left"},
     {"editor/previous cursor position", "Ctrl+Alt+Left"},
     {"editor/next cursor position", "Ctrl+Alt+Right"},
     {"editor/zoom in 1", "Ctrl++"},
     {"editor/zoom in 2", "Ctrl+="},
     {"editor/zoom out", "Ctrl+-"},
     {"editor/zoom reset", "Ctrl+0"},
     {"editor/close file 1", "Ctrl+W"},
     {"editor/close file 2", "Ctrl+F4"},
     {"editor/run cell", "Ctrl+Return"},
     {"editor/run cell and advance", "Shift+Return"},
     {"editor/go to next cell", "Ctrl+Down"},
     {"editor/go to previous cell", "Ctrl+Up"},
     {"editor/re-run last cell", "Alt+Return"},

     {"_/switch to breakpoints", "Ctrl+Shift+B"},

     {"console/inspect current object", "Ctrl+I"},
     {"console/clear shell", "Ctrl+L"},
     {"console/clear line", "Shift+Escape"},

     {"pylint/run analysis", "F8"},

     {"ipython_console/new tab", "Ctrl+T"},
     {"ipython_console/reset namespace", "Ctrl+Alt+R"},
     {"ipython_console/restart kernel", "Ctrl+."},

     {"array_builder/enter array inline", "Ctrl+Alt+M"},
     {"array_builder/enter array table", "Ctrl+M"},

     {"variable_explorer/copy", "Ctrl+C"}
    })),
    QPair<QString, QHash<QString,QVariant>>
    ("color_schemes", QHash<QString,QVariant>(
    {{"background", QVariant::fromValue(ColorBoolBool("#ffffff", false, false))},
     {"currentline", QVariant::fromValue(ColorBoolBool("#f7ecf8", false, false))},
     {"currentcell", QVariant::fromValue(ColorBoolBool("#fdfdde", false, false))},
     {"occurrence", QVariant::fromValue(ColorBoolBool("#ffff99", false, false))},
     {"ctrlclick", QVariant::fromValue(ColorBoolBool("#0000ff", false, false))},
     {"sideareas", QVariant::fromValue(ColorBoolBool("#efefef", false, false))},
     {"matched_p", QVariant::fromValue(ColorBoolBool("#99ff99", false, false))},
     {"unmatched_p", QVariant::fromValue(ColorBoolBool("#ff9999", false, false))},
     {"normal", QVariant::fromValue(ColorBoolBool("#000000", false, false))},
     {"keyword", QVariant::fromValue(ColorBoolBool("#0000ff", false, false))},
     {"builtin", QVariant::fromValue(ColorBoolBool("#900090", false, false))},
     {"definition", QVariant::fromValue(ColorBoolBool("#000000", true, false))},
     {"comment", QVariant::fromValue(ColorBoolBool("#adadad", false, true))},
     {"string", QVariant::fromValue(ColorBoolBool("#00aa00", false, false))},
     {"number", QVariant::fromValue(ColorBoolBool("#800000", false, false))},
     {"instance", QVariant::fromValue(ColorBoolBool("#924900", false, true))}
    }))
};

QString qbytearray_to_str(const QByteArray& qba)
{
    // x = bytes(qba.toHex().data())
    // return str(x.decode())
    return QString(qba.toHex());
}

QVariant get_default(const QString& section, const QString& option)
{
    foreach (auto pair, DEFAULTS) {
        QString sec = pair.first;
        QHash<QString,QVariant> options = pair.second;
        if (sec == section) {
            if (options.contains(option))
                return options[option];
        }
    }
    return QVariant();
}

QVariant CONF_get(const QString& section, const QString& option, const QVariant& _default)
{
    QSettings settings;
    if (!settings.contains(section+"/"+option)) {
        if (_default == QVariant()) {
            qDebug()<<__FILE__<<__func__;
            //throw QString("No option %1 in section: %2").arg(option).arg(section);
        }
        else {
            CONF_set(section, option, _default);
            return _default;
        }
    }
    settings.beginGroup(section);
    QVariant val = settings.value(option, _default);
    settings.endGroup();
    return val;
}

void CONF_set(const QString& section, const QString& option, const QVariant& value)
{
    QSettings settings;
    settings.beginGroup(section);
    settings.setValue(option, value);
    settings.endGroup();
}

QVariant get_option(const QString& CONF_SECTION, const QString& option, const QVariant& _default)
{
    return CONF_get(CONF_SECTION, option, _default);
}

QHash<QString, QVariant> Factory(const QString& CONF_SECTION)
{
    QHash<QString, QVariant> kwargs;
    if (CONF_SECTION == "find_in_files") {
        QStringList supported_encodings = get_option(CONF_SECTION, "supported_encodings").toStringList();
        QStringList search_text_samples = get_option(CONF_SECTION, "search_text_samples").toStringList();
        QString search_text = search_text_samples[0];
        bool search_text_regexp = get_option(CONF_SECTION, "search_text_regexp").toBool();
        QString exclude = EXCLUDE_PATTERNS[0];
        int exclude_idx = get_option(CONF_SECTION, "exclude_idx", -1).toInt();
        bool exclude_regexp = get_option(CONF_SECTION, "exclude_regexp").toBool();
        bool more_options = get_option(CONF_SECTION, "more_options").toBool();
        bool case_sensitive = get_option(CONF_SECTION, "case_sensitive").toBool();
        QStringList path_history = get_option(CONF_SECTION, "path_history", QStringList()).toStringList();

        kwargs["search_text"] = search_text;
        kwargs["search_text_regexp"] = search_text_regexp;
        kwargs["exclude"] = exclude;
        kwargs["exclude_idx"] = exclude_idx;
        kwargs["exclude_regexp"] = exclude_regexp;
        kwargs["supported_encodings"] = supported_encodings;
        kwargs["more_options"] = more_options;
        kwargs["case_sensitive"] = case_sensitive;
        kwargs["external_path_history"] = path_history;
        return kwargs;
    }
    return kwargs;
}
