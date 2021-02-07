#pragma once

#include "plugins/plugins.h"
#include "widgets/explorer.h"


class Explorer : public ExplorerWidget, public SpyderPluginMixin
{
    Q_OBJECT
signals:
    void open_interpreter(const QString&);
    void edit(const QString&);
    void removed(const QString&);
    void removed_tree(const QString&);
    void renamed(const QString&, const QString&);
    void renamed_tree(const QString&, const QString&);
    void create_module(const QString&);
    void run(const QString&);
    //void open_dir(const QString&);
public:

    virtual void initialize_plugin();
    virtual void update_margins();

    virtual QPair<SpyderDockWidget*,Qt::DockWidgetArea> create_dockwidget();
    virtual QMainWindow* create_mainwindow();
    void create_toggle_view_action();

    virtual QString get_plugin_title() const;
    virtual QWidget* get_focus_widget() const;
    bool closing_plugin(bool cancelable=false) {return true;}
    virtual void refresh_plugin2(const QString& new_path=QString(), bool force_current=true);
    virtual void refresh_plugin(){}
    virtual void register_plugin();

public:
    Explorer(MainWindow* parent = nullptr);

public slots:
    void chdir(const QString& directory);

    void switch_to_plugin(){SpyderPluginMixin::switch_to_plugin();}
    void visibility_changed(bool enable){SpyderPluginMixin::visibility_changed(enable);}
    void plugin_closed(){SpyderPluginMixin::plugin_closed();}
    void set_option(const QString& option,const QVariant& value){SpyderPluginMixin::set_option(option, value);}
};

