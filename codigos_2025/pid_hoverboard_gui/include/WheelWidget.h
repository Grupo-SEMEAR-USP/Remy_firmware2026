#ifndef WHEELWIDGET_H
#define WHEELWIDGET_H

#include "QInfoSlider.h"
#include <QWidget>
#include <QProgressBar>

class WheelWidget : public QWidget
{
    Q_OBJECT

private:
    double m_maxSpeed = 6.0;            
    double m_wheelCircumference = 0.520; 
    double m_currentSpeed = 0.0;
    double m_targetSpeed = 0.0;
    double m_proportionalFactor = 100.0;
    double m_integralFactor = 50.0;
    double m_differentialFactor = 3.0;
    double m_dataFrequency = 400.0;     
    double m_pulsesPerRotation = 100.0; 

    double m_integralForce = 0.0;
    double m_proportionalForce = 0.0;
    double m_differentialForce = 0.0;
    double m_totalForce = 0.0;

    QProgressBar *m_speedWidget;
    QInfoSlider *m_targetSpeedWidget;
    QInfoSlider *m_propFactorWidget;
    QInfoSlider *m_intFactorWidget;
    QInfoSlider *m_diffFactorWidget;

public:
    explicit WheelWidget(QWidget *parent = nullptr);

    // ✅ Necessário para main.cpp
    void setSpeed(float rpm);

private:
    void regulate(double speed);

public slots:
    void onWheelData(quint8 speedByte);
    void onTargetSpeedChanged(double val);
    void onPropFactorChanged(double val);
    void onIntFactorChanged(double val);
    void onDiffFactorChanged(double val);
    void onDataFrequency(double val);

signals:
    void regulation(quint8 val);
};

#endif // WHEELWIDGET_H
