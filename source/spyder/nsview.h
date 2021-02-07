#pragma once

#include "str.h"
#include <QVariant>

int get_size(const QVariant& item);
QString get_color_name(const QVariant& value);
QString collections_display(const QVariant& value,int level);
QString value_to_display(const QVariant& value,int level=0);
QVariant display_to_value(QVariant value,const QVariant&  default_value);
