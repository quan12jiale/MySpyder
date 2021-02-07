#ifndef NETEASEENCODE_H
#define NETEASEENCODE_H

#include "os.h"
#include <QtCore>

//#include <Python.h>

QString createSecretKey(int size);

QHash<QString,QString> encrypted_request(const QHash<QString,QString>& dict);
QHash<QString,QString> encrypted_request(const QList<long long>& ids);

QHash<QString,QString> encrypted_request2(const QHash<QString,QString>& dict);

#endif // NETEASEENCODE_H
