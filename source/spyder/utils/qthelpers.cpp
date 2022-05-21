#include "qthelpers.h"

static QString SYMBOLS = "[^\'\"a-zA-Z0-9_.]";

QString getobj(QString txt, bool last)
{
    QString txt_end = "";
    QList<QPair<QChar,QChar>> list = {qMakePair(QChar('['), QChar(']')),
                                     qMakePair(QChar('('), QChar(')'))};
    foreach (auto pair, list) {
        QChar startchar = pair.first;
        QChar endchar = pair.second;
        if (txt.endsWith(endchar)) {
            int pos = txt.lastIndexOf(startchar);
            if (pos > -1) {
                txt_end = txt.mid(pos);
                txt = txt.left(pos);
            }
        }
    }
    QStringList tokens = txt.split(QRegularExpression(SYMBOLS));
    QString token = QString();
    try {
        QRegularExpression re(SYMBOLS);
        while (token.isEmpty() || (re.match(token).capturedStart() == 0)) {
            token = tokens.takeLast();
        }
        if (token.endsWith('.'))
            token.chop(1);
        if (token.startsWith('.'))
            return QString();
        if (last)
            token += txt[ txt.lastIndexOf(token) + token.size() ];
        token += txt_end;
        if (!token.isEmpty())
            return token;
    } catch (...) {
        return QString();
    }
    return QString();
}

QLabel* get_image_label(const QString& name, const QString& _default="not_found.png")
{
    QLabel* label = new QLabel;
    label->setPixmap(QPixmap(get_image_path(name, _default)));
    return label;
}

QString file_uri(const QString& fname)
{
    if (os::name == "nt") {
        QRegularExpression re("^[a-zA-Z]:");
        QRegularExpressionMatch match = re.match(fname);
        if (match.hasMatch())
            return "file:///" + fname;
        else
            return "file://" + fname;
    }
    else
        return "file://" + fname;
}

static QString _process_mime_path(QString path,const QStringList& extlist)
{
    if (path.startsWith("file://")) {
        if (os::name == "nt") {
            if (path.startsWith("file:///"))
                path = path.mid(8);
            else
                path = path.mid(5);
        }
        else
            path = path.mid(7);
    }
    path.replace("%5C", os::sep);
    QFileInfo info(path);
    if (info.exists()) {
        if (extlist.isEmpty() || extlist.contains('.'+info.suffix()))
            return path;
    }
    return QString();
}

QStringList mimedata2url(const QMimeData* source,const QStringList& extlist)
{
    QStringList pathlist;
    if (source->hasUrls()) {
        foreach (auto url, source->urls()) {
            QString path = _process_mime_path(url.toString(), extlist);
            if (!path.isEmpty())
                pathlist.append(path);
        }
    }
    else if (source->hasText()) {
        foreach (const QString& rawpath, source->text().split("\n")) {
            QString path = _process_mime_path(rawpath, extlist);
            if (!path.isEmpty())
                pathlist.append(path);
        }
    }
    return pathlist;
}

void toggle_actions(const QList<QAction*>& actions, bool enable)
{
    foreach (QAction* action, actions) {
        if (action != nullptr)
            action->setEnabled(enable);
    }
}

QAction* create_action(QObject* parent, const QString& text, 
	const char* triggeredSlot /*= nullptr*/,
	const QString& shortcut /*= QString()*/, 
	const QIcon& icon /*= QIcon()*/, const QString& tip /*= QString()*/, 
	const char* toggledSlot /*= nullptr*/,
	const QString& data /*= QString()*/, QAction::MenuRole menurole /*= QAction::NoRole*/, 
	Qt::ShortcutContext context /*= Qt::WindowShortcut*/)
{
	QAction* action = new QAction(text, parent);
	bool success = true;//该函数信号槽连接的receiver都是parent
	if (triggeredSlot)
	{
		success = QObject::connect(action, SIGNAL(triggered()), parent, triggeredSlot);
	}
	if (toggledSlot)
	{
		success = QObject::connect(action, SIGNAL(toggled(bool)), parent, toggledSlot);
		action->setCheckable(true);
	}
	Q_ASSERT(success);
	if (!icon.isNull())
	{
		action->setIcon(icon);
	}
	if (!tip.isEmpty())
	{
		action->setToolTip(tip);
		action->setStatusTip(tip);
	}
	if (!data.isEmpty())
	{
		action->setData(data);
	}
	if (menurole != QAction::NoRole)
	{
		action->setMenuRole(menurole);
	}
	if (!shortcut.isEmpty())
	{
		action->setShortcut(QKeySequence(shortcut));
	}
	action->setShortcutContext(context);
	return action;
}

void add_shortcut_to_tooltip(QAction* action, const QString& context, const QString& name)
{
    action->setToolTip(action->toolTip() + QString(" (%1)").arg(gui::get_shortcut(context, name)));
}

void add_actions(QMenu* target,const QList<QAction*>& actions,QAction* insert_before)
{
    QAction* previous_action = nullptr;
    QList<QAction*> target_actions = target->actions();
    if (!target_actions.empty()) {
        previous_action = target_actions.last();
        if (previous_action->isSeparator())
            previous_action = nullptr;
    }

    foreach (QAction* action, actions) {
        if ((action==nullptr) && (previous_action)) {
            if (insert_before == nullptr)
                target->addSeparator();
            else
                target->insertSeparator(insert_before);
        }
        else if (action != nullptr) {
            if (insert_before == nullptr) {
                // This is needed in order to ignore adding an action whose
                // wrapped C/C++ object has been deleted.
                try {
                    target->addAction(action);
                } catch (...) {
                    continue;
                }
            }
            else {
                target->insertAction(insert_before, action);
            }
        }
        previous_action = action;
    }
}

