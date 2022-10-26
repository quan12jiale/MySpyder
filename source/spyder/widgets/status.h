#pragma once

#include <QtWidgets>
#include "str.h"

class StatusBarWidget : public QWidget
{
    Q_OBJECT
public:
    QFont label_font;

    StatusBarWidget(QWidget* parent, QStatusBar* statusbar);
};


class BaseTimerStatus : public StatusBarWidget
{
    Q_OBJECT
public:
    QString TITLE;
    QString TIP;
    QLabel* label;
    QLabel* value;
    QTimer* timer;

    BaseTimerStatus(QWidget* parent, QStatusBar* statusbar);
    void set_interval(int interval);
    virtual quint32 get_value() = 0;
public slots:
    void update_label();
};


class MemoryStatus : public BaseTimerStatus
{
    Q_OBJECT
public:
    MemoryStatus(QWidget* parent, QStatusBar* statusbar);
	quint32 get_value() override;
};


class ReadWriteStatus : public StatusBarWidget
{
    Q_OBJECT
public:
    QLabel* label;
    QLabel* readwrite;

    ReadWriteStatus(QWidget* parent, QStatusBar* statusbar);
public slots:
    void readonly_changed(bool readonly);
};


class EOLStatus : public StatusBarWidget
{
    Q_OBJECT
public:
    QLabel* label;
    QLabel* eol;

    EOLStatus(QWidget* parent, QStatusBar* statusbar);
public slots:
    void eol_changed(const QString& os_name);
};


class EncodingStatus : public StatusBarWidget
{
    Q_OBJECT
public:
    QLabel* label;
    QLabel* encoding;

    EncodingStatus(QWidget* parent, QStatusBar* statusbar);
public slots:
    void encoding_changed(const QString& encoding);
};


class CursorPositionStatus : public StatusBarWidget
{
    Q_OBJECT
public:
    QLabel* label_line;
    QLabel* label_column;
    QLabel* column;
    QLabel* line;

    CursorPositionStatus(QWidget* parent, QStatusBar* statusbar);
public slots:
    void cursor_position_changed(int line,int index);
};
