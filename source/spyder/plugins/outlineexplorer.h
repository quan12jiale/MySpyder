#pragma once

#include "plugins/plugins.h"
#include "widgets/editortools.h"


class OutlineExplorer : public OutlineExplorerWidget, public SpyderPluginMixin
{
    Q_OBJECT
signals:
    void sig_option_changed(const QString&, const QVariant&);
public:

    virtual void initialize_plugin();
    virtual void update_margins();

    virtual QPair<SpyderDockWidget*,Qt::DockWidgetArea> create_dockwidget();
    virtual QMainWindow* create_mainwindow();
    void create_toggle_view_action();

    virtual QString get_plugin_title() const;
    virtual QIcon get_plugin_icon() const;
    virtual QWidget* get_focus_widget() const;
    bool closing_plugin(bool cancelable=false);
    virtual void refresh_plugin(){}
    virtual void register_plugin();
public:
    OutlineExplorer(MainWindow* parent = nullptr);
    void save_config();
    void load_config();
public slots:
    void restore_scrollbar_position();

    void switch_to_plugin(){SpyderPluginMixin::switch_to_plugin();}
    void visibility_changed(bool enable);
    void plugin_closed(){SpyderPluginMixin::plugin_closed();}
    void set_option(const QString& option,const QVariant& value){SpyderPluginMixin::set_option(option, value);}
};

