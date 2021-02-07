#include "arraybuilder.h"
#include <QDebug>

static QString SHORTCUT_TABLE = "Ctrl+M";
static QString SHORTCUT_INLINE = "Ctrl+Alt+M";
static QString ELEMENT_SEPARATOR = ", ";
static QString ROW_SEPARATOR = ";";
static QString BRACES = "], [";
static QStringList NAN_VALUES = {"nan", "NAN", "NaN", "Na", "NA", "na"};

NumpyArrayInline::NumpyArrayInline(NumpyArrayDialog* parent)
    : QLineEdit (parent)
{
    _parent = parent;
}

void NumpyArrayInline::keyPressEvent(QKeyEvent *event)
{
    if (event->key()==Qt::Key_Enter || event->key()==Qt::Key_Return) {
        _parent->process_text();
        if (_parent->is_valid())
            _parent->keyPressEvent(event);
    }
    else
        QLineEdit::keyPressEvent(event);
}

bool NumpyArrayInline::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key()==Qt::Key_Tab || keyEvent->key()==Qt::Key_Escape) {
            QString text = this->text();
            int cursor = cursorPosition();
            if (cursor!=0 && text[cursor-1]==' ')
                text = text.left(cursor-1) + ROW_SEPARATOR + " " + text.mid(cursor);
            else
                text = text.left(cursor) + " " + text.mid(cursor);
            setCursorPosition(cursor);
            setText(text);
            setCursorPosition(cursor + 1);
            return false;
        }
    }
    return QWidget::event(event);
}


//********** NumpyArrayTable **********
NumpyArrayTable::NumpyArrayTable(NumpyArrayDialog* parent)
    : QTableWidget (parent)
{
    _parent = parent;
    setRowCount(2);
    setColumnCount(2);
    reset_headers();

    connect(this,SIGNAL(cellChanged(int,int)),
            this,SLOT(cell_changed(int,int)));
}

void NumpyArrayTable::keyPressEvent(QKeyEvent *event)
{
    if (event->key()==Qt::Key_Enter || event->key()==Qt::Key_Return) {
        QTableWidget::keyPressEvent(event);
        setDisabled(true);
        setDisabled(false);
        _parent->keyPressEvent(event);
    }
    else
        QTableWidget::keyPressEvent(event);
}

void NumpyArrayTable::cell_changed(int row,int col)
{
    QTableWidgetItem* item = this->item(row, col);
    QString value;
    int rows=0, cols=0;
    if (item) {
        rows = rowCount();
        cols = columnCount();
        value = item->text();
    }

    if (!value.isEmpty()) {
        if (row == rows - 1)
            setRowCount(rows + 1);
        if (col == cols - 1)
            setColumnCount(cols + 1);
    }
    reset_headers();
}

void NumpyArrayTable::reset_headers()
{
    int rows = rowCount();
    int cols = columnCount();
    for (int r = 0; r < rows; ++r) {
        setVerticalHeaderItem(r, new QTableWidgetItem(QString::number(r)));
    }
    for (int c = 0; c < cols; ++c) {
        setHorizontalHeaderItem(c, new QTableWidgetItem(QString::number(c)));
        setColumnWidth(c, 40);
    }
}

QString NumpyArrayTable::text() const
{
    QStringList text;
    int rows = rowCount();
    int cols = columnCount();

    if (rows==2 && cols==2) {
        QTableWidgetItem* item = this->item(0, 0);
        if (!item)
            return "";
    }

    for (int r = 0; r < rows-1; ++r) {
        for (int c = 0; c < cols-1; ++c) {
            QTableWidgetItem* item = this->item(r, c);
            QString value;
            if (item)
                value = item->text();
            else
                value = "0";

            if (value.trimmed().isEmpty())
                value = "0";

            text.append(" ");
            text.append(value);
        }
        text.append(ROW_SEPARATOR);
    }
    text.pop_back();
    return text.join("");
}

