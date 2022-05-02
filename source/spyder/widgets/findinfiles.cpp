#include "findinfiles.h"
#include "plugins/plugins_findinfiles.h"

const QString ON = "on";
const QString OFF = "off";

const int CWD = 0;
const int PROJECT = 1;
const int FILE_PATH = 2;
const int SELECT_OTHER = 4;
const int CLEAR_LIST = 5;
const int EXTERNAL_PATHS = 7;

const int MAX_PATH_LENGTH = 60;
const int MAX_PATH_HISTORY = 15;

QString truncate_path(const QString& text)
{
    QString ellipsis = "...";
    double part_len = (MAX_PATH_LENGTH - ellipsis.size()) / 2.0;
    QString left_text = text.left(int(std::ceil(part_len)));
    QString right_text = text.right(int(std::floor(part_len)));
    return left_text + ellipsis + right_text;
}


/********** SearchThread **********/
SearchThread::SearchThread(QObject *parent)
    : QThread (parent)
{
    this->case_sensitive = true;
    this->total_matches = 0;
    this->is_file = false;
}

void SearchThread::initialize(const StruNotSave &stru)
{
    this->rootpath = stru.path;
    this->is_file = stru.is_file;
    if (!stru.exclude.isEmpty())
        this->exclude = stru.exclude;
    this->texts = stru.texts;
    this->text_re = stru.text_re;
    this->case_sensitive = stru.case_sensitive;

    this->stopped = false;
    this->completed = false;
}

void SearchThread::run()
{
    try {
        if (this->is_file)
            this->find_string_in_file(this->rootpath);
        else
            this->find_files_in_path(this->rootpath);
    } catch (...) {
        this->error_flag = "Unexpected error: see internal console";
    }
    this->stop();
    emit sig_finished(this->completed);
}

void SearchThread::stop()
{
    //QMutexLocker locker(&mutex);
	//QWriteLocker locker(&lock);
    this->stopped = true;
}

bool SearchThread::getStopped() const
{
	//QMutexLocker locker(&mutex);
	//QReadLocker locker(&lock);
	return this->stopped;
}

bool SearchThread::find_files_in_path(const QString &path)
{
    this->pathlist.append(path);
	QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
	QRegularExpression re(this->exclude);
	if (!this->exclude.isEmpty())
	{
		if (!re.isValid())
		{
			this->error_flag = "invalid regular expression";
			return false;
		}
	}
	while (it.hasNext()) {
		QString absolute_sub_path = it.next();
		{
			if (this->getStopped())
				return false;
		}
		if (absolute_sub_path.contains(".git") || absolute_sub_path.contains(".hg"))
			continue;
		if (!this->exclude.isEmpty())
		{
			QRegularExpressionMatch match = re.match(absolute_sub_path);
			if (match.hasMatch())
				continue;
		}
		if (encoding::is_text_file(absolute_sub_path))
		{
			this->find_string_in_file(absolute_sub_path);
		}
	}
    return true;
}

bool SearchThread::find_string_in_file(const QString &fname)
{
    this->error_flag = "False";
    emit sig_current_file(fname);
    QFile file(fname);
    bool ok = file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (ok) {
        int lineno = 0;
        while (!file.atEnd()) {
            QString line = file.readLine();
            QString text;
            foreach (auto pair, this->texts) {
                text = pair.first;
                {
                    if (this->getStopped())
                        return false;
                }
                QString line_search = line;
                if (!case_sensitive)
                    line_search = line.toLower();
                if (this->text_re) {
                    QRegularExpression re(text);
                    QRegularExpressionMatch match = re.match(line_search);
                    if (match.hasMatch())
                        break;
                }
                else {
                    int found = line_search.indexOf(text);
                    if (found > -1)
                        break;
                }
            }

            QString line_dec = line;
            if (!case_sensitive)
                line = line.toLower();
            if (text_re) {
                QRegularExpression re(text);
                QRegularExpressionMatchIterator iterator = re.globalMatch(line);
                while (iterator.hasNext()) {
                    QRegularExpressionMatch match = iterator.next();
                    {
                        if (this->getStopped())
                            return false;
                    }
                    this->total_matches++;
                    QFileInfo info(fname);
                    emit sig_file_match(info.absoluteFilePath(),
                                        lineno+1,
                                        match.capturedStart(),
                                        match.capturedEnd(),
                                        line_dec,
                                        this->total_matches);
                }
            }
            else {
                int found = line.indexOf(text);
                while (found > -1) {
                    {
                        if (this->getStopped())
                            return false;
                    }
                    this->total_matches++;
                    QFileInfo info(fname);
                    emit sig_file_match(info.absoluteFilePath(),
                                        lineno+1,
                                        found,
                                        found+text.size(),
                                        line_dec,
                                        this->total_matches);
                    for (int i = 0; i < this->texts.size(); ++i) {
                        QString text = this->texts[i].first;
                        found = line.indexOf(text, found+1);
                        if (found > -1)
                            break;
                    }
                }
            }

            lineno++;
        }
    }
    else {
        error_flag = "permission denied errors were encountered";
    }
    completed = true;
    return true;
}
/*
SearchResults SearchThread::get_results()
{
    return SearchResults(this->pathlist, this->total_matches, this->error_flag);
}*/


