#pragma once

#include "utils/icon_manager.h"

#include <QtWidgets>

class TextEditor : public QDialog
{
    Q_OBJECT
public:
    TextEditor(const QString& text,const QString& title="",
               const QFont& font=QFont(),QWidget* parent=nullptr,
               bool readonly=false,const QSize& size=QSize(400,300));
    QString get_value() const;
    bool setup_and_check();

    QString text;
    QPushButton* btn_save_and_close;
    bool is_binary;
    QVBoxLayout* layout;
    QTextEdit* edit;
    QPushButton* btn_close;

public slots:
    void text_changed();

};

