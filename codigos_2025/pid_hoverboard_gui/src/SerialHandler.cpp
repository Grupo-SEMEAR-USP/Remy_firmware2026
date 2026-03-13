#include "SerialHandler.h"
#include <QDebug>
#include <algorithm>
#include <cmath>

SerialHandler::SerialHandler(QObject *parent) : QObject(parent) {
    connect(&serial, &QSerialPort::readyRead, this, &SerialHandler::readData);
    connect(&pidTimer, &QTimer::timeout, this, &SerialHandler::updatePID);
    pidTimer.start(100); // 100 ms
}

void SerialHandler::connectPort(const QString &portName, int baudRate) {
    serial.setPortName(portName);
    serial.setBaudRate(baudRate);

    if (!serial.open(QIODevice::ReadWrite)) {
        qWarning() << "❌ Falha ao abrir porta serial:" << portName
                   << "-" << serial.errorString();
    } else {
        qDebug() << "✅ Porta serial conectada:" << portName;
    }
}

void SerialHandler::resetIntegrals() {
    integral1 = 0.0f;
    integral2 = 0.0f;
    prevError1 = 0.0f;
    prevError2 = 0.0f;
}

void SerialHandler::writeData(const QByteArray &data) {
    if (serial.isOpen()) {
        serial.write(data);
        qDebug() << "📤 Serial enviando:" << data.trimmed();
    } else {
        qWarning() << "❌ Porta serial não está aberta, não foi possível enviar!";
    }
}

void SerialHandler::readData() {
    while (serial.canReadLine()) {
        QByteArray line = serial.readLine().trimmed();

        if (line.startsWith("RPM,")) {
            QList<QByteArray> parts = line.mid(4).split(',');
            if (parts.size() == 2) {
                bool ok1 = false, ok2 = false;
                float r1 = parts[0].toFloat(&ok1);
                float r2 = parts[1].toFloat(&ok2);
                if (ok1 && ok2) {
                    rpm1 = r1;
                    rpm2 = r2;
                    emit rpmUpdated(rpm1, rpm2);
                }
            }
        }
    }
}

void SerialHandler::updatePID() {
    const float dt = 0.1f; // 100 ms

    // PID Motor 1
    float error1 = setpoint1 - rpm1;
    integral1 += error1 * dt;
    float derivative1 = (error1 - prevError1) / dt;
    float pwm1 = Kp1 * error1 + Ki1 * integral1 + Kd1 * derivative1;
    prevError1 = error1;

    // PID Motor 2
    float error2 = setpoint2 - rpm2;
    integral2 += error2 * dt;
    float derivative2 = (error2 - prevError2) / dt;
    float pwm2 = Kp2 * error2 + Ki2 * integral2 + Kd2 * derivative2;
    prevError2 = error2;

    // Saturação -255..255
    pwm1 = std::clamp(pwm1, -255.0f, 255.0f);
    pwm2 = std::clamp(pwm2, -255.0f, 255.0f);

    // Envia para a ESP usando versão interna (com sinal)
    sendPWM(static_cast<int>(pwm1), static_cast<int>(pwm2));
}

// Versão interna: recebe PWM com sinal, converte para dir + módulo
void SerialHandler::sendPWM(int pwm1, int pwm2) {
    if (!serial.isOpen())
        return;

    int dir1 = (pwm1 >= 0) ? 1 : 0;
    int dir2 = (pwm2 >= 0) ? 1 : 0;

    pwm1 = std::abs(pwm1);
    pwm2 = std::abs(pwm2);

    pwm1 = std::clamp(pwm1, 0, 255);
    pwm2 = std::clamp(pwm2, 0, 255);

    QString msg = QString("PWM,%1,%2,%3,%4\n")
                      .arg(pwm1)
                      .arg(dir1)
                      .arg(pwm2)
                      .arg(dir2);
    serial.write(msg.toUtf8());
}

// Versão pública: usa PWM e direção já calculados (se quiser zerar direto, etc.)
void SerialHandler::sendPWM(int pwm1, int dir1, int pwm2, int dir2) {
    if (!serial.isOpen())
        return;

    pwm1 = std::clamp(pwm1, 0, 255);
    pwm2 = std::clamp(pwm2, 0, 255);

    QString msg = QString("PWM,%1,%2,%3,%4\n")
                      .arg(pwm1)
                      .arg(dir1)
                      .arg(pwm2)
                      .arg(dir2);
    serial.write(msg.toUtf8());
}