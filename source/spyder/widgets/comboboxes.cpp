#include "comboboxes.h"

BaseComboBox::BaseComboBox(QWidget* parent)
    : QComboBox (parent)
{
    this->setEditable(true);
    this->setCompleter(new QCompleter(this));
    numpress = 0;
    selected_text = this->currentText();
}

bool BaseComboBox::event(QEvent *event)
{
    if ((event->type() == QEvent::KeyPress) && (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Tab)) {
        emit sig_tab_pressed(true);
        numpress += 1;
        if (numpress == 1)
            QTimer::singleShot(400, this, SLOT(handle_keypress()));
        return true;
    }
    return QComboBox::event(event);
}

void BaseComboBox::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        //按下回车键时，调用selected()函数发送valid()信号，连接到FindOptions类的find()信号
        //进行搜索
        if (this->add_current_text_if_valid()) {
            this->selected();
            this->hide_completer();
        }
    }
    else if (event->key() == Qt::Key_Escape) {
        this->set_current_text(selected_text);
        this->hide_completer();
    }
    else
        QComboBox::keyPressEvent(event);
}

void BaseComboBox::handle_keypress()
{
    if (numpress == 2)
        emit sig_double_tab_pressed(true);
    numpress = 0;
}

void BaseComboBox::selected()
{
    emit valid(true,true);
}

void BaseComboBox::add_text(const QString &text)
{
    int index = this->findText(text);
    while (index != -1) {
        this->removeItem(index);
        index = this->findText(text);
    }
    this->insertItem(0, text);
    index = this->findText("");
    if (index != -1) {
        this->removeItem(index);
        this->insertItem(0, "");
        if (text != "")
            this->setCurrentIndex(1);
        else
            this->setCurrentIndex(0);
    }
    else
        this->setCurrentIndex(0);
}

void BaseComboBox::set_current_text(const QString &text)
{
    this->lineEdit()->setText(text);
}

void BaseComboBox::add_current_text()
{
    // Add current text to combo box history (convenient method)
    QString text = this->currentText();
    this->add_text(text);
}

bool BaseComboBox::add_current_text_if_valid()
{
    bool valid = this->is_valid(this->currentText());
    if (valid) {
        this->add_current_text();
        return true;
    }
    else {
        this->set_current_text(selected_text);
        return false;
    }
}

void BaseComboBox::hide_completer()
{
    this->setCompleter(new QCompleter(QStringList(), this));
}


PatternComboBox::PatternComboBox(QWidget* parent,const QStringList& items,
                const QString& tip,bool adjust_to_minimum)
    : BaseComboBox (parent)
{// This property holds whether the line edit displays a clear button when it is not empty.
    this->lineEdit()->setClearButtonEnabled(true);
    if (adjust_to_minimum)
        setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    if (!items.isEmpty())
        this->addItems(items);
    if (!tip.isEmpty())
        this->setToolTip(tip);
}


EditableComboBox::EditableComboBox(QWidget* parent)
    : BaseComboBox (parent)
{
    this->font = QFont();
    selected_text = currentText();
    setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    connect(this, SIGNAL(editTextChanged(const QString &)), this, SLOT(editTextChanged_slot(const QString &)));

    this->tips.clear();
    this->tips[true] = "Press enter to validate this entry";
    this->tips[false]= "This entry is incorrect";
}

void EditableComboBox::show_tip(const QString &tip)
{
    QToolTip::showText(mapToGlobal(this->pos()), tip, this);
}

void EditableComboBox::selected()
{
    BaseComboBox::selected();
    selected_text = currentText();
}

void EditableComboBox::validate(const QString &qstr, bool editing)
{
    if (selected_text == qstr && qstr != "") {
        emit this->valid(true, true);
        return;
    }
    bool valid = is_valid(qstr);
    if (editing) {
        if (valid)
            emit this->valid(true, false);
        else
            emit this->valid(false, false);
    }
}