/********** SearchInComboBox **********/
SearchInComboBox::SearchInComboBox(const QStringList& external_path_history,QWidget* parent)
    : QComboBox (parent)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    this->setToolTip("Search directory");
    this->setEditable(false);

    this->path = "";

    this->addItem("Current working directory");
    QString ttip = "Search in all files and directories present on the current"
            " Spyder path";
    this->setItemData(0, ttip, Qt::ToolTipRole);

    this->addItem("Project");
    ttip = "Search in all files and directories present on the"
             " current project path (if opened)";
    this->setItemData(1, ttip, Qt::ToolTipRole);
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(this->model());
    model->item(1, 0)->setEnabled(false);

    this->addItem("File");
    ttip = "Search in current opened file";
    this->setItemData(2, ttip, Qt::ToolTipRole);

    this->insertSeparator(3);

    this->addItem("Select other directory");
    ttip = "Search in other folder present on the file system";
    this->setItemData(4, ttip, Qt::ToolTipRole);

    this->addItem("Clear this list");
    ttip = "Clear the list of other directories";
    this->setItemData(5, ttip, Qt::ToolTipRole);

    this->insertSeparator(6);

    foreach (QString path, external_path_history) {
        this->add_external_path(path);
    }

    connect(this,SIGNAL(currentIndexChanged(int)),this,SLOT(path_selection_changed()));
    this->view()->installEventFilter(this);
}

void SearchInComboBox::add_external_path(const QString &path)
{
    if (!QFileInfo::exists(path))
        return;
    this->removeItem(this->findText(path));
    this->addItem(path);
    this->setItemData(this->count()-1, path, Qt::ToolTipRole);
    while (this->count() > MAX_PATH_HISTORY + EXTERNAL_PATHS)
        this->removeItem(EXTERNAL_PATHS);
}

QStringList SearchInComboBox::get_external_paths() const
{
    QStringList list;
    for (int i=EXTERNAL_PATHS;i<this->count();i++)
        list.append(this->itemText(i));
    return list;
}

void SearchInComboBox::clear_external_paths()
{
    while (this->count() > EXTERNAL_PATHS)
        this->removeItem(EXTERNAL_PATHS);
}

QString SearchInComboBox::get_current_searchpath() const
{
    int idx = this->currentIndex();
    if (idx == CWD)
        return path;
    else if (idx == PROJECT)
        return project_path;
    else if (idx == FILE_PATH)
        return file_path;
    else
        return external_path;
}

bool SearchInComboBox::is_file_search() const
{
    if (this->currentIndex() == FILE_PATH)
        return true;
    else
        return false;
}

void SearchInComboBox::path_selection_changed()
{
    int idx = this->currentIndex();
    if (idx == SELECT_OTHER) {
        QString external_path = this->select_directory();
        if (external_path.size() > 0) {
            this->add_external_path(external_path);
            this->setCurrentIndex(this->count()-1);
        }
        else
            this->setCurrentIndex(CWD);
    }
    else if (idx == CLEAR_LIST) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,"Clear other directories",
                              "Do you want to clear the list of other directories?",
                              QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
            this->clear_external_paths();
        this->setCurrentIndex(CWD);
    }
    else if (idx >= EXTERNAL_PATHS)
        this->external_path = this->itemText(idx);
}

