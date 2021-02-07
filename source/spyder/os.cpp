#include "os.h"

namespace os {

QString curdir = ".";
QString pardir = "..";
QString extsep = ".";
QString sep = "\\";
QString pathsep = ";";
QString altsep = "/";
QString defpath = ".;C:\\bin";
QString devnull = "nul";

QPair<QString, QString> splitdrive(const QString& p)
{
    if (p.size() >= 2) {
        QString colon = ":";
        if (p.mid(1,1) == colon)
            return QPair<QString, QString>(p.left(2), p.mid(2));
    }
    return QPair<QString, QString>(QString(""), p);
}

void makedirs(const QString& name)
{
    int idx = name.lastIndexOf('/');
    QString head, tail;
    if (idx != -1) {
        head = name.left(idx);
        tail = name.mid(idx+1);
        if (tail.isEmpty()) {
            int idx2 = head.lastIndexOf('/');
            if (idx2 != -1) {
                tail = head.mid(idx2+1);
                head = head.left(idx2);
            }
        }
    }

    if (!head.isEmpty() && !tail.isEmpty() && !QFileInfo::exists(head)) {
        makedirs(head);

        if (tail == curdir)
            return;
    }

    if (!head.isEmpty() && !tail.isEmpty()) {
        QDir dir(head);
        if (dir.mkdir(tail))
            qDebug() << __func__ << head << tail;
        else {
            QMessageBox::critical(nullptr,"mkdir error",
                                  QString("Unable to create a %1 directory"
                                          " under the %2 directory!")
                                  .arg(tail).arg(head));
        }
    }
}

#if defined (Q_OS_UNIX) || defined (Q_OS_MAC)
    QString name = "posix";
    QString linesep = "\n";
#elif defined (Q_OS_WIN)
    QString name = "nt";
    QString linesep = "\r\n";
#else
    throw QString("no os specific module found");
#endif


bool walk(const QString& absolute_path, QStringList* list, const QString& exclude)
{
    QDir dir(absolute_path);
    if (exclude.isEmpty()) {
        foreach (QString relative_path, dir.entryList()) {
            if (relative_path == "." || relative_path == "..")
                continue;
            QString absolute_sub_path = absolute_path+'/'+relative_path;
            QFileInfo info(absolute_sub_path);
            if (info.isDir()) {
                if (relative_path == ".git" || relative_path == ".hg")
                    continue;
                walk(absolute_sub_path, list, exclude);
            }
            else if (info.isFile()) {
                if (encoding::is_text_file(absolute_sub_path))
                    list->append(absolute_sub_path);
            }
        }
    }
    else {
        QRegularExpression re(exclude);
        if (!re.isValid())
            return false;
        foreach (QString relative_path, dir.entryList()) {
            if (relative_path == "." || relative_path == "..")
                continue;
            QString absolute_sub_path = absolute_path+'/'+relative_path;
            QFileInfo info(absolute_sub_path);
            if (info.isDir()) {
                QRegularExpressionMatch match = re.match(absolute_sub_path+sep);
                if (match.hasMatch())
                    continue;
                if (relative_path == ".git" || relative_path == ".hg")
                    continue;
                walk(absolute_sub_path, list, exclude);
            }
            else if (info.isFile()) {
                QRegularExpressionMatch match = re.match(absolute_sub_path);
                if (match.hasMatch())
                    continue;
                if (encoding::is_text_file(absolute_sub_path))
                    list->append(absolute_sub_path);
            }
        }
    }
    return true;
}


} // namespace os
