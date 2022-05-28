#include "calltip.h"
#include "widgets/mixins.h"

CallTipWidget::CallTipWidget(BaseEditMixin<QTextEdit>* text_edit,bool hide_timer_on)
    : QLabel (nullptr,Qt::ToolTip)
{
    // 判断是否继承自QTextEdit是因为widgets/ipythonconsole/control.py
    // 的ControlWidget类继承自QTextEdit

    this->_text_edit = nullptr;
    this->_plain_edit = nullptr;

    this->hide_timer_on = hide_timer_on;
    this->tip = QString();
    this->_hide_timer = new QBasicTimer;
    this->_text_edit = text_edit;

    this->setFont(text_edit->document()->defaultFont());
    this->setForegroundRole(QPalette::ToolTipText);
    this->setBackgroundRole(QPalette::ToolTipBase);
    this->setPalette(QToolTip::palette());

    this->setAlignment(Qt::AlignLeft);
    this->setIndent(1);
    this->setFrameStyle(QFrame::NoFrame);
    this->setMargin(1 + this->style()->pixelMetric(
            QStyle::PM_ToolTipLabelFrameWidth, nullptr, this));
}

CallTipWidget::CallTipWidget(BaseEditMixin<QPlainTextEdit>* text_edit,bool hide_timer_on)
    : QLabel (nullptr,Qt::ToolTip)
{
    // 判断是否继承自QTextEdit是因为widgets/ipythonconsole/control.py
    // 的ControlWidget类继承自QTextEdit

    this->_text_edit = nullptr;
    this->_plain_edit = nullptr;

    this->hide_timer_on = hide_timer_on;
    this->tip = QString();
    this->_hide_timer = new QBasicTimer;
    this->_plain_edit = text_edit;

    this->setFont(text_edit->document()->defaultFont());
    this->setForegroundRole(QPalette::ToolTipText);
    this->setBackgroundRole(QPalette::ToolTipBase);
    this->setPalette(QToolTip::palette());

    this->setAlignment(Qt::AlignLeft);
    this->setIndent(1);
    this->setFrameStyle(QFrame::NoFrame);
    this->setMargin(1 + this->style()->pixelMetric(
            QStyle::PM_ToolTipLabelFrameWidth, nullptr, this));
}


bool CallTipWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (_text_edit) {
        if (obj == _text_edit) {
            QEvent::Type etype = event->type();

            if (etype == QEvent::KeyPress) {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                int key = keyEvent->key();
                QTextCursor cursor = _text_edit->textCursor();
                QString prev_char = _text_edit->get_character(cursor.position(),-1);

                if (key==Qt::Key_Enter || key==Qt::Key_Return ||
                        key==Qt::Key_Down || key==Qt::Key_Up)
                    hide();
                else if (key==Qt::Key_Escape) {
                    hide();
                    return true;
                }
                else if (prev_char == ")")
                    hide();
            }

            else if (etype == QEvent::FocusOut)
                hide();

            else if (etype == QEvent::Enter) {
                if (_hide_timer->isActive() &&
                        qApp->topLevelAt(QCursor::pos())==this)
                    _hide_timer->stop();
            }

            else if (etype == QEvent::Leave)
                this->_leave_event_hide();
        }
    }
    else if (_plain_edit) {
        if (obj == _plain_edit) {
            QEvent::Type etype = event->type();

            if (etype == QEvent::KeyPress) {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                int key = keyEvent->key();
                QTextCursor cursor = _plain_edit->textCursor();
                QString prev_char = _plain_edit->get_character(cursor.position(),-1);

                if (key==Qt::Key_Enter || key==Qt::Key_Return ||
                        key==Qt::Key_Down || key==Qt::Key_Up)
                    hide();
                else if (key==Qt::Key_Escape) {
                    hide();
                    return true;
                }
                else if (prev_char == ")")
                    hide();
            }

            else if (etype == QEvent::FocusOut)
                hide();

            else if (etype == QEvent::Enter) {
                if (_hide_timer->isActive() &&
                        qApp->topLevelAt(QCursor::pos())==this)
                    _hide_timer->stop();
            }

            else if (etype == QEvent::Leave)
                this->_leave_event_hide();
        }
    }

    return QLabel::eventFilter(obj, event);
}


void CallTipWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == _hide_timer->timerId()) {
        _hide_timer->stop();
        hide();
    }
}


void CallTipWidget::enterEvent(QEvent *event)
{
    QLabel::enterEvent(event);
    if (_hide_timer->isActive() &&
            qApp->topLevelAt(QCursor::pos())==this)
        _hide_timer->stop();
}

void CallTipWidget::hideEvent(QHideEvent *event)
{
    QLabel::hideEvent(event);
    if (_text_edit) {
        disconnect(_text_edit,SIGNAL(cursorPositionChanged()),
                   this,SLOT(_cursor_position_changed()));
        _text_edit->removeEventFilter(this);
    }
    else if (_plain_edit) {
        disconnect(_plain_edit,SIGNAL(cursorPositionChanged()),
                   this,SLOT(_cursor_position_changed()));
        _plain_edit->removeEventFilter(this);
    }
}