QString SearchInComboBox::select_directory()
{
    this->__redirect_stdio_emit(false);
    QString directory = QFileDialog::getExistingDirectory(this,"Select directory",path);
    //getExistingDirectory()函数返回绝对路径
    QFileInfo info(directory);
    directory = info.absoluteFilePath();
    this->__redirect_stdio_emit(true);
    return directory;
}

void SearchInComboBox::set_project_path(QString path)
{
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(this->model());
    if (path.isEmpty()) {
        project_path = QString();
        model->item(PROJECT, 0)->setEnabled(false);
        if (this->currentIndex() == PROJECT)
            this->setCurrentIndex(CWD);
    }
    else {
        QFileInfo info(path);
        path = info.absoluteFilePath();
        project_path = path;
        model->item(PROJECT, 0)->setEnabled(true);
    }
}

bool SearchInComboBox::eventFilter(QObject *widget, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Delete) {
        int index = this->view()->currentIndex().row();
        if (index >= EXTERNAL_PATHS) {
            this->removeItem(index);
            this->showPopup();
            int new_index = qMin(this->count()-1, index);
            if (new_index < EXTERNAL_PATHS)
                new_index = 0;
            this->view()->setCurrentIndex(model()->index(new_index, 0));
            this->setCurrentIndex(new_index);
        }
        return true;
    }
    return QComboBox::eventFilter(widget, event);
}

void SearchInComboBox::__redirect_stdio_emit(bool value)
{
    QObject* parent = this->parent();
    while (parent) {
        FindInFiles* tmp = qobject_cast<FindInFiles*>(parent);
        if (tmp) {
            emit tmp->redirect_stdio(value);
            break;
        }
        else
            parent = parent->parent();
    }
}


/********** FindOptions **********/
QString FindOptions::REGEX_INVALID = "background-color:rgb(255, 80, 80);";
QString FindOptions::REGEX_ERROR = "Regular expression error";

