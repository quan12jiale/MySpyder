#pragma once

#include <QtWidgets>

class QWaitingSpinner : public QWidget
{
    Q_OBJECT
public:
    bool _centerOnParent;
    bool _disableParentWhenSpinning;
    QColor _color;
    double _roundness;//圆度 是指工件的横截面接近理论圆的程度
    double _minimumTrailOpacity;//最小轨迹不透明度
    double _trailFadePercentage;//轨迹衰减百分比
    double _revolutionsPerSecond;//每秒钟转数
    int _numberOfLines;
    int _lineLength;
    int _lineWidth;
    int _innerRadius;
    int _currentCounter;
    bool _isSpinning;
    QTimer* _timer;
public:
    QWaitingSpinner(QWidget* parent, bool centerOnParent=true,
                    bool disableParentWhenSpinning=false,
                    Qt::WindowModality modality=Qt::NonModal);
    void start();
    void stop();

    void setNumberOfLines(int lines);
    void setLineLength(int length);
    void setLineWidth(int width);
    void setInnerRadius(int radius);

    QColor color() const { return _color; }
    double roundness() const { return _roundness; }
    double minimumTrailOpacity() const { return _minimumTrailOpacity; }
    double trailFadePercentage() const { return _trailFadePercentage; }
    double revolutionsPersSecond() const { return _revolutionsPerSecond; }
    int numberOfLines() const { return _numberOfLines; }
    int lineLength() const { return _lineLength; }
    int lineWidth() const { return _lineWidth; }
    int innerRadius() const { return _innerRadius; }
    bool isSpinning() const { return _isSpinning; }

    void setRoundness(double roundness) {_roundness = qMax(0.0, qMin(100.0,roundness));}
    void setColor(const QColor& color=Qt::black) { _color = QColor(color);}
    void setRevolutionsPerSecond(double revolutionsPerSecond) {_revolutionsPerSecond = revolutionsPerSecond;
                                                              updateTimer(); }
    void setTrailFadePercentage(double trail) {_trailFadePercentage = trail;}
    void setMinimumTrailOpacity(double minimumTrailOpacity) {_minimumTrailOpacity = minimumTrailOpacity;}

    void updateSize();
    void updateTimer();
    void updatePosition();
    int lineCountDistanceFromPrimary(int current,int primary,int totalNrOfLines) const;
    QColor currentLineColor(int countDistance,int totalNrOfLines,double trailFadePerc,
                            double minOpacity,const QColor& colorinput) const;
protected:
    void paintEvent(QPaintEvent *event) override;
public slots:
    void rotate();
};


class NavigatorSpinner : public QWaitingSpinner
{
    Q_OBJECT
public:
    NavigatorSpinner(QWidget* parent, int total_width=20);
};
