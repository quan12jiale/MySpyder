#pragma once

#include "utils/encoding.h"
#include <QDir>
#include <QPair>
#include <QString>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>

namespace os {

extern QString curdir; // 定义在D:\Anaconda3\lib\ntpath.py第11行
extern QString pardir;
extern QString extsep;
extern QString sep;
extern QString pathsep;
extern QString altsep;
extern QString defpath;
extern QString devnull;

QPair<QString, QString> splitdrive(const QString& p);

void makedirs(const QString& name);

extern QString name;
extern QString linesep;

bool walk(const QString& absolute_path, QStringList* list, const QString& exclude = QString());

} // namespace os
