#ifndef MAININTERPRETER_H
#define MAININTERPRETER_H

#include "plugins/configdialog.h"

class MainInterpreterConfigPage : public GeneralConfigPage
{
    Q_OBJECT
public:
    QRadioButton* cus_exec_radio;
    QLineEdit* pyexec_edit;
    QWidget* cus_exec_combo;

    QRadioButton* def_exec_radio;

    MainInterpreterConfigPage(QWidget* parent, MainWindow* main);

    virtual void setup_page();
    virtual void _save_lang(){}
    virtual QFont get_font(const QString &option) const{return QFont();}
    virtual void set_font(const QFont &font, const QString &option){}

    bool python_executable_changed(const QString& pyexec);
    bool warn_python_compatibility(const QString& pyexec);


    void set_custom_interpreters_list(const QString& value);
    void validate_custom_interpreters_list();
    virtual void apply_settings(const QStringList& options);
public slots:
    void set_umr_namelist();//
};

#endif // MAININTERPRETER_H
