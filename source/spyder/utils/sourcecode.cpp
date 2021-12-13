#include "sourcecode.h"
#include "utils/SyntaxHighlighterFactory.h"

namespace sourcecode {


//# Order is important:
static QList<QPair<QString, QString>> EOL_CHARS = {QPair<QString, QString>("\r\n", "nt"),
                                                   QPair<QString, QString>("\n", "posix"),
                                                   QPair<QString, QString>("\r", "mac"),};

const QHash<HighlighterEnumType,QStringList> ALL_LANGUAGES = {{PythonSHType,{"py", "pyw", "python", "ipy"}},
//                                            {"Cython",{"pyx", "pxi", "pxd"}},
//                                            {"Enaml",{"enaml"}},
//                                            {"Fortran77",{"f", "for", "f77"}},
//                                            {"Fortran",{"f90", "f95", "f2k"}},
//                                            {"Idl",{"pro"}},
//                                            {"Diff",{"diff", "patch", "rej"}},
//                                            {"GetText",{"po", "pot"}},
//                                            {"Nsis",{"nsi", "nsh"}},
//                                            {"Html",{"htm", "html"}},
                                           {CppSHType,{"c", "cc", "cpp", "cxx", "h", "hh", "hpp", "hxx"}},
//                                            {"OpenCL",{"cl"}},
//                                            {"Yaml",{"yaml", "yml"}},
                                           {MarkdownSHType,{"md", "mdw"}}
                                           };

const QHash<HighlighterEnumType,QStringList> CELL_LANGUAGES =
{{ PythonSHType, {"#%%", "# %%", "# <codecell>", "# In["}}};

QString get_eol_chars(const QString& text)
{
    foreach (auto pair, EOL_CHARS) {
        if (text.contains(pair.first))
            return pair.first;
    }
    return QString();
}

QString get_os_name_from_eol_chars(const QString& eol_chars)
{
    foreach (auto pair, EOL_CHARS) {
        QString chars = pair.first;
        QString os_name = pair.second;
        if (eol_chars == chars)
            return os_name;
    }
    return QString();
}

QString get_eol_chars_from_os_name(const QString& os_name)
{
    foreach (auto pair, EOL_CHARS) {
        QString eol_chars = pair.first;
        QString name = pair.second;
        if (name == os_name)
            return eol_chars;
    }
    return QString();
}

bool has_mixed_eol_chars(const QString& text)
{
    QString eol_chars = get_eol_chars(text);
    if (eol_chars.isEmpty())
        return false;
    QString tmp = text + eol_chars;
    QString correct_text = tmp.split(QRegExp("[\r\n]"),QString::SkipEmptyParts).join(eol_chars);
    return correct_text != text;
}

QString normalize_eols(QString text, const QString& eol)
{
    foreach (auto pair, EOL_CHARS) {
        QString eol_char = pair.first;
        if (eol_char != eol)
            text.replace(eol_char, eol);
    }
    return text;
}

QString fix_indentation(QString text)
{
    return text.replace("\t", QString(4,' '));
}

bool is_keyword(const QString& text)
{
    return keyword::kwlist.contains(text);
}

QString get_primary_at(const QString& source_code,int offset,bool retry)
{
    QString obj = "";
    QStringList left = source_code.left(offset).split(QRegularExpression("[^0-9a-zA-Z_.]"));
    if (!left.isEmpty() && !left.last().isEmpty())
        obj = left.last();
    QStringList right = source_code.mid(offset).split(QRegularExpression("\\W"));
    if (!right.isEmpty() && !right.first().isEmpty())
        obj += right.first();
    if (!obj.isEmpty() && obj[0].isDigit())
        obj = "";

    if (obj.isEmpty() && retry && offset &&
            (source_code[offset-1]=='(' ||
             source_code[offset-1]=='[' ||
             source_code[offset-1]=='.'))
        return get_primary_at(source_code, offset - 1, false);
    return obj;
}

} // namespace sourcecode
