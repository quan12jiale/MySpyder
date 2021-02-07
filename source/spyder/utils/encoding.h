#pragma once

#include "check.h"
#include "os.h"

namespace encoding {

bool write(const QString& text,const QString& filename,QIODevice::OpenMode mode=QIODevice::WriteOnly);
bool writelines(const QStringList& lines,const QString& filename,QIODevice::OpenMode mode=QIODevice::WriteOnly);

QString read(const QString& filename);
QStringList readlines(const QString& filename);

bool is_text_file(const QString& filename);

} // namespace encoding


