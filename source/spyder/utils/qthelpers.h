#pragma once

#include "os.h"
#include "config/gui.h"
#include "utils/icon_manager.h"
#include <QtWidgets>


QString getobj(QString txt, bool last=false);

QString file_uri(const QString& fname);

QStringList mimedata2url(const QMimeData* source,const QStringList& extlist=QStringList());

void toggle_actions(const QList<QAction*>& actions, bool enable);
QAction* create_action(QObject* parent, const QString& text, 
	const char* triggeredSlot = nullptr,
	const QString& shortcut = QString(),
	const QIcon& icon = QIcon(), const QString& tip = QString(),
	const char* toggledSlot = nullptr,
	const QString& data = QString(), QAction::MenuRole menurole = QAction::NoRole,
	Qt::ShortcutContext context = Qt::WindowShortcut);
void add_shortcut_to_tooltip(QAction* action, const QString& context, const QString& name);

void add_actions(QMenu* target,const QList<QAction*>& actions,QAction* insert_before=nullptr);
void add_actions(QMenu* target,const QList<QMenu*>& actions,QAction* insert_before=nullptr);
void add_actions(QMenu* target,const QList<QObject*>& actions,QAction* insert_before=nullptr);

void add_actions(QToolBar* target,const QList<QAction*>& actions,QAction* insert_before=nullptr);

void add_actions(QActionGroup* target,const QList<QAction*>& actions,QAction* insert_before=nullptr);
// plugins/editor.py第978行

QString get_item_user_text(QTreeWidgetItem* item);
void set_item_user_text(QTreeWidgetItem* item,const QString& text);

class DialogManager : public QObject
{
    Q_OBJECT
public:
    QHash<size_t, QDialog*> dialogs;
    DialogManager(QObject* parent);
    void show(QDialog* dialog);
    void dialog_finished(size_t dialog_id);
    void close_all();
};

QVBoxLayout* create_plugin_layout(QLayout* tools_layout,QWidget* main_widget=nullptr);
