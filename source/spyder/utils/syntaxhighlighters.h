#ifndef SYNTAXHIGHLIGHTERS_H
#define SYNTAXHIGHLIGHTERS_H

#include "str.h"
#include "keyword.h"
#include "builtins.h"

#include "utils/sourcecode.h"
#include "config/config_main.h"
#include <QSettings>
#include <QTextDocument>
#include <QApplication>
#include <QSyntaxHighlighter>
#include <QRegularExpression>


namespace sh {

extern QHash<QString, QString> COLOR_SCHEME_KEYS;

QHash<QString,ColorBoolBool> get_color_scheme();

class OutlineExplorerData;

class BaseSH : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    QRegularExpression PROG;
    QRegularExpression BLANKPROG;
    double BLANK_ALPHA_FACTOR;
public:
    BaseSH(QTextDocument *parent,const QFont& font,
           const QHash<QString,ColorBoolBool>& color_scheme);

    QColor get_background_color() const;
    QColor get_foreground_color() const;
    QColor get_currentline_color() const;
    QColor get_currentcell_color() const;
    QColor get_occurrence_color() const;
    QColor get_ctrlclick_color() const;
    QColor get_sideareas_color() const;
    QColor get_matched_p_color() const;
    QColor get_unmatched_p_color() const;
    QColor get_comment_color() const;

    QString get_color_name(const QString& fmt) const;
    virtual void setup_formats();
    virtual void setup_formats(const QFont& font);
    void set_color_scheme(const QHash<QString,ColorBoolBool>& color_scheme);
    void highlight_spaces(const QString& text,int offset=0);
    QHash<int,OutlineExplorerData> get_outlineexplorer_data() const;
protected:
    void highlightBlock(const QString &text) = 0;

public slots:
    void rehighlight();

public:
    QHash<int,OutlineExplorerData> outlineexplorer_data;
    bool found_cell_separators;
    QFont font;
    QHash<QString,ColorBoolBool> color_scheme;

    QString background_color;
    QString currentline_color;
    QString currentcell_color;
    QString occurrence_color;
    QString ctrlclick_color;
    QString sideareas_color;
    QString matched_p_color;
    QString unmatched_p_color;
    QHash<QString,QTextCharFormat> formats;
    QStringList cell_separators;
};


class TextSH : public BaseSH
{
    Q_OBJECT
public:
    TextSH(QTextDocument *parent,const QFont& font,
           const QHash<QString,ColorBoolBool>& color_scheme);
protected:
    void highlightBlock(const QString &text);
};


QString any(const QString& name,const QStringList& alternates);
QString make_python_patterns(const QStringList& additional_keywords=QStringList());


class OutlineExplorerData
{
public:
    enum {
        CLASS, FUNCTION, STATEMENT, COMMENT, CELL
    };
    static QString FUNCTION_TOKEN;
    static QString CLASS_TOKEN;
public:
    QString text;
    int fold_level;//类似行号，match.start()
    int def_type;//CLASS, FUNCTION, STATEMENT,等
    QString def_name;
    QTextCharFormat color; //出现在syntaxhighlighters.py474行
public:
    OutlineExplorerData();
    bool is_not_class_nor_function();
    bool is_class_nor_function();
    bool is_comment();
    QString get_class_name();
    QString get_function_name();
    QString get_token();
};


class PythonSH : public BaseSH
{
    Q_OBJECT
public:
    QStringList add_kw;
    QRegularExpression IDPROG;
    QRegularExpression ASPROG;
    enum {
        NORMAL, INSIDE_SQ3STRING, INSIDE_DQ3STRING,
             INSIDE_SQSTRING, INSIDE_DQSTRING
    };
    QHash<QString,int> DEF_TYPES;
    QRegularExpression OECOMMENT;

public:
    QHash<int,QString> import_statements;
    QList<int> match_index_list;
	QStringList strListCaptureGroups;

    PythonSH(QTextDocument *parent,const QFont& font,
           const QHash<QString,ColorBoolBool>& color_scheme);
    QStringList get_import_statements();

protected:
    void highlightBlock(const QString &text);

public slots:
    void rehighlight();
};


QString make_cpp_patterns();
class CppSH : public BaseSH
{
    Q_OBJECT
public:
    enum { NORMAL, INSIDE_COMMENT };
    QList<int> match_index_list;

public:
    CppSH(QTextDocument *parent,const QFont& font,
           const QHash<QString,ColorBoolBool>& color_scheme);

protected:
    void highlightBlock(const QString &text);
};


QString make_md_patterns();
class MarkdownSH : public BaseSH
{
    Q_OBJECT
public:
    enum { NORMAL, CODE };
    QList<int> match_index_list;
public:
    MarkdownSH(QTextDocument *parent,const QFont& font,
           const QHash<QString,ColorBoolBool>& color_scheme);
    void setup_formats();
    void setup_formats(const QFont& font);
    
protected:
    void highlightBlock(const QString &text);
};

} // namespace sh

#endif // SYNTAXHIGHLIGHTERS_H
