#include "importwizard.h"


ContentsWidget::ContentsWidget(QWidget* parent, const QString& text)
    : QWidget (parent)
{
    text_editor = new QTextEdit(this);
    text_editor->setText(text);
    text_editor->setReadOnly(true);

    QHBoxLayout* type_layout = new QHBoxLayout;
    QLabel* type_label = new QLabel("Import as");
    type_layout->addWidget(type_label);
    QRadioButton* data_btn = new QRadioButton("data");
    data_btn->setChecked(true);
    _as_data = true;
    type_layout->addWidget(data_btn);
    QRadioButton* code_btn = new QRadioButton("code");
    _as_code = false;
    type_layout->addWidget(code_btn);
    QRadioButton* txt_btn = new QRadioButton("text");
    type_layout->addWidget(txt_btn);

    QSpacerItem* h_spacer = new QSpacerItem(40, 20,
                                            QSizePolicy::Expanding,
                                            QSizePolicy::Minimum);
    type_layout->addItem(h_spacer);
    QFrame* type_frame = new QFrame;
    type_frame->setLayout(type_layout);

    QGridLayout* grid_layout = new QGridLayout;
    grid_layout->setSpacing(0);

    QLabel* col_label = new QLabel("Column separator:");
    grid_layout->addWidget(col_label, 0, 0);
    QWidget* col_w = new QWidget;
    QHBoxLayout* col_btn_layout = new QHBoxLayout;
    tab_btn = new QRadioButton("Tab");
    tab_btn->setChecked(false);
    col_btn_layout->addWidget(tab_btn);
    ws_btn = new QRadioButton("Whitespace");
    ws_btn->setChecked(false);
    col_btn_layout->addWidget(ws_btn);
    QRadioButton* other_btn_col = new QRadioButton("other");
    other_btn_col->setChecked(true);
    col_btn_layout->addWidget(other_btn_col);
    col_w->setLayout(col_btn_layout);
    grid_layout->addWidget(col_w, 0, 1);
    line_edt = new QLineEdit(",");
    line_edt->setMaximumWidth(30);
    line_edt->setEnabled(true);
    connect(other_btn_col, SIGNAL(toggled(bool)),
            line_edt, SLOT(setEnabled(bool)));
    grid_layout->addWidget(line_edt, 0, 2);

    QLabel* row_label = new QLabel("Row separator:");
    grid_layout->addWidget(row_label, 1, 0);
    QWidget* row_w = new QWidget;
    QHBoxLayout* row_btn_layout = new QHBoxLayout;
    eol_btn = new QRadioButton("EOL");
    eol_btn->setChecked(true);
    row_btn_layout->addWidget(eol_btn);
    QRadioButton* other_btn_row = new QRadioButton("other");
    row_btn_layout->addWidget(other_btn_row);
    row_w->setLayout(row_btn_layout);
    grid_layout->addWidget(row_w, 1, 1);
    line_edt_row = new QLineEdit(";");
    line_edt_row->setMaximumWidth(30);
    line_edt_row->setEnabled(false);
    connect(other_btn_row, SIGNAL(toggled(bool)),
            line_edt_row, SLOT(setEnabled(bool)));
    grid_layout->addWidget(line_edt_row, 1, 2);

    grid_layout->setRowMinimumHeight(2, 15);

    QGroupBox* other_group = new QGroupBox("Additional options");
    QGridLayout* other_layout = new QGridLayout;
    other_group->setLayout(other_layout);

    QLabel* skiprows_label = new QLabel("Skip rows:");
    other_layout->addWidget(skiprows_label, 0, 0);
    skiprows_edt = new QLineEdit("0");
    skiprows_edt->setMaximumWidth(30);
    QIntValidator* intvalid = new QIntValidator(0, text.split(QRegExp("[\r\n]"),QString::SkipEmptyParts).size(),
                                                skiprows_edt);
    skiprows_edt->setValidator(intvalid);
    other_layout->addWidget(skiprows_edt, 0, 1);

    other_layout->setColumnMinimumWidth(2, 5);

    QLabel* comments_label = new QLabel("Comments:");
    other_layout->addWidget(comments_label, 0, 3);
    comments_edt = new QLineEdit("#");
    comments_edt->setMaximumWidth(30);
    other_layout->addWidget(comments_edt, 0, 4);

    trnsp_box = new QCheckBox("Transpose");
    other_layout->addWidget(trnsp_box, 1, 0, 2, 0);

    grid_layout->addWidget(other_group, 3, 0, 2, 0);

    QFrame* opts_frame = new QFrame;
    opts_frame->setLayout(grid_layout);

    connect(data_btn, SIGNAL(toggled(bool)),
            opts_frame, SLOT(setEnabled(bool)));
    connect(data_btn, SIGNAL(toggled(bool)),
            opts_frame, SLOT(set_as_data(bool)));
    connect(code_btn, SIGNAL(toggled(bool)),
            opts_frame, SLOT(set_as_code(bool)));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(type_frame);
    layout->addWidget(text_editor);
    layout->addWidget(opts_frame);
    setLayout(layout);
}

bool ContentsWidget::get_as_data() const
{
    return _as_data;
}

bool ContentsWidget::get_as_code() const
{
    return _as_code;
}

bool ContentsWidget::get_as_num() const
{
    return _as_num;
}

QString ContentsWidget::get_col_sep() const
{
    if (tab_btn->isChecked())
        return "\t";
    else if (ws_btn->isChecked())
        return QString();
    return line_edt->text();
}

QString ContentsWidget::get_row_sep() const
{
    if (eol_btn->isChecked())
        return "\n";
    return line_edt->text();
}

int ContentsWidget::get_skiprows() const
{
    return skiprows_edt->text().toInt();
}

QString ContentsWidget::get_comments() const
{
    return comments_edt->text();
}

//@Slot(bool)
void ContentsWidget::set_as_data(bool as_data)
{
    _as_data = as_data;
    emit asDataChanged(as_data);
}

//@Slot(bool)
void ContentsWidget::set_as_code(bool as_code)
{
    _as_code = as_code;
}


/********** PreviewTableModel **********/
