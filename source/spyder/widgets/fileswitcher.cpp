#include "fileswitcher.h"
#include "editor.h"

struct IntIntStr
{
    int key;
    int fold_level;
    QString get_token;
    IntIntStr(int _key, int _fold_level, QString _get_token)
    {
        key = _key;
        fold_level = _fold_level;
        get_token = _get_token;
    }
};

struct IntStrIntStr
{
    int key;
    int fold_level;
    QString def_name;
    QString get_token;
    IntStrIntStr()
    {
        key = -1;
        def_name = QString();
        fold_level = -1;
        get_token = QString();
    }
    IntStrIntStr(int _key, QString _def_name, int _fold_level, QString _get_token)
    {
        key = _key;
        def_name = _def_name;
        fold_level = _fold_level;
        get_token = _get_token;
    }
};

bool comp(IntStrIntStr val1, IntStrIntStr val2)
{
    return val1.key <= val2.key;
}

QList<IntStrIntStr> process_python_symbol_data(QHash<int,sh::OutlineExplorerData> oedata)
{
    QList<IntStrIntStr> symbol_list;
    foreach (int key, oedata.keys()) {
        sh::OutlineExplorerData val = oedata[key];
        if (val.fold_level != -1) {
            if (val.is_class_nor_function())
                symbol_list.append(IntStrIntStr(key, val.def_name, val.fold_level, val.get_token()));
        }
    }
    std::sort(symbol_list.begin(), symbol_list.end(), comp);
    return symbol_list;
}

QList<QIcon> get_python_symbol_icons(QHash<int,sh::OutlineExplorerData> oedata)
{
    QIcon class_icon = ima::icon("class");
    QIcon method_icon = ima::icon("method");
    QIcon function_icon = ima::icon("function");
    QIcon private_icon = ima::icon("private1");
    QIcon super_private_icon = ima::icon("private2");

    QList<IntStrIntStr> symbols = process_python_symbol_data(oedata);

    QSet<int> set;
    foreach (auto pair, symbols) {
        set.insert(pair.fold_level);
    }
    QList<int> fold_levels = set.toList();
    std::sort(fold_levels.begin(), fold_levels.end());
    QList<IntStrIntStr> parents;
    QList<QIcon> icons;
    for (int i = 0; i < symbols.size(); ++i) {
        parents.append(IntStrIntStr());
        icons.append(QIcon());
    }
    //icons
    QList<int> indexes;

    IntStrIntStr parent;
    foreach (int level, fold_levels) {
        for (int index = 0; index < symbols.size(); ++index) {
            IntStrIntStr item = symbols[index];
            int fold_level = item.fold_level;
            if (indexes.contains(index))
                continue;

            if (fold_level == level) {
                indexes.append(index);
                parent = item;
            }
            else
                parents[index] = item;
        }
    }

    for (int index = 0; index < symbols.size(); ++index) {
        IntStrIntStr item = symbols[index];
        parent = parents[index];

        if (item.get_token == "def")
            icons[index] = function_icon;
        else if (item.get_token == "class")
            icons[index] = class_icon;

        if (parent.fold_level != -1) {
            if (parent.get_token == "class") {
                if (item.get_token == "def" && item.def_name.startsWith("__"))
                    icons[index] = super_private_icon;
                else if (item.get_token == "def" && item.def_name.startsWith("_"))
                    icons[index] = private_icon;
                else
                    icons[index] = method_icon;
            }
        }
    }
    return icons;
}


KeyPressFilter::KeyPressFilter(QObject* parent)
    : QObject (parent)
{}

bool KeyPressFilter::eventFilter(QObject *src, QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent* keyevent = static_cast<QKeyEvent*>(e);
        if (keyevent->key() == Qt::Key_Up)
            emit sig_up_key_pressed();
        else if (keyevent->key() == Qt::Key_Down)
            emit sig_down_key_pressed();
    }
    return QObject::eventFilter(src, e);
}


/********** FilesFilterLine **********/
FilesFilterLine::FilesFilterLine(QWidget* paren)
    : QLineEdit (paren)
{
    clicked_outside = false;
}