FindOptions::FindOptions(QWidget* parent,
            const QStringList& search_text,
            bool search_text_regexp,
            const QStringList& exclude,
            int exclude_idx,
            bool exclude_regexp,
            const QStringList& supported_encodings,
            bool more_options,
            bool case_sensitive,
            const QStringList& external_path_history)
    : QWidget (parent)
{
    this->supported_encodings = supported_encodings;

    QHBoxLayout* hlayout1 = new QHBoxLayout;
    this->search_text = new PatternComboBox(this, search_text,
                                            "Search pattern");
    edit_regexp = new QToolButton(this);
    edit_regexp->setIcon(ima::get_icon("regexp.svg"));
    edit_regexp->setToolTip("Regular expression");
    edit_regexp->setCheckable(true);
    edit_regexp->setChecked(search_text_regexp);

    case_button = new QToolButton(this);
    case_button->setIcon(ima::get_icon("upper_lower.png"));
    case_button->setToolTip("Case Sensitive");
    case_button->setCheckable(true);
    case_button->setChecked(case_sensitive);

    this->more_options = new QToolButton(this);
    connect(this->more_options,SIGNAL(clicked(bool)),this,SLOT(toggle_more_options(bool)));
    this->more_options->setCheckable(true);
    this->more_options->setChecked(more_options);

    ok_button = new QToolButton(this);
    ok_button->setText("Search");
    ok_button->setIcon(ima::icon("find"));
    connect(ok_button,SIGNAL(clicked(bool)),this,SIGNAL(find()));
    ok_button->setToolTip("Start search");
    ok_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(ok_button,SIGNAL(clicked(bool)),this,SLOT(update_combos()));

    stop_button = new QToolButton(this);
    stop_button->setText("Stop");
    stop_button->setIcon(ima::icon("stop"));
    connect(stop_button,SIGNAL(clicked(bool)),this,SIGNAL(stop()));
    stop_button->setToolTip("Stop search");
    stop_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    stop_button->setEnabled(false);

    hlayout1->addWidget(this->search_text);
    hlayout1->addWidget(this->edit_regexp);
    hlayout1->addWidget(this->case_button);
    hlayout1->addWidget(this->ok_button);
    hlayout1->addWidget(this->stop_button);
    hlayout1->addWidget(this->more_options);

    QHBoxLayout* hlayout2 = new QHBoxLayout;
    exclude_pattern = new PatternComboBox(this,exclude, "Exclude pattern");
    if (exclude_idx >= 0 && exclude_idx < exclude_pattern->count())
        exclude_pattern->setCurrentIndex(exclude_idx);
    this->exclude_regexp = new QToolButton(this);
    this->exclude_regexp->setIcon(ima::get_icon("regexp.svg"));
    this->exclude_regexp->setToolTip("Regular expression");
    this->exclude_regexp->setCheckable(true);
    this->exclude_regexp->setChecked(exclude_regexp);
    QLabel* exclude_label = new QLabel("Exclude:");
    exclude_label->setBuddy(this->exclude_pattern);
    hlayout2->addWidget(exclude_label);
    hlayout2->addWidget(exclude_pattern);
    hlayout2->addWidget(this->exclude_regexp);

    QHBoxLayout* hlayout3 = new QHBoxLayout;

    QLabel* search_on_label = new QLabel("Search in:");
    path_selection_combo = new SearchInComboBox(external_path_history, parent);
    hlayout3->addWidget(search_on_label);
    hlayout3->addWidget(path_selection_combo);

    connect(this->search_text,SIGNAL(valid(bool,bool)),this,SIGNAL(find()));
    connect(this->exclude_pattern,SIGNAL(valid(bool,bool)),this,SIGNAL(find()));

    QVBoxLayout* vlayout = new QVBoxLayout;
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->addLayout(hlayout1);
    vlayout->addLayout(hlayout2);
    vlayout->addLayout(hlayout3);

    more_widgets.append(hlayout2);
    toggle_more_options(more_options);
    setLayout(vlayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

//@Slot(bool)
void FindOptions::toggle_more_options(bool state)
{
    foreach (QLayout* layout, this->more_widgets) {
        for (int index = 0; index < layout->count(); ++index) {
            if ((state && isVisible()) || !state) {
                layout->itemAt(index)->widget()->setVisible(state);
            }

        }
    }
    QIcon icon;
    QString tip;
    if (state) {
        icon = ima::icon("options_less");
        tip = "Hide advanced options";
    }
    else {
        icon = ima::icon("options_more");
        tip = "Show advanced options";
    }
    more_options->setIcon(icon);
    more_options->setToolTip(tip);
}

//@Slot()
void FindOptions::update_combos()
{
    emit search_text->lineEdit()->returnPressed();
    emit exclude_pattern->lineEdit()->returnPressed();
}

void FindOptions::set_search_text(const QString &text)
{
    if (!text.isEmpty()) {
        search_text->add_text(text);
        search_text->lineEdit()->selectAll();
    }
    search_text->setFocus();
}

StruToSave FindOptions::get_options_to_save()
{
    //Return current options for them to be saved when closing Spyder.
    bool text_re = edit_regexp->isChecked();
    bool exclude_re = exclude_regexp->isChecked();
    bool case_sensitive = case_button->isChecked();


    QStringList search_text;
    for (int index=0;index<this->search_text->count();index++)
        search_text.append(this->search_text->itemText(index));
    QStringList exclude;
    for (int index=0;index<this->exclude_pattern->count();index++)
        exclude.append(this->exclude_pattern->itemText(index));
    int exclude_idx = this->exclude_pattern->currentIndex();
    QStringList path_history = this->path_selection_combo->get_external_paths();
    bool more_options = this->more_options->isChecked();
    return StruToSave(search_text, text_re, exclude, exclude_idx,
                      exclude_re, more_options, case_sensitive, path_history);
}

StruNotSave FindOptions::get_options_not_save()
{
    bool text_re = edit_regexp->isChecked();
    bool exclude_re = exclude_regexp->isChecked();
    bool case_sensitive = case_button->isChecked();


    this->search_text->lineEdit()->setStyleSheet("");
    exclude_pattern->lineEdit()->setStyleSheet("");
    this->search_text->setToolTip("");
    exclude_pattern->setToolTip("");

    QString utext = this->search_text->currentText();
    if (utext.isEmpty())
        return StruNotSave();

    QList<QPair<QString,QString>> texts;
    texts.append(QPair<QString,QString>(utext, "utf-8"));

    QString exclude = exclude_pattern->currentText();

    if (!case_sensitive) {
        for (int i=0;i<texts.size();i++)
            texts[i].first = texts[i].first.toLower();
    }

    bool file_search = this->path_selection_combo->is_file_search();
    QString path = this->path_selection_combo->get_current_searchpath();

    if (!exclude_re) {
        QStringList items;
        foreach (QString item, exclude.split(",")) {
            if (item.trimmed() != "")
                items.append(fnmatch::translate(item.trimmed()));
        }
        exclude = items.join('|');
    }

    if (!exclude.isEmpty()) {
        QString error_msg = misc::regexp_error_msg(exclude);
        if (!error_msg.isEmpty()) {
            QLineEdit* exclude_edit = exclude_pattern->lineEdit();
            exclude_edit->setStyleSheet(REGEX_INVALID);
            QString tooltip = REGEX_ERROR + ": " + error_msg;
            exclude_pattern->setToolTip(tooltip);
            return StruNotSave();
        }
        //else:
    }

    if (text_re) {
        QString error_msg = misc::regexp_error_msg(texts[0].first);
        if (!error_msg.isEmpty()) {
            search_text->lineEdit()->setStyleSheet(REGEX_INVALID);
            QString tooltip = REGEX_ERROR + ": " + error_msg;
            search_text->setToolTip(tooltip);
            return StruNotSave();
        }
    }
    return StruNotSave(path, file_search, exclude, texts, text_re, case_sensitive);
}

QString FindOptions::path() const
{
    return path_selection_combo->path;
}

void FindOptions::set_directory(const QString &directory)
{
    QFileInfo info(directory);
    path_selection_combo->path = info.absoluteFilePath();
}

QString FindOptions::project_path() const
{
    return path_selection_combo->project_path;
}

void FindOptions::set_project_path(const QString &path)
{
    path_selection_combo->set_project_path(path);
}

void FindOptions::disable_project_search()
{
    path_selection_combo->set_project_path(QString());
}

QString FindOptions::file_path() const
{
    return path_selection_combo->file_path;
}

void FindOptions::set_file_path(const QString &path)
{
    path_selection_combo->file_path = path;
}

void FindOptions::keyPressEvent(QKeyEvent *event)
{
    Qt::KeyboardModifiers ctrl = event->modifiers() & Qt::ControlModifier;
    Qt::KeyboardModifiers shift = event->modifiers() & Qt::ShiftModifier;
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        emit this->find();
    else if (event->key() == Qt::Key_F && ctrl && shift) {
		FindInFiles* parent = qobject_cast<FindInFiles*>(this->parent());
        if (parent)
            emit parent->toggle_visibility(!this->isVisible());
    }
    else
        QWidget::keyPressEvent(event);
}


/********** LineMatchItem **********/
LineMatchItem::LineMatchItem(QTreeWidgetItem* parent, const QStringList &title)
    : QTreeWidgetItem(parent, title, QTreeWidgetItem::Type)
{}

QStringList __repr__(int lineno, int colno, QString match)
{
    match = rstrip(match);
    QFont font = gui::get_font();
    QString _str = QString("<b>%2</b> (%3): "
                           "<span style='font-family:%1;"
                           "font-size:75%;'>%4</span>");
    return QStringList(_str.arg(font.family()).arg(lineno).arg(colno).arg(match));
}

bool LineMatchItem::operator<(const LineMatchItem &other) const
{
    return this->lineno < other.lineno;
}

bool LineMatchItem::operator>=(const LineMatchItem &other) const
{
    return this->lineno >= other.lineno;
}


/********** FileMatchItem **********/
QStringList fileMatchItemHelp(const QString filename)
{
    QFileInfo info(filename);
    QString title_format = "<b>%1</b><br>"
                           "<small><em>%2</em>"
                           "</small>";
    QString title = title_format.arg(info.fileName()).arg(info.absolutePath());
    return QStringList(title);

}
FileMatchItem::FileMatchItem(QTreeWidget *parent, const QString& filename,
	const QHash<QString,QString>& sorting, const QStringList &title)
    : QTreeWidgetItem(parent, title, QTreeWidgetItem::Type)
{
    this->sorting = sorting;
    QFileInfo info(filename);
    this->filename = info.fileName();

    setToolTip(0, filename);
}

bool FileMatchItem::operator<(const FileMatchItem &other) const
{
    if (sorting["status"] == ON) {
        int res = this->filename.compare(other.filename);
		return res < 0;
    }
    else
        return false;
}

bool FileMatchItem::operator>=(const FileMatchItem &other) const
{
    if (sorting["status"] == ON) {
        int res = this->filename.compare(other.filename);
        return res >= 0;
    }
    else
        return false;
}


/********** ItemDelegate **********/
ItemDelegate::ItemDelegate(QObject* parent)
    : QStyledItemDelegate (parent)
{}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem options(option);
    initStyleOption(&options, index);

    QStyle* style = options.widget ? options.widget->style() : QApplication::style();

    QTextDocument doc;
    doc.setDocumentMargin(0);
    doc.setHtml(options.text);

    options.text = "";
    style->drawControl(QStyle::CE_ItemViewItem, &options, painter);

    QAbstractTextDocumentLayout::PaintContext ctx;

    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &options);
    painter->save();

    painter->translate(textRect.topLeft());
    painter->setClipRect(textRect.translated(-textRect.topLeft()));
    doc.documentLayout()->draw(painter, ctx);
    painter->restore();
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem options(option);
    initStyleOption(&options, index);

    QTextDocument doc;
    doc.setHtml(options.text);
    doc.setTextWidth(options.rect.width());

    return QSize(int(doc.idealWidth()) ,int(doc.size().height()));
}


