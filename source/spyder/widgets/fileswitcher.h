#include "utils/icon_manager.h"
#include "widgets/helperwidgets.h"

#include "os.h"
#include "utils/syntaxhighlighters.h"
#include "utils/icon_manager.h"
#include "helperwidgets.h"

class FileInfo;
class EditorStack;

class KeyPressFilter : public QObject
{
    Q_OBJECT
signals:
    void sig_up_key_pressed();
    void sig_down_key_pressed();

public:
    KeyPressFilter(QObject* parent = nullptr);
    bool eventFilter(QObject *src, QEvent *e) override;
};


class FilesFilterLine : public QLineEdit
{
public:
    bool clicked_outside;//源码中该变量是static静态变量，总感觉不妥
    FilesFilterLine(QWidget* parent = nullptr);
protected:
    void focusOutEvent(QFocusEvent *event) override;
};


class FileSwitcher : public QDialog
{
    Q_OBJECT
signals:
    void sig_goto_file(int, EditorStack*);
public:
    static const int FILE_MODE;
    static const int SYMBOL_MODE;
    static const int MAX_WIDTH;
public:
    QList<QPair<QTabWidget*, EditorStack*>> plugins_tabs;
    QList<QPair<QList<FileInfo*>, QIcon>> plugins_data;
    QList<EditorStack*> plugins_instances;
    EditorStack* plugin;
    int mode;
    QHash<QString, QTextCursor> initial_cursors;
    QString initial_path;
    QWidget* initial_widget;
    int line_number;
    bool is_visible;

    FilesFilterLine* edit;
    HelperToolButton* help;
    QListWidget* list;
    KeyPressFilter* filter;

    QStringList filtered_path;//首次出现在源码606行
    QList<int> filtered_symbol_lines;//首次出现在源码626行

public:
    FileSwitcher(QWidget* parent, EditorStack* plugin, QTabWidget* tabs,
                 const QList<FileInfo*>& data, const QIcon& icon);
    QList<QPair<QWidget*, EditorStack*>> widgets();
    QList<int> line_count();
    QList<bool> save_status();
    QStringList paths();
    QStringList filenames();
    QList<QIcon> icons();

    QString current_path();
    QHash<QWidget*, QString> paths_by_widget();
    QHash<QString, QWidget*> widgets_by_path();
    QString filter_text() const;
    void set_search_text(const QString& _str);
    void save_initial_state();

    void set_dialog_position();
    QSize get_item_size(const QStringList& content);
    void fix_size(const QStringList& content);

    int count();
    int current_row();
    void set_current_row(int row);
    void select_row(int steps);
    int get_stack_index(int stack_index,int plugin_index);

    QList<FileInfo*> get_plugin_data(EditorStack* plugin);
    QTabWidget* get_plugin_tabwidget(EditorStack* plugin);
    QWidget* get_widget(int index=-1,const QString& path=QString(),QTabWidget* tabs=nullptr);
    void set_editor_cursor(QWidget* editor,QTextCursor cursor);
    void goto_line(int line_number);
    QHash<int,sh::OutlineExplorerData> get_symbol_list();
    void setup_file_list(QString filter_text,const QString& current_path);
    void setup_symbol_list(QString filter_text,const QString& current_path);
    void add_plugin(EditorStack* plugin,QTabWidget* tabs,const QList<FileInfo*>& data,const QIcon& icon);
public slots:
    void accept() override;
    void restore_initial_state();
    void previous_row();
    void next_row();
    void item_selection_changed();
    void setup();
    void show();
};
