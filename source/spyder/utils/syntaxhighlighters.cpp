#include "syntaxhighlighters.h"
#include "utils/SyntaxHighlighterFactory.h"

namespace sh {

QHash<QString, QString> COLOR_SCHEME_KEYS =
{{"background", "Background:"},
{"currentline", "Current line:"},
{"currentcell", "Current cell:"},
{"occurrence", "Occurrence:"},
{"ctrlclick", "Link:"},
{"sideareas", "Side areas:"},
{"matched_p", "Matched <br>parens:"},
{"unmatched_p", "Unmatched <br>parens:"},
{"normal", "Normal text:"},
{"keyword", "Keyword:"},
{"builtin", "Builtin:"},
{"definition", "Definition:"},
{"comment", "Comment:"},
{"string", "String:"},
{"number", "Number:"},
{"instance", "Instance:"}};

QHash<QString,ColorBoolBool> get_color_scheme()
{
    QHash<QString,ColorBoolBool> scheme;
    foreach (QString key, COLOR_SCHEME_KEYS.keys()) {
        scheme[key] = CONF_get("color_schemes", key).value<ColorBoolBool>();
    }
    return scheme;
}


BaseSH::BaseSH(QTextDocument *parent,const QFont& font,const QHash<QString,ColorBoolBool>& color_scheme)
    : QSyntaxHighlighter (parent)
{
    PROG = QRegularExpression();
    BLANKPROG = QRegularExpression("\\s+");
    BLANK_ALPHA_FACTOR = 0.31;

    outlineexplorer_data = QHash<int,OutlineExplorerData>();

    this->font = font;
    if (!color_scheme.isEmpty())
        this->color_scheme = color_scheme;
    else
        this->color_scheme = get_color_scheme();

    this->formats = QHash<QString,QTextCharFormat>();
    this->setup_formats(font);

    this->cell_separators = QStringList();
}

QColor BaseSH::get_background_color() const
{
    return QColor(background_color);
}

QColor BaseSH::get_foreground_color() const
{
    return formats["normal"].foreground().color();
}

QColor BaseSH::get_currentline_color() const
{
    return QColor(currentline_color);
}

QColor BaseSH::get_currentcell_color() const
{
    return QColor(currentcell_color);
}

QColor BaseSH::get_occurrence_color() const
{
    return QColor(occurrence_color);
}

QColor BaseSH::get_ctrlclick_color() const
{
    return QColor(ctrlclick_color);
}

QColor BaseSH::get_sideareas_color() const
{
    return QColor(sideareas_color);
}

QColor BaseSH::get_matched_p_color() const
{
    return QColor(matched_p_color);
}

QColor BaseSH::get_unmatched_p_color() const
{
    return QColor(unmatched_p_color);
}

QColor BaseSH::get_comment_color() const
{
    return formats["comment"].foreground().color();
}

QString BaseSH::get_color_name(const QString &fmt) const
{
    return formats[fmt].foreground().color().name();
}

void BaseSH::setup_formats()
{
    QTextCharFormat base_format;
    base_format.setFont(this->font);
    formats.clear();
    QHash<QString,ColorBoolBool> colors = this->color_scheme;

    this->background_color = colors.take("background").color;
    this->currentline_color = colors.take("currentline").color;
    this->currentcell_color = colors.take("currentcell").color;
    this->occurrence_color = colors.take("occurrence").color;
    this->ctrlclick_color = colors.take("ctrlclick").color;
    this->sideareas_color = colors.take("sideareas").color;
    this->matched_p_color = colors.take("matched_p").color;
    this->unmatched_p_color = colors.take("unmatched_p").color;

    for (auto it=colors.begin();it!=colors.end();it++) {
        QString name = it.key();
        QString color = it.value().color;
        bool bold = it.value().bold;
        bool italic = it.value().italic;

        QTextCharFormat format(base_format);
        format.setForeground(QColor(color));
        format.setBackground(QColor(background_color));
        if (bold)
            format.setFontWeight(QFont::Bold);
        format.setFontItalic(italic);
        this->formats[name] = format;
    }
}

void BaseSH::setup_formats(const QFont& font)
{
    this->font = font;
    this->setup_formats();
}


void BaseSH::set_color_scheme(const QHash<QString,ColorBoolBool> &color_scheme)
{
    if (!color_scheme.isEmpty())
        this->color_scheme = color_scheme;
    else
        this->color_scheme = get_color_scheme();
    setup_formats();
    rehighlight();
}

void BaseSH::highlight_spaces(const QString &text, int offset)
{
    QTextOption::Flags flags_text = this->document()->defaultTextOption().flags();
    QTextOption::Flags show_blanks = flags_text & QTextOption::ShowTabsAndSpaces;
    if (show_blanks) {
        QTextCharFormat format_leading = this->formats.value("leading",QTextCharFormat());
        QTextCharFormat format_trailing = this->formats.value("trailing",QTextCharFormat());
        QRegularExpressionMatch match = this->BLANKPROG.match(text,offset);
        while (match.hasMatch()) {
            int start = match.capturedStart(), end=match.capturedEnd();
            start = qMax(0, start+offset);
            end = qMax(0, end+offset);
            if (end==text.size() && !format_trailing.isEmpty())
                this->setFormat(start, end, format_trailing);
            if (start==0 && !format_leading.isEmpty())
                this->setFormat(start, end, format_leading);

            QTextCharFormat format = this->format(start);
            QColor color_foreground = format.foreground().color();
            double alpha_new = this->BLANK_ALPHA_FACTOR * color_foreground.alphaF();
            color_foreground.setAlphaF(alpha_new);
            this->setFormat(start, end-start, color_foreground);
            match = this->BLANKPROG.match(text, match.capturedEnd());
        }
    }
}

QHash<int,OutlineExplorerData> BaseSH::get_outlineexplorer_data() const
{
    return this->outlineexplorer_data;
}



void BaseSH::rehighlight()
{
    this->outlineexplorer_data.clear();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QSyntaxHighlighter::rehighlight();
    QApplication::restoreOverrideCursor();
}

SPYDER_REGISTER_HIGHLIGHTER(TextSHType, TextSH)
TextSH::TextSH(QTextDocument *parent,const QFont& font,const QHash<QString,ColorBoolBool>& color_scheme)
    : BaseSH (parent, font, color_scheme)
{}

void TextSH::highlightBlock(const QString &text)
{
    highlight_spaces(text);
}


//==============================================================================
// Python syntax highlighter
//==============================================================================
QString any(const QString& name,const QStringList& alternates)
{
    return QString("(?<%1>").arg(name) + alternates.join("|") + ")";
}

QString make_python_patterns(const QStringList& additional_keywords)
{
    QStringList kwlist = keyword::kwlist + additional_keywords;
    QStringList builtinlist;
    foreach (QString name, builtins::builtlist) {
        if (!name.startsWith('_'))
            builtinlist.append(name);
    }
    QSet<QString> repeated = QSet<QString>::fromList(kwlist) &
            QSet<QString>::fromList(builtinlist);
    foreach (QString repeated_element, repeated)
        kwlist.removeOne(repeated_element);

    QString kw = "\\b" + any("keyword", kwlist) + "\\b";
    QString builtin = "([^.\'\\\"\\\\#]\\b|^)" + any("builtin", builtinlist) + "\\b";
    QString comment = any("comment", QStringList({"#[^\\n]*"}));
    QString instance = any("instance", QStringList({"\\bself\\b",
                                                   "\\bcls\\b",
                                                   "^\\s*@([a-zA-Z_][a-zA-Z0-9_]*)(\\.[a-zA-Z_][a-zA-Z0-9_]*)*"}));

    QStringList number_regex;
    number_regex << "\\b[+-]?[0-9]+[lL]?\\b"
                 << "\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b"
                 << "\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b";
    QString number = any("number", number_regex);

    QString sqstring = "(\\b[rRuU])?'[^'\\\\\\n]*(\\\\.[^'\\\\\\n]*)*'?";
    QString dqstring = "(\\b[rRuU])?\"[^\"\\\\\\n]*(\\\\.[^\"\\\\\\n]*)*\"?";
    QString uf_sqstring = "(\\b[rRuU])?'[^'\\\\\\n]*(\\\\.[^'\\\\\\n]*)*(\\\\)$(?!')$";
    QString uf_dqstring = "(\\b[rRuU])?\"[^\"\\\\\\n]*(\\\\.[^\"\\\\\\n]*)*(\\\\)$(?!\")$";
    QString sq3string = "(\\b[rRuU])?'''[^'\\\\]*((\\\\.|'(?!''))[^'\\\\]*)*(''')?";
    QString dq3string = "(\\b[rRuU])?\"\"\"[^\"\\\\]*((\\\\.|\"(?!\"\"))[^\"\\\\]*)*(\"\"\")?";
    QString uf_sq3string = "(\\b[rRuU])?'''[^'\\\\]*((\\\\.|'(?!''))[^'\\\\]*)*(\\\\)?(?!''')$";
    QString uf_dq3string = "(\\b[rRuU])?\"\"\"[^\"\\\\]*((\\\\.|\"(?!\"\"))[^\"\\\\]*)*(\\\\)?(?!\"\"\")$";

    QString string = any("string", QStringList({sq3string, dq3string, sqstring, dqstring}));
    QString ufstring1 = any("uf_sqstring", QStringList(uf_sqstring));
    QString ufstring2 = any("uf_dqstring", QStringList(uf_dqstring));
    QString ufstring3 = any("uf_sq3string", QStringList(uf_sq3string));
    QString ufstring4 = any("uf_dq3string", QStringList(uf_dq3string));
    QString sync = any("SYNC", QStringList({"\\n"}));
    QStringList list = {instance, kw, builtin, comment,
                       ufstring1, ufstring2, ufstring3,
                        ufstring4, string, number, sync};
    return list.join("|");
}



const QString OutlineExplorerData::FUNCTION_TOKEN = "def";
const QString OutlineExplorerData::CLASS_TOKEN = "class";
OutlineExplorerData::OutlineExplorerData()
{
    this->text = QString();
    this->fold_level = -1;
    this->def_type = -1;
    this->def_name = QString();
}

bool OutlineExplorerData::is_not_class_nor_function()
{
    return this->def_type!=this->CLASS && this->def_type!=this->FUNCTION;
}

bool OutlineExplorerData::is_class_nor_function()
{
    return this->def_type==this->CLASS || this->def_type==this->FUNCTION;
}

bool OutlineExplorerData::is_comment()
{
    return this->def_type==this->COMMENT || this->def_type==this->CELL;
}

QString OutlineExplorerData::get_class_name()
{
    if (this->def_type == this->CLASS)
        return this->def_name;
    return QString();
}

QString OutlineExplorerData::get_function_name()
{
    if (this->def_type == this->FUNCTION)
        return this->def_name;
    return QString();
}

QString OutlineExplorerData::get_token()
{
    QString token;
    if (this->def_type == this->FUNCTION)
        token = this->FUNCTION_TOKEN;
    else if (this->def_type == this->CLASS)
        token = this->CLASS_TOKEN;
    return token;
}


/********** PythonSH **********/
SPYDER_REGISTER_HIGHLIGHTER(PythonSHType, PythonSH)
PythonSH::PythonSH(QTextDocument *parent,const QFont& font,const QHash<QString,ColorBoolBool>& color_scheme)
    : BaseSH (parent, font, color_scheme)
{
    add_kw = QStringList();
    add_kw << "async" << "await";
    PROG = QRegularExpression(make_python_patterns(add_kw),
                              QRegularExpression::DotMatchesEverythingOption);
    IDPROG = QRegularExpression("\\s+(\\w+)",
                                QRegularExpression::DotMatchesEverythingOption);
    ASPROG = QRegularExpression(".*?\\b(as)\\b");
    DEF_TYPES.clear();
    DEF_TYPES["def"] = OutlineExplorerData::FUNCTION;
    DEF_TYPES["class"] = OutlineExplorerData::CLASS;
	// Comments suitable for Outline Explorer
    OECOMMENT = QRegularExpression("^(# ?--[-]+|##[#]+ )[ -]*[^- ]+");

    import_statements = QHash<int,QString>();
    found_cell_separators = false;
    cell_separators = sourcecode::CELL_LANGUAGES[PythonSHType];

	int nCaptureCount = this->PROG.captureCount();
	strListCaptureGroups = this->PROG.namedCaptureGroups();
    for (int i = 1; i <= nCaptureCount; ++i) {
        if (!strListCaptureGroups[i].isEmpty())
            match_index_list.append(i);
    }
}

void PythonSH::highlightBlock(const QString &text)
{
    int prev_state = this->previousBlockState();
    int offset;
    QString text_ = text;
    switch (prev_state) {
    case INSIDE_DQ3STRING:
        offset = -4;
        text_ = "\"\"\" "+text;
        break;
    case INSIDE_SQ3STRING:
        offset = -4;
        text_ = "''' "+text;
        break;
    case INSIDE_DQSTRING:
        offset = -2;
        text_ = "\" "+text;
        break;
    case INSIDE_SQSTRING:
        offset = -2;
        text_ = "' "+text;
        break;
    default:
        offset = 0;
        prev_state = this->NORMAL;
        break;
    }

    OutlineExplorerData oedata;
    QString import_stmt;

    this->setFormat(0, text_.size(), formats["normal"]);

    int state = this->NORMAL;
    QRegularExpressionMatch match = this->PROG.match(text_);
    while (match.hasMatch()) {
        for (int i : match_index_list) {
            QString key = strListCaptureGroups[i];
            QString value = match.captured(i);

            if (!value.isEmpty()) {
                int start = match.capturedStart(i);
                int end = match.capturedEnd(i);
                start = qMax(0, start+offset);
                end = qMax(0, end+offset);

                if (key == "uf_sq3string") {
                    this->setFormat(start, end-start,
                                    this->formats["string"]);
                    state = INSIDE_SQ3STRING;
                }
                else if (key == "uf_dq3string") {
                    this->setFormat(start, end-start,
                                    this->formats["string"]);
                    state = INSIDE_DQ3STRING;
                }
                else if (key == "uf_sqstring") {
                    this->setFormat(start, end-start,
                                    this->formats["string"]);
                    state = INSIDE_SQSTRING;
                }
                else if (key == "uf_dqstring") {
                    this->setFormat(start, end-start,
                                    this->formats["string"]);
                    state = INSIDE_DQSTRING;
                }
                else {
                    this->setFormat(start, end-start, formats[key]);
                    if (key == "comment") {
                        if (startswith(lstrip(text_), cell_separators)) {
                            found_cell_separators = true;
                            oedata.text = text_.trimmed();
                            oedata.fold_level = start;
                            oedata.def_type = OutlineExplorerData::CELL;
                            oedata.def_name = text_.trimmed();
                        }
                        else {
							QRegularExpressionMatch match1 = OECOMMENT.match(lstrip(text_), 0,
								QRegularExpression::NormalMatch, QRegularExpression::AnchoredMatchOption);
							if (match1.hasMatch()) {
								oedata.text = text_.trimmed();
								oedata.fold_level = start;
								oedata.def_type = OutlineExplorerData::COMMENT;
								oedata.def_name = text_.trimmed();
							}
                        }
                    }
                    else if (key == "keyword") {
                        if (value == "def" || value == "class") {
                            QRegularExpressionMatch match1 = IDPROG.match(text_, end,
								QRegularExpression::NormalMatch, QRegularExpression::AnchoredMatchOption);
                            if (match1.hasMatch()) {
                                int start1 = match1.capturedStart(1);
                                int end1 = match1.capturedEnd(1);
                                setFormat(start1, end1-start1, formats["definition"]);
                                oedata.text = text_;
                                oedata.fold_level = start;
                                oedata.def_type = DEF_TYPES[value];
                                oedata.def_name = text_.mid(start1, end1-start1);
                                oedata.color = formats["definition"];
                            }
                        }
                        else if (value == "elif" || value == "else" ||
                                 value == "except" || value == "finally" ||
                                 value == "for" || value == "if" ||
                                 value == "try" || value == "while" ||
                                 value == "with") {
                            if (lstrip(text_).startsWith(value)) {
                                oedata.text = text_.trimmed();
                                oedata.fold_level = start;
                                oedata.def_type = OutlineExplorerData::STATEMENT;
                                oedata.def_name = text_.trimmed();
                            }
                        }
                        else if (value == "import") {
                            import_stmt = text_.trimmed();
							// color all the "as" words on same line, except if in a comment
                            int endpos;
                            if (text_.contains('#'))
                                endpos = text_.indexOf('#');
                            else
                                endpos = text_.size();
                            while (true) {
                                QRegularExpressionMatch match1 = ASPROG.match(text_.left(endpos), end,
									QRegularExpression::NormalMatch, QRegularExpression::AnchoredMatchOption);
                                if (!match1.hasMatch())
                                    break;
                                start = match1.capturedStart(1);
                                end = match1.capturedEnd(1);
                                setFormat(start, end-start, formats["keyword"]);
                            }
                        }
                    }
                }
            }
        }

        match = this->PROG.match(text_, match.capturedEnd());
    }

    setCurrentBlockState(state);

	// Use normal format for indentation and trailing spaces.
    formats["leading"] = formats["normal"];
    formats["trailing"] = formats["normal"];
    highlight_spaces(text_,offset);

    if (oedata.fold_level != -1) {
        int block_nb = currentBlock().blockNumber();
        outlineexplorer_data[block_nb] = oedata;
    }
    if (!import_stmt.isEmpty()) {
        int block_nb = currentBlock().blockNumber();
        import_statements[block_nb] = import_stmt;
    }
}

QStringList PythonSH::get_import_statements()
{
    return import_statements.values();
}

void PythonSH::rehighlight()
{
    import_statements.clear();
    found_cell_separators = false;
    BaseSH::rehighlight();
}


//==============================================================================
// C/C++ syntax highlighter
//==============================================================================
static QString C_TYPES = "bool char double enum float int long mutable short signed struct unsigned void";

static QString C_KEYWORDS1 = "and and_eq bitand bitor break case catch const const_cast continue default delete do dynamic_cast else explicit export extern for friend goto if inline namespace new not not_eq operator or or_eq private protected public register reinterpret_cast return sizeof static static_cast switch template throw try typedef typeid typename union using virtual while xor xor_eq";
static QString C_KEYWORDS2 = "a addindex addtogroup anchor arg attention author b brief bug c class code date def defgroup deprecated dontinclude e em endcode endhtmlonly ifdef endif endlatexonly endlink endverbatim enum example exception f$ file fn hideinitializer htmlinclude htmlonly if image include ingroup internal invariant interface latexonly li line link mainpage name namespace nosubgrouping note overload p page par param post pre ref relates remarks return retval sa section see showinitializer since skip skipline subsection test throw todo typedef union until var verbatim verbinclude version warning weakgroup";
static QString C_KEYWORDS3 = "asm auto class compl false true volatile wchar_t";

QString make_generic_c_patterns(const QString& keywords, const QString& builtins,
                                QString instance=QString(), QString define=QString(), QString comment=QString())
{
    QString kw = "\\b" + any("keyword", keywords.split(' ')) + "\\b";
    QString builtin = "\\b" + any("builtin", builtins.split(' ') + C_TYPES.split(' ')) + "\\b";
    if (comment.isEmpty())
        comment = any("comment", QStringList({"//[^\\n]*", "\\/\\*(.*?)\\*\\/"}));
    QString comment_start = any("comment_start", QStringList(QString("\\/\\*")));
    QString comment_end = any("comment_end", QStringList(QString("\\*\\/")));
    if (instance.isEmpty())
        instance = any("instance", QStringList(QString("\\bthis\\b")));

    QStringList tmp;
    tmp << "\\b[+-]?[0-9]+[lL]?\\b"
        << "\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b"
        << "\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b";
    QString number = any("number", tmp);
    QString sqstring = "(\\b[rRuU])?'[^'\\\\\\n]*(\\\\.[^'\\\\\\n]*)*'?";
    QString dqstring = "(\\b[rRuU])?\"[^\"\\\\\\n]*(\\\\.[^\"\\\\\\n]*)*\"?";

    QString string = any("string", QStringList({sqstring, dqstring}));
    if (define.isEmpty())
        define = any("define", QStringList(QString("#[^\\n]*")));

    QString sync = any("SYNC", QStringList(QString("\\n")));
    tmp.clear();
    tmp << instance << kw << comment << string << number
        << comment_start << comment_end << builtin
        << define << sync;
    return tmp.join("|");
}

QString make_cpp_patterns()
{
    return make_generic_c_patterns(C_KEYWORDS1+' '+C_KEYWORDS2, C_KEYWORDS3);
}

SPYDER_REGISTER_HIGHLIGHTER(CppSHType, CppSH)
CppSH::CppSH(QTextDocument *parent,const QFont& font,const QHash<QString,ColorBoolBool>& color_scheme)
    : BaseSH (parent, font, color_scheme)
{
    this->PROG = QRegularExpression(make_cpp_patterns(),
                              QRegularExpression::DotMatchesEverythingOption);
    for (int i = 1; i < this->PROG.namedCaptureGroups().size(); ++i) {
        if (!this->PROG.namedCaptureGroups()[i].isEmpty())
            match_index_list.append(i);
    }
}

void CppSH::highlightBlock(const QString &text)
{
    bool inside_comment = this->previousBlockState() == this->INSIDE_COMMENT;
    if (inside_comment)
        this->setFormat(0, text.size(), this->formats["comment"]);
    else
        this->setFormat(0, text.size(), this->formats["normal"]);

    QRegularExpressionMatch match = this->PROG.match(text);
    int index = 0;
    while (match.hasMatch()) {
        foreach (int i, match_index_list) {
            QString key = this->PROG.namedCaptureGroups()[i];
            QString value = match.captured(key);

            if (!value.isEmpty()) {
                int start = match.capturedStart(key);
                int end = match.capturedEnd(key);
                index += end-start;
				Q_UNUSED(index);
                if (key == "comment_start") {
                    inside_comment = true;
                    this->setFormat(start, text.size()-start,
                                    this->formats["comment"]);
                }
                else if (key == "comment_end") {
                    inside_comment = false;
                    this->setFormat(start, end-start,
                                    this->formats["comment"]);
                }
                else if (inside_comment)
                    this->setFormat(start, end-start,
                                    this->formats["comment"]);
                else if (key == "define")
                    this->setFormat(start, end-start,
                                    this->formats["number"]);
                else
                    this->setFormat(start, end-start,
                                    this->formats[key]);
            }
        }
        match = PROG.match(text, match.capturedEnd());
    }

    this->highlight_spaces(text);
    int last_state;
    if (inside_comment)
        last_state = this->INSIDE_COMMENT;
    else
        last_state = this->NORMAL;
    this->setCurrentBlockState(last_state);
}


/********** MarkdownSH **********/
QString make_md_patterns()
{
    QString h1 = "^#[^#]+";
    QString h2 = "^##[^#]+";
    QString h3 = "^###[^#]+";
    QString h4 = "^####[^#]+";
    QString h5 = "^#####[^#]+";
    QString h6 = "^######[^#]+";
    QStringList tmp;
    tmp << h1 << h2 << h3 << h4 << h5 << h6;

    QString titles = any("title", tmp);

    tmp = QStringList({"<", "[\\?/]?>", "(?<=<).*?(?=[ >])"});
    QString html_tags = any("builtin", tmp);
    QString html_symbols = "&[^; ].+;";
    QString html_comment = "<!--.+-->";

    QString strikethrough = any("strikethrough", QStringList("(~~)(.*?)~~"));
    QString strong = any("strong", QStringList("(\\*\\*)(.*?)\\*\\*"));

    QString _italic = "(__)(.*?)__";
    QString emphasis = "(//)(.*?)//";
    tmp.clear();
    tmp << _italic << emphasis;
    QString italic = any("italic", tmp);

    QString link_html = "(?<=(\\]\\())[^\\(\\)]*(?=\\))|(<https?://[^>]+>)|(<[^ >]+@[^ >]+>)";
    QString link = "!?\\[[^\\[\\]]*\\]";
    tmp.clear();
    tmp << link_html << link;
    QString links = any("link", tmp);

    QString blockquotes = "(^>+.*)|(^(?:    |\\t)*[0-9]+\\. )|(^(?:    |\\t)*- )|(^(?:    |\\t)*\\* )";

    QString code = any("code", QStringList("^`{3,}.*$"));
    QString inline_code = any("inline_code", QStringList("`[^`]*`"));

    tmp.clear();
    tmp.append("^(?:\\${2}).*$");
    tmp.append(html_symbols);
    QString math = any("number", tmp);

    tmp.clear();
    tmp << blockquotes << html_comment;
    QString comment = any("comment", tmp);

    tmp.clear();
    tmp << titles << comment << html_tags << math << links << italic
        << strong << strikethrough << code << inline_code;
    return tmp.join('|');
}

SPYDER_REGISTER_HIGHLIGHTER(MarkdownSHType, MarkdownSH)
MarkdownSH::MarkdownSH(QTextDocument *parent,const QFont& font,const QHash<QString,ColorBoolBool>& color_scheme)
    : BaseSH (parent, font, color_scheme)
{
    this->PROG = QRegularExpression(make_md_patterns(),
                              QRegularExpression::DotMatchesEverythingOption);
    for (int i = 1; i < this->PROG.namedCaptureGroups().size(); ++i) {
        if (!this->PROG.namedCaptureGroups()[i].isEmpty())
            match_index_list.append(i);
    }
}

void MarkdownSH::highlightBlock(const QString &text)
{
    int previous_state = this->previousBlockState();

    if (previous_state == this->CODE)
        this->setFormat(0, text.size(), this->formats["code"]);
    else {
        previous_state = this->NORMAL;
        this->setFormat(0, text.size(), this->formats["normal"]);
    }

    this->setCurrentBlockState(previous_state);

    QRegularExpressionMatch match = this->PROG.match(text);
    int match_count = 0;
    int n_characters = text.size();

    while (match.hasMatch() && match_count< n_characters) {
        foreach (int i, match_index_list) {
            QString key = this->PROG.namedCaptureGroups()[i];
            QString value = match.captured(key);

            int start = match.capturedStart(key);
            int end = match.capturedEnd(key);
            if (!value.isEmpty()) {
                previous_state = this->previousBlockState();

                if (previous_state == this->CODE) {
                    if (key == "code") {
                        this->setFormat(0, text.size(), this->formats["normal"]);
                        this->setCurrentBlockState(this->NORMAL);
                    }
                    else
                        continue;
                }
                else {
                    if (key == "code") {
                        this->setFormat(0, text.size(), this->formats["code"]);
                        this->setCurrentBlockState(this->CODE);
                        continue;
                    }
                }
                this->setFormat(start, end-start, this->formats[key]);
            }
        }

        match = PROG.match(text, match.capturedEnd());
        match_count++;
    }
    this->highlight_spaces(text);
}


void MarkdownSH::setup_formats()
{
    BaseSH::setup_formats();

    QTextCharFormat font = QTextCharFormat(this->formats["normal"]);
    font.setFontItalic(true);
    this->formats["italic"] = font;

    this->formats["strong"] = this->formats["definition"];

    font = QTextCharFormat(this->formats["normal"]);
    font.setFontStrikeOut(true);
    this->formats["strikethrough"] = font;

    font = QTextCharFormat(this->formats["string"]);
    font.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    this->formats["link"] = font;

    this->formats["code"] = this->formats["sting"];
    this->formats["inline_code"] = this->formats["sting"];

    font = QTextCharFormat(this->formats["keyword"]);
    font.setFontWeight(QFont::Bold);
    this->formats["title"] = font;
}

void MarkdownSH::setup_formats(const QFont& font)
{
    this->font = font;
    this->setup_formats();
}

} // namespace sh
