#pragma once

#include "plugins/plugins.h"
#include "widgets/tabs.h"
#include "widgets/sourcecode/codeeditor.h"
#include "widgets/findreplace.h"

class HistoryLog : public SpyderPluginWidget
{
    Q_OBJECT
signals:
    void focus_changed();
public:
    Tabs* tabwidget;
    QList<QAction*> menu_actions;

    QAction* wrap_action;
    QList<CodeEditor*> editors;
    QStringList filenames;
    FindReplace* find_widget;

    HistoryLog(MainWindow* parent);
    virtual QString get_plugin_title() const;
    virtual QIcon get_plugin_icon() const;
    virtual QWidget* get_focus_widget() const;
    virtual bool closing_plugin(bool cancelable=false);

    virtual QList<QAction*> get_plugin_actions();
    virtual void on_first_registration();
    virtual void initialize_plugin_in_mainwindow_layout();
    virtual void register_plugin();
    virtual void update_font();
    virtual void apply_plugin_settings(const QStringList& options);

    void add_history(const QString& filename);
    void append_to_history(const QString& filename, const QString& command);
public slots:
    void switch_to_plugin(){SpyderPluginMixin::switch_to_plugin();}
    void refresh_plugin();
    void move_tab(int index_from, int index_to);
    void change_history_depth();
    void toggle_wrap_mode(bool checked);
};