void FilesFilterLine::focusOutEvent(QFocusEvent *event)
{
    clicked_outside = true;
    QLineEdit::focusOutEvent(event);
}


/********** FileSwitcher **********/
const int FileSwitcher::FILE_MODE = 1;
const int FileSwitcher::SYMBOL_MODE = 2;
const int FileSwitcher::MAX_WIDTH = 600;

FileSwitcher::FileSwitcher(QWidget* parent, EditorStack* plugin, QTabWidget* tabs,
                           const QList<FileInfo*>& data, const QIcon& icon)
    : QDialog (parent)
{
    add_plugin(plugin, tabs, data, icon);
    this->mode = this->FILE_MODE;
    line_number = -1;
    is_visible = false;

    QString help_text = "Press <b>Enter</b> to switch files or <b>Esc</b> to "
                        "cancel.<br><br>Type to filter filenames.<br><br>"
                        "Use <b>:number</b> to go to a line, e.g. "
                        "<b><code>main:42</code></b><br>"
                        "Use <b>@symbol_text</b> to go to a symbol, e.g. "
                        "<b><code>@init</code></b>"
                        "<br><br> Press <b>Ctrl+W</b> to close current tab.<br>";
    QRegExp regex("([A-Za-z0-9_]{0,100}@[A-Za-z0-9_]{0,100})|([A-Za-z0-9_]{0,100}:{0,1}[0-9]{0,100})");

    edit = new FilesFilterLine(this);
    help = new HelperToolButton;
    list = new QListWidget(this);
    filter = new KeyPressFilter;
    QRegExpValidator* regex_validator = new QRegExpValidator(regex, edit);

    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setWindowOpacity(0.95);
    edit->installEventFilter(filter);
    edit->setValidator(regex_validator);
    help->setToolTip(help_text);
    list->setItemDelegate(new HTMLDelegate(this));

    QHBoxLayout* edit_layout = new QHBoxLayout;
    edit_layout->addWidget(edit);
    edit_layout->addWidget(help);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addLayout(edit_layout);
    layout->addWidget(list);
    setLayout(layout);

    connect(this,SIGNAL(rejected()),this,SLOT(restore_initial_state()));
    connect(filter,SIGNAL(sig_up_key_pressed()),this,SLOT(previous_row()));
    connect(filter,SIGNAL(sig_down_key_pressed()),this,SLOT(next_row()));
    connect(edit,SIGNAL(returnPressed()),this,SLOT(accept()));
    connect(edit,SIGNAL(textChanged(QString)),this,SLOT(setup()));
    connect(list,SIGNAL(itemSelectionChanged()),this,SLOT(item_selection_changed()));
    connect(list,SIGNAL(clicked(QModelIndex)),edit,SLOT(setFocus()));
}

QList<QPair<QWidget*, EditorStack*>> FileSwitcher::widgets()
{
    QList<QPair<QWidget*, EditorStack*>> widgets;
    foreach (EditorStack* plugin, plugins_instances) {
        QTabWidget* tabs = get_plugin_tabwidget(plugin);
        for (int index = 0; index < tabs->count(); ++index) {
            widgets.append(qMakePair(tabs->widget(index), plugin));
        }
    }
    return widgets;
}

QList<int> FileSwitcher::line_count()
{
    QList<int> line_count;
    foreach (auto pair, widgets()) {
        int current_line_count;
        CodeEditor* code_editor = qobject_cast<CodeEditor*>(pair.first);
        if (code_editor)
            current_line_count = code_editor->get_line_count();
        else
            current_line_count = 0;
        line_count.append(current_line_count);
    }
    return line_count;
}

QList<bool> FileSwitcher::save_status()
{
    QList<bool> save_status;
    foreach (auto pair, plugins_data) {
        QList<FileInfo*> da = pair.first;
        foreach (FileInfo* td, da) {
            save_status.append(td->newly_created);
        }
    }
    return save_status;
}

QStringList FileSwitcher::paths()
{
    QStringList paths;
    foreach (EditorStack* plugin, plugins_instances) {
        QList<FileInfo*> da = get_plugin_data(plugin);
        foreach (FileInfo* td, da) {
            paths.append(td->filename);
        }
    }
    return paths;
}

