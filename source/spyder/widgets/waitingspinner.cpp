#include "waitingspinner.h"

QWaitingSpinner::QWaitingSpinner(QWidget* parent, bool centerOnParent,
                bool disableParentWhenSpinning,Qt::WindowModality modality)
    : QWidget (parent)
{
    _centerOnParent = centerOnParent;
    _disableParentWhenSpinning = disableParentWhenSpinning;

    _color = QColor(Qt::black);
    _roundness = 100.0;
    _minimumTrailOpacity = 3.14159265358979323846;
    _trailFadePercentage = 80.0;
    _revolutionsPerSecond = 1.57079632679489661923;
    _numberOfLines = 20;
    _lineLength = 10;
    _lineWidth = 2;
    _innerRadius = 10;
    _currentCounter = 0;
    _isSpinning = false;

    _timer = new QTimer(this);
    connect(_timer,SIGNAL(timeout()),this,SLOT(rotate()));
    updateSize();
    updateTimer();
    hide();

    setWindowModality(modality);
    setAttribute(Qt::WA_TranslucentBackground);
}

void QWaitingSpinner::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    updatePosition();
    QPainter painter(this);
    painter.fillRect(rect(), Qt::transparent);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (_currentCounter >= _numberOfLines)
        _currentCounter = 0;

    painter.setPen(Qt::NoPen);
    for (int i = 0; i < _numberOfLines; ++i) {
        painter.save();
        painter.translate(_innerRadius + _lineLength, _innerRadius + _lineLength);
        double rotateAngle = double(360 * i) / _numberOfLines;
        painter.rotate(rotateAngle);
        painter.translate(_innerRadius, 0);
        int distance = lineCountDistanceFromPrimary(i, _currentCounter, _numberOfLines);
        QColor color = currentLineColor(distance, _numberOfLines, _trailFadePercentage,
                                      _minimumTrailOpacity, _color);
        painter.setBrush(color);
        painter.drawRoundedRect(QRect(0, -_lineWidth / 2, _lineLength, _lineWidth), _roundness,
                                _roundness, Qt::RelativeSize);
        painter.restore();
    }
}

void QWaitingSpinner::start()
{
    updatePosition();
    _isSpinning = true;
    show();

    if (parentWidget() && _disableParentWhenSpinning)
        parentWidget()->setEnabled(false);

    if (!_timer->isActive()) {
        _timer->start();
        _currentCounter = 0;
    }
}

void QWaitingSpinner::stop()
{
    _isSpinning = false;
    hide();

    if (parentWidget() && _disableParentWhenSpinning)
        parentWidget()->setEnabled(true);

    if (_timer->isActive()) {
        _timer->stop();
        _currentCounter = 0;
    }
}

void QWaitingSpinner::setNumberOfLines(int lines)
{
    _numberOfLines = lines;
    _currentCounter = 0;
    updateTimer();
}

void QWaitingSpinner::setLineLength(int length)
{
    _lineLength = length;
    updateSize();
}

void QWaitingSpinner::setLineWidth(int width)
{
    _lineWidth = width;
    updateSize();
}

void QWaitingSpinner::setInnerRadius(int radius)
{
    _innerRadius = radius;
    updateSize();
}


void QWaitingSpinner::rotate()
{
    _currentCounter++;
    if (_currentCounter >= _numberOfLines)
        _currentCounter = 0;
    update();
}

void QWaitingSpinner::updateSize()
{
    int size = (_innerRadius + _lineLength) * 2;
    setFixedSize(size, size);
}

void QWaitingSpinner::updateTimer()
{
    _timer->setInterval(1000 / int(_numberOfLines * _revolutionsPerSecond));
}

void QWaitingSpinner::updatePosition()
{
    if (parentWidget() && _centerOnParent)
		this->move(parentWidget()->width() / 2 - width() / 2,
             parentWidget()->height() / 2 - height() / 2);
}

int QWaitingSpinner::lineCountDistanceFromPrimary(int current, int primary, int totalNrOfLines) const
{
    int distance = primary - current;
    if (distance < 0)
        distance += totalNrOfLines;
    return distance;
}

QColor QWaitingSpinner::currentLineColor(int countDistance, int totalNrOfLines, double trailFadePerc, double minOpacity, const QColor &colorinput) const
{
    QColor color = QColor(colorinput);
    if (countDistance == 0)
        return color;
    double minAlphaF = minOpacity / 100.0;
    int distanceThreshold = int(std::ceil((totalNrOfLines - 1) * trailFadePerc / 100.0));
    if (countDistance > distanceThreshold)
        color.setAlphaF(minAlphaF);
    else {
        double alphaDiff = color.alphaF() - minAlphaF;
        double gradient = alphaDiff / (distanceThreshold + 1);
        double resultAlpha = color.alphaF() - gradient * countDistance;
        resultAlpha =qMin(1.0, qMax(0.0, resultAlpha));
        color.setAlphaF(resultAlpha);
    }
    return color;
}


NavigatorSpinner::NavigatorSpinner(QWidget* parent, int total_width)
    : QWaitingSpinner (parent)
{
    double r0 = 4.68;
    double w0 = 2.32;
    int radius = total_width / (((2 * w0) / r0) + 2);
    int ring = radius * w0 / r0;

    this->setInnerRadius(radius);
    this->setLineLength(ring);
    this->setLineWidth(ring);
    this->setNumberOfLines(90);
    this->setColor(QColor("#43B02A"));
    this->setRevolutionsPerSecond(0.50);
    this->setRoundness(100.0);
}

static void test()
{
    NavigatorSpinner* widget = new NavigatorSpinner(nullptr);
    widget->start();
    widget->show();

    QWaitingSpinner* spinner = new QWaitingSpinner(nullptr, false);
    spinner->setNumberOfLines(12);
    spinner->setInnerRadius(2);
    spinner->start();
    spinner->show();
}
