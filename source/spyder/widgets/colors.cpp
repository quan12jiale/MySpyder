#include "colors.h"

ColorButton::ColorButton(QWidget* parent)
    : QPushButton (parent)
{
    setFixedSize(20, 20);
    setIconSize(QSize(12, 12));
    connect(this,SIGNAL(clicked()),this,SLOT(choose_color()));
    _color = QColor();
}

void ColorButton::choose_color()
{
    QColor color = QColorDialog::getColor(_color, parentWidget(),
                                          "Select Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid())
        set_color(color);
}

QColor ColorButton::get_color() const
{
    return _color;
}

void ColorButton::set_color(const QColor& color)
{
    if (color != this->_color) {
        this->_color = color;
        emit colorChanged(this->_color);
        QPixmap pixmap(iconSize());
        pixmap.fill(color);
        setIcon(QIcon(pixmap));
    }
}


QColor text_to_qcolor(const QString& text)
{
    QColor color;
    if (text.startsWith('#') && text.size() == 7) {
        QString correct = "#0123456789abcdef";
        foreach (QChar ch, text) {
            if (! correct.contains(ch.toLower()))
                return color;
        }
    }
    else if (! QColor::colorNames().contains(text))
        return color;
    color.setNamedColor(text);
    return color;
}


ColorLayout::ColorLayout(const QColor& color, QWidget* parent)
    : QHBoxLayout ()
{
    lineedit = new QLineEdit(color.name(), parent);
    connect(lineedit,SIGNAL(textChanged(QString)),this,SLOT(update_color(QString)));
    addWidget(lineedit);

    colorbtn = new ColorButton(parent);
    colorbtn->setProperty("color", color);
    connect(colorbtn,SIGNAL(colorChanged(QColor)),this,SLOT(update_text(QColor)));
    addWidget(colorbtn);
}

void ColorLayout::update_color(const QString& text)
{
    QColor color = text_to_qcolor(text);
    if (color.isValid())
        colorbtn->setProperty("color", color);
}

void ColorLayout::update_text(const QColor& color)
{
    lineedit->setText(color.name());
}

QString ColorLayout::text() const
{
    return lineedit->text();
}