QStringList FileSwitcher::filenames()
{
    QStringList filenames;
    foreach (EditorStack* plugin, plugins_instances) {
        QList<FileInfo*> da = get_plugin_data(plugin);
        foreach (FileInfo* td, da) {
            QFileInfo info(td->filename);
            filenames.append(info.fileName());
        }
    }
    return filenames;
}

QList<QIcon> FileSwitcher::icons()
{
    QList<QIcon> icons;
    foreach (auto pair, plugins_data) {
        QList<FileInfo*> da = pair.first;
        QIcon icon = pair.second;
        for (int i=0;i<da.size();i++)
            icons.append(icon);
    }
    return icons;
}

QString FileSwitcher::current_path()
{
    return paths_by_widget().value(get_widget(), QString());
}

QHash<QWidget*, QString> FileSwitcher::paths_by_widget()
{
    QHash<QWidget*, QString> dict;
    int len = qMin(widgets().size(), paths().size());
    for (int i=0;i<len;i++) {
        QWidget* widget = widgets()[i].first;
        dict[widget] = paths()[i];
    }
    return dict;
}

QHash<QString, QWidget*> FileSwitcher::widgets_by_path()
{
    QHash<QString, QWidget*> dict;
    int len = qMin(widgets().size(), paths().size());
    for (int i=0;i<len;i++) {
        QWidget* widget = widgets()[i].first;
        dict[paths()[i]] = widget;
    }
    return dict;
}

QString FileSwitcher::filter_text() const
{
    return edit->text().toLower();
}

void FileSwitcher::set_search_text(const QString &_str)
{
    edit->setText(_str);
}

void FileSwitcher::save_initial_state()
{
    QStringList paths = this->paths();
    initial_widget = get_widget();
    initial_cursors.clear();

    // TODO
}

//@Slot()
void FileSwitcher::accept()
{
    is_visible = false;
    QDialog::accept();
    list->clear();
}

//@Slot()
void FileSwitcher::restore_initial_state()
{
    this->list->clear();
    is_visible = false;
    QHash<QString, QWidget*> widgets = widgets_by_path();

    if (!edit->clicked_outside) {
        foreach (QString path, initial_cursors.keys()) {
            QTextCursor cursor = initial_cursors[path];
            if (widgets.contains(path))
                set_editor_cursor(widgets[path], cursor);
        }

        if (paths_by_widget().contains(initial_widget)) {
            int index = paths().indexOf(initial_path);
            if (index > -1)
                emit sig_goto_file(index, nullptr);
        }
    }
}

void FileSwitcher::set_dialog_position()
{
    QWidget* parent = qobject_cast<QWidget*>(this->parent());
    QRect geo = parent->geometry();
    int width = list->width();
    int left = parent->geometry().width()/2 - width/2;

    int top;
    //if isinstance(parent, QMainWindow)
    //parent.toolbars_menu
    top = plugins_tabs[0].first->tabBar()->geometry().height()+1;

    while (parent) {
        geo = parent->geometry();
        top += geo.top();
        left += geo.left();
        parent = qobject_cast<QWidget*>(parent->parent());
    }
	this->move(left, top);
}

QSize FileSwitcher::get_item_size(const QStringList &content)
{
    QStringList strings;
    if (!content.isEmpty()) {
        for (int i = 0; i < content.size()-1; ++i) {
            QLabel label(content[i]);
            label.setTextFormat(Qt::PlainText);
            strings.append(label.text());
        }
        QLabel label(content.last());
        label.setTextFormat(Qt::PlainText);
        strings.append(label.text());
        QFontMetrics fm(label.fontMetrics());

        double width = fm.width(strings[0]) * 1.3;
        for (int i = 1; i < strings.size(); ++i) {
            if (fm.width(strings[i]) * 1.3 > width)
                width = fm.width(strings[i]) * 1.3;
        }
        return QSize(static_cast<int>(width), fm.height());
    }
    return QSize();
}

