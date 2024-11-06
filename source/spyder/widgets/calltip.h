#pragma once

#include <QtWidgets>
#include <QApplication>

template <typename T>
class BaseEditMixin;

class CallTipWidget : public QLabel
{
    Q_OBJECT
public:
    CallTipWidget(BaseEditMixin<QTextEdit>* text_edit,bool hide_timer_on=false);
    CallTipWidget(BaseEditMixin<QPlainTextEdit>* text_edit,bool hide_timer_on=false);

    bool eventFilter(QObject *obj, QEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    void enterEvent(QEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void setFont(const QFont& font);
    void showEvent(QShowEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

    bool show_tip(QPoint point, const QString& tip, QStringList wrapped_tiplines);
    QPair<int,int> _find_parenthesis(int position, bool forward=true);
    void _leave_event_hide();
public slots:
    void _cursor_position_changed();
public:
    bool hide_timer_on;
    QString tip;
    QScopedPointer<QBasicTimer> _hide_timer;
    BaseEditMixin<QTextEdit>* _text_edit;
    BaseEditMixin<QPlainTextEdit>* _plain_edit;

    int _start_position;
};

