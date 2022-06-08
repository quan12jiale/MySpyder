#include "textwrap.h"

namespace textwrap {

static QString _whitespace = "\t\n\x0b\x0c\r ";

const QString TextWrapper::word_punct = "[\\w!\"\'&.,?]";
const QString TextWrapper::letter = "[^\\d\\W]";
const QString TextWrapper::whitespace = QString("[%1]").arg(QRegularExpression::escape(_whitespace));
const QString TextWrapper::nowhitespace = "[^" + whitespace.mid(1);

static QString pattern = QString("(%1+|(?<=%2)-{2,}(?=\\w)|%3+?(?:-(?:(?<=%4{2}-)|(?<=%5-%6-))(?=%7-?%8)|(?=%9|\\Z)|(?<=%10)(?=-{2,}\\w)))")
        .arg(TextWrapper::whitespace)
        .arg(TextWrapper::word_punct)
        .arg(TextWrapper::nowhitespace)
        .arg(TextWrapper::letter)
        .arg(TextWrapper::letter)
        .arg(TextWrapper::letter)
        .arg(TextWrapper::letter)
        .arg(TextWrapper::letter)
        .arg(TextWrapper::whitespace)
        .arg(TextWrapper::word_punct);
const QRegularExpression TextWrapper::wordsep_re = QRegularExpression(pattern);
const QRegularExpression TextWrapper::wordsep_simple_re = QRegularExpression(QString("(%1+)").arg(TextWrapper::whitespace));
const QRegularExpression TextWrapper::sentence_end_re = QRegularExpression("[a-z][\\.\\!\?][\"\']?\\Z");

TextWrapper::TextWrapper(int width,
                   const QString& initial_indent,
                   const QString& subsequent_indent,
                   bool expand_tabs,
                   bool replace_whitespace,
                   bool fix_sentence_endings,
                   bool break_long_words,
                   bool drop_whitespace,
                   bool break_on_hyphens,
                   int tabsize,
                   int max_lines,
                   const QString& placeholder)
{
    this->width = width;
    this->initial_indent = initial_indent;
    this->subsequent_indent = subsequent_indent;
    this->expand_tabs = expand_tabs;
    this->replace_whitespace = replace_whitespace;
    this->fix_sentence_endings = fix_sentence_endings;
    this->break_long_words = break_long_words;
    this->drop_whitespace = drop_whitespace;
    this->break_on_hyphens = break_on_hyphens;
    this->tabsize = tabsize;
    this->max_lines = max_lines;
    this->placeholder = placeholder;
}

QString TextWrapper::_munge_whitespace(QString text)
{
    if (this->expand_tabs) {
        QString after(this->tabsize,' ');
        text.replace("\t",after);
    }
    if (this->replace_whitespace) {
        text.replace("\t"," ");
        text.replace("\n"," ");
        text.replace("\x0b"," ");
        text.replace("\x0c"," ");
        text.replace("\r"," ");
    }
    return text;
}

QStringList TextWrapper::_split(QString text)
{
    QStringList chunks;
    if (this->break_on_hyphens)
        chunks = text.split(wordsep_re);
    else
        chunks = text.split(wordsep_simple_re);
    QStringList res;
    foreach (const QString& str, chunks) {
        if (!str.isEmpty())
            res.append(str);
    }
    return res;
}

void TextWrapper::_fix_sentence_endings(QStringList &chunks)
{
    int i = 0;
    while (i < chunks.size()-1) {
        QRegularExpressionMatch match = sentence_end_re.match(chunks[i]);
        if (chunks[i+1] == " " && match.hasMatch()) {
            chunks[i+1] = "  ";
            i += 2;
        }
        else
            i++;
    }
}

void TextWrapper::_handle_long_word(QStringList &reversed_chunks,
                                    QStringList &cur_line, int cur_len, int width)
{
    int space_left;
    if (width < 1)
        space_left = 1;
    else
        space_left = width - cur_len;

    if (this->break_long_words) {
        cur_line.append(reversed_chunks.last().left(space_left));
        reversed_chunks.last() = reversed_chunks.last().mid(space_left);
    }
    else if (cur_line.empty()) {
        cur_line.append(reversed_chunks.takeLast());
    }
}

// 参考http://www.cplusplus.com/reference/algorithm/reverse/
void reverse(QStringList& list) {
    int first=0, last=list.size();
    while ((first!=last) && (first!=--last)) {
        list.swap(first,last);
        ++first;
    }
}
QStringList TextWrapper::_wrap_chunks(QStringList chunks)
{
    QStringList lines;
    if (this->width <= 0)
        throw QString("invalid width %1 (must be > 0)").arg(this->width);
    if (this->max_lines > 0) {
        QString indent;
        if (this->max_lines > 1)
            indent = this->subsequent_indent;
        else
            indent = this->initial_indent;
        if (indent.size() + lstrip(this->placeholder).size() > this->width)
            throw QString("placeholder too large for max width");
    }

    reverse(chunks);
    while (!chunks.empty()) {
        QStringList cur_line;
        int cur_len = 0;

        QString indent;
        if (!lines.empty())
            indent = this->subsequent_indent;
        else
            indent = this->initial_indent;
        int width = this->width - indent.size();

        if (this->drop_whitespace && chunks.last().trimmed()=="" && !lines.empty())
            chunks.pop_back();

        while (!chunks.empty()) {
            int l = chunks.last().size();
            if (cur_len + l <= width) {
                cur_line.append(chunks.takeLast());
                cur_len++;
            }
            else
                break;
        }

        if (!chunks.empty() && chunks.last().size() > width) {
            this->_handle_long_word(chunks,cur_line,cur_len,width);
            cur_len = 0;
            foreach (const QString& str, cur_line) {
                cur_len += str.size();
            }
        }

        if (this->drop_whitespace && !cur_line.empty() && cur_line.last().trimmed()=="") {
            cur_len -= cur_line.last().size();
            cur_line.pop_back();
        }

        if (!cur_line.empty()) {
            if (this->max_lines<=0 ||
                    lines.size() + 1 < this->max_lines ||
                    (chunks.empty() ||
                     this->drop_whitespace &&
                     chunks.size() == 1 &&
                     chunks[0].trimmed().isEmpty()) && cur_len <= width)
                lines.append(indent + cur_line.join(""));
            else {
                bool break_flag = true;
                // 源代码这里是while else语句
                while (!cur_line.empty()) {
                    if (!cur_line.last().trimmed().isEmpty() &&
                            cur_len + this->placeholder.size() <= width) {
                        cur_line.append(this->placeholder);
                        lines.append(indent + cur_line.join(""));
                        break_flag = false;
                        break;
                    }
                    cur_len -= cur_line.last().size();
                    cur_line.pop_back();
                }
                if (break_flag) {
                    if (!lines.empty()) {
                        QString prev_line = rstrip(lines.last());
                        if (prev_line.size() + this->placeholder.size() <=
                                this->width) {
                            lines.last() = prev_line + this->placeholder;
                            break;
                        }
                    }
                    lines.append(indent + lstrip(this->placeholder));
                }
                break;
            }
        }
    }
    return lines;
}

QStringList TextWrapper::_split_chunks(QString text)
{
    text = this->_munge_whitespace(text);
    return this->_split(text);
}

QStringList TextWrapper::wrap(QString text)
{
    QStringList chunks = this->_split_chunks(text);
    if (this->fix_sentence_endings)
        this->_fix_sentence_endings(chunks);
    return this->_wrap_chunks(chunks);
}

QString TextWrapper::fill(QString text)
{
    return this->wrap(text).join("\n");
}


// -- Convenience interface ---------------------------------------------
QStringList wrap(QString text,int width,
                 const QString& initial_indent,
                 const QString& subsequent_indent,
                 bool expand_tabs,
                 bool replace_whitespace,
                 bool fix_sentence_endings,
                 bool break_long_words,
                 bool drop_whitespace,
                 bool break_on_hyphens,
                 int tabsize,
                 int max_lines,
                 const QString& placeholder)
{
    TextWrapper w = TextWrapper(width,initial_indent,
                                subsequent_indent,
                                expand_tabs,
                                replace_whitespace,
                                fix_sentence_endings,
                                break_long_words,
                                drop_whitespace,
                                break_on_hyphens,
                                tabsize,
                                max_lines,
                                placeholder);
    return w.wrap(text);
}

QString fill(QString text,int width,
                 const QString& initial_indent,
                 const QString& subsequent_indent,
                 bool expand_tabs,
                 bool replace_whitespace,
                 bool fix_sentence_endings,
                 bool break_long_words,
                 bool drop_whitespace,
                 bool break_on_hyphens,
                 int tabsize,
                 int max_lines,
                 const QString& placeholder)
{
    TextWrapper w = TextWrapper(width,initial_indent,
                                subsequent_indent,
                                expand_tabs,
                                replace_whitespace,
                                fix_sentence_endings,
                                break_long_words,
                                drop_whitespace,
                                break_on_hyphens,
                                tabsize,
                                max_lines,
                                placeholder);
    return w.fill(text);
}

QString shorten(QString text,int width,
                 const QString& initial_indent,
                 const QString& subsequent_indent,
                 bool expand_tabs,
                 bool replace_whitespace,
                 bool fix_sentence_endings,
                 bool break_long_words,
                 bool drop_whitespace,
                 bool break_on_hyphens,
                 int tabsize,
                 const QString& placeholder)
{
    TextWrapper w = TextWrapper(width,initial_indent,
                                subsequent_indent,
                                expand_tabs,
                                replace_whitespace,
                                fix_sentence_endings,
                                break_long_words,
                                drop_whitespace,
                                break_on_hyphens,
                                tabsize,
                                1,
                                placeholder);
    return w.fill(text.trimmed().split(QRegExp("\\s+")).join(" "));
}

} // namespace textwrap