/********** ResultsBrowser **********/
ResultsBrowser::ResultsBrowser(QWidget* parent)
    : OneColumnTree (parent)
{
    set_title("");
    set_sorting(OFF);
    setSortingEnabled(false);
    //root_items = None  没有用到
    sortByColumn(0, Qt::AscendingOrder);
    setItemDelegate(new ItemDelegate(this));
    setUniformRowHeights(false);
    connect(header(),SIGNAL(sectionClicked(int)),this,SLOT(sort_section(int)));
}

void ResultsBrowser::activated(QTreeWidgetItem *item)
{
    Q_UNUSED(item);
    StrIntInt itemdata = this->data.value(reinterpret_cast<size_t>(currentItem()), StrIntInt());
    if (itemdata.lineno != -1) {
        QString filename = itemdata.filename;
        int lineno = itemdata.lineno;
        FindInFiles* parent = qobject_cast<FindInFiles*>(this->parent());
        if (parent)
            emit parent->edit_goto(filename, lineno, this->search_text);
    }
}

void ResultsBrowser::set_sorting(const QString &flag)
{
    sorting["status"] = flag;
    header()->setSectionsClickable(flag == ON);
}

//@Slot(int)
void ResultsBrowser::sort_section(int idx)
{
    Q_UNUSED(idx);
    setSortingEnabled(true);
}

