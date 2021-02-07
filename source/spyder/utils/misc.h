#pragma once

#include "os.h"
#include "config/base.h"
#include <QDir>
#include <QDebug>
#include <QString>
#include <QFileInfo>
#include <QRegularExpression>

namespace misc {

void rename_file(const QString& source,const QString& dest);
void remove_file(const QString& fname);
void move_file(const QString& source,const QString& dest);

QPair<int,int> count_lines(const QString& path, QStringList extensions=QStringList(),
                           QStringList excluded_dirnames=QStringList());

bool get_error_match(const QString& text);

extern QString PYTHON_EXECUTABLE;
QString get_python_executable();

QString get_common_path(const QStringList& pathlist);

QString getcwd_or_home();
QString regexp_error_msg(const QString& pattern);

} // namespace misc
