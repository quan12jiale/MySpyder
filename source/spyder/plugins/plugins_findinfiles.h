#pragma once
#include "utils/misc.h"
#include "utils/icon_manager.h"
#include "widgets/sourcecode/widgets_base.h"
#include "widgets/findinfiles.h"
#include "plugins/plugins.h"


class FindInFiles : public FindInFilesWidget, public SpyderPluginMixin
{
    Q_OBJECT
signals:
    void sig_option_changed(const QString&, const QVariant&);
    void toggle_visibility(bool);
    void edit_goto(const QString&, int, const QString&);
    void redirect_stdio(bool);
public:

    virtual void initialize_plugin();
    virtual void update_margins();

    virtual QPair<SpyderDockWidget*,Qt::DockWidgetArea> create_dockwidget();
    virtual QMainWindow* create_mainwindow();
    void create_toggle_view_action();

    virtual QString get_plugin_title() const;
    virtual QWidget* get_focus_widget() const;
    bool closing_plugin(bool cancelable=false);
    virtual void refresh_plugin(){}
    virtual void register_plugin();


public:
    FindInFiles(MainWindow* parent = nullptr);

    void set_project_path(const QString& path);

    void unset_project_path();
    void findinfiles_callback();
public slots:
    void refreshdir();
    void toggle(bool state);
    void set_current_opened_file(const QString& path);

    void switch_to_plugin();
    void visibility_changed(bool enable){SpyderPluginMixin::visibility_changed(enable);}
    void plugin_closed(){SpyderPluginMixin::plugin_closed();}
    void set_option(const QString& option,const QVariant& value){SpyderPluginMixin::set_option(option, value);}
};