//@Slot()
void ResultsBrowser::clicked(QTreeWidgetItem *item)
{
    this->activated(item);
}

void ResultsBrowser::clear_title(const QString &search_text)
{
    clear();
    setSortingEnabled(false);
    num_files = 0;
    data.clear();
    files.clear();
    set_sorting(OFF);
    this->search_text = search_text;
    QString title = QString("'%1' - ").arg(search_text);
    QString text = "String not found";
    set_title(title + text);
}

QString ResultsBrowser::html_escape(const QString &text)
{
    QHash<QChar,QString> html_escape_table = {{'&', "&amp;"},
                                              {'"', "&quot;"},
                                              {'\'', "&apos;"},
                                              {'>', "&gt;"},
                                              {'<', "&lt;"}};
    QStringList list;
    foreach (QChar c, text) {
        QString tmp = html_escape_table.value(c, c);
        list.append(tmp);
    }
    return list.join("");
}

QString ResultsBrowser::truncate_result(const QString &line, int start, int end)
{
    QString ellipsis = "...";
    int max_line_length = 80;
    int max_num_char_fragment = 40;

    QString left = line.left(start);
    QString match = line.mid(start, end-start);
    QString right = line.mid(end);

    if (line.size() > max_line_length) {
        int offset = (line.size() - match.size()) / 2;

        QStringList left_list = left.split(' ');
        int num_left_words = left_list.size();

        if (num_left_words == 1) {
            left = left_list[0];
            if (left.size() > max_num_char_fragment)
                left = ellipsis + left.right(offset);
            left_list.clear();
            left_list.append(left);
        }

        QStringList right_list = right.split(' ');
        int num_right_words = right_list.size();

        if (num_right_words == 1) {
            right = right_list[0];
            if (right.size() > max_num_char_fragment)
                right = ellipsis + right.right(offset);
            right_list.clear();
            right_list.append(right);
        }

        int idx = qMax(0, left_list.size()-4);
        int len = left_list.size()-idx;
        QStringList left_tmp_list = left_list.mid(idx, len);
        len = qMin(4, right_list.size());
        QStringList right_tmp_list = right_list.mid(0, len);

        if (left_tmp_list.size() < num_left_words)
            left_tmp_list.push_front(ellipsis);
        if (right_tmp_list.size() < num_right_words)
            right_tmp_list.push_back(ellipsis);

        left = left_tmp_list.join(' ');
        right = right_tmp_list.join(' ');

        if (left.size() > max_num_char_fragment)
            left = ellipsis + left.right(30);
        if (right.size() > max_num_char_fragment)
            right  = right.left(30) + ellipsis;
    }
    QString line_match_format = "%1<b>%2</b>%3";
    left = html_escape(left);
    right = html_escape(right);
    match = html_escape(match);
    QString trunc_line = line_match_format.arg(left).arg(match).arg(right);
    return trunc_line;
}

