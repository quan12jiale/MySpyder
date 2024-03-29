﻿#pragma once

#include "keyword.h"

#include <QRegularExpression>
#include <QStringList>
#include <QHash>
#include "utils/SyntaxHighlighterFactory.h"


namespace sourcecode {

extern const QHash<HighlighterEnumType,QStringList> ALL_LANGUAGES;

extern const QHash<HighlighterEnumType,QStringList> CELL_LANGUAGES;

QString get_eol_chars(const QString& text);
QString get_os_name_from_eol_chars(const QString& eol_chars);
QString get_eol_chars_from_os_name(const QString& os_name);

bool has_mixed_eol_chars(const QString& text);
QString normalize_eols(QString text, const QString& eol="\n");

QString fix_indentation(QString text);

bool is_keyword(const QString& text);

QString get_primary_at(const QString& source_code,int offset,bool retry=true);

} // namespace sourcecode
