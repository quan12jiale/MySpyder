#include "misc.h"

namespace misc {

void __remove_pyc_pyo(const QString& fname)
{
    QFileInfo info(fname);
    if (info.suffix() == "py") {
        QList<char> list = {'c', 'o'};
        foreach (char ending, list) {
            if (QFile::exists(fname+ending)) {
                QFile::remove(fname+ending);
            }
        }
    }
}

void rename_file(const QString& source,const QString& dest)
{
    __remove_pyc_pyo(source);
    QFile::rename(source, dest);
}

void remove_file(const QString& fname)
{
    __remove_pyc_pyo(fname);
    QFile::remove(fname);
}

void move_file(const QString& source,const QString& dest)
{
    QFile::copy(source, dest);
    remove_file(source);
}

QPair<int,int> get_filelines(const QString& path, const QStringList& extensions)
{
    //该函数返回的行数可能会比python版本的略高，
    //因为python版本去除了文件头尾的空格
    int dfiles=0, dlines=0;
    QFileInfo info(path);
    if (extensions.contains(info.suffix())) {
        dfiles = 1;
        QFile textfile(path);
        if (textfile.open(QIODevice::ReadOnly)) {
            while (!textfile.atEnd()) {
                textfile.readLine();
                dlines++;
            }
        }
    }
    return QPair<int,int>(dfiles, dlines);
}

void walk(const QString& absolute_path, int* files, int* lines,
          const QStringList& extensions, const QStringList& excluded_dirnames)
{
    QDir dir(absolute_path);

    foreach (QString relative_path, dir.entryList()) {
        if (relative_path == "." || relative_path == "..")
            continue;

        QString absolute_sub_path = absolute_path+'/'+relative_path;
        QFileInfo info(absolute_sub_path);
        if (info.isDir()) {
            if (excluded_dirnames.contains(relative_path))
                continue;
            walk(absolute_sub_path, files, lines, extensions, excluded_dirnames);
        }
        else if (info.isFile()) {
            if (encoding::is_text_file(absolute_sub_path)) {
                auto pair = get_filelines(absolute_sub_path, extensions);
                *files += pair.first;
                *lines += pair.second;
            }
        }
    }

}


QPair<int,int> count_lines(const QString& path, QStringList extensions,
                           QStringList excluded_dirnames)
{
    if (extensions.isEmpty())
        extensions << "py" << "pyw" << "ipy" << "enaml" << "c" << "h" << "cpp"
                   << "hpp" << "inc" << "hh" << "hxx" << "cc" << "cxx"
                   << "cl" << "f" << "for" << "f77" << "f90" << "f95" << "f2k";
    if (excluded_dirnames.isEmpty())
        excluded_dirnames << "build" << "dist" << ".hg" << ".svn";
    int files=0, lines=0;
    QFileInfo info(path);
    if (info.isDir()) {
        walk(path, &files, &lines, extensions, excluded_dirnames);
    }
    else {
        auto pair = get_filelines(path, extensions);
        files += pair.first;
        lines += pair.second;
    }
    return QPair<int,int>(files, lines);
}

bool get_error_match(const QString& text)
{
    QRegularExpression re("  File \"(.*)\", line (\\d*)");
    QRegularExpressionMatch match = re.match(text);
    if (match.capturedStart() == 0)
        return true;
    else
        return false;
}

QString PYTHON_EXECUTABLE = "D:/Anaconda3/python.exe";

QString get_python_executable()
{
    /*
     * 以后实现读取PATH环境变量，获得python安装路径
     * 可以参考programs.cpp中的is_program_installed()函数
    */
    return "D:/Anaconda3/python.exe";
}


QString commonprefix(const QStringList& m)
{
    if (m.isEmpty())
        return "";
    QString s1 = m[0];
    QString s2 = m[0];
    for (int i = 1; i < m.size(); ++i) {
        if (m[i].compare(s1) < 0)
            s1 = m[i];
        if (m[i].compare(s2) > 0)
            s2 = m[i];
    }
    for (int i = 0; i < s1.size(); ++i) {
        if (i>=s2.size() || s1[i] != s2[i]) {
            s1 = s1.left(i);
            break;
        }
    }

    int len = s1.size();
    while (len-- > 0) {
        if (QFileInfo::exists(s1))
            return s1;
        s1.chop(1);
    }
    return "";
}

QString abspardir(const QString& path)
{
    QString tmp = path + '/' + os::pardir;
    QFileInfo info(tmp);
    return info.absoluteFilePath();
}

QString get_common_path(const QStringList& pathlist)
{
    QString common = commonprefix(pathlist);
    if (common.size() > 1) {
        QFileInfo info(common);
        if (!info.isDir())
            return abspardir(common);
        else {
            bool break_flag = true;
            foreach (QString path, pathlist) {
                QString tmp = common + '/' + path.left(common.size()+1);
                QFileInfo fileinfo(tmp);
                if (!fileinfo.isDir()) {
                    break_flag = false;
                    return abspardir(common);
                }
            }
            if (break_flag) {
                QFileInfo fileinfo(common);
                return fileinfo.absoluteFilePath();
            }
        }
    }
    return QString();
}

QString getcwd_or_home()
{
    try {
        return QDir::currentPath();
        // 因此os.getcwd也不等价于QDir::currentPath(),
        // 不过暂时可以用QDir::currentPath()代替
    } catch (...) {
        qDebug() << "WARNING: Current working directory was deleted, "
                    "falling back to home dirertory";
        return get_home_dir();
    }
}

QString regexp_error_msg(const QString& pattern)
{
    QRegularExpression re(pattern);
    if (re.isValid())
        return QString();
    else
        return re.errorString();
}

} // namespace misc
