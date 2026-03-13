#pragma once
#include <QObject>
#include <QSerialPort>
#include <QTimer>

class SerialHandler : public QObject {
    Q_OBJECT

public:
    explicit SerialHandler(QObject *parent = nullptr);

    void connectPort(const QString &portName, int baudRate = 115200);

    void resetIntegrals();
    void writeData(const QByteArray &data);

    // Setpoints em RPM (com sinal: + frente, - ré)
    float setpoint1 = 0.0f;
    float setpoint2 = 0.0f;

    // Ganhos PID
    float Kp1 = 0.2f, Ki1 = 0.11f, Kd1 = 0.0f;
    float Kp2 = 0.2f, Ki2 = 0.11f, Kd2 = 0.0f;

    // Versão pública: apenas se você quiser mandar PWM "manual" (ex: zerar tudo)
    void sendPWM(int pwm1, int dir1, int pwm2, int dir2);

signals:
    void rpmUpdated(float rpm1, float rpm2);

private slots:
    void readData();
    void updatePID();

private:
    QSerialPort serial;
    QTimer pidTimer;

    float rpm1 = 0.0f, rpm2 = 0.0f;
    float integral1 = 0.0f, integral2 = 0.0f;
    float prevError1 = 0.0f, prevError2 = 0.0f;

    // Versão interna: recebe PWM com sinal, converte para módulo + direção
    void sendPWM(int pwm1, int pwm2);
};