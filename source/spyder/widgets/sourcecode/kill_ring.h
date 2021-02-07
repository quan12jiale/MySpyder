#pragma once

#include <QPlainTextEdit>

class KillRing
{
public:
    int _index;
    QStringList _ring;
public:
    KillRing();
    void clear();
    void kill(const QString& text);
    QString yank();
    QString rotate();
};


class QtKillRing : public QObject
{
    Q_OBJECT
public:
    KillRing* _ring;
    QString _prev_yank;
    bool _skip_cursor;
    QPlainTextEdit* _text_edit;
public:
    QtKillRing(QPlainTextEdit* text_edit);
    void clear();
    void kill(const QString& text);
    void kill_cursor(QTextCursor* cursor);

public slots:
    void _cursor_position_changed();
    void yank();
    void rotate();
};
