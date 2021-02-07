#pragma once

#include "os.h"
#include <QString>
#include <QVariant>
#include <QStringList>

// split(QRegExp("\\s+"))

bool isdigit(const QString& text);

QString lstrip(const QString& text);
QString lstrip(const QString& text,const QString& before);
QString rstrip(const QString& text);
QString rstrip(const QString& text,const QString& before);

QString ljust(const QString& text,int width,QChar fillchar=' ');
QString rjust(const QString& text,int width,QChar fillchar=' ');

QStringList splitlines(const QString& text, bool keepends=false);

QString repr(const QList<QList<QVariant>>& breakpoints);
QList<QVariant> eval(const QString& breakpoints);

bool startswith(const QString& text,const QStringList& iterable);
bool endswith(const QString& text,const QStringList& iterable);