void CallTipWidget::leaveEvent(QEvent *event)
{
    QLabel::leaveEvent(event);
    _leave_event_hide();
}


void CallTipWidget::mousePressEvent(QMouseEvent *event)
{
    QLabel::mousePressEvent(event);
    hide();
}


void CallTipWidget::paintEvent(QPaintEvent *event)
{
    QStylePainter painter(this);
    QStyleOptionFrame option;
    option.initFrom(this);
    painter.drawPrimitive(QStyle::PE_PanelTipLabel, option);
    painter.end();

    QLabel::paintEvent(event);
}


void CallTipWidget::setFont(const QFont &font)
{
    QLabel::setFont(font);
}


void CallTipWidget::showEvent(QShowEvent *event)
{
    QLabel::showEvent(event);
    if (_text_edit) {
        connect(_text_edit,SIGNAL(cursorPositionChanged()),
                this,SLOT(_cursor_position_changed()));
        _text_edit->installEventFilter(this);
    }
    else if (_plain_edit) {
        connect(_plain_edit,SIGNAL(cursorPositionChanged()),
                this,SLOT(_cursor_position_changed()));
        _plain_edit->installEventFilter(this);
    }
}


void CallTipWidget::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    hide();
}


bool CallTipWidget::show_tip(QPoint point, const QString &tip, QStringList wrapped_tiplines)
{
    if (isVisible()) {
        if (this->tip == tip)
            return true;
        else
            hide();
    }

    if (_text_edit) {
        BaseEditMixin<QTextEdit>* text_edit = _text_edit;
        QTextCursor cursor = text_edit->textCursor();
        int search_pos = cursor.position()-1;
        _start_position = _find_parenthesis(search_pos,false).first;

        if (_start_position == -1)
            return false;

        if (hide_timer_on) {
            _hide_timer->stop();

            int hide_time;
            if (wrapped_tiplines.size() == 1) {
                QString args = wrapped_tiplines[0].split('(')[1];
                int nargs = (args.split(',')).size();
                if (nargs == 1)
                    hide_time = 1400;
                else if (nargs == 2)
                    hide_time = 1600;
                else
                    hide_time = 1800;
            }
            else if (wrapped_tiplines.size() == 2) {
                QString args1 = wrapped_tiplines[1].trimmed();
                int nargs1 = (args1.split(',')).size();
                if (nargs1 == 1)
                    hide_time = 2500;
                else
                    hide_time = 2800;
            }
            else
                hide_time = 3500;
            _hide_timer->start(hide_time, this);
        }

        this->tip = tip;
        setText(tip);
        resize(sizeHint());

        int padding = 3;
        QRect cursor_rect = text_edit->cursorRect(cursor);
        QRect screen_rect = qApp->desktop()->screenGeometry(text_edit);
        point.setY(point.y()+padding);
        int tip_height = size().height();
        int tip_width = size().width();

        QString vertical = "bottom";
        QString horizontal = "Right";
        if (point.y()+tip_height > screen_rect.height()+screen_rect.y()) {
            QPoint point_ = text_edit->mapToGlobal(cursor_rect.topRight());
            if (point_.y() - tip_height < padding) {
                if (2*point.y() < screen_rect.height())
                    vertical = "bottom";
                else
                    vertical = "top";
            }
            else
                vertical = "top";
        }
        if (point.x()+tip_width > screen_rect.width()+screen_rect.x()) {
            QPoint point_ = text_edit->mapToGlobal(cursor_rect.topRight());
            if (point_.x() - tip_width < padding) {
                if (2*point.x() < screen_rect.width())
                    horizontal = "Right";
                else
                    horizontal = "Left";
            }
            else
                horizontal = "Left";
        }

        QPoint pos;
        if (vertical=="top" && horizontal=="Left")
            pos = cursor_rect.topLeft();
        else if (vertical=="top" && horizontal=="Right")
            pos = cursor_rect.topRight();
        else if (vertical=="bottom" && horizontal=="Left")
            pos = cursor_rect.bottomLeft();
        else if (vertical=="bottom" && horizontal=="Right")
            pos = cursor_rect.bottomRight();

        QPoint adjusted_point = text_edit->mapToGlobal(pos);
        if (vertical == "top")
            point.setY(adjusted_point.y() - tip_height - padding);
        if (horizontal == "Left")
            point.setX(adjusted_point.x() - tip_width - padding);

        this->move(point);
    }
    else if (_plain_edit) {
        BaseEditMixin<QPlainTextEdit>* text_edit = _plain_edit;
        QTextCursor cursor = text_edit->textCursor();
        int search_pos = cursor.position()-1;
        _start_position = _find_parenthesis(search_pos,false).first;

        if (_start_position == -1)
            return false;

        if (hide_timer_on) {
            _hide_timer->stop();

            int hide_time;
            if (wrapped_tiplines.size() == 1) {
                QString args = wrapped_tiplines[0].split('(')[1];
                int nargs = (args.split(',')).size();
                if (nargs == 1)
                    hide_time = 1400;
                else if (nargs == 2)
                    hide_time = 1600;
                else
                    hide_time = 1800;
            }
            else if (wrapped_tiplines.size() == 2) {
                QString args1 = wrapped_tiplines[1].trimmed();
                int nargs1 = (args1.split(',')).size();
                if (nargs1 == 1)
                    hide_time = 2500;
                else
                    hide_time = 2800;
            }
            else
                hide_time = 3500;
            _hide_timer->start(hide_time, this);
        }

        this->tip = tip;
        setText(tip);
        resize(sizeHint());

        int padding = 3;
        QRect cursor_rect = text_edit->cursorRect(cursor);
        QRect screen_rect = qApp->desktop()->screenGeometry(text_edit);
        point.setY(point.y()+padding);
        int tip_height = size().height();
        int tip_width = size().width();

        QString vertical = "bottom";
        QString horizontal = "Right";
        if (point.y()+tip_height > screen_rect.height()+screen_rect.y()) {
            QPoint point_ = text_edit->mapToGlobal(cursor_rect.topRight());
            if (point_.y() - tip_height < padding) {
                if (2*point.y() < screen_rect.height())
                    vertical = "bottom";
                else
                    vertical = "top";
            }
            else
                vertical = "top";
        }
        if (point.x()+tip_width > screen_rect.width()+screen_rect.x()) {
            QPoint point_ = text_edit->mapToGlobal(cursor_rect.topRight());
            if (point_.x() - tip_width < padding) {
                if (2*point.x() < screen_rect.width())
                    horizontal = "Right";
                else
                    horizontal = "Left";
            }
            else
                horizontal = "Left";
        }

        QPoint pos;
        if (vertical=="top" && horizontal=="Left")
            pos = cursor_rect.topLeft();
        else if (vertical=="top" && horizontal=="Right")
            pos = cursor_rect.topRight();
        else if (vertical=="bottom" && horizontal=="Left")
            pos = cursor_rect.bottomLeft();
        else if (vertical=="bottom" && horizontal=="Right")
            pos = cursor_rect.bottomRight();

        QPoint adjusted_point = text_edit->mapToGlobal(pos);
        if (vertical == "top")
            point.setY(adjusted_point.y() - tip_height - padding);
        if (horizontal == "Left")
            point.setX(adjusted_point.x() - tip_width - padding);

		this->move(point);
    }

    this->show();
    return true;
}


