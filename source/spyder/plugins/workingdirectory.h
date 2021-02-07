#ifndef WORKINGDIRECTORY_H
#define WORKINGDIRECTORY_H

#include "config/base.h"
#include "plugins/plugins.h"
#include "utils/misc.h"
#include "utils/icon_manager.h"
#include "widgets/comboboxes.h"

class WorkingDirectory : public QToolBar, public SpyderPluginMixin
{
    Q_OBJECT
signals:
    void sig_option_changed(const QString&, const QVariant&);
    void set_previous_enabled(bool);
    void set_next_enabled(bool);
    void redirect_stdio(bool);
    void set_explorer_cwd(const QString&);
    void refresh_findinfiles();
    void set_current_console_wd(const QString&);
private:
    int histindex;

public:
    QString LOG_PATH;

    QStringList history;
    QAction* previous_action;
    QAction* next_action;
    PathComboBox* pathedit;

    WorkingDirectory(MainWindow* parent, QString workdir=QString());

    //------ SpyderPluginWidget API ---------
    virtual QString get_plugin_title() const;
    virtual QIcon get_plugin_icon() const;
    virtual QList<QAction*> get_plugin_actions();
    virtual void register_plugin();
    virtual void refresh_plugin();
    virtual bool closing_plugin(bool cancelable=false){return true;}

    virtual void initialize_plugin();
    virtual void update_margins();

    virtual QPair<SpyderDockWidget*,Qt::DockWidgetArea> create_dockwidget();
    virtual QMainWindow* create_mainwindow();
    void create_toggle_view_action();
    virtual QWidget* get_focus_widget() const {return nullptr;}

    // ------ WorkingDirectory API ---------
    QStringList load_wdhistory(QString workdir=QString()) const;
    void save_wdhistory();

public slots:
    void select_directory();
    void previous_directory();
    void next_directory();
    void parent_directory();
    void chdir(QString directory, bool browsing_history=false,
               bool refresh_explorer=true, bool refresh_console=true);

    void visibility_changed(bool enable){SpyderPluginMixin::visibility_changed(enable);}
    void plugin_closed(){SpyderPluginMixin::plugin_closed();}
    void set_option(const QString& option,const QVariant& value){SpyderPluginMixin::set_option(option, value);}

};

#endif // WORKINGDIRECTORY_H
