#pragma once


#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

extern bool DEV;
extern bool SAFE_MODE;

bool running_under_pytest();

extern int DEBUG;

QString get_home_dir();
QString get_conf_path(const QString& filename = QString());
QString get_image_path(const QString& name,const QString& _default="not_found.png");
extern QHash<QString, QString> LANGUAGE_CODES;

void save_lang_conf(const QString& value);
QString load_lang_conf();//返回诸如"en"或者"de"

extern QStringList EXCLUDED_NAMES;
