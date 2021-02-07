#pragma once

#include "os.h"
#include "helperwidgets.h"

class BaseComboBox : public QComboBox
{
    Q_OBJECT
signals:
    void valid(bool,bool);
    void sig_tab_pressed(bool);
    void sig_double_tab_pressed(bool);
public:
    int numpress;
    QString selected_text;
    //QTimer* presstimer;

    BaseComboBox(QWidget* parent);
    bool event(QEvent *event);
    virtual bool is_valid(QString) const {return true;}
    virtual void selected();
    void add_text(const QString& text);
    void set_current_text(const QString& text);
    virtual void add_current_text();
    bool add_current_text_if_valid();
    void hide_completer();

protected:
    void keyPressEvent(QKeyEvent *event);
public slots:
    void handle_keypress();
};


class PatternComboBox : public BaseComboBox
{
    Q_OBJECT
public:
    PatternComboBox(QWidget* parent,const QStringList& items=QStringList(),
                    const QString& tip=QString(),bool adjust_to_minimum=true);
};


class EditableComboBox : public BaseComboBox
{
    Q_OBJECT
public:
    QFont font;
    QHash<bool, QString> tips;

    EditableComboBox(QWidget* parent);
    void show_tip(const QString& tip = "");
    void selected();
    void validate(const QString& qstr, bool editing = true);
public slots:
    void editTextChanged_slot(const QString &text) { this->validate(text); }
};


class PathComboBox : public EditableComboBox
{
    Q_OBJECT
signals:
    void open_dir(const QString&);
public:
    PathComboBox(QWidget* parent,bool adjust_to_contents=false);

    QStringList _complete_options();
    bool is_valid(QString qstr = QString()) const;
    void selected();
    void add_current_text();
public slots:
    void double_tab_complete();
    void tab_complete();
    void add_tooltip_to_highlighted_item(int index);
protected:
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);
};


class UrlComboBox : public PathComboBox
{
    Q_OBJECT
public:
    UrlComboBox(QWidget* parent,bool adjust_to_contents=false);
    bool is_valid(QString qstr = QString()) const;
};


class FileComboBox : public PathComboBox
{
    Q_OBJECT
public:
    FileComboBox(QWidget* parent,bool adjust_to_contents=false,
                 bool default_line_edit=false);
    bool is_valid(QString qstr = QString()) const;
};


class PythonModulesComboBox : public PathComboBox
{
    Q_OBJECT
public:
    PythonModulesComboBox(QWidget* parent,bool adjust_to_contents=false);
    bool is_valid(QString qstr = QString()) const;
    void selected();
};
