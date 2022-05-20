//#include "fileswitcher.h"
#include "editor.h"
#include "plugins/plugins_editor.h"

static bool DEBUG_EDITOR = DEBUG >= 3;//

QList<QList<QVariant>> check_with_pyflakes(const QString&)
{
    qDebug() << __func__;
    QList<QList<QVariant>> res;
    QList<QVariant> tmp;
    tmp.append(QString("pyflakes"));
    tmp.append(1);
    res.append(tmp);

    tmp.clear();
    tmp.append(QString("check_with_pyflakes"));
    tmp.append(2);
    res.append(tmp);
    return res;
}

QList<QList<QVariant>> check_with_pep8(const QString&)
{
    qDebug() << __func__;
    QList<QList<QVariant>> res;
    QList<QVariant> tmp;
    tmp.append(QString("pep8"));
    tmp.append(3);
    res.append(tmp);

    tmp.clear();
    tmp.append(QString("check_with_pep8"));
    tmp.append(4);
    res.append(tmp);
    return res;
}

static QString TASKS_PATTERN = "(^|#)[ ]*(TODO|FIXME|XXX|HINT|TIP|@todo|"
                               "HACK|BUG|OPTIMIZE|!!!|\\?\\?\\?)([^#]*)";
static QString CPP_TASKS_PATTERN = "(^|//)[ ]*(TODO|FIXME|XXX|HINT|TIP|@todo|"
                               "HACK|BUG|OPTIMIZE|!!!|\\?\\?\\?)([^/{2}]*)";

QList<QList<QVariant>> find_tasks(const QString& source_code)
{
    QList<QList<QVariant>> results;

    QStringList lines = splitlines(source_code);
    for (int line = 0; line < lines.size(); ++line) {
        QString text = lines[line];
        QRegularExpression re(TASKS_PATTERN);
        QRegularExpressionMatchIterator it = re.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString todo_text;
            QString x = match.captured(3);
            if (!x.isEmpty()) {
                x = lstrip(x, " :");
                x = rstrip(x, " :");
                if (!x.isEmpty())
                    x[0] = x[0].toUpper();
                todo_text = x;
            }
            else
                todo_text = match.captured(2);

            QList<QVariant> tmp;
            tmp.append(todo_text);
            tmp.append(line + 1);
            results.append(tmp);
        }
    }
    return results;
}

QList<QList<QVariant>> cpp_find_tasks(const QString& source_code)
{
    QList<QList<QVariant>> results;

    QStringList lines = splitlines(source_code);
    for (int line = 0; line < lines.size(); ++line) {
        QString text = lines[line];
        QRegularExpression re(CPP_TASKS_PATTERN);
        QRegularExpressionMatchIterator it = re.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString todo_text;
            QString x = match.captured(3);
            if (!x.isEmpty()) {
                x = lstrip(x, " :");
                x = rstrip(x, " :");
                if (!x.isEmpty())
                    x[0] = x[0].toUpper();
                todo_text = x;
            }
            else
                todo_text = match.captured(2);

            QList<QVariant> tmp;
            tmp.append(todo_text);
            tmp.append(line + 1);
            results.append(tmp);
        }
    }
    return results;
}

AnalysisThread::AnalysisThread(QObject* parent, FUNC_CHECKER checker,
               const QString& source_code)
    : QThread (parent)
{
    this->checker = checker;
    this->results.clear();
    this->source_code = source_code;
}

void AnalysisThread::run()
{
	// Run analysis
    this->results = this->checker(this->source_code);
}


ThreadManager::ThreadManager(QObject* parent, int max_simultaneous_threads)
    : QObject (parent)
{
    this->max_simultaneous_threads = max_simultaneous_threads;
    this->started_threads.clear();
    this->pending_threads.clear();
    this->end_callbacks.clear();
}

void ThreadManager::close_threads(FileInfo *parent)
{
	// Close threads associated to parent_id
    if (DEBUG_EDITOR)
        qDebug() << "Call to 'close_threads'";

    QList<AnalysisThread*> threadlist;
    if (parent == nullptr) {
		// Closing all threads
        this->pending_threads.clear();
        foreach (auto val, this->started_threads.values()) {
            threadlist.append(val);
        }
    }
    else {
        QList<QPair<AnalysisThread*, FileInfo*>> tmp = this->pending_threads;
        this->pending_threads.clear();
        foreach (auto pair, tmp) {
            if (pair.second != parent)
                this->pending_threads.append(pair);
        }
        threadlist = this->started_threads.value(parent,QList<AnalysisThread*>());
    }

    foreach (AnalysisThread* thread, threadlist) {
        if (DEBUG_EDITOR)
            qDebug() << "Waiting for thread " << thread << " to finish";
        while (thread->isRunning()) {
			// We can't terminate thread safely, so we simply wait...
            QApplication::processEvents();
        }
    }
}

void ThreadManager::close_all_threads()
{
    if (DEBUG_EDITOR)
        qDebug() << "Call to 'close_all_threads'";
    this->close_threads(nullptr);
}

void ThreadManager::add_thread(FUNC_CHECKER checker, FUNC_END_CALLBACK end_callback,
                               const QString &source_code, FileInfo *parent)
{
	// Add thread to queue
    AnalysisThread* thread = new AnalysisThread(this, checker, source_code);
    this->end_callbacks[thread] = end_callback;
    this->pending_threads.append(QPair<AnalysisThread*, FileInfo*>(thread,parent));
    if (DEBUG_EDITOR)
        qDebug() << "Added thread " << thread << " to queue";
    QTimer::singleShot(50, this, SLOT(update_queue()));
}

void ThreadManager::update_queue()
{
    int started = 0;
    QHash<FileInfo*,QList<AnalysisThread*>> tmp = this->started_threads;

    for (auto it=tmp.begin(); it!=tmp.end(); it++) {
		FileInfo* parent = it.key();
        QList<AnalysisThread*> threadlist = it.value();

        QList<AnalysisThread*> still_running;
        foreach (AnalysisThread* thread, threadlist) {
            if (thread->isFinished()) {
				/*
				auto pair = this->end_callbacks.take(reinterpret_cast<size_t>(thread));
				FileInfo* finfo = pair.first;
				FUNC_END_CALLBACK end_callback = pair.second;
				if (!thread->results.isEmpty()) {
					(finfo->*end_callback)(thread->results);
					//如何在外部调用类的成员函数指针，见上一行
				}
				*/
				FUNC_END_CALLBACK end_callback = this->end_callbacks.take(thread);
				if (!thread->results.isEmpty()) {
					// The thread was executed successfully
					end_callback(thread->results);
				}
                thread->setParent(nullptr);
                thread->deleteLater();
                thread = nullptr;
            }
            else {
                still_running.append(thread);
                started += 1;
            }
        }
        // threadlist = None 这句在c++中应该没用
        if (!still_running.isEmpty())
            this->started_threads[parent] = still_running;
        else
            this->started_threads.remove(parent);
    }

    if (DEBUG_EDITOR) {
        qDebug() << "Updating queue:";
        qDebug() << "    started:" << started;
        qDebug() << "    pending:" << this->pending_threads.size();
    }
    if (!pending_threads.isEmpty() && started < max_simultaneous_threads) {
        auto pair = this->pending_threads.takeAt(0);
        AnalysisThread* thread = pair.first;
		FileInfo* parent = pair.second;
        connect(thread, SIGNAL(finished()), this, SLOT(update_queue()));
        QList<AnalysisThread*> threadlist = this->started_threads.value(parent,QList<AnalysisThread*>());
        threadlist.append(thread);
        this->started_threads[parent] = threadlist;
        if (DEBUG_EDITOR)
            qDebug() << "===>starting:" << thread;
        thread->start();
    }
}


FileInfo::FileInfo(const QString& filename, const QString& encoding,
                   CodeEditor* editor, bool _new, ThreadManager *threadmanager)
    : QObject ()
{
    this->threadmanager = threadmanager;
    this->filename = filename;
    this->newly_created = _new;
    this->_default = false;// Default untitled file
    this->encoding = encoding;
    this->editor = editor;
    this->path = QStringList();//在mainwindow或是plugins/editor中用到

    // classes = (filename, None, None)没用到
    this->analysis_results.clear();
    this->todo_results.clear();
    lastmodified = QFileInfo(filename).lastModified();

    connect(editor, SIGNAL(textChanged()),this,SLOT(text_changed()));
    connect(editor, SIGNAL(breakpoints_changed()),this,SLOT(breakpoints_changed()));

    this->pyflakes_results.clear();
    this->pep8_results.clear();
}

//@Slot()
void FileInfo::text_changed()
{
    _default = false;
    emit text_changed_at(filename, editor->get_position("cursor"));
}

QString FileInfo::get_source_code() const
{
    return editor->toPlainText();
}

void FileInfo::run_code_analysis(bool run_pyflakes, bool run_pep8)
{
    this->pyflakes_results.clear();
    this->pep8_results.clear();/*
    if (editor->is_python()) {
        QString source_code = this->get_source_code();
        if (run_pyflakes) {
            this->threadmanager->add_thread(check_with_pyflakes, this,
                                            &FileInfo::pyflakes_analysis_finished,
                                            source_code, this);
        }
        if (run_pep8) {
            this->threadmanager->add_thread(check_with_pep8, this,
                                            &FileInfo::pep8_analysis_finished,
                                            source_code, this);
        }
    }*/
}

void FileInfo::pyflakes_analysis_finished(const QList<QList<QVariant> >& results)
{
    this->pyflakes_results = results;
    if (!this->pep8_results.isEmpty())
        this->code_analysis_finished();
}

void FileInfo::pep8_analysis_finished(const QList<QList<QVariant> >& results)
{
    this->pep8_results = results;
    if (!this->pyflakes_results.isEmpty())
        this->code_analysis_finished();
}

void FileInfo::code_analysis_finished()
{
    QList<QList<QVariant> > tmp = pyflakes_results + pep8_results;
    this->set_analysis_results(tmp);
    emit analysis_results_changed();
}

void FileInfo::set_analysis_results(const QList<QList<QVariant> > &results)
{
    this->analysis_results = results;
    this->editor->process_code_analysis(results);
}

void FileInfo::cleanup_analysis_results()
{
    this->analysis_results.clear();
    this->editor->cleanup_code_analysis();
}

void FileInfo::run_todo_finder()
{
	auto end_callback = [&](const QList<QList<QVariant> >& results)
		->void { this->todo_finished(results); };
    if (this->editor->is_python()) {
        this->threadmanager->add_thread(find_tasks, end_callback,
                                        this->get_source_code(), this);
    }
    else if (this->editor->is_cpp()) {
        this->threadmanager->add_thread(cpp_find_tasks, end_callback,
                                        this->get_source_code(), this);
    }
}

void FileInfo::todo_finished(const QList<QList<QVariant> >& results)
{
    this->set_todo_results(results);
    emit todo_results_changed();
}

void FileInfo::set_todo_results(const QList<QList<QVariant> > &results)
{
    this->todo_results = results;
    this->editor->process_todo(results);
}

void FileInfo::cleanup_todo_results()
{
    todo_results.clear();
}

void FileInfo::breakpoints_changed()
{
    QList<QList<QVariant>> breakpoints = editor->get_breakpoints();
    if (editor->breakpoints != breakpoints) {
        editor->breakpoints = breakpoints;

        emit save_breakpoints(filename, repr(breakpoints));
    }
}


/********** StackHistory **********/
StackHistory::StackHistory(EditorStack* editor)
{
    this->history.clear();
    this->id_list.clear();
    this->editor = editor;
}

void StackHistory::_update_id_list()
{
    id_list.clear();
    for (int i = 0; i < editor->tabs->count(); ++i) {
        size_t val = reinterpret_cast<size_t>(editor->tabs->widget(i));
        id_list.append(val);
    }
}

void StackHistory::refresh()
{
    this->_update_id_list();
    QList<size_t> history = this->history;
    foreach (size_t _id, history) {
        if (!id_list.contains(_id))
            this->history.removeOne(_id);
    }
}

int StackHistory::len()
{
    return history.size();
}

int StackHistory::getitem(int i)
{
    this->_update_id_list();
    if (i >= 0 && i < history.size()) {
        int idx = id_list.indexOf(history[i]);
        if (idx == -1)
            this->refresh();
        return idx;
        //返回值为-1，对应源码except ValueError:raise IndexError
    }
    return -2;//对应其他错误
}

bool StackHistory::contains(int value)
{
    for (int i = 0; i < this->len(); ++i) {
        if (this->getitem(i) == value)
            return true;
    }
    return false;
}

