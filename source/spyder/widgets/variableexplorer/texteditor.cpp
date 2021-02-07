#include "texteditor.h"

TextEditor::TextEditor(const QString& text,const QString& title,
                       const QFont& font,QWidget* parent,
                       bool readonly,const QSize& size)
    : QDialog (parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    this->text = QString();
    btn_save_and_close = nullptr;

    is_binary = false;

    layout = new QVBoxLayout;
    setLayout(layout);

    edit = new QTextEdit(parent);
    edit->setReadOnly(readonly);
    connect(edit,SIGNAL(textChanged()),this,SLOT(text_changed()));
    edit->setPlainText(text);
    edit->setFont(font);
    layout->addWidget(edit);

    QHBoxLayout* btn_layout = new QHBoxLayout;
    btn_layout->addStretch();
    if (!readonly) {
        btn_save_and_close = new QPushButton("Save and close");
        btn_save_and_close->setDisabled(true);
        connect(btn_save_and_close,SIGNAL(clicked(bool)),this,SLOT(accept()));
        btn_layout->addWidget(btn_save_and_close);
    }

    btn_close = new QPushButton("Close");
    btn_close->setAutoRepeat(true);
    btn_close->setDefault(true);
    connect(btn_close,SIGNAL(clicked(bool)),this,SLOT(reject()));
    btn_layout->addWidget(btn_close);

    layout->addLayout(btn_layout);

    setWindowFlags(Qt::Window);

    setWindowIcon(ima::icon("edit"));
    if (!title.isEmpty())
        setWindowTitle(QString("Text editor%1").arg(" - "+title));
    else
        setWindowTitle(QString("Text editor%1").arg(""));
    resize(size);
}

void TextEditor::text_changed()
{
    this->text = edit->toPlainText();
    if (btn_save_and_close) {
        btn_save_and_close->setEnabled(true);
        btn_save_and_close->setAutoDefault(true);
        btn_save_and_close->setDefault(true);
    }
}

QString TextEditor::get_value() const
{
    return this->text;
}

bool TextEditor::setup_and_check()
{
    return true;
}