PathComboBox::PathComboBox(QWidget* parent,bool adjust_to_contents)
    : EditableComboBox (parent)
{
    IconLineEdit* lineedit = new IconLineEdit(this);
    if (adjust_to_contents)
        setSizeAdjustPolicy(QComboBox::AdjustToContents);
    else {
        setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    this->tips[true] = "Press enter to validate this path";
    this->tips[false] = "";
    setLineEdit(lineedit);

    connect(this,SIGNAL(highlighted(int)),this,SLOT(add_tooltip_to_highlighted_item(int)));
    connect(this,SIGNAL(sig_tab_pressed(bool)),this,SLOT(tab_complete()));
    connect(this,SIGNAL(sig_double_tab_pressed(bool)),this,SLOT(double_tab_complete()));
    connect(this,SIGNAL(valid(bool, bool)),lineedit,SLOT(update_status(bool, bool)));
}

void PathComboBox::focusInEvent(QFocusEvent *event)
{
    IconLineEdit* lineedit = qobject_cast<IconLineEdit*>(this->lineEdit());
    if (lineedit)
        lineedit->show_status_icon();
    QComboBox::focusInEvent(event);
}

void PathComboBox::focusOutEvent(QFocusEvent *event)
{
    if (! is_valid()) {
        QLineEdit* edit = this->lineEdit();
        QTimer::singleShot(50, [=](){edit->setText(this->selected_text);});
        // singleShot(int msec, Functor functor)
    }
    IconLineEdit* lineedit = qobject_cast<IconLineEdit*>(this->lineEdit());
    if (lineedit)
        lineedit->hide_status_icon();
    QComboBox::focusOutEvent(event);
}

QStringList PathComboBox::_complete_options()
{
    QString text = this->currentText();
    // dir.entryList不会进入子目录，glob.glob也不会进入子目录
    // dir.entryList返回的是相对路径，glob.glob返回绝对路径
    QDir dir(QDir::currentPath());
    QStringList opts;
    foreach (QString opt, dir.entryList(QStringList(text+"*"))) {
        QFileInfo info(opt);
        if (info.isDir())
            opts.append(opt);
    }
    this->setCompleter(new QCompleter(opts, this));
    return opts;
}

void PathComboBox::double_tab_complete()
{
    QStringList opts = this->_complete_options();
    if (opts.size() > 1)
        this->completer()->complete();
}

void PathComboBox::tab_complete()
{
    QStringList opts = this->_complete_options();
    if (opts.size() == 1) {
        this->set_current_text(opts[0] + os::sep);
        this->hide_completer();
    }
}

bool PathComboBox::is_valid(QString qstr) const
{
    if (qstr.isEmpty())
        qstr = currentText();
    QFileInfo info(qstr);
    return info.isDir();
}

void PathComboBox::selected()
{
    this->selected_text = this->currentText();
    emit this->valid(true, true);
    emit this->open_dir(this->selected_text);
}

void PathComboBox::add_current_text()
{
    QString text = this->currentText();
    QFileInfo info(text);
    if (info.isDir() && !text.isEmpty()) {
        // 尽管os::sep="\\", 但是在c++中就是chop(1)
        if (text.right(1) == os::sep)
            text.chop(1);
    }
    this->add_text(text);
}

void PathComboBox::add_tooltip_to_highlighted_item(int index)
{
    this->setItemData(index, this->itemText(index), Qt::ToolTipRole);
}


UrlComboBox::UrlComboBox(QWidget* parent,bool adjust_to_contents)
    : PathComboBox (parent, adjust_to_contents)
{
    disconnect(this, SIGNAL(editTextChanged(const QString &)), this, SLOT(editTextChanged_slot(const QString &)));
}

bool UrlComboBox::is_valid(QString qstr) const
{
    if (qstr.isEmpty())
        qstr = this->currentText();
    return QUrl(qstr).isValid();
}


FileComboBox::FileComboBox(QWidget* parent,bool adjust_to_contents,
                           bool default_line_edit)
    : PathComboBox (parent, adjust_to_contents)
{
    if (default_line_edit) {
        QLineEdit* line_edit = new QLineEdit(this);
        this->setLineEdit(line_edit);
    }

    if (adjust_to_contents)
        this->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    else {
        this->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
}

bool FileComboBox::is_valid(QString qstr) const
{
    if (qstr.isEmpty())
        qstr = this->currentText();
    QFileInfo info(qstr);
    return info.isFile();
}


static bool is_module_or_package(const QString& path)
{
    QFileInfo info(path);
    QString suffix = info.suffix();
    bool is_module = info.isFile() && (suffix == "py" || suffix == "pyw");
    QString file = path + "/__init__.py";
    QFileInfo fileinfo(file);
    bool is_package = info.isDir() && fileinfo.isFile();
    return is_module || is_package;
}


PythonModulesComboBox::PythonModulesComboBox(QWidget* parent,bool adjust_to_contents)
    : PathComboBox (parent, adjust_to_contents)
{}

bool PythonModulesComboBox::is_valid(QString qstr) const
{
    if (qstr.isEmpty())
        qstr = this->currentText();
    return is_module_or_package(qstr);
}

void PythonModulesComboBox::selected()
{
    EditableComboBox::selected();
    emit this->open_dir(this->currentText());
}