//@Slot()
void ResultsBrowser::append_result(QString filename,int lineno,int colno,
                                   int match_end,QString line,int num_matches)
{
    //qDebug()<<filename<<lineno<<line;
    if (!this->files.contains(filename)) {
        QStringList title = fileMatchItemHelp(filename);
        FileMatchItem* file_item = new FileMatchItem(this, filename, sorting, title);
        file_item->setExpanded(true);
        files[filename] = file_item;
        num_files++;
    }

    QString search_text = this->search_text;
    QString title = QString("'%1' - ").arg(search_text);
    int nb_files = this->num_files;
    QString text;
    if (nb_files == 0)
        text = "String not found";
    else {
        QString text_matches = "matches in";
        QString text_files = "file";
        if (nb_files > 1)
            text_files += 's';
        text = QString("%1 %2 %3 %4").arg(num_matches).arg(text_matches)
                .arg(nb_files).arg(text_files);
    }

    set_title(title + text);

    FileMatchItem* file_item = this->files[filename];
    line = truncate_result(line, colno, match_end);
    QStringList tmp = __repr__(lineno, colno, line);
    LineMatchItem* item = new LineMatchItem(file_item, tmp);
    this->data[reinterpret_cast<size_t>(item)] = StrIntInt(filename, lineno, colno);
}

/********** FileProgressBar **********/
FileProgressBar::FileProgressBar(QWidget* parent)
    : QWidget (parent)
{
    status_text = new QLabel(this);
    spinner = new QWaitingSpinner(this, false);
    spinner->setNumberOfLines(12);
    spinner->setInnerRadius(2);
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(spinner);
    layout->addWidget(status_text);
    setLayout(layout);
}

void FileProgressBar::set_label_path(const QString &path, bool folder)
{
    QString text = truncate_path(path);
    QString status_str;
    if (!folder)
        status_str = QString(" Scanning: %1").arg(text);
    else
        status_str = QString(" Searching for files in folder: %1").arg(text);
    status_text->setText(status_str);
}

void FileProgressBar::reset()
{
    status_text->setText("  Searching for files...");
}

void FileProgressBar::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    spinner->start();
}

void FileProgressBar::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    spinner->stop();
}


