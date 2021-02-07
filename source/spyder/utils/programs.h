#pragma once

#include "os.h"
#include "str.h"
#include "utils/encoding.h"
#include "utils/misc.h"
#include <QUrl>
#include <QFileInfo>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QProcessEnvironment>

namespace programs {

QString is_program_installed(const QString& basename);
QString find_program(const QString& basename);

void run_python_script_in_terminal(QString fname, QString wdir, QString args, bool interact,
                                   bool debug, QString python_args, QString executable=QString());

bool start_file(const QString& filename);

bool is_python_interpreter(const QString& filename);

} // namespace programs

