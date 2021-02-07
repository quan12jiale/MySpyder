#include "programs.h"
#include <QDebug>

namespace programs {

QString is_program_installed(const QString& basename)
{
    QFileInfo info(basename);
    if (info.isFile())
        return basename;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    QString PATH = environment.value("PATH");
    foreach (QString path, PATH.split(os::pathsep)) { //pathsep = ";";
        QString abspath = path + '/' + basename;
        QFileInfo info(abspath);
        if (info.isFile())
            return abspath;
    }
    return QString();
}

QString find_program(const QString& basename)
{
    QStringList names(basename);
    if (os::name == "nt") {
        QStringList extensions({".exe", ".bat", ".cmd"});
        if (!endswith(basename, extensions)) {
            names.clear();
            foreach (QString ext, extensions)
                names.append(basename + ext);
            names.append(basename);
        }
    }
    foreach (QString name, names) {
        QString path = is_program_installed(name);
        if (!path.isEmpty())
            return path;
    }
    return QString();
}

//alter_subprocess_kwargs_by_platform(**kwargs)
//run_shell_command(cmdstr, **subprocess_kwargs)

QString run_program(const QString& program, const QStringList& args=QStringList())
{
    QString path = find_program(program);
    if (path.isEmpty()) {
        QMessageBox::warning(nullptr, QString(),
                             QString("Program %1 was not found").arg(program));
        return QString();
    }
    QStringList fullcmd;
    fullcmd << path;
    fullcmd.append(args);
    QProcess process;
    process.start(fullcmd.join(' '));
    return process.readAllStandardOutput();
    //源码中proc = programs.run_program()
    //proc.communicate() returns a tuple (stdout, stderr).
}

bool start_file(const QString& filename)
{
    QUrl url;
    url.setUrl(filename);
    return QDesktopServices::openUrl(url);
}

//python_script_exists(package=None, module=None)
//run_python_script(package=None, module=None, args=[], p_args=[])
QStringList shell_split(const QString& text)
{
    QString pattern = "(\\s+|(?<!\\\\)\".*?(?<!\\\\)\"|(?<!\\\\)\\\'.*?(?<!\\\\)\\\')";
    QRegularExpression re(pattern);
    QStringList out;
    foreach (QString token, text.split(re)) {
        if (!token.isEmpty()) {
            QString tmp = token;
            tmp = lstrip(tmp, "\"");
            tmp = rstrip(tmp, "\"");
            tmp = lstrip(tmp, "'");
            tmp = rstrip(tmp, "'");
            out.append(tmp);
        }
    }
    qDebug() << __FILE__ << __func__ << out;
    return out;
}

QStringList get_python_args(QString fname, QString python_args,
                            bool interact, bool debug, QString end_args)
{
    QStringList p_args;
    if (!python_args.isEmpty())
        p_args.append(python_args.split(QRegExp("\\s+")));
    if (interact)
        p_args.append("-i");
    if (debug)
        p_args << "-m" << "pdb";
    if (!fname.isEmpty()) {
        if (os::name == "nt" && debug) {
            QFileInfo info(fname);
            fname = info.canonicalFilePath();
            fname.replace(os::sep, "/");
            p_args.append(fname);
        }
        else
            p_args.append(fname);
    }
    if (!end_args.isEmpty())
        p_args.append(shell_split(end_args));
    return p_args;
}

void run_python_script_in_terminal(QString fname, QString wdir, QString args, bool interact,
                                   bool debug, QString python_args, QString executable)
{
    if (executable.isEmpty())
        executable = misc::get_python_executable();

    //If fname or python_exe contains spaces, it can't be ran on Windows

    QStringList p_args(executable);
    QFileInfo info(fname);
    p_args.append(get_python_args(fname, python_args, interact, debug, args));

    if (os::name == "nt") {
        QString cmd = p_args.join(' ');
        qDebug() <<__func__<< cmd;

        QProcess process;
        if (!wdir.isEmpty())
            process.setWorkingDirectory(wdir);
        process.start(cmd);
        process.waitForFinished();
        qDebug() << process.readAllStandardOutput();
    }

    /* 下面的代码其实正确执行了，只是在子进程中，所以没有任何输出
    QStringList p_args(executable);
    p_args.append(get_python_args(fname, python_args, interact, debug, args));

    if (os::name == "nt") {
        QProcess process;
        process.start("cd", QStringList(wdir));//cd F:/MyPython\r\n
        process.waitForStarted();

        if (!process.readAllStandardError().isEmpty())
            return;

        //D:\Anaconda3\python.exe F:\MyPython\test.py\r\n
        process.write(QString(p_args.join(' ') + "\r\n").toUtf8());
        qDebug() << "cd" << QStringList(wdir) << QString(p_args.join(' ') + "\r\n");
        qDebug() << process.readAllStandardOutput();
        qDebug() << process.readAllStandardError();
    }*/
}

bool is_stable_version(const QString& version)
{
    //A stable version has no letters in the final component, but only numbers.
    //Not stable version: 1.2alpha, 1.3.4beta, 0.1.0rc1, 3.0.0dev
    QStringList list = version.split('.');
    QString last_part = list.last();
    QRegularExpression re("[a-zA-Z]");
    QRegularExpressionMatch match = re.match(last_part);
    return !match.hasMatch();
}

//check_version(actver, version, cmp_op)
//get_module_version(module_name)
//is_module_installed(module_name, version=None)

bool is_python_interpreter_valid_name(const QString& filename)
{
    // re.I是大小写不敏感
    QString pattern = ".*python(\\d\\.?\\d*)?(w)?(.exe)?$";
    QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(filename);
    if (!match.hasMatch())
        return false;
    else
        return true;
}

bool is_pythonw(const QString& filename);
bool check_python_help(const QString& filename);
bool is_python_interpreter(const QString& filename)
{
    //源码是real_filename = os.path.realpath(filename)
    QFileInfo info(filename);
    QString real_filename = info.absoluteFilePath();
    if (!info.isFile() || !is_python_interpreter_valid_name(filename))
        return false;
    else if (is_pythonw(filename)) {
        if (os::name == "nt") {
            // pythonw is a binary on Windows
            if (!encoding::is_text_file(real_filename))
                return true;
            else
                return false;
        }
        else
            return false;
    }
    else if (encoding::is_text_file(real_filename))
        return false;
    else
        return check_python_help(filename);
        //return info.isExecutable();
}

bool is_pythonw(const QString& filename)
{
    QString pattern = ".*python(\\d\\.?\\d*)?w(.exe)?$";
    QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(filename);
    if (!match.hasMatch())
        return false;
    else
        return true;
}

bool check_python_help(const QString& filename)
{
    QString output = run_program(filename, QStringList(QString("-h")));
    QString valid = "Options and arguments (and corresponding"
                    " environment variables)";
    if (output.contains("usage:") && output.contains(valid))
        return true;
    else
        return false;
}

} // namespace programs