void FileSwitcher::fix_size(const QStringList &content)
{
    if (!content.isEmpty()) {
        QSize size = get_item_size(content);
        int height = size.height();

        QWidget* parent = qobject_cast<QWidget*>(this->parent());
        double relative_width = parent->geometry().width() * 0.65;
        if (relative_width > MAX_WIDTH)
            relative_width = MAX_WIDTH;
        list->setMinimumWidth(static_cast<int>(relative_width));

        int max_entries;
        if (content.size() < 8)
            max_entries = content.size();
        else
            max_entries = 8;
        double max_height = height * max_entries * 2.5;
        list->setMinimumHeight(static_cast<int>(max_height));

        list->resize(static_cast<int>(relative_width), list->height());
    }
}

int FileSwitcher::count()
{
    return this->list->count();
}

int FileSwitcher::current_row()
{
    return this->list->currentRow();
}

void FileSwitcher::set_current_row(int row)
{
    this->list->setCurrentRow(row);
}

void FileSwitcher::select_row(int steps)
{
    int row = current_row() + steps;
    if (row >= 0 && row < count())
        set_current_row(row);
}

//@Slot()
void FileSwitcher::previous_row()
{
    if (mode == SYMBOL_MODE) {
        select_row(-1);
        return;
    }
    int prev_row = current_row()-1;
    QString title;
    if (prev_row >= 0)
        title = list->item(prev_row)->text();
    else
        title = "";
    if (prev_row == 0 && title.contains("</b></big><br>"))
        list->scrollToTop();
    else if (title.contains("</b></big><br>"))
        select_row(-2);
    else
        select_row(-1);
}

//@Slot()
void FileSwitcher::next_row()
{
    if (mode == SYMBOL_MODE) {
        select_row(1);
        return;
    }
    int next_row = current_row() + 1;
    if (next_row < count()) {
        if (list->item(next_row)->text().contains("</b></big><br>"))
            select_row(2);
        else
            select_row(1);
    }
}

int FileSwitcher::get_stack_index(int stack_index, int plugin_index)
{
    int other_plugins_count = 0;
    for (int i=0;i<plugin_index;i++) {
        QTabWidget* other_tabs = plugins_tabs[i].first;
        other_plugins_count += other_tabs->count();
    }
    int real_index = stack_index - other_plugins_count;
    return real_index;
}

// --- Helper methods: Widget
QList<FileInfo*> FileSwitcher::get_plugin_data(EditorStack* plugin)
{
    return plugin->get_current_tab_manager()->data;
}

QTabWidget* FileSwitcher::get_plugin_tabwidget(EditorStack* plugin)
{
    return plugin->get_current_tab_manager()->tabs;
}

QWidget* FileSwitcher::get_widget(int index,const QString& path,QTabWidget* tabs)
{
    if ((index >= 0 && tabs) || (!path.isEmpty() && tabs))
        return tabs->widget(index);
    else if (plugin)
        return get_plugin_tabwidget(plugin)->currentWidget();
    else
        return plugins_tabs[0].first->currentWidget();
}

void FileSwitcher::set_editor_cursor(QWidget *editor, QTextCursor cursor)
{
    int pos = cursor.position();
    int anchor = cursor.anchor();

    QTextCursor new_cursor;
    // TODO
}

void FileSwitcher::goto_line(int line_number)
{
    if (line_number >= 0) {
        plugin->go_to_line(line_number);
    }
}

QHash<int,sh::OutlineExplorerData> FileSwitcher::get_symbol_list()
{
    QHash<int,sh::OutlineExplorerData> oedata;
    CodeEditor* code_editor = qobject_cast<CodeEditor*>(get_widget());
    if (code_editor)
        oedata = code_editor->get_outlineexplorer_data();
    return oedata;
}

