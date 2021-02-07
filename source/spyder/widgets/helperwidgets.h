#pragma once

#include "utils/icon_manager.h"

#include <QtWidgets>

class HelperToolButton : public QToolButton
{
public:
    HelperToolButton();
    void setToolTip(const QString& text);
    QString toolTip() const;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
private:
    QString _tip_text;
};


class MessageCheckBox : public QMessageBox
{
public:
    QCheckBox* _checkbox;

    MessageCheckBox(QMessageBox::Icon icon = QMessageBox::NoIcon,
                    QWidget *parent = nullptr);
    bool is_checked() const;
    void set_checked(bool value);
    void set_check_visible(bool value);
    bool is_check_visible() const;
    QString checkbox_text() const;
    void set_checkbox_text(const QString& text);
};


class HTMLDelegate : public QStyledItemDelegate
{
public:
    int _margin;

    HTMLDelegate(QObject* parent, int margin = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};


class IconLineEdit : public QLineEdit
{
    Q_OBJECT
private:
    bool _status;
    bool _status_set;
    QIcon _valid_icon;
    QIcon _invalid_icon;
    QIcon _set_icon;
    QString _application_style;
    int _paint_count;
    bool _icon_visible;
public:
    IconLineEdit(QWidget* parent = nullptr);
    void _refresh();
    void hide_status_icon();
    void show_status_icon();
protected:
    void paintEvent(QPaintEvent *event) override;
public slots:
    void update_status(bool value,bool value_set);
};
