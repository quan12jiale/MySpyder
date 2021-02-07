#pragma once

#include "utils/syntaxhighlighters.h"
#include <QHash>
#include <QSettings>
#include <QDebug>

namespace gui {

QFont get_font(const QString& section="main", const QString& option="font", int font_size_delta=0);
void set_font(const QFont& font, const QString& section="main", const QString& option="font");

QString get_shortcut(const QString& context, const QString& name);
void set_shortcut(const QString& context, const QString& name, const QString& keystr);

QHash<QString, ColorBoolBool> get_color_scheme();

} // namespace gui