void StackHistory::setitem(int i, int v)
{
    Q_ASSERT(0 <= v && v < editor->tabs->count());
    size_t _id = reinterpret_cast<size_t>(editor->tabs->widget(v));
    history[i] = _id;
}

void StackHistory::reverse()
{
    int n = this->len();
    for (int i = 0; i < n/2; ++i) {
        int tmp_left = this->getitem(i);
        int tmp_right = this->getitem(n-i-1);
        this->setitem(i, tmp_right);
        this->setitem(n-i-1, tmp_left);
    }
}

void StackHistory::insert(int i, int tab_index)
{
    Q_ASSERT(0 <= tab_index && tab_index < editor->tabs->count());
    size_t _id = reinterpret_cast<size_t>(editor->tabs->widget(tab_index));
    history.insert(i, _id);
}

void StackHistory::append(int tab_index)
{
    this->insert(this->len(), tab_index);
}

void StackHistory::remove(int tab_index)
{
    Q_ASSERT(0 <= tab_index && tab_index < editor->tabs->count());
    size_t _id = reinterpret_cast<size_t>(editor->tabs->widget(tab_index));
    if (history.contains(_id))
        history.removeOne(_id);
}


/********** TabSwitcherWidget **********/
// Show tabs in mru order and change between them.
TabSwitcherWidget::TabSwitcherWidget(QWidget* parent, StackHistory stack_history, BaseTabs* tabs)
    : QListWidget (parent), stack_history(stack_history)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);

    editor = qobject_cast<EditorStack*>(parent);
    this->tabs = tabs;

    setSelectionMode(QListWidget::SingleSelection);
    connect(this,SIGNAL(itemActivated(QListWidgetItem*)),
            this,SLOT(item_selected(QListWidgetItem*)));

    this->load_data();
    QSize size = CONF_get("main", "completion/size").toSize();
    this->resize(size);
    this->set_dialog_position();
    this->setCurrentRow(0);

    QString context = "Editor";
    QString name = "Go to previous file";
    QString keystr = gui::get_shortcut(context, name);//"Ctrl+Shift+Tab"
    QShortcut* qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [this](){ this->select_row(-1); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);

    context = "Editor";
    name = "Go to next file";
    keystr = gui::get_shortcut(context, name);//"Ctrl+Tab";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [this](){ this->select_row(1); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
}

void TabSwitcherWidget::load_data()
{
    StackHistory list = stack_history;
    list.reverse();
    for (int i = 0; i < list.len(); ++i) {
        int index = list.getitem(i);
        QString text = tabs->tabText(index);
        text.replace("&", "");
        QListWidgetItem* item = new QListWidgetItem(ima::icon("TextFileIcon"),text);
        this->addItem(item);
    }
}

void TabSwitcherWidget::item_selected(QListWidgetItem* item)
{
    if (item == nullptr) {
        item = this->currentItem();
		Q_UNUSED(item);
	}
    int len = stack_history.len();
    // stack history is in inverse order
    int index = stack_history.getitem(len - (this->currentRow()+1));
    if (index >= 0) {
        editor->set_stack_index(index);
        editor->current_changed(index);
    }
    this->hide();//可以通过注释该行看到TabSwitcherWidget
}

void TabSwitcherWidget::select_row(int steps)
{
    int tmp = this->currentRow() + steps;
    if (tmp < 0)
        // 当currentRow()=0，steps=-1时需要这样处理
        tmp += this->count();
    int row = tmp % this->count();
    this->setCurrentRow(row);
}

void TabSwitcherWidget::set_dialog_position()
{
    int left = editor->geometry().width()/2 - width()/2;
    int top = editor->tabs->tabBar()->geometry().height();
    this->move(editor->mapToGlobal(QPoint(left, top)));
}

void TabSwitcherWidget::keyReleaseEvent(QKeyEvent *event)
{
    // Handle "most recent used" tab behavior,
    if (this->isVisible()) {
        QString qsc = gui::get_shortcut("Editor", "Go to next file");//"Ctrl+Tab"
        foreach (QString key, qsc.split('+')) {
            QString _key = key.toLower();
            if ((_key == "ctrl" && event->key() == Qt::Key_Control) ||
                    (_key == "alt" && event->key() == Qt::Key_Alt))
                this->item_selected();
        }
    }
    event->accept();
}

void TabSwitcherWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Down)
        this->select_row(1);
    else if (event->key() == Qt::Key_Up)
        this->select_row(-1);
}

void TabSwitcherWidget::focusOutEvent(QFocusEvent *event)
{
    // Reimplement Qt method to close the widget when loosing focus.
    event->ignore();
#if defined (Q_OS_MAC)
    if (event->reason() != Qt::ActiveWindowFocusReason)
        close();
#else
    this->close();
#endif
}


/********** EditorStack **********/
EditorStack::EditorStack(QWidget* parent, const QList<QAction*>& actions)
    : QWidget (parent), stack_history(StackHistory(this))
{
    setAttribute(Qt::WA_DeleteOnClose);

    this->threadmanager = new ThreadManager(this);

    this->newwindow_action = nullptr;
    this->horsplit_action = nullptr;
    this->versplit_action = nullptr;
    this->close_action = nullptr;
    this->__get_split_actions();

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);

    this->menu = nullptr;
    //self.fileswitcher_dlg = None
    this->tabs = nullptr;
    this->tabs_switcher = nullptr;


    this->setup_editorstack(parent, layout);

    this->find_widget = nullptr;

    this->data.clear();
    QAction* fileswitcher_action = new QAction("File switcher...",this);
    fileswitcher_action->setIcon(ima::icon("filelist"));
    connect(fileswitcher_action,SIGNAL(triggered(bool)),
            this,SLOT(open_fileswitcher_dlg()));

    QAction* symbolfinder_action = new QAction("Find symbols in file...",this);
    symbolfinder_action->setIcon(ima::icon("symbol_find"));
    connect(symbolfinder_action,SIGNAL(triggered(bool)),
            this,SLOT(open_symbolfinder_dlg()));

    QAction* copy_to_cb_action = new QAction("Copy path to clipboard", this);
    copy_to_cb_action->setIcon(ima::icon("editcopy"));
    connect(copy_to_cb_action,&QAction::triggered,
            [=](){QApplication::clipboard()->setText(get_current_filename());});

    QAction* close_right = new QAction("Close all to the right", this);
    connect(close_right,SIGNAL(triggered()),this,SLOT(close_all_right()));

    QAction* close_all_but_this = new QAction("Close all but this", this);
    connect(close_all_but_this,SIGNAL(triggered()),this,SLOT(close_all_but_this()));

    QString text;
#if defined (Q_OS_MAC)
    text = "Show in Finder";
#else
    text = "Show in external file explorer";
#endif
    QAction* external_fileexp_action = new QAction(text, this);
    connect(external_fileexp_action,&QAction::triggered,
            [=](){this->show_in_external_file_explorer();});

    menu_actions = actions;
    menu_actions << external_fileexp_action
                 << nullptr << fileswitcher_action
                 << symbolfinder_action
                 << copy_to_cb_action << nullptr
                 << close_right << close_all_but_this;

    this->outlineexplorer = nullptr;
    //this->help = nullptr;
    is_closable = false;
    new_action = nullptr;
    open_action = nullptr;
    save_action = nullptr;
    revert_action = nullptr;
    tempfile_path = QString();
    title = "Editor";
    pyflakes_enabled = true;//源码中为true
    pep8_enabled = false;
    todolist_enabled = true;
    realtime_analysis_enabled = false;
    is_analysis_done = false;
    linenumbers_enabled = true;
    blanks_enabled = false;
    edgeline_enabled = true;
    edgeline_column = 79;
    codecompletion_auto_enabled = true;
    codecompletion_case_enabled = false;
    codecompletion_enter_enabled = false;
    calltips_enabled = true;
    go_to_definition_enabled = true;
    close_parentheses_enabled = true;
    close_quotes_enabled = true;
    add_colons_enabled = true;
    auto_unindent_enabled = true;
    indent_chars = QString(4,' ');
    tab_stop_width_spaces = 4;
    help_enabled = false;
    //self.default_font = None
    wrap_enabled = false;
    tabmode_enabled = false;
    intelligent_backspace_enabled = true;
    highlight_current_line_enabled = false;
    highlight_current_cell_enabled = false;
    occurrence_highlighting_enabled = true;
    occurrence_highlighting_timeout=1500;
    checkeolchars_enabled = true;
    always_remove_trailing_spaces = false;
    focus_to_editor = true;
    create_new_file_if_empty = true;
    //QString ccs = "Spyder";
    //if (!sh::COLOR_SCHEME_NAMES.contains(ccs))
    //    ccs = sh::COLOR_SCHEME_NAMES[0];
    this->color_scheme = sh::get_color_scheme();
    //introspector = None
    __file_status_flag = false;

    analysis_timer = new QTimer(this);
    analysis_timer->setSingleShot(true);
    analysis_timer->setInterval(2000);
    connect(analysis_timer,&QTimer::timeout,[=](){this->analyze_script();});

    setAcceptDrops(true);

    shortcuts = create_shortcuts();

    last_closed_files = QStringList();

    this->msgbox = nullptr;

    edit_filetypes.clear();
    edit_filters.clear();

    save_dialog_on_tests = !running_under_pytest();
}

void EditorStack::show_in_external_file_explorer(QStringList fnames)
{
    if (fnames.isEmpty()) {
        QString filename = get_current_filename();
        fnames.append(filename);
    }
    ::show_in_external_file_explorer(fnames);
}