void add_actions(QMenu* target,const QList<QMenu*>& actions,QAction* insert_before)
{
    QAction* previous_action = nullptr;
    QList<QAction*> target_actions = target->actions();
    if (!target_actions.empty()) {
        previous_action = target_actions.last();
        if (previous_action->isSeparator())
            previous_action = nullptr;
    }
    void* previous = previous_action;
    foreach (QMenu* action, actions) {
        if (action==nullptr && previous) {
            if (insert_before == nullptr)
                target->addSeparator();
            else
                target->insertSeparator(insert_before);
        }
        else if (action != nullptr) {
            if (insert_before == nullptr)
                target->addMenu(action);
            else
                target->insertMenu(insert_before, action);
        }
        previous = action;
    }
}

void add_actions(QMenu* target,const QList<QObject*>& actions,QAction* insert_before)
{
    QAction* previous_action = nullptr;
    QList<QAction*> target_actions = target->actions();
    if (!target_actions.empty()) {
        previous_action = target_actions.last();
        if (previous_action->isSeparator())
            previous_action = nullptr;
    }
    void* previous = previous_action;
    foreach (QObject* object, actions) {
        QMenu* menu = qobject_cast<QMenu*>(object);
        QAction* action = qobject_cast<QAction*>(object);
        if (object == nullptr && previous) {
            if (insert_before == nullptr)
                target->addSeparator();
            else
                target->insertSeparator(insert_before);
        }
        else if (menu) {

            if (insert_before == nullptr)
                target->addMenu(menu);
            else
                target->insertSeparator(insert_before);
        }
        else if (action) {

            if (insert_before == nullptr) {
                try {
                    target->addAction(action);
                } catch (...) {
                    continue;
                }
            }
            else
                target->insertAction(insert_before, action);
        }
        previous = object;
    }
}

void add_actions(QToolBar* target,const QList<QAction*>& actions,QAction* insert_before)
{
    QAction* previous_action = nullptr;
    QList<QAction*> target_actions = target->actions();
    if (!target_actions.empty()) {
        previous_action = target_actions.last();
        if (previous_action->isSeparator())
            previous_action = nullptr;
    }

    foreach (QAction* action, actions) {
        if (action == nullptr && previous_action != nullptr) {
            if (insert_before == nullptr)
                target->addSeparator();
            else
                target->insertSeparator(insert_before);
        }
        else if (action != nullptr) {
            if (insert_before == nullptr) {
                try {
                    target->addAction(action);
                } catch (...) {
                    continue;
                }
            }
            else {
                target->insertAction(insert_before, action);
            }
        }
        previous_action = action;
    }
}

void add_actions(QActionGroup* target,const QList<QAction*>& actions,QAction* insert_before)
{
    QAction* previous_action = nullptr;
    QList<QAction*> target_actions = target->actions();
    if (!target_actions.empty()) {
        previous_action = target_actions.last();
        if (previous_action->isSeparator())
            previous_action = nullptr;
    }

    foreach (QAction* action, actions) {
        if (action != nullptr) {
            if (insert_before == nullptr) {
                try {
                    target->addAction(action);
                } catch (...) {
                    continue;
                }
            }
        }
    }
}

QString get_item_user_text(QTreeWidgetItem* item)
{
    QVariant data = item->data(0, Qt::UserRole);
    return data.toString();
}

void set_item_user_text(QTreeWidgetItem* item,const QString& text)
{
    item->setData(0, Qt::UserRole, text);
}

DialogManager::DialogManager()
    : QObject ()
{
    this->dialogs = QHash<size_t, QDialog*>();
}

void DialogManager::show(QDialog *dialog)
{
    bool break_flag = true;
    foreach (QDialog* dlg, this->dialogs.values()) {
        if (dlg->windowTitle() == dialog->windowTitle()) {
            dlg->show();
            dlg->raise();
            break_flag = false;
            break;
        }
    }

    if (break_flag) {
        dialog->show();
        size_t eid = reinterpret_cast<size_t>(dialog);
        this->dialogs[eid] = dialog;
        connect(dialog,&QDialog::accepted,[=](){this->dialog_finished(eid);});
        connect(dialog,&QDialog::rejected,[=](){this->dialog_finished(eid);});
    }
}

void DialogManager::dialog_finished(size_t dialog_id)
{
    this->dialogs.remove(dialog_id);
}

void DialogManager::close_all()
{
    foreach (QDialog* dlg, this->dialogs.values())
        dlg->reject();
}

QIcon get_filetype_icon(const QString& fname)
{
    QFileInfo info(fname);
    QString ext = info.suffix();
    return ima::get_icon(QString("%1.png").arg(ext),
                         ima::icon("FileIcon"));
}


static int calc_tools_spacing(QLayout* tools_layout)
{
    int tabbar_height = 21;
    int offset = 3;
    int tools_height = tools_layout->sizeHint().height();
    int spacing = tabbar_height - tools_height + offset;
    return qMax(spacing, 0);
}

QVBoxLayout* create_plugin_layout(QLayout* tools_layout,QWidget* main_widget)
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    int spacing = calc_tools_spacing(tools_layout);
    layout->setSpacing(spacing);

    layout->addLayout(tools_layout);
    if (main_widget != nullptr)
        layout->addWidget(main_widget);
    return layout;
}
