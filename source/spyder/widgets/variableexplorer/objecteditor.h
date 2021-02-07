#pragma once

#include "arrayeditor.h"

QWidget* create_dialog(const QVector<QVector<QVariant>>& obj,const QString& obj_name);
QVector<QVector<QVariant>> oedit(const QVector<QVector<QVariant>>& obj, bool modal=true);
void test();