QList<ShortCutStrStr> EditorStack::create_shortcuts()
{
    QString keystr = "Ctrl+I";
    QShortcut* qsc = new QShortcut(QKeySequence(keystr),this,SLOT(inspect_current_object()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr inspect(qsc, "Editor", "Inspect current object");

    keystr = "F12";
    qsc = new QShortcut(QKeySequence(keystr),this,SLOT(set_or_clear_breakpoint()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr set_breakpoint(qsc, "Editor", "Breakpoint");

    keystr = "Shift+F12";
    qsc = new QShortcut(QKeySequence(keystr),this,SLOT(set_or_edit_conditional_breakpoint()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr set_cond_breakpoint(qsc, "Editor", "Conditional Breakpoint");

    keystr = "Ctrl+L";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->go_to_line(); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr gotoline(qsc,"Editor","Go to line");

    keystr = "Ctrl+Shift+Tab";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->tab_navigation_mru(false); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr tab(qsc,"Editor","Go to previous file");

    keystr = "Ctrl+Tab";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->tab_navigation_mru(); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr tabshift(qsc,"Editor","Go to next file");

    keystr = "Ctrl+PgUp";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->tabs->tab_navigate(-1); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr prevtab(qsc,"Editor","Cycle to previous file");

    keystr = "Ctrl+PgDown";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->tabs->tab_navigate(1); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr nexttab(qsc,"Editor","Cycle to next file");

    keystr = "F9";
    qsc = new QShortcut(QKeySequence(keystr),this,SLOT(run_selection()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr run_selection(qsc, "Editor", "Run selection");

    keystr = "Ctrl+N";              // 连接SIGNAL
    qsc = new QShortcut(QKeySequence(keystr),this,SIGNAL(sig_new_file()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr new_file(qsc, "Editor", "New file");

    keystr = "Ctrl+O";
    qsc = new QShortcut(QKeySequence(keystr),this,SIGNAL(plugin_load()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr open_file(qsc, "Editor", "Open file");

    keystr = "Ctrl+S";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->save(); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr save_file(qsc,"Editor","Save file");

    keystr = "Ctrl+Alt+S";
    qsc = new QShortcut(QKeySequence(keystr),this,SLOT(save_all()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr save_all(qsc, "Editor", "Save all");

    keystr = "Ctrl+Shift+S";
    qsc = new QShortcut(QKeySequence(keystr),this,SIGNAL(sig_save_as()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr save_as(qsc, "Editor", "Save As");

    keystr = "Ctrl+Shift+W";
    qsc = new QShortcut(QKeySequence(keystr),this,SLOT(close_all_files()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr close_all(qsc, "Editor", "Close all");

    keystr = "Ctrl+Alt+Shift+Left";
    qsc = new QShortcut(QKeySequence(keystr),this,SIGNAL(sig_prev_edit_pos()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr prev_edit_pos(qsc, "Editor", "Last edit location");

    keystr = "Ctrl+Alt+Left";
    qsc = new QShortcut(QKeySequence(keystr),this,SIGNAL(sig_prev_cursor()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr prev_cursor(qsc, "Editor", "Previous cursor position");

    keystr = "Ctrl+Alt+Right";
    qsc = new QShortcut(QKeySequence(keystr),this,SIGNAL(sig_next_cursor()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr next_cursor(qsc, "Editor", "Next cursor position");

    keystr = "Ctrl++";
    qsc = new QShortcut(QKeySequence(keystr),this,SIGNAL(zoom_in()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr zoom_in_1(qsc, "Editor", "zoom in 1");

    keystr = "Ctrl+=";
    qsc = new QShortcut(QKeySequence(keystr),this,SIGNAL(zoom_in()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr zoom_in_2(qsc, "Editor", "zoom in 2");

    keystr = "Ctrl+-";
    qsc = new QShortcut(QKeySequence(keystr),this,SIGNAL(zoom_out()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr zoom_out(qsc, "Editor", "zoom out");

    keystr = "Ctrl+0";
    qsc = new QShortcut(QKeySequence(keystr),this,SIGNAL(zoom_reset()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr zoom_reset(qsc, "Editor", "zoom reset");

    keystr = "Ctrl+W";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->close_file(); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr close_file_1(qsc,"Editor","close file 1");

    keystr = "Ctrl+F4";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->close_file(); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr close_file_2(qsc,"Editor","close file 2");

    keystr = "Ctrl+Return";
    qsc = new QShortcut(QKeySequence(keystr),this,SLOT(run_cell()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr run_cell(qsc, "Editor", "run cell");

    keystr = "Shift+Return";
    qsc = new QShortcut(QKeySequence(keystr),this,SLOT(run_cell_and_advance()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr run_cell_and_advance(qsc, "Editor", "run cell and advance");

    keystr = "Ctrl+Down";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->advance_cell(); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr go_to_next_cell(qsc,"Editor","go to next cell");

    keystr = "Ctrl+Up";
    qsc = new QShortcut(QKeySequence(keystr), this);
    connect(qsc,&QShortcut::activated, [=](){ this->advance_cell(true); });
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr go_to_previous_cell(qsc,"Editor","go to previous cell");

    keystr = "Alt+Return";
    qsc = new QShortcut(QKeySequence(keystr),this,SLOT(re_run_last_cell()));
    qsc->setContext(Qt::WidgetWithChildrenShortcut);
    ShortCutStrStr re_run_last_cell(qsc, "Editor", "re-run last cell");

    QList<ShortCutStrStr> res;
    res << inspect<< set_breakpoint<< set_cond_breakpoint<< gotoline<< tab
        << tabshift<< run_selection<< new_file<< open_file<< save_file
        << save_all<< save_as<< close_all<< prev_edit_pos<< prev_cursor
        << next_cursor<< zoom_in_1<< zoom_in_2<< zoom_out<< zoom_reset
        << close_file_1<< close_file_2<< run_cell<< run_cell_and_advance
        << go_to_next_cell<< go_to_previous_cell<< re_run_last_cell
        << prevtab<< nexttab;
    return res;
}

QList<ShortCutStrStr> EditorStack::get_shortcut_data()
{
    return shortcuts;
}

void EditorStack::setup_editorstack(QWidget *parent, QVBoxLayout *layout)
{
    Q_UNUSED(parent);
    QToolButton* menu_btn = new QToolButton(this);
    menu_btn->setIcon(ima::icon("tooloptions"));
    menu_btn->setToolTip("Options");

    menu = new QMenu(this);
    menu_btn->setMenu(menu);
    menu_btn->setPopupMode(QToolButton::InstantPopup);
    connect(this->menu,SIGNAL(aboutToShow()),this,SLOT(__setup_menu()));

    QHash<Qt::Corner, QList<QPair<int, QToolButton*>>> corner_widgets;
    QList<QPair<int, QToolButton*>> btn_list;
    btn_list.append(qMakePair(-1,menu_btn));
    corner_widgets[Qt::TopRightCorner] = btn_list;
    tabs = new BaseTabs(this, QList<QAction*>(), this->menu,
                        corner_widgets, true);
    tabs->tabBar()->setObjectName("plugin-tab");
    tabs->set_close_function([this](int index){this->close_file(index);});
    connect(tabs->tabBar(),SIGNAL(tabMoved(int,int)),
            this,SLOT(move_editorstack_data(int,int)));
    tabs->setMovable(true);

    stack_history.refresh();

#if !defined (Q_OS_MAC)
    tabs->setDocumentMode(true);
#endif
    connect(this->tabs,SIGNAL(currentChanged(int)),this,SLOT(current_changed(int)));

    layout->addWidget(tabs);
}

void EditorStack::add_corner_widgets_to_tabbar(const QList<QPair<int, QToolButton *> > &widgets)
{
    tabs->add_corner_widgets(widgets);
}

void EditorStack::closeEvent(QCloseEvent *event)
{
    threadmanager->close_all_threads();
    disconnect(analysis_timer,SIGNAL(timeout()), nullptr, nullptr);

    // Remove editor references from the outline explorer settings
    if (outlineexplorer) {
        foreach (FileInfo* finfo, this->data) {
            outlineexplorer->remove_editor(finfo->editor);
        }
    }
    QWidget::closeEvent(event);
}

CodeEditor* EditorStack::clone_editor_from(FileInfo *other_finfo, bool set_current)
{
    QString fname = other_finfo->filename;
    QString enc = other_finfo->encoding;
    bool _new = other_finfo->newly_created;
    FileInfo* finfo = this->create_new_editor(fname, enc, "", set_current,
                                              _new, other_finfo->editor);
    finfo->set_analysis_results(other_finfo->analysis_results);
    finfo->set_todo_results(other_finfo->todo_results);
    return finfo->editor;
}

void EditorStack::clone_from(EditorStack *other)
{
    foreach (FileInfo* other_finfo, other->data) {
        clone_editor_from(other_finfo, true);
    }
    set_stack_index(other->get_stack_index());
}

void EditorStack::open_fileswitcher_dlg()
{
    qDebug() << __func__;
}

void EditorStack::open_symbolfinder_dlg()
{
    qDebug() << __func__;
}

EditorStack* EditorStack::get_current_tab_manager()
{
    return this;
}

void EditorStack::go_to_line(int line)
{
    if (line >= 0)
        get_current_editor()->go_to_line(line);
    else {
        if (!this->data.isEmpty())
            get_current_editor()->exec_gotolinedialog();
    }
}

void EditorStack::set_or_clear_breakpoint()
{
    if (!this->data.isEmpty()) {
        CodeEditor* editor = get_current_editor();
        if (editor)
            editor->add_remove_breakpoint();
    }
}

void EditorStack::set_or_edit_conditional_breakpoint()
{
    if (!this->data.isEmpty()) {
        CodeEditor* editor = get_current_editor();
        if (editor)
            editor->add_remove_breakpoint(-1, QString(), true);
    }
}

void EditorStack::inspect_current_object()
{}

//------ Editor Widget Settings
void EditorStack::set_closable(bool state)
{
    is_closable = state;
}

void EditorStack::set_io_actions(QAction *new_action, QAction *open_action,
                                 QAction *save_action, QAction *revert_action)
{
    this->new_action = new_action;
    this->open_action = open_action;
    this->save_action = save_action;
    this->revert_action = revert_action;
}

void EditorStack::set_find_widget(FindReplace* find_widget)
{
    this->find_widget = find_widget;
}

void EditorStack::set_outlineexplorer(OutlineExplorerWidget *outlineexplorer)
{
    this->outlineexplorer = outlineexplorer;
    connect(this->outlineexplorer, &OutlineExplorerWidget::outlineexplorer_is_visible,
            [=](){this->_refresh_outlineexplorer();});
}

void EditorStack::initialize_outlineexplorer()
{
    for (int index = 0; index < get_stack_count(); ++index) {
        if (index != get_stack_index())
            this->_refresh_outlineexplorer(index);
    }
}


//add_outlineexplorer_button(self, editor_plugin)
//set_help


void EditorStack::set_tempfile_path(const QString &path)
{
    tempfile_path = path;
}

void EditorStack::set_title(const QString &text)
{
    title = text;
}

void EditorStack::__update_editor_margins(CodeEditor *editor)
{
    editor->setup_margins(linenumbers_enabled, this->has_markers());
}

void EditorStack::__codeanalysis_settings_changed(FileInfo* current_finfo)
{
    if (!this->data.isEmpty()) {
        bool run_pyflakes = this->pyflakes_enabled;
        bool run_pep8 = this->pep8_enabled;
        foreach (FileInfo* finfo, this->data) {
            this->__update_editor_margins(finfo->editor);
            finfo->cleanup_analysis_results();
            if ((run_pyflakes || run_pep8) && current_finfo) {
                if (current_finfo != finfo)
                    finfo->run_code_analysis(run_pyflakes, run_pep8);
            }
        }
    }
}

void EditorStack::set_pyflakes_enabled(bool state, FileInfo* current_finfo)
{
    this->pyflakes_enabled = state;
    this->__codeanalysis_settings_changed(current_finfo);
}

void EditorStack::set_pep8_enabled(bool state, FileInfo* current_finfo)
{
    this->pep8_enabled = state;
    this->__codeanalysis_settings_changed(current_finfo);
}

bool EditorStack::has_markers() const
{
    return todolist_enabled || pyflakes_enabled || pep8_enabled;
}

void EditorStack::set_todolist_enabled(bool state, FileInfo *current_finfo)
{
    todolist_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data) {
            __update_editor_margins(finfo->editor);
            finfo->cleanup_todo_results();
            if (state && current_finfo) {
                if (current_finfo != finfo)
                    finfo->run_todo_finder();
            }
        }
    }
}

void EditorStack::set_realtime_analysis_enabled(bool state)
{
    realtime_analysis_enabled = state;
}

void EditorStack::set_realtime_analysis_timeout(int timeout)
{
    analysis_timer->setInterval(timeout);
}

void EditorStack::set_linenumbers_enabled(bool state, FileInfo *current_finfo)
{
    Q_UNUSED(current_finfo);
    linenumbers_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            __update_editor_margins(finfo->editor);
    }
}

void EditorStack::set_blanks_enabled(bool state)
{
    blanks_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_blanks_enabled(state);
    }
}

void EditorStack::set_edgeline_enabled(bool state)
{
    edgeline_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_edge_line_enabled(state);
    }
}

void EditorStack::set_edgeline_column(int column)
{
    edgeline_column = column;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_edge_line_column(column);
    }
}

void EditorStack::set_codecompletion_auto_enabled(bool state)
{
    codecompletion_auto_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_codecompletion_auto(state);
    }
}

void EditorStack::set_codecompletion_case_enabled(bool state)
{
    codecompletion_case_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_codecompletion_case(state);
    }
}

void EditorStack::set_codecompletion_enter_enabled(bool state)
{
    codecompletion_enter_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_codecompletion_enter(state);
    }
}

void EditorStack::set_calltips_enabled(bool state)
{
    calltips_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_calltips(state);
    }
}

void EditorStack::set_go_to_definition_enabled(bool state)
{
    go_to_definition_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_go_to_definition_enabled(state);
    }
}

void EditorStack::set_close_parentheses_enabled(bool state)
{
    close_parentheses_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_close_parentheses_enabled(state);
    }
}

void EditorStack::set_close_quotes_enabled(bool state)
{
    close_quotes_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_close_quotes_enabled(state);
    }
}

void EditorStack::set_add_colons_enabled(bool state)
{
    add_colons_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_add_colons_enabled(state);
    }
}

void EditorStack::set_auto_unindent_enabled(bool state)
{
    auto_unindent_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_auto_unindent_enabled(state);
    }
}

void EditorStack::set_indent_chars(QString indent_chars)
{
    indent_chars = indent_chars.mid(1, indent_chars.size()-2);
    this->indent_chars = indent_chars;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_indent_chars(indent_chars);
    }
}

void EditorStack::set_tab_stop_width_spaces(int tab_stop_width_spaces)
{
    this->tab_stop_width_spaces = tab_stop_width_spaces;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data) {
            finfo->editor->tab_stop_width_spaces = tab_stop_width_spaces;
            finfo->editor->update_tab_stop_width_spaces();
        }
    }
}

void EditorStack::set_help_enabled(bool state)
{
    help_enabled = state;
}

void EditorStack::set_default_font(const QFont &font, const QHash<QString,ColorBoolBool> &color_scheme)
{
    default_font = font;
    if (!color_scheme.isEmpty())
        this->color_scheme = color_scheme;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_font(font, color_scheme);
    }
}

void EditorStack::set_color_scheme(const QHash<QString, ColorBoolBool> &color_scheme)
{
    this->color_scheme = color_scheme;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_color_scheme(color_scheme);
    }
}

void EditorStack::set_wrap_enabled(bool state)
{
    this->wrap_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->toggle_wrap_mode(state);
    }
}

void EditorStack::set_tabmode_enabled(bool state)
{
    this->tabmode_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_tab_mode(state);
    }
}

void EditorStack::set_intelligent_backspace_enabled(bool state)
{
    this->intelligent_backspace_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->toggle_intelligent_backspace(state);
    }
}

void EditorStack::set_occurrence_highlighting_enabled(bool state)
{
    this->occurrence_highlighting_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_occurrence_highlighting(state);
    }
}

void EditorStack::set_occurrence_highlighting_timeout(int timeout)
{
    occurrence_highlighting_timeout = timeout;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_occurrence_timeout(timeout);
    }
}

void EditorStack::set_highlight_current_line_enabled(bool state)
{
    highlight_current_line_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_highlight_current_line(state);
    }
}

void EditorStack::set_highlight_current_cell_enabled(bool state)
{
    highlight_current_cell_enabled = state;
    if (!this->data.isEmpty()) {
        foreach (FileInfo* finfo, this->data)
            finfo->editor->set_highlight_current_cell(state);
    }
}

void EditorStack::set_checkeolchars_enabled(bool state)
{
    checkeolchars_enabled = state;
}

void EditorStack::set_always_remove_trailing_spaces(bool state)
{
    always_remove_trailing_spaces = state;
}

void EditorStack::set_focus_to_editor(bool state)
{
    focus_to_editor = state;
}

//set_introspector

//------ Stacked widget management
int EditorStack::get_stack_index() const
{
    return tabs->currentIndex();
}

FileInfo* EditorStack::get_current_finfo() const
{
    if (!data.isEmpty()) {
        Q_ASSERT(0 <= this->get_stack_index() &&
                 this->get_stack_index() < this->data.size());
        return data[get_stack_index()];
    }
    return nullptr;
}

CodeEditor* EditorStack::get_current_editor() const
{
    return qobject_cast<CodeEditor*>(tabs->currentWidget());
}

int EditorStack::get_stack_count() const
{
    return tabs->count();
}

void EditorStack::set_stack_index(int index, EditorStack *instance)
{
    if (instance == this || instance == nullptr)
        tabs->setCurrentIndex(index);
}

void EditorStack::set_tabbar_visible(bool state)
{
    tabs->tabBar()->setVisible(state);
}

void EditorStack::remove_from_data(int index)
{
    tabs->blockSignals(true);
    tabs->removeTab(index);
    this->data.removeAt(index);
    tabs->blockSignals(false);
    update_actions();
}

QString EditorStack::__modified_readonly_title(QString title, bool is_modified, bool is_readonly)
{
    if (is_modified)
        title += "*";
    if (is_readonly)
        title = '(' + title + ')';
    return title;
}

QString EditorStack::get_tab_text(int index, bool is_modified, bool is_readonly)
{
    Q_ASSERT(0 <= index && index < this->data.size());
    QString fname = this->data[index]->filename;

    QStringList files_path_list;    
    for (int i = 0; i < this->data.size(); ++i) {
        if (i != index) {
            QString filename = this->data[i]->filename;
            QFileInfo info(filename);
            files_path_list.append(info.fileName());
        }
    }
    QFileInfo info(fname);
    if (!files_path_list.contains(info.fileName()))
        fname = info.fileName();
    // 获取无歧义标题
    return __modified_readonly_title(fname, is_modified, is_readonly);
}

QString EditorStack::get_tab_tip(const QString &filename, bool is_modified, bool is_readonly)
{
    QString text = "%1 - %2";
    text = __modified_readonly_title(text, is_modified, is_readonly);

    if (!tempfile_path.isEmpty() && filename == tempfile_path) {
        QString temp_file_str = "Temporary file";
        return text.arg(temp_file_str, tempfile_path);
    }
    else {
        QFileInfo info(filename);
        return text.arg(info.fileName()).arg(info.absolutePath());
    }
}

void EditorStack::add_to_data(FileInfo *finfo, bool set_current)
{
    this->data.append(finfo);
    int index = this->data.indexOf(finfo);
    CodeEditor* editor = finfo->editor;
    this->tabs->insertTab(index, editor, get_tab_text(index));
    // 在这里向tabs中插入CodeEditor
    this->set_stack_title(index, false);
    if (set_current) {
        this->set_stack_index(index);
        this->current_changed(index);
    }
    this->update_actions();
}

void EditorStack::__repopulate_stack()
{
    tabs->blockSignals(true);
    tabs->clear();
    foreach (FileInfo* finfo, this->data) {
        bool is_modified;
        if (finfo->newly_created)
            is_modified = true;
        else
            is_modified = false;
        int index = data.indexOf(finfo);
        QString tab_text = get_tab_text(index, is_modified);
        QString tab_tip = get_tab_tip(finfo->filename);
        index = tabs->addTab(finfo->editor, tab_text);
        tabs->setTabToolTip(index, tab_tip);
    }
    tabs->blockSignals(false);
}

int EditorStack::rename_in_data(const QString &original_filename, const QString &new_filename)
{
    int index = has_filename(original_filename);
    if (index == -1)
        return -1;
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = this->data[index];
    QFileInfo info(finfo->filename);
    QFileInfo new_info(new_filename);
    if (info.suffix() != new_info.suffix()) {
        QString txt = finfo->editor->get_text_with_eol();
        QString language = get_file_language(new_filename, txt);
        finfo->editor->set_language(language);
    }
    bool set_new_index = index == get_stack_index();
    QString current_fname = get_current_filename();
    finfo->filename = new_filename;
    int new_index = data.indexOf(finfo);
    this->__repopulate_stack();
    if (set_new_index)
        set_stack_index(new_index);
    else
        set_current_filename(current_fname);
    if (outlineexplorer)
        outlineexplorer->file_renamed(finfo->editor, finfo->filename);
    return new_index;
}

void EditorStack::set_stack_title(int index, bool is_modified)
{
    FileInfo* finfo = this->data[index];
    QString fname = finfo->filename;
    //qDebug() << __func__ << is_modified << finfo->newly_created << finfo->_default;
    is_modified = (is_modified || finfo->newly_created) && ! finfo->_default;
    bool is_readonly = finfo->editor->isReadOnly();
    QString tab_text = get_tab_text(index, is_modified, is_readonly);
    QString tab_tip = get_tab_tip(fname, is_modified, is_readonly);

    if (tab_text != tabs->tabText(index))
        tabs->setTabText(index, tab_text);
    tabs->setTabToolTip(index, tab_tip);
}

//@Slot()
void EditorStack::__setup_menu()
{
    this->menu->clear();
    QList<QAction*> actions;
    if (!data.isEmpty())
        actions = menu_actions;
    else {
        actions << new_action << open_action;
        setFocus();
    }
    actions.append(__get_split_actions());
    add_actions(this->menu, actions);
    close_action->setEnabled(is_closable);
}

QList<QAction*> EditorStack::__get_split_actions()
{
    newwindow_action = new QAction("New window", this);
    newwindow_action->setIcon(ima::icon("newwindow"));
    newwindow_action->setToolTip("Create a new editor window");
    connect(newwindow_action,SIGNAL(triggered(bool)),this,SIGNAL(create_new_window()));

    versplit_action = new QAction("Split vertically", this);
    versplit_action->setIcon(ima::icon("versplit"));
    versplit_action->setToolTip("Split vertically this editor window");
    connect(versplit_action,SIGNAL(triggered(bool)),this,SIGNAL(split_vertically()));

    horsplit_action = new QAction("Split horizontally", this);
    horsplit_action->setIcon(ima::icon("horsplit"));
    horsplit_action->setToolTip("Split horizontally this editor window");
    connect(horsplit_action,SIGNAL(triggered(bool)),this,SIGNAL(split_horizontally()));

    close_action = new QAction("Close this panel", this);
    close_action->setIcon(ima::icon("close_panel"));
    connect(close_action,SIGNAL(triggered(bool)),this,SLOT(close()));

    QList<QAction*> actions;
    actions << nullptr << newwindow_action << nullptr
            << versplit_action << horsplit_action << close_action;
    return actions;
}

void EditorStack::reset_orientation()
{
    horsplit_action->setEnabled(true);
    versplit_action->setEnabled(true);
}

void EditorStack::set_orientation(Qt::Orientation orientation)
{
    horsplit_action->setEnabled(orientation == Qt::Horizontal);
    versplit_action->setEnabled(orientation == Qt::Vertical);
}

void EditorStack::update_actions()
{
    bool state = get_stack_count() > 0;
    horsplit_action->setEnabled(state);
    versplit_action->setEnabled(state);
}

// ------ Accessors
QString EditorStack::get_current_filename() const
{
    if (!this->data.isEmpty()) {
        Q_ASSERT(0 <= get_stack_index() && get_stack_index() < this->data.size());
        return data[get_stack_index()]->filename;
    }
    return QString();
}

int EditorStack::has_filename(const QString &filename) const
{
    //osp.realpath <==> QFileInfo::canonicalFilePath
    for (int index = 0; index < this->data.size(); ++index) {
        FileInfo* finfo = this->data[index];
        QFileInfo info1(filename);
        QFileInfo info2(finfo->filename);
        if (info1.canonicalFilePath() == info2.canonicalFilePath())
            return index;
    }
    return -1;
}

CodeEditor* EditorStack::set_current_filename(const QString &filename)
{
    int index = this->has_filename(filename);
    if (index != -1) {
        this->set_stack_index(index);
        Q_ASSERT(0 <= index && index < this->data.size());
        CodeEditor* editor = data[index]->editor;
        editor->setFocus();
        return editor;
    }
    return nullptr;
}

bool EditorStack::is_file_opened() const
{
    return data.size() > 0;
}

int EditorStack::is_file_opened(const QString &filename) const
{
    return this->has_filename(filename);
}

int EditorStack::get_index_from_filename(const QString &filename) const
{
    QStringList filenames;
    foreach (FileInfo* d, this->data) {
        filenames.append(d->filename);
    }
    return filenames.indexOf(filename);
}

//@Slot(int, int)
void EditorStack::move_editorstack_data(int start,int end)
{
    int direction;
    if (start < 0 || end < 0)
        return;
    else {
        int steps = qAbs(end - start);
        direction = (end-start) / steps;
    }

    this->blockSignals(true);
    for (int i = start; i != end; i+=direction)
        qSwap(this->data[i], this->data[i+direction]);
    this->blockSignals(false);
    this->refresh();
}


//------ Close file, tabwidget...
bool EditorStack::close_file(int index, bool force)
{
    int current_index = this->get_stack_index();
    int count = this->get_stack_count();

    if (index == -1) {
        if (count > 0)
            index = current_index;
        else {
            this->find_widget->set_editor(nullptr);
            return false;//源码是return
        }
    }

    int new_index = -1;
    if (count > 1) {
        if (current_index == index)
            new_index  = _get_previous_file_index();
        else
            new_index = current_index;
    }

    bool is_ok = force || save_if_changed(true, index);
    if (is_ok) {
        Q_ASSERT(0 <= index && index < this->data.size());
        FileInfo* finfo = data[index];
        threadmanager->close_threads(finfo);

        if (outlineexplorer)
            outlineexplorer->remove_editor(finfo->editor);

        QString filename = data[index]->filename;
        remove_from_data(index);

        size_t id = reinterpret_cast<size_t>(this);
        emit sig_close_file(QString::number(id), filename);

        emit opened_files_list_changed();
        emit update_code_analysis_actions();
        this->_refresh_outlineexplorer();
        emit refresh_file_dependent_actions();
        emit update_plugin_title();

        CodeEditor* editor = get_current_editor();
        if (editor)
            editor->setFocus();

        if (new_index != -1) {
            if (index < new_index)
                new_index--;
            this->set_stack_index(new_index);
        }
        this->add_last_closed_file(finfo->filename);
    }

    if (this->get_stack_count() == 0 && create_new_file_if_empty) {
        emit sig_new_file();
        return false;
    }
    this->__modify_stack_title();
    return is_ok;
}

void EditorStack::close_all_files()
{
    while (this->close_file())
        ;
}

void EditorStack::close_all_right()
{
    int num = this->get_stack_index();
    int n = this->get_stack_count();
    for (int i=num;i<n-1;i++)
        this->close_file(num+1);
}

void EditorStack::close_all_but_this()
{
    this->close_all_right();
    // 下面的循环不能照着源码写成for循环，因为python中range独特的机制
    //当第一次get_stack_count()=4时,for i in range(0, 3):
    //注定要迭代3次，不会进行后续的比较
    while (this->get_stack_count()-1 > 0) {
        this->close_file(0);
    }
}

void EditorStack::add_last_closed_file(const QString &fname)
{
    if (last_closed_files.contains(fname))
        last_closed_files.removeOne(fname);
    last_closed_files.insert(0, fname);
    if (last_closed_files.size() > 10)
        last_closed_files.removeLast();
}

QStringList EditorStack::get_last_closed_files() const
{
    return last_closed_files;
}

void EditorStack::set_last_closed_files(const QStringList &fnames)
{
    last_closed_files = fnames;
}


//------ Save
bool EditorStack::save_if_changed(bool cancelable, int index)
{
    QList<int> indexes;
    if (index == -1) {
        for (int i=0;i<get_stack_count();i++)
            indexes.append(i);
    }
    else
        indexes.append(index);
    QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No;
    if (cancelable)
        buttons |= QMessageBox::Cancel;
    int unsaved_nb = 0;
    foreach (index, indexes) {
        Q_ASSERT(0 <= index && index < this->data.size());
        if (data[index]->editor->document()->isModified())
            unsaved_nb++;
    }
    if (!unsaved_nb)
        return true;
    if (unsaved_nb > 1)
        buttons |= QMessageBox::YesAll | QMessageBox::NoAll;
    bool yes_all = false;
    foreach (index, indexes) {
        set_stack_index(index);
        FileInfo* finfo = data[index];
        if (finfo->filename == tempfile_path || yes_all) {
            if (!this->save(index))
                return false;
        }
        else if (finfo->editor->document()->isModified() &&
                 save_dialog_on_tests) {
            QFileInfo info(finfo->filename);
            msgbox = new QMessageBox(QMessageBox::Question,
                                     title,
                                     QString("<b>%1</b> has been modified."
                                             "<br>Do you want to save changes?")
                                     .arg(info.fileName()),
                                     buttons,
                                     this);
            int answer = msgbox->exec();
            if (answer == QMessageBox::Yes) {
                if (!this->save(index))
                    return false;
            }
            else if (answer == QMessageBox::YesAll) {
                if (!this->save(index))
                    return false;
                yes_all = true;
            }
            else if (answer == QMessageBox::NoAll)
                return true;
            else if (answer == QMessageBox::Cancel)
                return false;
        }
    }
    return true;
}

bool EditorStack::save(int index, bool force)
{
    if (index == -1) {
        if (!get_stack_count())
            return false;//源码是return
        index = get_stack_index();
    }

    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    if (!(finfo->editor->document()->isModified() ||
          finfo->newly_created) && !force)
        return true;
    QFileInfo info(finfo->filename);
    if (!info.isFile() && !force)
        return this->save_as(index);
    if (always_remove_trailing_spaces)
        this->remove_trailing_spaces(index);
    QString txt = finfo->editor->get_text_with_eol();
    //源码是encoding.write(txt, finfo.filename,finfo.encoding)
    //encoding是类似"utf-8"的QString,本代码没有用到编码
    bool ok = encoding::write(txt, finfo->filename);
    if (ok) {
        finfo->newly_created = false;
        emit encoding_changed(finfo->encoding);
        finfo->lastmodified = QFileInfo(finfo->filename).lastModified();

        size_t id = reinterpret_cast<size_t>(this);
        emit file_saved(QString::number(id), finfo->filename, finfo->filename);
        finfo->editor->document()->setModified(false);
        this->modification_changed(-1, index);
        this->analyze_script(index);
        //introspector.validate()

        finfo->editor->rehighlight();

        this->_refresh_outlineexplorer(index);
        return true;
    }
    else {
        msgbox = new QMessageBox(QMessageBox::Critical,
                                 "Save Error",
                                 QString("<b>Unable to save file '%1'</b>").arg(info.fileName()),
                                 QMessageBox::NoButton,
                                 this);
        msgbox->exec();
        return false;
    }
}

void EditorStack::file_saved_in_other_editorstack(const QString &original_filename, const QString &filename)
{
    int index = has_filename(original_filename);
    if (index == -1)
        return;
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    finfo->newly_created = false;
    finfo->filename = filename;
    finfo->lastmodified = QFileInfo(finfo->filename).lastModified();
}

QString EditorStack::select_savename(const QString &original_filename)
{
    if (edit_filetypes.isEmpty())
        edit_filetypes = get_edit_filetypes();
    if (edit_filters.isEmpty())
        edit_filters = get_edit_filters();

    QString filters = this->edit_filters;
    QFileInfo info(original_filename);
    QString ext = '.' + info.suffix();
    QString selectedfilter = get_filter(edit_filetypes, ext);

    emit redirect_stdio(false);
    QString filename = QFileDialog::getSaveFileName(this, "Save file",
                                                    original_filename,
                                                    filters,
                                                    &selectedfilter,
                                                    QFileDialog::HideNameFilterDetails);
    emit redirect_stdio(true);
    if (!filename.isEmpty()) {
        QFileInfo fileinfo(filename);
        return fileinfo.absoluteFilePath();
    }
    return QString();
}

bool EditorStack::save_as(int index)
{
    if (index == -1)
        index = this->get_stack_index();
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    finfo->newly_created = true;
    QString original_filename = finfo->filename;
    QString filename = this->select_savename(original_filename);
    if (!filename.isEmpty()) {
        int ao_index = this->has_filename(filename);
        // Note: ao_index == index --> saving an untitled file
        if (ao_index != -1 && ao_index != index) {
            if (!close_file(ao_index))
                return false;//源码是return
            if (ao_index < index)
                index--;
        }

        int new_index = this->rename_in_data(original_filename, filename);

        size_t id = reinterpret_cast<size_t>(this);
        emit file_renamed_in_data(QString::number(id), original_filename, filename);

        bool ok = this->save(new_index, true);
        this->refresh(new_index);
        this->set_stack_index(new_index);
        return ok;
    }
    else
        return false;
}

bool EditorStack::save_copy_as(int index)
{
    if (index == -1)
        index = this->get_stack_index();
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    QString original_filename = finfo->filename;
    QString filename = this->select_savename(original_filename);
    if (!filename.isEmpty()) {
        int ao_index = has_filename(filename);
        if (ao_index != -1 && ao_index != index) {
            if (!close_file(ao_index))
                return false;//源码是return
            if (ao_index < index)
                index--;
        }
        QString txt = finfo->editor->get_text_with_eol();
        bool ok = encoding::write(txt, filename);
        if (ok) {
            emit plugin_load(filename);
            return true;
        }
        else {
            QFileInfo info(finfo->filename);
            msgbox = new QMessageBox(QMessageBox::Critical,
                                     "Save Error",
                                     QString("<b>Unable to save file '%1'</b>")
                                     .arg(info.fileName()));
            msgbox->exec();
        }
    }
    return false;
}

void EditorStack::save_all()
{
    int count = this->get_stack_count();
    for (int index = 0; index < count; ++index) {
        Q_ASSERT(0 <= index && index < this->data.size());
        if (data[index]->editor->document()->isModified())
            this->save(index);
    }
}


//------ Update UI
void EditorStack::start_stop_analysis_timer()
{

    is_analysis_done = false;
    if (realtime_analysis_enabled) {
        analysis_timer->stop();
        analysis_timer->start();
    }
}

void EditorStack::analyze_script(int index)
{
    if (is_analysis_done)
        return;
    if (index == -1)
        index = get_stack_index();
    if (!this->data.isEmpty()) {
        Q_ASSERT(0 <= index && index < this->data.size());
        FileInfo* finfo = this->data[index];
        bool run_pyflakes = this->pyflakes_enabled;
        bool run_pep8 = this->pep8_enabled;
        if (run_pyflakes || run_pep8)
            finfo->run_code_analysis(run_pyflakes, run_pep8);
        if (this->todolist_enabled)
            finfo->run_todo_finder();
    }
    is_analysis_done = true;
}

void EditorStack::set_analysis_results(const QString& filename,
                                       const QList<QList<QVariant>>& analysis_results)
{
    int index = this->has_filename(filename);
    if (index == -1)
        return;
    Q_ASSERT(0 <= index && index < this->data.size());
    this->data[index]->set_analysis_results(analysis_results);
}

QList<QList<QVariant>> EditorStack::get_analysis_results()
{
    if (!this->data.isEmpty()) {
        Q_ASSERT(0 <= this->get_stack_index() && this->get_stack_index() < this->data.size());
        return this->data[this->get_stack_index()]->analysis_results;
    }
    return QList<QList<QVariant>>();
}

void EditorStack::set_todo_results(const QString& filename,
                                       const QList<QList<QVariant>>& analysis_results)
{
    int index = this->has_filename(filename);
    if (index == -1)
        return;
    Q_ASSERT(0 <= index && index < this->data.size());
    this->data[index]->set_todo_results(analysis_results);
}

QList<QList<QVariant>> EditorStack::get_todo_results()
{
    if (!this->data.isEmpty()) {
        Q_ASSERT(0 <= this->get_stack_index() && this->get_stack_index() < this->data.size());
        return this->data[this->get_stack_index()]->todo_results;
    }
    return QList<QList<QVariant>>();
}

void EditorStack::current_changed(int index)
{
    CodeEditor* editor = get_current_editor();
    if (!editor) {
        qDebug() << __FILE__ << __FUNCTION__;
        return;
    }

    if (index != -1) {
        editor->setFocus();
        if (DEBUG_EDITOR)
            qDebug() << "setfocusto:" << editor;
    }
    else
        emit reset_statusbar();
    emit opened_files_list_changed();

    stack_history.refresh();

    while (stack_history.contains(index))
        stack_history.remove(index);
    stack_history.append(index);
    if (DEBUG_EDITOR) {
        qDebug()<<"current_changed:"<<index<<data[index]->editor
               << data[index]->editor->get_document_id();
    }
    emit update_plugin_title();
    if (editor) {
        try {
            emit current_file_changed(data[index]->filename,
                                      editor->get_position("cursor"));
        } catch (...) {
        }
    }
}

int EditorStack::_get_previous_file_index()
{
    try {
        return stack_history.getitem(stack_history.len()-2);
    } catch (...) {
        return -1;
    }
}

void EditorStack::tab_navigation_mru(bool forward)
{
    tabs_switcher = new TabSwitcherWidget(this,stack_history,tabs);
    tabs_switcher->show();
    tabs_switcher->select_row(forward ? 1 : -1);
    tabs_switcher->setFocus();
}

void EditorStack::focus_changed()
{
    QWidget* fwidget = QApplication::focusWidget();
    foreach (FileInfo* finfo, data) {
        if (fwidget == finfo->editor)
            refresh();
    }
    emit editor_focus_changed();
}

void EditorStack::_refresh_outlineexplorer(int index, bool update, bool clear)
{
    OutlineExplorerWidget* oe = this->outlineexplorer;
    if (oe == nullptr)
        return;
    if (index == -1)
        index = get_stack_index();
    bool enable = false;
    if (!data.isEmpty()) {
        Q_ASSERT(0 <= index && index < this->data.size());
        FileInfo* finfo = data[index];
        if (finfo->editor->is_python()) {
            enable = true;
            oe->setEnabled(true);
            oe->set_current_editor(finfo->editor, finfo->filename, update, clear);
        }
    }
    if (!enable)
        oe->setEnabled(false);
}

void EditorStack::__refresh_statusbar(int index)
{
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    emit encoding_changed(finfo->encoding);
    QPair<int,int> pair = finfo->editor->get_cursor_line_column();
    int line = pair.first;
    index = pair.second;
    emit sig_editor_cursor_position_changed(line, index);
}

void EditorStack::__refresh_readonly(int index)
{
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    QFileInfo info(finfo->filename);
    bool read_only = !info.isWritable();
    if (!info.isFile())
        read_only = false;
    finfo->editor->setReadOnly(read_only);
    emit readonly_changed(read_only);
}

void EditorStack::__check_file_status(int index)
{
    if (__file_status_flag)
        return;
    __file_status_flag = true;

    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    QFileInfo info(finfo->filename);
    QString name = info.fileName();

    if (finfo->newly_created)
        ;
    else if (!info.isFile()) {
        msgbox = new QMessageBox(QMessageBox::Warning,
                                 title,
                                 QString("<b>%1</b> is unavailable "
                                         "(this file may have been removed, moved "
                                         "or renamed outside Spyder)."
                                         "<br>Do you want to close it?")
                                 .arg(name),
                                 QMessageBox::Yes | QMessageBox::No,
                                 this);
        int answer = msgbox->exec();
        if (answer == QMessageBox::Yes)
            close_file(index);
        else {
            finfo->newly_created = true;
            finfo->editor->document()->setModified(true);
            modification_changed(-1, index);
        }
    }
    else {
        QDateTime lastm = QFileInfo(finfo->filename).lastModified();
        if (lastm.toString() != finfo->lastmodified.toString()) {
            if (finfo->editor->document()->isModified()) {
                msgbox = new QMessageBox(QMessageBox::Question,
                                         title,
                                         QString("<b>%1</b> has been modified outside Spyder."
                                                 "<br>Do you want to reload it and lose all "
                                                 "your changes?").arg(name),
                                         QMessageBox::Yes | QMessageBox::No,
                                         this);
                int answer = msgbox->exec();
                if (answer == QMessageBox::Yes)
                    this->reload(index);
                else
                    finfo->lastmodified = lastm;
            }
            else
                this->reload(index);
        }
    }
    __file_status_flag = false;
}

void EditorStack::__modify_stack_title()
{
    for (int index = 0; index < data.size(); ++index) {
        FileInfo* finfo = data[index];
        bool state = finfo->editor->document()->isModified();
        this->set_stack_title(index, state);
    }
}

void EditorStack::refresh(int index)
{
    if (index == -1)
        index = get_stack_index();
    CodeEditor* editor;
    if (get_stack_count()) {
        index = get_stack_index();
        Q_ASSERT(0 <= index && index < this->data.size());
        FileInfo* finfo = data[index];
        editor = finfo->editor;
        editor->setFocus();
        this->_refresh_outlineexplorer(index, false);
        emit update_code_analysis_actions();
        this->__refresh_statusbar(index);
        this->__refresh_readonly(index);
        this->__check_file_status(index);
        this->__modify_stack_title();
        emit update_plugin_title();
    }
    else
        editor = nullptr;
    this->modification_changed();
    this->find_widget->set_editor(editor, false);
}

void EditorStack::modification_changed(int state, int index, size_t editor_id)
{
    if (editor_id != 0) { // 因为editor_id类型是size_t,所以默认值设为0
        for (index = 0; index < data.size(); ++index) {
            FileInfo* _finfo = data[index];
            if (reinterpret_cast<size_t>(_finfo->editor) == editor_id)
                break;
        }
    }
    emit opened_files_list_changed();

    if (index == -2) //该函数中用-2表示None
        index = get_stack_index();
    if (index == -1)
        return;
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    bool is_modified = false;
    if (state == 1)
        is_modified = true;
    else if (state == -1)
        is_modified = finfo->editor->document()->isModified() || finfo->newly_created;
    //qDebug() << __func__ << finfo->editor->document()->isModified() << finfo->newly_created;
    set_stack_title(index, is_modified);
    save_action->setEnabled(is_modified);
    emit refresh_save_all_action();
    QString eol_chars = finfo->editor->get_line_separator();
    this->refresh_eol_chars(eol_chars);
    stack_history.refresh();
}

void EditorStack::refresh_eol_chars(const QString &eol_chars)
{
    QString os_name = sourcecode::get_os_name_from_eol_chars(eol_chars);
    emit sig_refresh_eol_chars(os_name);//不发送该信号就可以不用注释switch_to_plugin
}

void EditorStack::reload(int index)
{
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    QString txt = encoding::read(finfo->filename);
    finfo->lastmodified = QFileInfo(finfo->filename).lastModified();
    int position = finfo->editor->get_position("cursor");
    finfo->editor->set_text(txt);
    finfo->editor->document()->setModified(false);
    finfo->editor->set_cursor_position(position);
    //introspector.validate()

    finfo->editor->rehighlight();

    this->_refresh_outlineexplorer(index);
}

void EditorStack::revert()
{
    int index = this->get_stack_index();
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    QString filename = finfo->filename;
    QFileInfo info(filename);
    if (finfo->editor->document()->isModified()) {
        msgbox = new QMessageBox(QMessageBox::Warning,
                                 title,
                                 QString("All changes to <b>%1</b> will be lost."
                                         "<br>Do you want to revert file from disk?")
                                 .arg(info.fileName()),
                                 QMessageBox::Yes | QMessageBox::No,
                                 this);
        int answer = msgbox->exec();
        if (answer != QMessageBox::Yes)
            return;
    }
    this->reload(index);
}

FileInfo* EditorStack::create_new_editor(const QString &fname, const QString &enc, const QString &txt,
                                         bool set_current, bool _new, CodeEditor *cloned_from)
{
    CodeEditor* editor = new CodeEditor(this);
    // TODO introspector = self.introspector

    FileInfo* finfo = new FileInfo(fname, enc, editor, _new, this->threadmanager);

    this->add_to_data(finfo, set_current);
    //add_to_data()函数中调用tabs->insertTab(index, editor)添加CodeEditor
    connect(finfo,SIGNAL(send_to_help(QString, QString, QString, QString, bool)),
            this,SLOT(send_to_help(QString, QString, QString, QString, bool)));
    connect(finfo,SIGNAL(analysis_results_changed()),this,SIGNAL(analysis_results_changed()));
    connect(finfo,SIGNAL(todo_results_changed()),this,SIGNAL(todo_results_changed()));
    connect(finfo,SIGNAL(edit_goto(QString, int, QString)),this,SIGNAL(edit_goto(QString, int, QString)));
    connect(finfo,SIGNAL(save_breakpoints(QString, QString)),this,SIGNAL(save_breakpoints(QString, QString)));

    connect(editor,SIGNAL(run_selection()),this,SLOT(run_selection()));
    connect(editor,SIGNAL(run_cell()),this,SLOT(run_cell()));
    connect(editor,SIGNAL(run_cell_and_advance()),this,SLOT(run_cell_and_advance()));
    connect(editor,SIGNAL(re_run_last_cell()),this,SLOT(re_run_last_cell()));
    connect(editor,SIGNAL(sig_new_file(QString)),this,SIGNAL(sig_new_file(QString)));

    QString language = get_file_language(fname, txt);

    QHash<QString, QVariant> kwargs;
    kwargs["linenumbers"] = this->linenumbers_enabled;
    kwargs["show_blanks"] = this->blanks_enabled;
    kwargs["edge_line"] = this->edgeline_enabled;
    kwargs["edge_line_column"] = this->edgeline_column;
    kwargs["language"] = language;
    kwargs["markers"] = this->has_markers();
    kwargs["font"] = QVariant::fromValue(this->default_font);

    QHash<QString, QVariant> dict;
    for (auto it=this->color_scheme.begin();it!=this->color_scheme.end();it++) {
        QString key = it.key();
        QVariant val = QVariant::fromValue(it.value());
        dict[key] = val;
    }
    kwargs["color_scheme"] = dict;

    kwargs["wrap"] = this->wrap_enabled;
    kwargs["tab_mode"] = this->tabmode_enabled;
    kwargs["intelligent_backspace"] = this->intelligent_backspace_enabled;
    kwargs["highlight_current_line"] = this->highlight_current_line_enabled;
    kwargs["highlight_current_cell"] = this->highlight_current_cell_enabled;
    kwargs["occurrence_highlighting"] = this->occurrence_highlighting_enabled;
    kwargs["occurrence_timeout"] = this->occurrence_highlighting_timeout;
    kwargs["codecompletion_auto"] = this->codecompletion_auto_enabled;
    kwargs["codecompletion_case"] = this->codecompletion_case_enabled;
    kwargs["codecompletion_enter"] = this->codecompletion_enter_enabled;
    kwargs["calltips"] = this->calltips_enabled;
    kwargs["go_to_definition"] = this->go_to_definition_enabled;
    kwargs["close_parentheses"] = this->close_parentheses_enabled;
    kwargs["close_quotes"] = this->close_quotes_enabled;
    kwargs["add_colons"] = this->add_colons_enabled;
    kwargs["auto_unindent"] = this->auto_unindent_enabled;
    kwargs["indent_chars"] = this->indent_chars;
    kwargs["tab_stop_width_spaces"] = this->tab_stop_width_spaces;
    if (cloned_from != nullptr) {
        size_t id = reinterpret_cast<size_t>(cloned_from);
        kwargs["cloned_from"] = id;
    }
    kwargs["filename"] = fname;

    editor->setup_editor(kwargs);

    if (cloned_from == nullptr) {
        editor->set_text(txt);
        editor->document()->setModified(false);
    }
    connect(finfo,SIGNAL(text_changed_at(QString, int)),this,SIGNAL(text_changed_at(QString, int)));
    connect(editor,SIGNAL(sig_cursor_position_changed(int,int)),
            this,SLOT(editor_cursor_position_changed(int,int)));
    connect(editor,SIGNAL(textChanged()),this,SLOT(start_stop_analysis_timer()));
    connect(editor,&QPlainTextEdit::modificationChanged,
            [=](bool state){this->modification_changed((state ? 1 :0),-2,reinterpret_cast<size_t>(editor));});
    connect(editor,SIGNAL(focus_in()),this,SLOT(focus_changed()));
    connect(editor,SIGNAL(zoom_in()),this,SIGNAL(zoom_in()));
    connect(editor,SIGNAL(zoom_out()),this,SIGNAL(zoom_out()));
    connect(editor,SIGNAL(zoom_reset()),this,SIGNAL(zoom_reset()));
    connect(editor,SIGNAL(sig_eol_chars_changed(QString)),
            this,SLOT(refresh_eol_chars(QString)));

    this->find_widget->set_editor(editor);
    emit refresh_file_dependent_actions();
    this->modification_changed(-1, data.indexOf(finfo));

    editor->run_pygments_highlighter();
    return finfo;
}

void EditorStack::editor_cursor_position_changed(int line, int index)
{
    emit sig_editor_cursor_position_changed(line, index);
}

void EditorStack::send_to_help(QString qstr1, QString qstr2, QString qstr3, QString qstr4, bool force)
{
    // TODO
}

FileInfo* EditorStack::_new(const QString &filename, const QString &encoding,
                            const QString &text, bool default_content, bool empty)
{
    FileInfo* finfo = this->create_new_editor(filename, encoding, text, false, true);
    finfo->editor->set_cursor_position("eof");
    if (!empty)
        finfo->editor->insert_text(os::linesep);
    if (default_content) {
        finfo->_default = true;
        finfo->editor->document()->setModified(false);
    }
    return finfo;
}

FileInfo* EditorStack::load(QString filename, bool set_current)
{
    QFileInfo info(filename);
    filename = info.absoluteFilePath();
    emit starting_long_process(QString("Loading %1...").arg(filename));
    QString text = encoding::read(filename);
    FileInfo* finfo = this->create_new_editor(filename,"utf-8",text,set_current);
    int index = data.indexOf(finfo);
    this->_refresh_outlineexplorer(index, true);
    emit ending_long_process("");
    /*if (isVisible() && checkeolchars_enabled
            && sourcecode::has_mixed_eol_chars(text)) {
        QString name = info.fileName();
        msgbox = new QMessageBox(QMessageBox::Warning,
                                 this->title,
                                 QString("<b>%1</b> contains mixed end-of-line "
                                         "characters.<br>Spyder will fix this "
                                         "automatically.").arg(name),
                                 QMessageBox::Ok,
                                 this);
        msgbox->exec();
        set_os_eol_chars(index);
    }*/
    is_analysis_done = false;
    return finfo;
}

void EditorStack::set_os_eol_chars(int index)
{
    if (index == -1)
        index = get_stack_index();
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    QString eol_chars = sourcecode::get_eol_chars_from_os_name(os::name);
    finfo->editor->set_eol_chars(eol_chars);
    finfo->editor->document()->setModified(true);
}

void EditorStack::remove_trailing_spaces(int index)
{
    if (index == -1)
        index = get_stack_index();
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    finfo->editor->remove_trailing_spaces();
}

void EditorStack::fix_indentation(int index)
{
    if (index == -1)
        index = get_stack_index();
    Q_ASSERT(0 <= index && index < this->data.size());
    FileInfo* finfo = data[index];
    finfo->editor->fix_indentation();
}


//------ Run
void EditorStack::run_selection()
{
    QString text = get_current_editor()->get_selection_as_executable_code();
    if (!text.isEmpty()) {
        emit exec_in_extconsole(text, focus_to_editor);
        //该信号用来和ipyconsole相连，来运行代码
        return;
    }
    CodeEditor* editor = get_current_editor();
    QString line = editor->get_current_line();
    text = lstrip(line);
    if (!text.isEmpty())
        emit exec_in_extconsole(text, focus_to_editor);
    if (editor->is_cursor_on_last_line() && !text.isEmpty())
        editor->append(editor->get_line_separator());
    editor->move_cursor_to_next("line", "down");
}

void EditorStack::run_cell()
{
    QString text = get_current_editor()->get_selection_as_executable_code();
    FileInfo* finfo = get_current_finfo();
    if (finfo->editor->is_python() && !text.isEmpty())
        emit exec_in_extconsole(text, focus_to_editor);
}

void EditorStack::run_cell_and_advance()
{
    run_cell();
    advance_cell();
}

void EditorStack::advance_cell(bool reverse)
{
    std::function<void()> move_func;
    if (!reverse)
        move_func = [this](){this->get_current_editor()->go_to_next_cell();};
    else
        move_func = [this](){this->get_current_editor()->go_to_previous_cell();};

    if (focus_to_editor)
        move_func();
    else {
        QWidget* term = QApplication::focusWidget();
        move_func();
        term->setFocus();
        term = QApplication::focusWidget();
        move_func();
        term->setFocus();
    }
}

void EditorStack::re_run_last_cell()
{
    QString text = get_current_editor()->get_last_cell_as_executable_code();
    FileInfo* finfo = get_current_finfo();
    if (finfo->editor->is_python() && !text.isEmpty())
        emit exec_in_extconsole(text, focus_to_editor);
}

void EditorStack::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData* source = event->mimeData();
    if (source->hasUrls() && !source->urls().isEmpty() && !mimedata2url(source).isEmpty()) {
        QStringList all_urls = mimedata2url(source);
        QList<bool> text;
        foreach (QString url, all_urls) {
            text.append(encoding::is_text_file(url));
        }
		if (std::any_of(text.cbegin(), text.cend(), [](bool is_text) { return is_text; }))
            event->acceptProposedAction();
        else
            event->ignore();
    }
    else if (source->hasText())
        event->acceptProposedAction();
    else if (os::name == "nt")
        event->acceptProposedAction();
    else
        event->ignore();
}

void EditorStack::dropEvent(QDropEvent *event)
{
    const QMimeData* source = event->mimeData();
    if (source->hasUrls() && !mimedata2url(source).isEmpty()) {
        QStringList tmp = mimedata2url(source);
        QStringList files;
        foreach (QString file, tmp) {
            if (encoding::is_text_file(file))
                files.append(file);
        }
        QSet<QString> set = QSet<QString>::fromList(files);
        for (auto it=set.begin();it!=set.end();it++)
            emit plugin_load(*it);
    }
    else if (source->hasText()) {
        CodeEditor* editor = get_current_editor();
        if (editor)
            editor->insert_text(source->text());
    }
    else
        event->ignore();
    event->acceptProposedAction();
}


/********** EditorSplitter **********/
EditorSplitter::EditorSplitter(QWidget* parent, Editor *plugin,
                               const QList<QAction*>& menu_actions, bool first,
                               std::function<void(EditorStack*)> register_editorstack_cb,
                               std::function<bool(EditorStack*)> unregister_editorstack_cb)
    : QSplitter (parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setChildrenCollapsible(false);

    this->toolbar_list.clear();
    this->menu_list.clear();

    this->plugin = plugin;

    if (register_editorstack_cb == nullptr)
        register_editorstack_cb = [this](EditorStack* stack){this->plugin->register_editorstack(stack);};
    this->register_editorstack_cb = register_editorstack_cb;
    if (unregister_editorstack_cb == nullptr)
        unregister_editorstack_cb = [this](EditorStack* stack){return this->plugin->unregister_editorstack(stack);};
    this->unregister_editorstack_cb = unregister_editorstack_cb;

    this->menu_actions = menu_actions;
    editorstack = new EditorStack(this,menu_actions);
    this->register_editorstack_cb(this->editorstack);
    if (!first)
        this->plugin->clone_editorstack(this->editorstack);
    connect(editorstack,SIGNAL(destroyed(QObject*)),this,SLOT(editorstack_closed()));
    connect(editorstack,&EditorStack::split_vertically,
            [this](){this->split(Qt::Vertical);});
    connect(editorstack,&EditorStack::split_horizontally,
            [this](){this->split(Qt::Horizontal);});
    addWidget(editorstack);
}

void EditorSplitter::__give_focus_to_remaining_editor()
{
    QWidget* focus_widget = plugin->get_focus_widget();
    if (focus_widget)
        focus_widget->setFocus();
}

void EditorSplitter::editorstack_closed()
{
    if (DEBUG_EDITOR) {
        qDebug() << "method 'editorstack_closed':";
        qDebug() << "    self  :" << this;
    }
    bool close_splitter;
    try {
        unregister_editorstack_cb(this->editorstack);
        editorstack = nullptr;
        close_splitter = count() == 1;
    } catch (...) {
        return;
    }
    if (close_splitter) {
        close();
        return;
    }
    __give_focus_to_remaining_editor();
}

void EditorSplitter::editorsplitter_closed()
{
    if (DEBUG_EDITOR) {
        qDebug() << "method 'editorsplitter_closed':";
        qDebug() << "    self  :" << this;
    }
    bool close_splitter;
    try {
        close_splitter = count() == 1 && editorstack == nullptr;
    } catch (...) {
        return;
    }
    if (close_splitter) {
        close();
        return;
    }
    else if (count() == 2 && editorstack)
        editorstack->reset_orientation();
    __give_focus_to_remaining_editor();
}

void EditorSplitter::split(Qt::Orientation orientation)
{
    setOrientation(orientation);
    editorstack->set_orientation(orientation);
    QWidget* parent = qobject_cast<QWidget*>(this->parent());
    EditorSplitter* editorsplitter = new EditorSplitter(parent,plugin,menu_actions,false,
                                                        this->register_editorstack_cb,
                                                        this->unregister_editorstack_cb);
    addWidget(editorsplitter);
    connect(editorsplitter,SIGNAL(destroyed(QObject*)),this,SLOT(editorstack_closed()));
    CodeEditor* current_editor = editorsplitter->editorstack->get_current_editor();
    if (current_editor)
        current_editor->setFocus();
}

QList<QPair<EditorStack *, Qt::Orientation> > EditorSplitter::iter_editorstacks()
{
    QList<QPair<EditorStack*, Qt::Orientation>> editorstacks;
    EditorStack* stack = qobject_cast<EditorStack*>(widget(0));
    editorstacks.append(QPair<EditorStack*, Qt::Orientation>(stack, orientation()));
    if (count() > 1) {
        EditorSplitter* editorsplitter = qobject_cast<EditorSplitter*>(widget(1));
        if (editorsplitter)
            editorstacks.append(editorsplitter->iter_editorstacks());
    }
    return editorstacks;
}

SplitSettings EditorSplitter::get_layout_settings()
{
    QList<OrientationStrIntlist> splitsettings;
    foreach (auto pair, iter_editorstacks()) {
        EditorStack* editorstack = pair.first;
        Qt::Orientation orientation = pair.second;
        QList<int> clines;
        foreach (FileInfo* finfo, editorstack->data) {
            clines.append(finfo->editor->get_cursor_line_number());
        }
        QString cfname = editorstack->get_current_filename();
        splitsettings.append(OrientationStrIntlist(orientation == Qt::Vertical, cfname, clines));
    }
    return SplitSettings(qbytearray_to_str(this->saveState()), this->sizes(), splitsettings);
}

void EditorSplitter::set_layout_settings(const SplitSettings &settings)
{
    QList<OrientationStrIntlist> splitsettings = settings.splitsettings;
    EditorSplitter* splitter = this;
    CodeEditor* editor = nullptr;
    for (int index = 0; index < splitsettings.size(); ++index) {
        bool is_vertical = splitsettings[index].is_vertical;
        QString cfname = splitsettings[index].cfname;
        QList<int> clines = splitsettings[index].clines;
        if (index > 0) {
            if (is_vertical)
                splitter->split(Qt::Vertical);
            else
                splitter->split(Qt::Horizontal);
            splitter = qobject_cast<EditorSplitter*>(splitter->widget(1));
        }
        EditorStack* editorstack = qobject_cast<EditorStack*>(splitter->widget(0));
        for (int i = 0; i < editorstack->data.size(); ++i) {
            FileInfo* finfo = editorstack->data[i];
            editor = finfo->editor;
            try {
                editor->go_to_line(clines[i]);
            } catch (...) {
            }
        }
        editorstack->set_current_filename(cfname);
    }
    QString hexstate = settings.hexstate;
    if (!hexstate.isEmpty()) {
        restoreState(QByteArray::fromHex(hexstate.toUtf8()));
    }
    QList<int> sizes = settings.sizes;
    if (!sizes.isEmpty())
        setSizes(sizes);
    if (editor) {
        editor->clearFocus();
        editor->setFocus();
    }
}


/********** EditorWidget **********/
EditorWidget::EditorWidget(QMainWindow* parent, Editor *plugin,
             const QList<QAction*>& menu_actions, bool show_fullpath,
             bool show_all_files, bool show_comments)
    : QSplitter (parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    QStatusBar* statusbar = parent->statusBar();
    readwrite_status = new ReadWriteStatus(this, statusbar);
    eol_status = new EOLStatus(this, statusbar);
    encoding_status = new EncodingStatus(this, statusbar);
    cursorpos_status = new CursorPositionStatus(this, statusbar);

    this->editorstacks.clear();

    this->plugin = plugin;

    this->find_widget = new FindReplace(this, true);
    this->plugin->register_widget_shortcuts(this->find_widget);
    this->find_widget->hide();
    outlineexplorer = new OutlineExplorerWidget(this, show_fullpath,
                                                show_all_files,
                                                show_comments);
    connect(outlineexplorer, &OutlineExplorerWidget::edit_goto,
            [this,plugin](QString filenames, int _goto, QString word)
    {EditorMainWindow* window = qobject_cast<EditorMainWindow*>(this->parent());
        plugin->load(filenames, _goto, word, window);});

    QWidget* editor_widgets = new QWidget(this);
    QVBoxLayout* editor_layout = new QVBoxLayout;
    editor_layout->setContentsMargins(0, 0, 0, 0);
    editor_widgets->setLayout(editor_layout);
    editorsplitter = new EditorSplitter(this, plugin, menu_actions, false,
                                        [this](EditorStack* stack){this->register_editorstack(stack);},
                                        [this](EditorStack* stack){return this->unregister_editorstack(stack);});
    editor_layout->addWidget(editorsplitter);
    editor_layout->addWidget(this->find_widget);

    QSplitter* splitter = new QSplitter(this);
    splitter->setContentsMargins(0, 0, 0, 0);
    splitter->addWidget(editor_widgets);
    splitter->addWidget(outlineexplorer);
    splitter->setStretchFactor(0, 5);
    splitter->setStretchFactor(1, 1);

    editorsplitter->editorstack->initialize_outlineexplorer();
}

void EditorWidget::register_editorstack(EditorStack *editorstack)
{
    this->editorstacks.append(editorstack);
    if (DEBUG_EDITOR) {
        qDebug() << "EditorWidget.register_editorstack:" << editorstack;
        __print_editorstacks();
    }
    QWidget* parent = qobject_cast<QWidget*>(this->parent());
    plugin->last_focus_editorstack[parent] = editorstack;
    editorstack->set_closable(this->editorstacks.size() > 1);
    editorstack->set_outlineexplorer(this->outlineexplorer);
    editorstack->set_find_widget(this->find_widget);
    connect(editorstack,SIGNAL(reset_statusbar()),readwrite_status,SLOT(hide()));
    connect(editorstack,SIGNAL(reset_statusbar()),encoding_status,SLOT(hide()));
    connect(editorstack,SIGNAL(reset_statusbar()),cursorpos_status,SLOT(hide()));

    connect(editorstack,SIGNAL(readonly_changed(bool)),readwrite_status,SLOT(readonly_changed(bool)));
    connect(editorstack,SIGNAL(encoding_changed(QString)),encoding_status,SLOT(encoding_changed(QString)));
    connect(editorstack,SIGNAL(sig_editor_cursor_position_changed(int,int)),
            cursorpos_status,SLOT(cursor_position_changed(int,int)));
    connect(editorstack,SIGNAL(sig_refresh_eol_chars(QString)),eol_status,SLOT(eol_changed(QString)));
    plugin->register_editorstack(editorstack);

    QToolButton* oe_btn = new QToolButton(this);
    oe_btn->setDefaultAction(outlineexplorer->visibility_action);
    QList<QPair<int, QToolButton*>> widgets;
    widgets.append(qMakePair(5,nullptr));
    widgets.append(qMakePair(-1, oe_btn));
    editorstack->add_corner_widgets_to_tabbar(widgets);
}

void EditorWidget::__print_editorstacks()
{
    qDebug() << QString::asprintf("%d editorstack(s) in editorwidget:", editorstacks.size());
    foreach (EditorStack* edst, editorstacks) {
        qDebug() << "    " << edst;
    }
}

bool EditorWidget::unregister_editorstack(EditorStack *editorstack)
{
    if (DEBUG_EDITOR)
        qDebug() << "EditorWidget.unregister_editorstack:" << editorstack;
    plugin->unregister_editorstack(editorstack);
    this->editorstacks.removeOne(editorstack);
    if (DEBUG_EDITOR)
        __print_editorstacks();
    return true;
}


/********** EditorMainWindow **********/
EditorMainWindow::EditorMainWindow(Editor *plugin,
                                   const QList<QAction*>& menu_actions,
                                   QList<StrStrActions> toolbar_list,
                                   QList<QPair<QString, QList<QObject *> > > menu_list,
                                   bool show_fullpath,
                                   bool show_all_files, bool show_comments)
    : QMainWindow ()
{
    setAttribute(Qt::WA_DeleteOnClose);

    this->window_size = QSize();

    editorwidget = new EditorWidget(this, plugin, menu_actions,
                                    show_fullpath, show_all_files, show_comments);
    setCentralWidget(editorwidget);

    EditorStack* editorstack = editorwidget->editorsplitter->editorstack;
    CodeEditor* editor = editorstack->get_current_editor();
    if (editor)
        editor->setFocus();

    setWindowTitle(QString("Spyder - %1").arg(plugin->windowTitle()));
    setWindowIcon(plugin->windowIcon());

    if (!toolbar_list.isEmpty()) {
        toolbars.clear();
        foreach (auto pair, toolbar_list) {
            QString title = pair.title;
            QString object_name = pair.object_name;
            QList<QAction*> actions = pair.actions;

            QToolBar* toolbar = addToolBar(title);
            toolbar->setObjectName(object_name);
            add_actions(toolbar, actions);
            this->toolbars.append(toolbar);
        }
    }

    if (!menu_list.isEmpty()) {
        QAction* quit_action = new QAction("Close window", this);
        quit_action->setIcon(ima::get_icon("close_panel.png"));
        quit_action->setToolTip("Close this window");
        connect(quit_action,SIGNAL(triggered()),this,SLOT(close()));

        this->menus.clear();
        for (int index = 0; index < menu_list.size(); ++index) {
            QString title = menu_list[index].first;
            QList<QObject*> actions = menu_list[index].second;

            QMenu* menu = menuBar()->addMenu(title);
            if (index == 0) {
                QList<QObject*> tmp = actions;
                tmp << nullptr << quit_action;
                add_actions(menu, tmp);
            }
            else
                add_actions(menu, actions);
            this->menus.append(menu);
        }
    }
}

QList<QToolBar*> EditorMainWindow::get_toolbars() const
{
    return toolbars;
}

void EditorMainWindow::add_toolbars_to_menu(const QString& menu_title, const QList<QToolBar*>& actions)
{
    Q_UNUSED(menu_title);
    Q_ASSERT(this->menus.size() >= 7);
    QMenu* view_menu = this->menus[6];

    if (actions == this->toolbars && view_menu) {
        QList<QAction*> toolbars;
        foreach (QToolBar* toolbr, this->toolbars) {
            QAction* action = toolbr->toggleViewAction();
            toolbars.append(action);
        }
        add_actions(view_menu, toolbars);
    }
}

void EditorMainWindow::load_toolbars()
{
    QStringList toolbars_names = CONF_get("main", "last_visible_toolbars",
                                          QStringList()).toStringList();
    //toolbars_names << "file_toolbar" << "run_toolbar" << "debug_toolbar"
    //               << "main_toolbar" << "Current working directory";
    if (!toolbars_names.isEmpty()) {
        QHash<QString, QToolBar*> dic;
        foreach (QToolBar* toolbar, this->toolbars) {
            dic[toolbar->objectName()] = toolbar;
            toolbar->toggleViewAction()->setChecked(false);
            toolbar->setVisible(false);
        }
        foreach (QString name, toolbars_names) {
            if (dic.contains(name)) {
                dic[name]->toggleViewAction()->setChecked(true);
                dic[name]->setVisible(true);
            }
        }
    }
}

void EditorMainWindow::resizeEvent(QResizeEvent *event)
{
    if (!isMaximized() && !isFullScreen())
        window_size = this->size();
    QMainWindow::resizeEvent(event);
}

LayoutSettings EditorMainWindow::get_layout_settings()
{
    SplitSettings splitsettings = editorwidget->editorsplitter->get_layout_settings();
    return LayoutSettings(window_size,
                          this->pos(),
                          isMaximized(),
                          isFullScreen(),
                          qbytearray_to_str(this->saveState()),
                          splitsettings);
}

void EditorMainWindow::set_layout_settings(const LayoutSettings &settings)
{
    QSize size = settings.size;
    if (size.isValid()) {
        resize(size);
        window_size = this->size();
    }
    QPoint pos = settings.pos;
    if (!pos.isNull())
        this->move(pos);
    QString hexstate = settings.hexstate;
    if (!hexstate.isEmpty()) {
        restoreState(QByteArray::fromHex(hexstate.toUtf8()));
    }
    if (settings.is_maximized)
        setWindowState(Qt::WindowMaximized);
    if (settings.is_fullscreen)
        setWindowState(Qt::WindowFullScreen);
    SplitSettings splitsettings = settings.splitsettings;
    editorwidget->editorsplitter->set_layout_settings(splitsettings);
}


/********** EditorPluginExample **********/
/*
EditorPluginExample::EditorPluginExample()
    : QSplitter ()
{
    QList<QAction*> menu_actions;

    this->find_widget = new FindReplace(this, true);
    outlineexplorer = new OutlineExplorerWidget(this,false,false);
    connect(outlineexplorer,SIGNAL(edit_goto(QString, int, QString)),
            this,SLOT(go_to_file(QString, int, QString)));
    editor_splitter = new EditorSplitter(this,this,menu_actions,true);

    QWidget* editor_widgets = new QWidget(this);
    QVBoxLayout* editor_layout = new QVBoxLayout;
    editor_layout->setContentsMargins(0, 0, 0, 0);
    editor_widgets->setLayout(editor_layout);
    editor_layout->addWidget(editor_splitter);
    editor_layout->addWidget(this->find_widget);

    setContentsMargins(0, 0, 0, 0);
    addWidget(editor_widgets);
    addWidget(this->outlineexplorer);

    setStretchFactor(0, 5);
    setStretchFactor(1, 1);

    this->menu_actions = menu_actions;
}

void EditorPluginExample::go_to_file(const QString& fname,int lineno,const QString& text)
{
    EditorStack* editorstack = this->editorstacks[0];
    editorstack->set_current_filename(fname);
    CodeEditor* editor = editorstack->get_current_editor();
    editor->go_to_line(lineno, text);
}

void EditorPluginExample::closeEvent(QCloseEvent *event)
{
    QList<EditorMainWindow*> tmp = this->editorwindows;
    foreach (EditorMainWindow* win, tmp) {
        win->close();
    }
    if (DEBUG_EDITOR) {
        qDebug() << this->editorwindows.size() << ":" << this->editorwindows;
        qDebug() << this->editorstacks.size() << ":" << this->editorstacks;
    }
    event->accept();
}

void EditorPluginExample::load(const QString& fname)
{
    QApplication::processEvents();
    EditorStack* editorstack = this->editorstacks[0];
    editorstack->load(fname);
    editorstack->analyze_script();
}

void EditorPluginExample::register_editorstack(EditorStack* editorstack)
{
    if (DEBUG_EDITOR)
        qDebug() << "FakePlugin.register_editorstack:" << editorstack;
    this->editorstacks.append(editorstack);
    if (isAncestorOf(editorstack)) {
        editorstack->set_closable(this->editorstacks.size() > 1);
        editorstack->set_outlineexplorer(this->outlineexplorer);
        editorstack->set_find_widget(this->find_widget);
        QToolButton* oe_btn = new QToolButton(this);
        oe_btn->setDefaultAction(outlineexplorer->visibility_action);

        QList<QPair<int, QToolButton*>> widgets;
        widgets.append(qMakePair(5,nullptr));
        widgets.append(qMakePair(-1, oe_btn));
        editorstack->add_corner_widgets_to_tabbar(widgets);
    }
    QAction* action = new QAction(this);
    editorstack->set_io_actions(action, action, action, action);
    QFont font("Courier New");
    font.setPointSize(10);
    editorstack->set_default_font(font);
    connect(editorstack,SIGNAL(sig_close_file(QString, QString)),
            this,SLOT(close_file_in_all_editorstacks(QString, QString)));
    connect(editorstack,SIGNAL(file_saved(QString, QString, QString)),
            this,SLOT(file_saved_in_editorstack(QString, QString, QString)));
    connect(editorstack,SIGNAL(file_renamed_in_data(QString, QString, QString)),
            this,SLOT(file_renamed_in_data_in_editorstack(QString, QString, QString)));
    connect(editorstack,SIGNAL(create_new_window()),this,SLOT(create_new_window()));
    connect(editorstack,SIGNAL(plugin_load(QString)),this,SLOT(load(QString)));
}

bool EditorPluginExample::unregister_editorstack(EditorStack* editorstack)
{
    if (DEBUG_EDITOR)
        qDebug() << "FakePlugin.unregister_editorstack:" << editorstack;
    this->editorstacks.removeOne(editorstack);
    return true;
}

void EditorPluginExample::clone_editorstack(EditorStack* editorstack)
{
    editorstack->clone_from(this->editorstacks[0]);
}

void EditorPluginExample::setup_window(QList<StrStrActions> toolbar_list,
                                       QList<QPair<QString, QList<QObject*>>> menu_list)
{
    this->toolbar_list = toolbar_list;
    this->menu_list = menu_list;
}

void EditorPluginExample::create_new_window()
{
    EditorMainWindow* window = new EditorMainWindow(this,this->menu_actions,
                                                    toolbar_list, menu_list,
                                                    false, false, true);
    window->resize(this->size());
    window->show();
    register_editorwindow(window);
    connect(window,&QObject::destroyed,
            [this,window](){this->unregister_editorwindow(window);});
}

void EditorPluginExample::register_editorwindow(EditorMainWindow* window)
{
    if (DEBUG_EDITOR)
        qDebug() << "register_editorwindowQObject*:" << window;
    editorwindows.append(window);
}

void EditorPluginExample::unregister_editorwindow(EditorMainWindow* window)
{
    if (DEBUG_EDITOR)
        qDebug() << "unregister_editorwindowQObject*:" << window;
    editorwindows.removeOne(window);
}

QWidget* EditorPluginExample::get_focus_widget()
{
    return nullptr;
}

void EditorPluginExample::close_file_in_all_editorstacks(QString editorstack_id_str,QString filename)
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

void EditorPluginExample::file_saved_in_editorstack(QString editorstack_id_str,
                                                    QString original_filename,QString filename)
{
    foreach (EditorStack* editorstack, this->editorstacks) {
        size_t id = reinterpret_cast<size_t>(editorstack);
        if (QString::number(id) != editorstack_id_str) {
            editorstack->file_saved_in_other_editorstack(original_filename, filename);
        }
    }
}

void EditorPluginExample::file_renamed_in_data_in_editorstack(QString editorstack_id_str,
                                         QString original_filename,QString filename)
{
    foreach (EditorStack* editorstack, this->editorstacks) {
        size_t id = reinterpret_cast<size_t>(editorstack);
        if (QString::number(id) != editorstack_id_str) {
            editorstack->rename_in_data(original_filename, filename);
        }
    }
}

void EditorPluginExample::register_widget_shortcuts(QWidget *widget)
{}

static void test()
{

    EditorPluginExample* test = new EditorPluginExample;
    test->resize(900, 700);
    test->show();
    QTime time;
    time.start();
    test->load("F:/MyPython/spyder/widgets/editor.py");
    test->load("F:/MyPython/spyder/widgets/explorer.py");
    test->load("F:/MyPython/spyder/widgets/variableexplorer/collectionseditor.py");
    test->load("F:/MyPython/spyder/widgets/sourcecode/codeeditor.py");
    qDebug() << QString::asprintf("Elapsed time: %.3f s", time.elapsed()/1000.0);

}
*/
