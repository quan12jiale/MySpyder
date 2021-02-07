#pragma once

#include "plugins/plugins.h"
#include "utils/icon_manager.h"
#include "utils/syntaxhighlighters.h"
#include "utils/misc.h"
#include "widgets/colors.h"
#include "widgets/comboboxes.h"
#include "widgets/sourcecode/codeeditor.h"


class SpyderConfigPage;

class ConfigPage : public QWidget
{
    Q_OBJECT
signals:
    void apply_button_enabled(bool);
    void show_this_page();
public:
    QString CONF_SECTION;
    virtual void set_option(const QString& option,const QVariant& value);
    virtual QVariant get_option(const QString& option, const QVariant& _default=QVariant());

    bool is_modified;

    ConfigPage(QWidget* parent);
    virtual void initialize();
    virtual QString get_name() const = 0;
    virtual QIcon get_icon() const = 0;
    virtual void setup_page() = 0;
    virtual void set_modified(bool state);
    virtual bool is_valid() = 0;
    void apply_changes();
    virtual void load_from_conf() = 0;
    virtual void save_to_conf() = 0;

    virtual void _save_lang() = 0;
};


class ConfigDialog : public QDialog
{
    Q_OBJECT
signals:
    void check_settings();
    void size_change(const QSize&);
public:
    MainWindow* main;
    QStackedWidget* pages_widget;
    QListWidget* contents_widget;
    QPushButton* button_reset;
    QPushButton* apply_btn;

    ConfigDialog(MainWindow* parent = nullptr);
    int get_current_index() const;
    void set_current_index(int index);
    QWidget* get_page(int index=-1);
    void add_page(SpyderConfigPage* widget);
    void check_all_settings();
protected:
    void resizeEvent(QResizeEvent *event);
public slots:
    void accept();
    void button_clicked(QAbstractButton *button);
    void current_page_changed(int index);
};

typedef bool (*FUNC_PT)(QString);

struct SceditKey
{
    ColorLayout* clayout;
    QCheckBox* cb_bold;
    QCheckBox* cb_italic;
    SceditKey() = default;
    SceditKey(ColorLayout* _clayout, QCheckBox* _cb_bold, QCheckBox* _cb_italic)
    {
        clayout = _clayout;
        cb_bold = _cb_bold;
        cb_italic = _cb_italic;
    }
    friend uint qHash(const SceditKey &t, uint seed)
    { return qHash(t.clayout) ^ qHash(t.cb_bold) ^ qHash(t.cb_italic) ^ seed; }
    bool operator==(const SceditKey &other) const  //提供这两个函数后该类能作为QHash的key
    { return other.clayout==this->clayout && other.cb_bold==this->cb_bold && other.cb_italic==this->cb_italic; }
};

struct Scedit_Without_Layout
{
    QLabel* label;
    ColorLayout* clayout;
    QCheckBox* cb_bold;
    QCheckBox* cb_italic;
    Scedit_Without_Layout(QLabel* _label, ColorLayout* _clayout, QCheckBox* _cb_bold, QCheckBox* _cb_italic)
    {
        label = _label;
        clayout = _clayout;
        cb_bold = _cb_bold;
        cb_italic = _cb_italic;
    }
};

class SpyderConfigPage : public ConfigPage
{
    Q_OBJECT
public:
    QHash<QCheckBox*,QPair<QString,QVariant>> checkboxes;
    QHash<QRadioButton*,QPair<QString,QVariant>> radiobuttons;
    QHash<QLineEdit*,QPair<QString,QVariant>> lineedits;
    QHash<QLineEdit*,QPair<FUNC_PT,QString>> validate_data;
    QHash<QSpinBox*,QPair<QString,QVariant>> spinboxes;
    QHash<ColorLayout*,QPair<QString,QVariant>> coloredits;
    QHash<SceditKey,QPair<QString,QVariant>> scedits;
    QHash<QComboBox*,QPair<QString,QVariant>> comboboxes;
    QHash<QPair<QFontComboBox*,QSpinBox*>,QString> fontboxes;

    QSet<QString> changed_options;
    QHash<QString, QString> restart_options;
    QButtonGroup* default_button_group;

    SpyderConfigPage(QWidget* parent);
    virtual void apply_settings(const QStringList& options) = 0;
    virtual void set_modified(bool state);
    virtual bool is_valid();
    virtual void load_from_conf();
    virtual void save_to_conf();

    void show_message(QString msg_warning, QString msg_info,
                      bool msg_if_enabled, bool is_checked=false);
    void select_directory(QLineEdit* edit);
    void select_file(QLineEdit* edit, QString filters=QString());

