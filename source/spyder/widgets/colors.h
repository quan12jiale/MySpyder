#pragma once

#include <QtWidgets>

class ColorButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ get_color WRITE set_color)
signals:
    void colorChanged(const QColor&);
private:
    QColor _color;
public:
    ColorButton(QWidget* parent = nullptr);
    QColor get_color() const;
public slots:
    void choose_color();
    void set_color(const QColor& color);
};


class ColorLayout : public QHBoxLayout
{
    Q_OBJECT
public:
    QLineEdit* lineedit;
    ColorButton* colorbtn;
    ColorLayout(const QColor& color, QWidget* parent = nullptr);
    QString text() const;
public slots:
    void update_color(const QString& text);
    void update_text(const QColor& color);
};
