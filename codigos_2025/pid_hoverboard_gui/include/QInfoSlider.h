#ifndef QINFOSLIDER_H
#define QINFOSLIDER_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>

class QInfoSlider : public QWidget
{
    Q_OBJECT

private:
    QSlider *m_slider;
    QLabel *m_label;
    double m_max = 100;
    double m_min = 0;
    double m_value = 0;

public:
    // ✅ Agora com min, max, valor inicial
    explicit QInfoSlider(const QString &name, double min = 0, double max = 100, double value = 0, QWidget *parent = nullptr);
    void setRange(double min, double max);
    void setValue(double value);
    double value() const { return m_value; }

signals:
    void valueChanged(double value);
};

#endif // QINFOSLIDER_H
