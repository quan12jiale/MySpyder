#pragma once


#include <QHash>


QString escape(const QString& data,
               const QHash<QString,QString>& entities=QHash<QString,QString>());

