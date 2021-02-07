#pragma once

#include "str.h"

#include <QRegularExpression>

namespace textwrap {

class TextWrapper
{
public:
    TextWrapper(int width=70,
             const QString& initial_indent="",
             const QString& subsequent_indent="",
             bool expand_tabs=true,
             bool replace_whitespace=true,
             bool fix_sentence_endings=false,
             bool break_long_words=true,
             bool drop_whitespace=true,
             bool break_on_hyphens=true,
             int tabsize=8,
             int max_lines=0,
             const QString& placeholder=" [...]");

    QString _munge_whitespace(QString text);
    QStringList _split(QString text);
    void _fix_sentence_endings(QStringList& chunks);
    void _handle_long_word(QStringList& reversed_chunks,
                           QStringList& cur_line, int cur_len, int width);
    QStringList _wrap_chunks(QStringList chunks);
    QStringList _split_chunks(QString text);
    QStringList wrap(QString text);
    QString fill(QString text);

    static QString word_punct;
    static QString letter;
    static QString whitespace;
    static QString nowhitespace;
    static QRegularExpression wordsep_re;
    static QRegularExpression wordsep_simple_re;
    static QRegularExpression sentence_end_re;
private:
    int width=70;
    QString initial_indent;
    QString subsequent_indent;
    bool expand_tabs;
    bool replace_whitespace;
    bool fix_sentence_endings;
    bool break_long_words;
    bool drop_whitespace;
    bool break_on_hyphens;
    int tabsize;
    int max_lines;
    QString placeholder;
};

QStringList wrap(QString text,int width=70,
                 const QString& initial_indent="",
                 const QString& subsequent_indent="",
                 bool expand_tabs=true,
                 bool replace_whitespace=true,
                 bool fix_sentence_endings=false,
                 bool break_long_words=true,
                 bool drop_whitespace=true,
                 bool break_on_hyphens=true,
                 int tabsize=8,
                 int max_lines=0,
                 const QString& placeholder=" [...]");
QString fill(QString text,int width=70,
                 const QString& initial_indent="",
                 const QString& subsequent_indent="",
                 bool expand_tabs=true,
                 bool replace_whitespace=true,
                 bool fix_sentence_endings=false,
                 bool break_long_words=true,
                 bool drop_whitespace=true,
                 bool break_on_hyphens=true,
                 int tabsize=8,
                 int max_lines=0,
                 const QString& placeholder=" [...]");
QString shorten(QString text,int width,
                 const QString& initial_indent="",
                 const QString& subsequent_indent="",
                 bool expand_tabs=true,
                 bool replace_whitespace=true,
                 bool fix_sentence_endings=false,
                 bool break_long_words=true,
                 bool drop_whitespace=true,
                 bool break_on_hyphens=true,
                 int tabsize=8,
                 const QString& placeholder=" [...]");

} // namespace textwrap