//@Slot()
void FileSwitcher::item_selection_changed()
{
    int row = current_row();
    if (count() && row >= 0) {
        if (list->currentItem()->text().contains("</b></big><br>") && row == 0)
            next_row();
        if (mode == FILE_MODE) {
            try {
                int stack_index = paths().indexOf(filtered_path[row]);
                plugin = widgets()[stack_index].second;
                int plugin_index = plugins_instances.indexOf(plugin);

                int real_index = get_stack_index(stack_index, plugin_index);
                emit sig_goto_file(real_index, plugin->get_current_tab_manager());
                goto_line(line_number);

                /*
                 * try:
                 * self.plugin.switch_to_plugin()
                 * self.raise_()
                */
                edit->setFocus();
            } catch (...) {

            }
        }
        else {
            int line_number = filtered_symbol_lines[row];
            goto_line(line_number);
        }
    }
}

void FileSwitcher::setup_file_list(QString filter_text, const QString &current_path)
{
    // shorten_paths()函数太麻烦了，不写了
    QStringList short_paths = paths();
    QStringList paths = this->paths();
    QList<QIcon> icons = this->icons();
    QList<QPair<int, QString>> results;
    bool trying_for_line_number = filter_text.contains(':');

    int line_number;
    if (trying_for_line_number) {
        bool ok;
        line_number = filter_text.split(':')[1].toInt(&ok);
        if (ok == false)
            line_number = -1;
    }
    else
        line_number = -1;

    fix_size(paths);

    QStringList filenames = this->filenames();
    QList<int> line_count = this->line_count();
    for (int index=0;index<filenames.size();index++) {
        QString rich_text = filenames[index];
        QString text_item = "<big>" + rich_text.replace("&", "") + "</big>";
        if (trying_for_line_number)
            text_item += QString(" [%1 %2]").arg(line_count[index]).arg("lines");
        text_item += QString("<br><i>%1</i>").arg(paths[index]);
        if ((trying_for_line_number && line_count[index] != 0) ||
                !trying_for_line_number)
            results.append(QPair<int, QString>(index, text_item));
    }

    foreach (auto result, results) {
        int index = result.first;
        QString path = paths[index];
        QIcon icon = icons[index];
        QString text = "";
        text += result.second;
        QListWidgetItem* item = new QListWidgetItem(icon, text);
        item->setToolTip(path);
        item->setSizeHint(QSize(0, 25));
        list->addItem(item);
        filtered_path.append(path);
    }

    //list.files_list = True

    if (filtered_path.contains(current_path))
        set_current_row(filtered_path.indexOf(current_path));
    else if (!filtered_path.empty())
        set_current_row(0);

    this->line_number = line_number;
    goto_line(line_number);
}

void FileSwitcher::setup_symbol_list(QString filter_text, const QString &current_path)
{
    QStringList tmp = filter_text.split('@');
    filter_text = tmp[0];
    QString symbol_text = tmp[1];

    QHash<int,sh::OutlineExplorerData> oedata = get_symbol_list();
    QList<QIcon> icons = get_python_symbol_icons(oedata);

    QStringList paths = this->paths();
    fix_size(paths);

    QList<IntStrIntStr> symbol_list = process_python_symbol_data(oedata);
    QList<IntIntStr> line_fold_token;
    QStringList choices;
    foreach (auto pair, symbol_list) {
        line_fold_token.append(IntIntStr(pair.key, pair.fold_level, pair.get_token));
        choices.append(pair.def_name);
    }

    this->edit->setFocus();
}

void FileSwitcher::setup()
{
    if (plugins_tabs.size() == 0) {
        close();
        return;
    }

    this->list->clear();
    QString current_path = this->current_path();
    QString filter_text = this->filter_text();

    bool trying_for_symbol = filter_text.contains('@');

    if (trying_for_symbol) {
        this->mode = SYMBOL_MODE;
        setup_symbol_list(filter_text, current_path);
    }
    else {
        this->mode = FILE_MODE;
        setup_file_list(filter_text, current_path);
    }
    set_dialog_position();
}

void FileSwitcher::show()
{
    setup();
    QDialog::show();
}

void FileSwitcher::add_plugin(EditorStack *plugin, QTabWidget *tabs, const QList<FileInfo *> &data, const QIcon &icon)
{
    plugins_tabs.append(qMakePair(tabs, plugin));
    plugins_data.append(qMakePair(data, icon));
    plugins_instances.append(plugin);
}

//该文件功能未实现
