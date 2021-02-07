#include "utils.h"

QList<QPair<QString, QStringList>> EDIT_FILETYPES =
{QPair<QString, QStringList>(QString("Python files"), QStringList({".py", ".pyw", ".ipy"})),
QPair<QString, QStringList>(QString("Cython/Pyrex files"), QStringList({".pyx", ".pxd", ".pxi"})),
QPair<QString, QStringList>(QString("C files"), QStringList({".c", ".h"})),
QPair<QString, QStringList>(QString("C++ files"), QStringList({".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"})),
QPair<QString, QStringList>(QString("OpenCL files"), QStringList({".cl"})),
QPair<QString, QStringList>(QString("Fortran files"), QStringList({".f", ".for", ".f77", ".f90", ".f95", ".f2k"})),
QPair<QString, QStringList>(QString("IDL files"), QStringList({".pro"})),
QPair<QString, QStringList>(QString("MATLAB files"), QStringList({".m"})),
QPair<QString, QStringList>(QString("Julia files"), QStringList({".jl"})),
QPair<QString, QStringList>(QString("Yaml files"), QStringList({".yaml", ".yml"})),
QPair<QString, QStringList>(QString("Patch and diff files"), QStringList({".patch", ".diff", ".rej"})),
QPair<QString, QStringList>(QString("Batch files"), QStringList({".bat", ".cmd"})),
QPair<QString, QStringList>(QString("Text files"), QStringList({".txt"})),
QPair<QString, QStringList>(QString("reStructuredText files"), QStringList({".txt", ".rst"})),
QPair<QString, QStringList>(QString("gettext files"), QStringList({".po", ".pot"})),
QPair<QString, QStringList>(QString("NSIS files"), QStringList({".nsi", ".nsh"})),
QPair<QString, QStringList>(QString("Web page files"), QStringList({".scss", ".css", ".htm", ".html"})),
QPair<QString, QStringList>(QString("XML files"), QStringList({".xml"})),
QPair<QString, QStringList>(QString("Javascript files"), QStringList({".js"})),
QPair<QString, QStringList>(QString("Json files"), QStringList({".json"})),
QPair<QString, QStringList>(QString("IPython notebooks"), QStringList({".ipynb"})),
QPair<QString, QStringList>(QString("Enaml files"), QStringList({".enaml"})),
QPair<QString, QStringList>(QString("Configuration files"), QStringList({".properties", ".session", ".ini", ".inf", ".reg", ".cfg", ".desktop"})),
QPair<QString, QStringList>(QString("Markdown files"), QStringList({".md"}))};

QString ALL_FILTER = QString("%1 (*)").arg("All files");

QString _create_filter(const QString& title,const QStringList& ftypes)
{
    return QString("%1 (*%2)").arg(title).arg(ftypes.join(" *"));
}

QString _get_filters(const QList<QPair<QString, QStringList>>& filetypes)
{
    QStringList filters;
    foreach (auto pair, filetypes) {
        QString title = pair.first;
        QStringList ftypes = pair.second;
        filters.append(_create_filter(title, ftypes));
    }
    filters.append(ALL_FILTER);
    return filters.join(";;");
}

QString get_filter(const QList<QPair<QString, QStringList>>& filetypes,const QString& ext)
{
    if (ext.isEmpty())
        return ALL_FILTER;
    foreach (auto pair, filetypes) {
        QString title = pair.first;
        QStringList ftypes = pair.second;
        if (ftypes.contains(ext))
            return _create_filter(title, ftypes);
    }
    return "";
}

QList<QPair<QString, QStringList>> get_edit_filetypes()
{
    QStringList favorite_exts;
    favorite_exts << ".py"<< ".R"<< ".jl"<< ".ipynb"<< ".md"<< ".pyw"<< ".pyx"
           << ".c"<< ".cpp"<< ".json"<< ".dat"<< ".csv"<< ".tsv"<< ".txt"
           << ".ini"<< ".html"<< ".js"<< ".h"<< ".bat";
    QPair<QString, QStringList> text_filetypes("Supported text files", favorite_exts);
    QList<QPair<QString, QStringList>> res;
    res.append(text_filetypes);
    res.append(EDIT_FILETYPES);
    return res;
}

QString get_edit_filters()
{
    QList<QPair<QString, QStringList>> edit_filetypes = get_edit_filetypes();
    return _get_filters(edit_filetypes);
}

bool is_gtk_desktop()
{
    return false;
}

// 该函数位于spyder/__init__.py
QHash<QString,QString> get_versions(bool reporev)
{
    QHash<QString,QString> versions;
    versions["spyder"] = "3.3.3";
    versions["python"] = "3.7.3";
    versions["bitness"] = "64";
    versions["qt"] = "5.9.6";
    versions["qt_api"] = "PyQt5";
    versions["qt_api_ver"] = "5.9.2";
    versions["system"] = "Windows";
    versions["release"] = "10";
    versions["revision"] = QString();
    return versions;
}
