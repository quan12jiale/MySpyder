#include "helperwidgets.h"

HelperToolButton::HelperToolButton()
    : QToolButton ()
{
    setIcon(ima::get_std_icon("MessageBoxInformation"));
    QString style = "QToolButton {"
                    "border: 1px solid grey;"
                    "padding:0px;"
                    "border-radius: 2px;"
                    "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                    "stop: 0 #f6f7fa, stop: 1 #dadbde);"
                    "}";
    setStyleSheet(style);
}

void HelperToolButton::setToolTip(const QString &text)
{

    _tip_text = text;
}

QString HelperToolButton::toolTip() const
{
    return  _tip_text;
}

void HelperToolButton::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    QToolTip::hideText();
}

void HelperToolButton::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    QToolTip::showText(mapToGlobal(QPoint(0, height())),
                       _tip_text);
}


/********** MessageCheckBox **********/
MessageCheckBox::MessageCheckBox(QMessageBox::Icon icon, QWidget *parent)
    : QMessageBox (icon, "", "", QMessageBox::Ok, parent)
{
    this->_checkbox = new QCheckBox;

    int size = 9;
    QVBoxLayout* check_layout = new QVBoxLayout;
    check_layout->addItem(new QSpacerItem(size, size));
    check_layout->addWidget(_checkbox, 0, Qt::AlignRight);
    check_layout->addItem(new QSpacerItem(size,size));

    QGridLayout* tmp = qobject_cast<QGridLayout*>(this->layout());
    if (tmp)
        tmp->addLayout(check_layout, 1, 2);
}

bool MessageCheckBox::is_checked() const
{
    return this->_checkbox->isChecked();
}

void MessageCheckBox::set_checked(bool value)
{
    this->_checkbox->setChecked(value);
}

void MessageCheckBox::set_check_visible(bool value)
{
    this->_checkbox->setVisible(value);
}

bool MessageCheckBox::is_check_visible() const
{
    return this->_checkbox->isVisible();
}

QString MessageCheckBox::checkbox_text() const
{
    return this->_checkbox->text();
}

void MessageCheckBox::set_checkbox_text(const QString &text)
{
    this->_checkbox->setText(text);
}


/********** HTMLDelegate **********/
HTMLDelegate::HTMLDelegate(QObject* parent,int margin)
    : QStyledItemDelegate (parent)
{
    _margin = margin;
}

void HTMLDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem options(option);
    initStyleOption(&options, index);

    QStyle* style;
    if (options.widget == nullptr)
        style = QApplication::style();
    else
        style = options.widget->style();

    QTextDocument doc;
    doc.setDocumentMargin(_margin);
    doc.setHtml(options.text);

    options.text = "";
    style->drawControl(QStyle::CE_ItemViewItem, &options, painter);

    QAbstractTextDocumentLayout::PaintContext ctx;

    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &options);
    painter->save();

    //if hasattr(options.widget, 'files_list')  //TODO

    painter->translate(textRect.topLeft() + QPoint(0, -3));
    doc.documentLayout()->draw(painter, ctx);
    painter->restore();
}

QSize HTMLDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem options(option);
    initStyleOption(&options, index);

    QTextDocument doc;
    doc.setHtml(options.text);

    return QSize(static_cast<int>(doc.idealWidth()), static_cast<int>(doc.size().height()) - 2);
}


/********** IconLineEdit **********/
IconLineEdit::IconLineEdit(QWidget* parent)
    : QLineEdit (parent)
{
    _status = true;
    _status_set = true;
    _valid_icon = ima::icon("todo");
    _invalid_icon = ima::icon("warning");
    _set_icon = ima::icon("todo_list");
    _application_style = QApplication::style()->objectName();
    this->_refresh();
    _paint_count = 0;
    _icon_visible = false;
}

void IconLineEdit::_refresh()
{
    int padding = this->height();
    QString css_base = "QLineEdit {{"
                       "border: none;"
                       "padding-right: %1px;"
                       "}}";
    QString css_oxygen = "QLineEdit {{background: transparent;"
                         "border: none;"
                         "padding-right: %1px;"
                         "}}";
    QString css_template;
    if (_application_style == "oxygen")
        css_template = css_oxygen;
    else
        css_template = css_base;

    QString css = css_template.arg(padding);
    this->setStyleSheet(css);
    this->update();
}

void IconLineEdit::hide_status_icon()
{
    _icon_visible = false;
    this->repaint();
    this->update();
}

void IconLineEdit::show_status_icon()
{
    _icon_visible = true;
    this->repaint();
    this->update();
}

void IconLineEdit::update_status(bool value,bool value_set)
{
    _status = value;
    _status_set = value_set;
    this->repaint();
    this->update();
}

void IconLineEdit::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);
    QPainter painter(this);

    QRect rect = geometry();
    int space = rect.height()/6;
    int h = rect.height() - space;
    int w = rect.width() - h;

    if (_icon_visible) {
        QPixmap pixmap;
        if (_status && _status_set)
            pixmap = _set_icon.pixmap(h, h);
        else if (_status)
            pixmap = _valid_icon.pixmap(h,h);
        else
            pixmap = _invalid_icon.pixmap(h,h);

        painter.drawPixmap(w, space, pixmap);
    }

    QString application_style = QApplication::style()->objectName();;
    if (_application_style != application_style) {
        _application_style = application_style;
        this->_refresh();
    }

    if (_paint_count < 5) {
        _paint_count++;
        this->_refresh();
    }
}


void test_msgcheckbox()
{
    MessageCheckBox box;
    box.setWindowTitle("Spyder updates");
    box.setText("Testing checkbox");
    box.set_checkbox_text("Check for updates on startup?");
    box.setStandardButtons(QMessageBox::Ok);
    box.setDefaultButton(QMessageBox::Ok);
    box.setIcon(QMessageBox::Information);
    box.exec();
}