QPair<int,int> CallTipWidget::_find_parenthesis(int position, bool forward)
{
    int commas=0, depth = 0;
    QTextDocument* document = nullptr;
    if (_text_edit)
        document = _text_edit->document();
    else if (_plain_edit) {
        document = _plain_edit->document();
    }
	if (!document)
	{
		return qMakePair(position, commas);
	}
    QChar _char = document->characterAt(position);

    int breakflag = 1;
    while (_char.category()!=QChar::Other_Control && position > 0) {
        if (_char == ',' && depth == 0)
            commas++;
        else if (_char == ')') {
            if (forward && depth == 0) {
                breakflag = 0;
                break;
            }
            depth++;
        }
        else if (_char == '(') {
            if (!forward && depth == 0) {
                breakflag = 0;
                break;
            }
            depth--;
        }
        position += (forward) ? 1 : -1;
        _char = document->characterAt(position);
    }
    if (breakflag)
        position--;
    return qMakePair(position, commas);
}


void CallTipWidget::_leave_event_hide()
{
    if (hide_timer_on && !_hide_timer->isActive() &&
            qApp->topLevelAt(QCursor::pos())!=this)
        _hide_timer->start(800, this);
}


void CallTipWidget::_cursor_position_changed()
{
    if (_text_edit) {
        QTextCursor cursor = _text_edit->textCursor();
        int position = cursor.position();
        QTextDocument* document = _text_edit->document();
        QChar _char = document->characterAt(position-1);
        if (position <= _start_position)
            this->hide();
        else if (_char == ')') {
            int pos = _find_parenthesis(position-1,false).first;
            if (pos == -1)
                this->hide();
        }
    }
    else if (_plain_edit) {
        QTextCursor cursor = _plain_edit->textCursor();
        int position = cursor.position();
        QTextDocument* document = _plain_edit->document();
        QChar _char = document->characterAt(position-1);
        if (position <= _start_position)
            this->hide();
        else if (_char == ')') {
            int pos = _find_parenthesis(position-1,false).first;
            if (pos == -1)
                this->hide();
        }
    }
}
