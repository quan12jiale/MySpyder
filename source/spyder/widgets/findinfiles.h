#pragma once
#include <atomic>
#include "fnmatch.h"
#include "os.h"
#include "config/config_main.h"
#include "utils/icon_manager.h"
#include "utils/encoding.h"
#include "utils/misc.h"
#include "widgets/comboboxes.h"
#include "widgets/onecolumntree.h"
#include "utils/qthelpers.h"
#include "config/gui.h"
#include "widgets/waitingspinner.h"

struct StruNotSave
{
    QString path;
    bool is_file;
    QString exclude;
    QList<QPair<QString,QString>> texts;
    bool text_re;
    bool case_sensitive;
    StruNotSave() = default;
    StruNotSave(QString _path,
                bool _is_file,
                QString _exclude,
                QList<QPair<QString,QString>> _texts,
                bool _text_re,
                bool _case_sensitive)
    {
        path = _path;
        is_file = _is_file;
        exclude = _exclude;
        texts = _texts;
        text_re = _text_re;
        case_sensitive = _case_sensitive;
    }
};



class SearchThread : public QThread
{
    Q_OBJECT
signals:
    void sig_finished(bool);
    void sig_current_file(const QString&);
    void sig_current_folder(const QString&);
    void sig_file_match(QString,int,int,int,QString,int);
    void sig_out_print(QString);//源码中是object，而且该项目中并没有出现过发送该信号的情况

private:
	//mutable QMutex mutex;// 1、互斥量
	//mutable QReadWriteLock lock;// 2、读写锁
    std::atomic_bool stopped;// 3、原子操作
    //results
    QStringList pathlist;
    int total_matches;
    QString error_flag;//既有bool还有字符串
    QString rootpath;
    QString exclude;
    QList<QPair<QString,QString>> texts;
    bool text_re;
    bool completed;
    bool case_sensitive;
    bool is_file;

    //QStringList filenames;//仅出现在111行一次，在该文件再没有使用过
public:
    SearchThread(QObject* parent);
    void initialize(const StruNotSave& stru);
    void stop();
	bool getStopped() const;
    bool find_files_in_path(const QString& path);
    bool find_string_in_file(const QString& fname);
    //SearchResults get_results();
protected:
    void run() override;
};


class SearchInComboBox : public QComboBox
{
    Q_OBJECT
public:
    QString path;
    QString project_path;
    QString file_path;
    QString external_path;

public:
    SearchInComboBox(const QStringList& external_path_history=QStringList(),QWidget* parent=nullptr);
    void add_external_path(const QString& path);
    QStringList get_external_paths() const;
    void clear_external_paths();
    QString get_current_searchpath() const;
    bool is_file_search() const;
    void set_project_path(QString path);
    bool eventFilter(QObject *widget, QEvent *event) override;
    void __redirect_stdio_emit(bool value);
public slots:
    void path_selection_changed();
    QString select_directory();
};


struct StruToSave
{
    QStringList search_text;
    bool text_re;
    QStringList exclude;
    int exclude_idx;
    bool exclude_re;
    bool more_options;
    bool case_sensitive;
    QStringList path_history;
    StruToSave(QStringList _search_text,
               bool _text_re,
               QStringList _exclude,
               int _exclude_idx,
               bool _exclude_re,
               bool _more_options,
               bool _case_sensitive,
               QStringList _path_history)
    {
        search_text = _search_text;
        text_re = _text_re;
        exclude = _exclude;
        exclude_idx = _exclude_idx;
        exclude_re = _exclude_re;
        more_options = _more_options;
        case_sensitive = _case_sensitive;
        path_history = _path_history;
    }
};


