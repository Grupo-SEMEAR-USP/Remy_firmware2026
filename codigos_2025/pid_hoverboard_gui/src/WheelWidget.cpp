#include "WheelWidget.h"
#include <QGridLayout>
#include <QLabel>
#include <QtMath>

namespace {
    double lero(double a, double b, double c) { return a + c * (b - a); }
    quint8 clamp(double val, int min, int max) { return qBound(static_cast<double>(min), val, static_cast<double>(max)); }
}

static double old_difference = 0;
static bool inside_hysteresis = false;  // estado interno da histerese

WheelWidget::WheelWidget(QWidget *parent) : QWidget(parent)
{
    QGridLayout *layout = new QGridLayout(this);

    m_speedWidget = new QProgressBar(this);
   // m_targetSpeedWidget = new QInfoSlider("Target Speed (m/s)", 0.0, m_maxSpeed, 0.0, this);
   // m_propFactorWidget = new QInfoSlider("Kp", 0, 500, m_proportionalFactor, this);
   // m_intFactorWidget = new QInfoSlider("Ki", 0, 200, m_integralFactor, this);
   // m_diffFactorWidget = new QInfoSlider("Kd", 0, 10, m_differentialFactor, this);

    layout->addWidget(new QLabel("Velocidade Atual:"), 0, 0);
    layout->addWidget(m_speedWidget, 0, 1);
    //layout->addWidget(m_targetSpeedWidget, 1, 0, 1, 2);
    //layout->addWidget(m_propFactorWidget, 2, 0, 1, 2);
    //layout->addWidget(m_intFactorWidget, 3, 0, 1, 2);
    //layout->addWidget(m_diffFactorWidget, 4, 0, 1, 2);

   // connect(m_targetSpeedWidget, &QInfoSlider::valueChanged, this, &WheelWidget::onTargetSpeedChanged);
   // connect(m_propFactorWidget, &QInfoSlider::valueChanged, this, &WheelWidget::onPropFactorChanged);
   // connect(m_intFactorWidget, &QInfoSlider::valueChanged, this, &WheelWidget::onIntFactorChanged);
   // connect(m_diffFactorWidget, &QInfoSlider::valueChanged, this, &WheelWidget::onDiffFactorChanged);

    m_speedWidget->setRange(0, 100);
    m_speedWidget->setFormat("0.00 m/s");

    setLayout(layout);
}

void WheelWidget::regulate(double speed)
{
    m_currentSpeed = lero(m_currentSpeed, speed, 0.05);
    double difference = m_targetSpeed - m_currentSpeed;

    // ⚙️ Histerese adaptativa (zona morta entre 0.05 e 0.08 m/s)
    static const double hysteresis_inner = 1;  // entra na zona
    static const double hysteresis_outer = 1.5;  // sai da zona

    if (inside_hysteresis) {
        if (fabs(difference) > hysteresis_outer)
            inside_hysteresis = false;  // sai da zona neutra
        else
            difference = 0.0;           // ignora pequenas variações
    } else {
        if (fabs(difference) < hysteresis_inner) {
            inside_hysteresis = true;   // entra na zona neutra
            difference = 0.0;
        }
    }

    // 🚀 PID com histerese
    m_proportionalForce = difference * m_proportionalFactor;
    m_integralForce += difference * m_integralFactor / m_dataFrequency;
    m_differentialForce = (difference - old_difference) * m_differentialFactor * m_dataFrequency;
    old_difference = difference;

    m_totalForce = m_proportionalForce + m_integralForce + m_differentialForce;

    // Limita a faixa de saída (PWM)
    quint8 regbyte = clamp(m_totalForce, 0, 255);
    emit regulation(regbyte);
}

void WheelWidget::onWheelData(quint8 speedByte)
{
    double speed = static_cast<double>(speedByte) / 255.0 * m_maxSpeed;
    regulate(speed);
    m_speedWidget->setValue(static_cast<int>(100 * m_currentSpeed / m_maxSpeed));
    m_speedWidget->setFormat(QString("%1 m/s").arg(m_currentSpeed, 0, 'f', 2));
}

void WheelWidget::onTargetSpeedChanged(double val) { m_targetSpeed = val; }
void WheelWidget::onPropFactorChanged(double val) { m_proportionalFactor = val; }
void WheelWidget::onIntFactorChanged(double val) { m_integralFactor = val; }
void WheelWidget::onDiffFactorChanged(double val) { m_differentialFactor = val; }
void WheelWidget::onDataFrequency(double val) { m_dataFrequency = val; }

void WheelWidget::setSpeed(float rpm)
{
    double speed_mps = (rpm / 60.0) * m_wheelCircumference;
    m_currentSpeed = speed_mps;
    m_speedWidget->setValue(static_cast<int>(100 * m_currentSpeed / m_maxSpeed));
    m_speedWidget->setFormat(QString("%1 m/s").arg(m_currentSpeed, 0, 'f', 2));
}
