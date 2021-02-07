#pragma once

#include "config/base.h"

#include <QIcon>
#include <QStyle>
#include <QWidget>
#include <QMetaEnum>

namespace ima {

QIcon get_std_icon(QString name, int size=0);
QIcon get_icon(const QString& name,const QIcon& _default,bool resample=false);
QIcon get_icon(const QString& name,const QString& _default=QString(),bool resample=false);
QIcon icon(const QString& name,bool resample=false,const QString& icon_path=QString());

} // namespace ima

