#pragma once

#include "utils/icon_manager.h"
#include "helperwidgets.h"

class NumpyArrayDialog;


class NumpyArrayInline : public QLineEdit
{
public:
    NumpyArrayInline(NumpyArrayDialog* parent);
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;
public:
    NumpyArrayDialog* _parent;
};


class NumpyArrayTable : public QTableWidget
{
    Q_OBJECT
public:
    NumpyArrayTable(NumpyArrayDialog* parent);
    void keyPressEvent(QKeyEvent *event) override;
public slots:
    void cell_changed(int row,int col);
public:
    void reset_headers();
    QString text() const;
public:
    NumpyArrayDialog* _parent;
};


class NumpyArrayDialog : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(QWidget* array_widget MEMBER _widget READ array_widget)
public:
    NumpyArrayDialog(QWidget* parent=nullptr,bool inline_=true,
                     int offset=0,bool force_float=false);
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;
    void process_text(bool array=true);
    void update_warning();
    bool is_valid() const;
    QString text() const;
    QWidget* array_widget();

public:
    QWidget* _parent;
    QString _text;
    bool _valid;
    int _offset;
    bool _force_float;
    QString _help_inline;
    QString _help_table;
    QToolButton* _button_warning;
    HelperToolButton* _button_help;

    bool inline_flag;
    QWidget* _widget;
    QHBoxLayout* _layout;
};


