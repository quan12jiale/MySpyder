#ifndef RUNCONFIG_H
#define RUNCONFIG_H

#include "plugins/configdialog.h"
#include "utils/icon_manager.h"
#include "utils/misc.h"

extern QString ALWAYS_OPEN_FIRST_RUN_OPTION;

class RunConfiguration
{
public:
    QString args;//命令行参数
    bool args_enabled;//是否启用命令行
    bool current;//是否Execute in current console
    bool systerm;//是否Execute in an external system terminal
    bool interact;//是否Interact with the Python console after execution
    bool post_mortem;//是否Directly enter debugging when errors appear
    QString python_args;//External system terminal 命令行参数
    bool python_args_enabled;//是否启用命令行
    bool clear_namespace;//是否Remove all variables before execution
    bool file_dir;//工作目录是否为 被执行文件所在目录
    bool cw_dir;//工作目录是否为 当前工作目录
    bool fixed_dir;//是否自己选择目录
    QString dir;//选择该目录

    QString wdir;
    bool wdir_enabled;

public:
    RunConfiguration(const QString& fname = QString());
    void set(const QHash<QString,QVariant>& options);
    QHash<QString,QVariant> get() const;
    QString get_working_directory() const;
    QString get_arguments() const;
    QString get_python_arguments() const;
};

RunConfiguration* get_run_configuration(const QString& fname);

class RunConfigOptions : public QWidget
{
    Q_OBJECT
public:
    QString dir;
    RunConfiguration* runconf;

    QRadioButton* current_radio;
    QRadioButton* dedicated_radio;
    QRadioButton* systerm_radio;

    QCheckBox* clear_var_cb;
    QCheckBox* post_mortem_cb;
    QCheckBox* clo_cb;
    QLineEdit* clo_edit;

    QRadioButton* file_dir_radio;
    QRadioButton* cwd_radio;
    QRadioButton* fixed_dir_radio;

    QCheckBox* interact_cb;
    QCheckBox* pclo_cb;
    QLineEdit* pclo_edit;
    QCheckBox* firstrun_cb;

    QLineEdit* wd_edit;//215行

    RunConfigOptions(QWidget* parent = nullptr);

    void set(const QHash<QString,QVariant>& options);
    QHash<QString,QVariant> get() const;

    bool is_valid();

public slots:
    void select_directory();//
    void set_firstrun_o();//
};


class BaseRunConfigDialog : public QDialog
{
    Q_OBJECT
signals:
    void size_change(const QSize&);
public:
    BaseRunConfigDialog(QWidget* parent = nullptr);
    void add_widgets(const QList<QPair<int,QWidget*>>& widgets_or_spacings);
    void add_button_box(QDialogButtonBox::StandardButtons stdbtns);
    void resizeEvent(QResizeEvent *event);

    virtual void setup(const QString& fname) = 0;
public slots:
    virtual void run_btn_clicked(){}//
};


class RunConfigOneDialog : public BaseRunConfigDialog
{
    Q_OBJECT
public:
    QString filename;
    RunConfigOptions* runconfigoptions;

    RunConfigOneDialog(QWidget* parent = nullptr);
    virtual void setup(const QString& fname);
    RunConfiguration* get_configuration() const;

public slots:
    void accept();
};


class RunConfigDialog : public BaseRunConfigDialog
{
    Q_OBJECT
public:
    QString file_to_run;
    QComboBox* combo;
    QStackedWidget* stack;

    RunConfigDialog(QWidget* parent = nullptr);
    void setup(const QString& fname);

public slots:
    void run_btn_clicked();//
    void accept();
};

/*
class RunConfigPage : public GeneralConfigPage
{
    Q_OBJECT
public:
    QRadioButton* current_radio;
    QRadioButton* dedicated_radio;
    QRadioButton* systerm_radio;

    RunConfigPage(QWidget* parent, MainWindow* main);

    virtual void setup_page();
    virtual void _save_lang(){}
    virtual QFont get_font(const QString &option) const{}
    virtual void set_font(const QFont &font, const QString &option){}
    virtual void apply_settings(const QStringList& options){}
};
*/
#endif // RUNCONFIG_H