NumpyArrayDialog::NumpyArrayDialog(QWidget* parent,bool inline_,
                                   int offset,bool force_float)
    : QDialog (parent)
{
    _parent = parent;
    _text = QString();
    _valid = false;
    _offset = offset;

    _force_float = force_float;
    _help_inline = "<b>Numpy Array/Matrix Helper</b><br>"
                         "Type an array in Matlab    : <code>[1 2;3 4]</code><br>"
                         "or Spyder simplified syntax : <code>1 2;3 4</code>"
                         "<br><br>"
                         "Hit 'Enter' for array or 'Ctrl+Enter' for matrix."
                         "<br><br>"
                         "<b>Hint:</b><br>"
                         "Use two spaces or two tabs to generate a ';'.";
    _help_table = "<b>Numpy Array/Matrix Helper</b><br>"
                        "Enter an array in the table. <br>"
                        "Use Tab to move between cells."
                        "<br><br>"
                        "Hit 'Enter' for array or 'Ctrl+Enter' for matrix."
                        "<br><br>"
                        "<b>Hint:</b><br>"
                        "Use two tabs at the end of a row to move to the next row.";

    _button_warning = new QToolButton();
    _button_help = new HelperToolButton();
    _button_help->setIcon(ima::icon("MessageBoxInformation"));

    QString style = "QToolButton {"
                    "border: 1px solid grey;"
                    "padding:0px;"
                    "border-radius: 2px;"
                    "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                    "stop: 0 #f6f7fa, stop: 1 #dadbde);"
                    "}";
    _button_help->setStyleSheet(style);

    inline_flag = inline_;
    if (inline_) {
        _button_help->setToolTip(this->_help_inline);
        _widget = new NumpyArrayInline(this);
    }
    else {
        _button_help->setToolTip(this->_help_table);
        _widget = new NumpyArrayTable(this);
    }

    style = "QDialog {"
            "margin:0px;"
            "border: 1px solid grey;"
            "padding:0px;"
            "border-radius: 2px;"
            "}";
    setStyleSheet(style);

    style = "QToolButton {"
            "margin:1px;"
            "border: 0px solid grey;"
            "padding:0px;"
            "border-radius: 0px;"
            "}";
    _button_warning->setStyleSheet(style);

    setWindowFlags(Qt::Window | Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setWindowOpacity(0.90);
    _widget->setMinimumWidth(200);

    _layout = new QHBoxLayout();
    _layout->addWidget(_widget);
    _layout->addWidget(_button_warning, 1, Qt::AlignTop);
    _layout->addWidget(_button_help, 1, Qt::AlignTop);
    setLayout(_layout);

    _widget->setFocus();
}

void NumpyArrayDialog::keyPressEvent(QKeyEvent *event)
{
    QToolTip::hideText();
    Qt::KeyboardModifiers ctrl = event->modifiers() & Qt::ControlModifier;

    if (event->key()==Qt::Key_Enter || event->key()==Qt::Key_Return) {
        if (ctrl)
            process_text(false);
        else
            process_text(true);
        accept();
    }
    else
        QDialog::keyPressEvent(event);
}

bool NumpyArrayDialog::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab) {
            return false;
        }
    }
    return QWidget::event(event);
}

void NumpyArrayDialog::process_text(bool array)
{
    QString prefix;
    if (array)
        prefix = "np.array([[";
    else
        prefix = "np.matrix([[";

    QString suffix = "]])";
    QString values;
    if (inline_flag) {
        NumpyArrayInline * arrayInline = dynamic_cast<NumpyArrayInline*>(_widget);
        values = arrayInline->text().trimmed();
    }
    else {
        NumpyArrayTable * arrayTable = dynamic_cast<NumpyArrayTable*>(_widget);
        values = arrayTable->text().trimmed();
    }

    if (!values.isEmpty()) {
        QString exp = "(\\s*)" + ROW_SEPARATOR + "(\\s*)";
        values.replace(QRegExp(exp), ROW_SEPARATOR);
        values.replace(QRegExp("\\s+"), " ");
        values.replace(QRegExp("]$"), "");
        values.replace(QRegExp("^\\["), "");
        values.replace(QRegExp(ROW_SEPARATOR+"*$"), "");

        values.replace(" ", ELEMENT_SEPARATOR);

        QStringList new_values;
        QStringList rows = values.split(ROW_SEPARATOR);
        int nrows = rows.size();
        QList<int> ncols;
        foreach (const QString& row, rows) {
            QStringList new_row;
            QStringList elements = row.split(ELEMENT_SEPARATOR);
            ncols.append(elements.size());
            foreach (const QString& e, elements) {
                QString num = e;

                if (NAN_VALUES.contains(num))
                    num = "np.nan";

                if (_force_float) {
                    try {
                        num = QString::number(e.toDouble());
                    }
                    catch (...) {
                        ;
                    }
                }
                new_row.append(num);
            }
            new_values.append(new_row.join(ELEMENT_SEPARATOR));
        }
        values = new_values.join(ROW_SEPARATOR);

        QSet<int> set = QSet<int>::fromList(ncols);
        if (set.size() == 1)
            _valid = true;
        else
            _valid = false;

        if (nrows == 1) {
            prefix.chop(1);
            suffix.replace("]])", "])");
        }

        int offset = _offset;
        QString after(offset+prefix.size()-1, ' ');
        QString braces = BRACES;
        braces.replace(" ", "\n" + after);
        values.replace(ROW_SEPARATOR,  braces);
        QString text = QString("%1%2%3").arg(prefix).arg(values).arg(suffix);
        _text = text;
    }
    else
        _text = "";
    update_warning();
}

void NumpyArrayDialog::update_warning()
{
    QToolButton* widget = _button_warning;
    if (!is_valid()) {
        QString tip = "Array dimensions not valid";
        widget->setIcon(ima::icon("MessageBoxWarning"));
        widget->setToolTip(tip);
        QToolTip::showText(_widget->mapToGlobal(QPoint(0, 5)), tip);
    }
    else
        _button_warning->setToolTip("");
}

bool NumpyArrayDialog::is_valid() const
{
    return _valid;
}

QString NumpyArrayDialog::text() const
{
    return _text;
}

QWidget* NumpyArrayDialog::array_widget()
{
    return _widget;
}
