#pragma once

#include "config/gui.h"
#include "utils/icon_manager.h"
#include "utils/qthelpers.h"

#include <QtWidgets>
class MainWindow;

class SpyderPluginMixin;
class TabFilter : public QObject
{
    Q_OBJECT
public:
    QTabBar* dock_tabbar;
    MainWindow* main;
    bool moving;
    int from_index;
    int to_index;

    TabFilter(QTabBar* dock_tabbar, MainWindow* main);
    SpyderPluginMixin* _get_plugin(int index) const;
    QList<SpyderPluginMixin*> _get_plugins() const;
    void _fix_cursor(int from_index, int to_index);
    bool eventFilter(QObject *obj, QEvent *event);
    void tab_pressed(QMouseEvent* event);
    void tab_moved(QMouseEvent* event);
    void tab_released(QMouseEvent* event);
    void move_tab(int from_index, int to_index);
    void show_tab_menu(QMouseEvent* event);
    void show_nontab_menu(QMouseEvent* event);
};


class SpyderDockWidget : public QDockWidget
{
    Q_OBJECT
signals:
    void plugin_closed();
public:
    QString title;
    MainWindow* main;
    QTabBar* dock_tabbar;

    SpyderDockWidget(const QString& title, MainWindow* parent);
protected:
    void closeEvent(QCloseEvent *event);
public slots:
    void install_tab_event_filter(bool value);
};


class SpyderPluginMixin
{
public:
    QString CONF_SECTION;
    //CONFIGWIDGET_CLASS
    int FONT_SIZE_DELTA;
    int RICH_FONT_SIZE_DELTA;
    QString IMG_PATH;
    Qt::DockWidgetAreas ALLOWED_AREAS;
    Qt::DockWidgetArea LOCATION;
    QDockWidget::DockWidgetFeatures FEATURES;
    bool DISABLE_ACTIONS_WHEN_HIDDEN;

public:
    QString PLUGIN_PATH;
    MainWindow* main;
    QMargins default_margins;
    QList<QAction*> plugin_actions;
    SpyderDockWidget* dockwidget;
    QMainWindow* mainwindow;
    bool ismaximized;
    bool isvisible;

    QString shortcut;
    QAction* toggle_view_action;

    virtual void initialize_plugin()=0;
    //仅在help.py和history.py有on_first_registration
    virtual void initialize_plugin_in_mainwindow_layout(){}
    virtual void update_margins()=0;
    virtual void __update_plugin_title();//
    virtual QPair<SpyderDockWidget*,Qt::DockWidgetArea> create_dockwidget()=0;//
    virtual QMainWindow* create_mainwindow()=0;//
    // void create_configwidget()该方法用到CONFIGWIDGET_CLASS
    virtual void apply_plugin_settings(const QStringList& options){}
    void register_shortcut(QObject* qaction_or_qshortcut, const QString& context,
                                   const QString& name, bool add_sc_to_tip=false);
    void register_widget_shortcuts(Widget_get_shortcut_data* widget);
    virtual void switch_to_plugin();
    virtual void visibility_changed(bool enable);
    virtual void plugin_closed();
    void set_option(const QString& option,const QVariant& value);
    QVariant get_option(const QString& option, const QVariant& _default=QVariant());
    QFont get_plugin_font(bool rich_text=false) const;
    QFont get_plugin_font2(const QString& rich_text) const;
    // void set_plugin_font
    virtual void update_font(){}
    virtual void __show_message(const QString& message, int timeout=0);
    virtual void starting_long_process(const QString& message);
    virtual void ending_long_process(const QString& message="");
    QHash<QString, ColorBoolBool> get_color_scheme() const;
    virtual void create_toggle_view_action()=0;
    void toggle_view(bool checked);

    virtual QString get_plugin_title() const=0;
    virtual QIcon get_plugin_icon() const;
    virtual QWidget* get_focus_widget() const=0;
    virtual bool closing_plugin(bool cancelable=false) = 0;
    virtual void refresh_plugin() = 0;
    virtual QList<QAction*> get_plugin_actions();
    virtual void register_plugin() = 0;
};


class SpyderPluginWidget : public QWidget, public SpyderPluginMixin
{
    Q_OBJECT
signals:
    void sig_option_changed(const QString&, const QVariant&);
    void show_message(const QString&, int);
    void update_plugin_title();
public:
    SpyderPluginWidget(MainWindow* parent);
    virtual void initialize_plugin();

    virtual void update_margins();

    virtual QPair<SpyderDockWidget*,Qt::DockWidgetArea> create_dockwidget();
    virtual QMainWindow* create_mainwindow();
    void create_toggle_view_action();

    virtual QWidget* get_focus_widget() const {return nullptr;}
    bool closing_plugin(bool cancelable=false) {return true;}

public:

    virtual QPair<bool,QString> check_compatibility() const;
    void show_compatibility_message(const QString& message);

public slots:
    void switch_to_plugin(){SpyderPluginMixin::switch_to_plugin();}
    void __update_plugin_title(){SpyderPluginMixin::__update_plugin_title();}
    void __show_message(const QString& message, int timeout=0){SpyderPluginMixin::__show_message(message, timeout);}

    void visibility_changed(bool enable){SpyderPluginMixin::visibility_changed(enable);}
    void plugin_closed(){SpyderPluginMixin::plugin_closed();}
    void set_option(const QString& option,const QVariant& value){SpyderPluginMixin::set_option(option, value);}

};