    QCheckBox* create_checkbox(QString text, QString option, QVariant _default=QVariant(),
                               QString tip=QString(), QString msg_warning=QString(),
                               QString msg_info=QString(), bool msg_if_enabled=false);
    QRadioButton* create_radiobutton(QString text, QString option, QVariant _default=QVariant(),
                                     QString tip=QString(), QString msg_warning=QString(),
                                     QString msg_info=QString(), bool msg_if_enabled=false,
                                     QButtonGroup* button_group=nullptr, bool restart=false);
    QWidget* create_lineedit(QString text, QString option, QVariant _default=QVariant(),
                             QString tip=QString(), Qt::Orientation alignment=Qt::Vertical,
                             QString regex=QString(), bool restart=false);
    QWidget* create_browsedir(QString text, QString option, QVariant _default=QVariant(),
                              QString tip=QString());
    QWidget* create_browsefile(QString text, QString option, QVariant _default=QVariant(),
                               QString tip=QString(), QString filters=QString());
    QWidget* create_spinbox(QString prefix, QString suffix,
                            QString option, QVariant _default=QVariant(),
                            int min_=-1, int max_=-1, int step=-1, QString tip=QString());
    QPair<QLabel*,ColorLayout*> create_coloredit(QString text, QString option, QVariant _default=QVariant(),
                                                                QString tip=QString());
    QWidget* create_coloredit_with_layout(QString text, QString option, QVariant _default=QVariant(),
                                          QString tip=QString());

    Scedit_Without_Layout create_scedit(QString text, QString option, QVariant _default=QVariant(),
                                                       QString tip=QString());
    QWidget* create_scedit_with_layout(QString text, QString option, QVariant _default=QVariant(),
                                          QString tip=QString());
    QWidget* create_combobox(QString text, QList<QPair<QString,QString>> choices, QString option,
                             QVariant _default=QVariant(), QString tip=QString(), bool restart=false);
    QWidget* create_file_combobox(QString text, QStringList choices, QString option,
                                  QVariant _default=QVariant(), QString tip=QString(), bool restart=false,
                                  QString filters=QString(), bool adjust_to_contents=false,
                                  bool default_line_edit=false);
    QWidget* create_fontgroup(QString option, QString title=QString(),
                              QFontComboBox::FontFilters fontfilters=QFontComboBox::AllFonts);
    QPushButton* create_button(const QString& text);
    QWidget* create_tab(const QList<QWidget*>& widgets);

    virtual QFont get_font(const QString &option) const = 0;
    virtual void set_font(const QFont &font, const QString &option) = 0;
public slots:
    virtual void check_settings(){}
    void has_been_modified(const QString& option);
};


class PluginConfigPage : public SpyderConfigPage
{
    Q_OBJECT
public:
    SpyderPluginMixin* plugin;
    PluginConfigPage(SpyderPluginMixin* plugin, QWidget* parent);
    QVariant get_option(const QString& option, const QVariant& _default=QVariant())
    {return plugin->get_option(option, _default);}
    void set_option(const QString& option,const QVariant& value)
    {plugin->set_option(option, value);}
    virtual QFont get_font(const QString &option) const
    {return plugin->get_plugin_font();}
    QString get_name() const;
    QIcon get_icon() const;
};


class GeneralConfigPage : public SpyderConfigPage
{
    Q_OBJECT
public:
    QString NAME;
    QIcon ICON;
    MainWindow* main;
    GeneralConfigPage(QWidget* parent, MainWindow* main);
    QString get_name() const;
    QIcon get_icon() const;
    void prompt_restart_required();
    void restart();
};


class MainConfigPage : public GeneralConfigPage
{
    Q_OBJECT
public:
    MainConfigPage(QWidget* parent, MainWindow* main);

    void setup_page();
    virtual QFont get_font(const QString &option) const;
    virtual void set_font(const QFont &font, const QString &option);
    virtual void apply_settings(const QStringList& options);
    virtual void _save_lang();
};

/*
class SchemeEditor;
class ColorSchemeConfigPage : public GeneralConfigPage
{
    Q_OBJECT
public:
    QPushButton* delete_button;
    CodeEditor* preview_editor;
    QStackedWidget* stacked_widget;
    QPushButton* reset_button;
    SchemeEditor* scheme_editor_dialog;

    QHash<QString, QString> scheme_choices_dict;
    QComboBox* schemes_combobox;

    ColorSchemeConfigPage(QWidget* parent, MainWindow* main);
    void setup_page();
    virtual void apply_settings(const QStringList& options);
    QString current_scheme_name() const;
    QString current_scheme() const;
    int current_scheme_index() const;

    void update_combobox();
    void set_scheme(const QString& scheme_name);

    virtual void _save_lang(){}

public slots:
    void update_buttons();
    void update_preview(int index=-1);
    void create_new_scheme();
    void edit_scheme();
    void delete_scheme();
    void reset_to_default();
};


class SchemeEditor : public QDialog
{
    Q_OBJECT
public:
    ColorSchemeConfigPage* parent;
    QStackedWidget* stack;
    QStringList order;

    QHash<QString, QHash<QString, SceditKey>> widgets;
    QString last_used_scheme;

    QHash<QString, QLineEdit*> scheme_name_textbox;
public:
    SchemeEditor(ColorSchemeConfigPage* parent=nullptr, QStackedWidget* stack=nullptr);
    virtual void set_scheme(const QString& scheme_name);
    virtual QString get_scheme_name() const;
    virtual QHash<QString, ColorBoolBool> get_edited_color_scheme() const;
    virtual void add_color_scheme_stack(const QString& scheme_name, bool custom=false);

    virtual void delete_color_scheme_stack(const QString& scheme_name);
};
*/
