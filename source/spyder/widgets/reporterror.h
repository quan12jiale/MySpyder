#pragma once

#include "config/gui.h"
#include "utils/misc.h"
#include "utils/icon_manager.h"
#include "widgets/sourcecode/codeeditor.h"

extern QString __project_url__;
extern QString __forum_url__;
extern QString __trouble_url__;
extern QString __trouble_url_short__;
extern QString __website_url__;

class DescriptionWidget : public CodeEditor
{
    Q_OBJECT
public:
    QString header;
    int header_end_pos;

    DescriptionWidget(QWidget* parent=nullptr);
    void remove_text();
public slots:
    void cut();
protected:
    void keyPressEvent(QKeyEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
};


class ShowErrorWidget : public ConsoleBaseWidget
{
    Q_OBJECT
signals:
    void go_to_error(const QString&);
public:
    bool __cursor_changed;

    ShowErrorWidget(QWidget* parent = nullptr);

protected:
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);
};


class SpyderErrorDialog : public QDialog
{
    Q_OBJECT
public:
    bool is_report;
    QString error_traceback;
    QLineEdit* title;
    QLabel* title_chars_label;
    DescriptionWidget* input_description;
    ShowErrorWidget* details;

    int initial_chars;
    QLabel* desc_chars_label;
    QCheckBox* dismiss_box;
    QPushButton* submit_btn;
    QPushButton* details_btn;
    QPushButton* close_btn;

    SpyderErrorDialog(QWidget* parent = nullptr, bool is_report=false);
    void append_traceback(const QString& text);
public slots:
    void _submit_to_github();
    void _show_details();
    void _contents_changed();
};
