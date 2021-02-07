#pragma once

#include "os.h"
#include <QStringList>
#include <QTextCharFormat>

class ANSIEscapeCodeHandler
{
public:
    QList<QStringList> ANSI_COLORS;

    int intensity;
    bool italic;
    bool bold;
    bool underline;
    int foreground_color;
    int background_color;
    int default_foreground_color;
    int default_background_color;

    QTextCharFormat current_format;//出现在109行
    ANSIEscapeCodeHandler();
    void set_code(int code);
    virtual void set_style() = 0;
    void reset();
};