class FindOptions : public QWidget
{
    Q_OBJECT
public:
    static const QString REGEX_INVALID;
    static const QString REGEX_ERROR;
signals:
    void find();
    void stop();
public:
    QStringList supported_encodings;
    PatternComboBox* search_text;
    QToolButton* edit_regexp;
    QToolButton* case_button;
    QList<QLayout*> more_widgets;
    QToolButton* more_options;
    QToolButton* ok_button;
    QToolButton* stop_button;
    PatternComboBox* exclude_pattern;
    QToolButton* exclude_regexp;
    SearchInComboBox* path_selection_combo;

public:
    FindOptions(QWidget* parent,
                const QStringList& search_text,
                bool search_text_regexp,
                const QStringList& exclude,
                int exclude_idx,
                bool exclude_regexp,
                const QStringList& supported_encodings,
                bool more_options,
                bool case_sensitive,
                const QStringList& external_path_history);
    void set_search_text(const QString& text);
    StruNotSave get_options_not_save();//默认是not_save
    StruToSave get_options_to_save();
    QString path() const;
    void set_directory(const QString& directory);
    QString project_path() const;
    void set_project_path(const QString& path);
    void disable_project_search();
    QString file_path() const;
    void set_file_path(const QString& path);
protected:
    void keyPressEvent(QKeyEvent *event) override;
public slots:
    void toggle_more_options(bool state);
    void update_combos();
};


class LineMatchItem : public QTreeWidgetItem
{
public:
    int lineno;
    int colno;
    QString match;
    LineMatchItem(QTreeWidgetItem* parent, const QStringList &title);
    bool operator<(const LineMatchItem &other) const;
    bool operator>=(const LineMatchItem &other) const;
};


class FileMatchItem : public QTreeWidgetItem
{
public:
    QHash<QString,QString> sorting;
    QString filename;
    FileMatchItem(QTreeWidget *parent,const QString& filename,
                  const QHash<QString,QString>& sorting, const QStringList& title);
    bool operator<(const FileMatchItem &other) const;
    bool operator>=(const FileMatchItem &other) const;
};


class ItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ItemDelegate(QObject* parent);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

struct StrIntInt
{
    QString filename;
    int lineno;
    int colno;
    StrIntInt() {lineno = -1;}
    StrIntInt(QString _filename,int _lineno,int _colno)
    {
        filename = _filename;
        lineno = _lineno;
        colno = _colno;
    }
};


class ResultsBrowser : public OneColumnTree
{
    Q_OBJECT
public:
    QString search_text;
    //results
    int total_matches;
    QString error_flag;
    bool completed;
    QHash<QString,QString> sorting;
    QHash<size_t, StrIntInt> data;
    QHash<QString,FileMatchItem*> files;

    int num_files;
public:
    ResultsBrowser(QWidget* parent);
    void set_sorting(const QString& flag);
    void clear_title(const QString& search_text);
    QString truncate_result(const QString& line,int start,int end);
    QString html_escape(const QString& text);
public slots:
    void activated(QTreeWidgetItem *item) override;
    void sort_section(int idx);
    void clicked(QTreeWidgetItem *item) override;
    void append_result(QString filename,int lineno,int colno,
                       int match_end,QString line,int num_matches);
};


class FileProgressBar : public QWidget
{
    Q_OBJECT
public:
    QLabel* status_text;
    QWaitingSpinner* spinner;

    FileProgressBar(QWidget* parent);
    void set_label_path(const QString& path,bool folder=false);
    void reset();
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
};


class FindInFilesWidget : public QWidget
{
    Q_OBJECT
signals:
    void sig_finished();
public:
    SearchThread* search_thread;
    FileProgressBar* status_bar;
    FindOptions* find_options;
    ResultsBrowser* result_browser;

    FindInFilesWidget(QWidget* parent,
                      QString search_text="",
                      bool search_text_regexp=false,
                      QString exclude=EXCLUDE_PATTERNS[0],
                      int exclude_idx=-1,
                      bool exclude_regexp=false,
                      QStringList supported_encodings={"utf-8", "iso-8859-1", "cp1252"},
                      bool more_options=true,
                      bool case_sensitive=false,
                      QStringList external_path_history=QStringList());
    void set_search_text(const QString& text);
    void stop_and_reset_thread(bool ignore_results=false);
    void closing_widget();
public slots:
    void find();
    void search_complete(bool completed);
};