/********** FindInFilesWidget **********/
FindInFilesWidget::FindInFilesWidget(QWidget* parent,
                  QString search_text,
                  bool search_text_regexp,
                  QString exclude,
                  int exclude_idx,
                  bool exclude_regexp,
                  QStringList supported_encodings,
                  bool more_options,
                  bool case_sensitive,
                  QStringList external_path_history)
    : QWidget (parent)
{
    setWindowTitle("Find in files");
    this->search_thread = nullptr;
    status_bar = new FileProgressBar(this);
    status_bar->hide();
    QStringList search_text_list(search_text);
    QStringList exclude_list(exclude);
    find_options = new FindOptions(this, search_text_list,search_text_regexp,
                                   exclude_list, exclude_idx, exclude_regexp,
                                   supported_encodings,
                                   more_options, case_sensitive,
                                   external_path_history);
    connect(find_options,SIGNAL(find()),this,SLOT(find()));
    connect(find_options,&FindOptions::stop,
            [=](){this->stop_and_reset_thread();});

    result_browser = new ResultsBrowser(this);

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(result_browser);

    QVBoxLayout* layout = new QVBoxLayout;
    int left,right,bottom;
    layout->getContentsMargins(&left,nullptr,&right,&bottom);
    layout->setContentsMargins(left, 0, right, bottom);
    layout->addWidget(find_options);
    layout->addLayout(hlayout);
    layout->addWidget(status_bar);
    setLayout(layout);
}

void FindInFilesWidget::set_search_text(const QString &text)
{
    find_options->set_search_text(text);
}

void FindInFilesWidget::find()
{
    StruNotSave options = find_options->get_options_not_save();
    if (options.texts.isEmpty())
        return;
    stop_and_reset_thread(true);
    search_thread = new SearchThread(this);
    connect(search_thread,SIGNAL(sig_finished(bool)),this,SLOT(search_complete(bool)));
	// 不加this参数，会走qobject.h文件287行的connect函数模板，连接类型是DirectConnection。多线程需要加一个context参数，走297行的函数模板
    connect(search_thread,&SearchThread::sig_current_file, this,
            [=](QString x){status_bar->set_label_path(x,false);});
    connect(search_thread,&SearchThread::sig_current_folder, this,
            [=](QString x){status_bar->set_label_path(x,true);});
    connect(search_thread,&SearchThread::sig_file_match,
            result_browser,&ResultsBrowser::append_result);
    connect(search_thread,&SearchThread::sig_out_print, this,
            [=](QString x){qDebug() << x;});
    status_bar->reset();
    result_browser->clear_title(find_options->search_text->currentText());
    search_thread->initialize(options);
    search_thread->start();
    find_options->ok_button->setEnabled(false);
    find_options->stop_button->setEnabled(true);
    status_bar->show();
}

void FindInFilesWidget::stop_and_reset_thread(bool ignore_results)
{
    if (this->search_thread != nullptr) {
        if (this->search_thread->isRunning()) {
            if (ignore_results) {
                disconnect(search_thread,SIGNAL(sig_finished(bool)),
                           this,SLOT(search_complete(bool)));
            }
            search_thread->stop();
            search_thread->wait();
        }
        search_thread->setParent(nullptr);
        search_thread->deleteLater();
        search_thread = nullptr;
    }
}

void FindInFilesWidget::closing_widget()
{
    stop_and_reset_thread(true);
}

void FindInFilesWidget::search_complete(bool completed)
{
    Q_UNUSED(completed);
    result_browser->set_sorting(ON);
    find_options->ok_button->setEnabled(true);
    find_options->stop_button->setEnabled(false);
    status_bar->hide();
    result_browser->expandAll();
    if (!search_thread)
        return;
    emit sig_finished();
    stop_and_reset_thread();
    result_browser->show();
}


static void test()
{
    //测试通过，未发现bug
    FindInFilesWidget* widget = new FindInFilesWidget(nullptr);
    widget->resize(640, 480);
    widget->show();
    QStringList external_paths;
    external_paths << "F:/python/spyder/widgets"
                   << "F:/python/spyder" << "F:/python" << "F:/";
    foreach (QString path, external_paths)
        widget->find_options->path_selection_combo->add_external_path(path);
}
