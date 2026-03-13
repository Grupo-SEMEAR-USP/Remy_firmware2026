#include "QInfoSlider.h"

QInfoSlider::QInfoSlider(const QString &name, double min, double max, double value, QWidget *parent)
    : QWidget(parent), m_min(min), m_max(max), m_value(value)
{
    m_label = new QLabel(this);
    m_slider = new QSlider(Qt::Horizontal, this);

    m_slider->setRange(0, 100);
    m_slider->setValue(static_cast<int>((value - m_min) / (m_max - m_min) * 100.0));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel(name));
    layout->addWidget(m_slider);
    layout->addWidget(m_label);
    setLayout(layout);

    // Atualiza label e emite sinal quando muda
    connect(m_slider, &QSlider::valueChanged, this, [=](int val){
        m_value = static_cast<double>(val) / 100.0 * (m_max - m_min) + m_min;
        m_label->setText(QString::number(m_value, 'f', 2));
        emit valueChanged(m_value);
    });

    m_label->setText(QString::number(m_value, 'f', 2));
}

void QInfoSlider::setRange(double min, double max)
{
    m_min = min;
    m_max = max;
    setValue(m_value); // Atualiza exibição
}

void QInfoSlider::setValue(double value)
{
    m_value = value;
    int val = static_cast<int>((value - m_min) / (m_max - m_min) * 100.0);
    m_slider->setValue(val);
}
