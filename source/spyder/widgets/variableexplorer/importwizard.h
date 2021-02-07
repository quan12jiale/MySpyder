#pragma once

#include <QtWidgets>


class ContentsWidget : public QWidget
{
    Q_OBJECT
signals:
    void asDataChanged(bool);
public:
    QTextEdit* text_editor;
    bool _as_data;
    bool _as_code;
    bool _as_num;
    QRadioButton* tab_btn;
    QRadioButton* ws_btn;
    QLineEdit* line_edt;
    QRadioButton* eol_btn;
    QLineEdit* line_edt_row;
    QLineEdit* skiprows_edt;
    QLineEdit* comments_edt;
    QCheckBox* trnsp_box;
public:
    ContentsWidget(QWidget* parent, const QString& text);
    bool get_as_data() const;
    bool get_as_code() const;
    bool get_as_num() const;
    QString get_col_sep() const;
    QString get_row_sep() const;
    int get_skiprows() const;
    QString get_comments() const;

public slots:
    void set_as_data(bool as_data);
    void set_as_code(bool as_code);
};

