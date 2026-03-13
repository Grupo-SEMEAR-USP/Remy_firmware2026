#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtGamepad/QGamepad>
#include <QtGamepad/QGamepadManager>
#include <QDebug>
#include <QTimer>
#include <cmath>

#include "SerialHandler.h"
#include "QInfoSlider.h"
#include "WheelWidget.h"

using namespace QtCharts;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("PID Hoverboard Controller");

    QVBoxLayout *mainLayout = new QVBoxLayout(&window);

    // ======= LOGO NO TOPO =======
    QPixmap logoPixmap("/home/sarah/Documentos/HOME/remy/firmware/pid_hoverboard_gui/resources/logo.png");
    QPixmap scaledLogo = logoPixmap.scaledToHeight(100, Qt::SmoothTransformation);

    QLabel *logoLabel = new QLabel();
    logoLabel->setPixmap(scaledLogo);
    logoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(logoLabel);

    // ======= SERIAL =======
    SerialHandler serial;
    serial.connectPort("/dev/ttyUSB0", 115200);

    QLabel *rpmLabel = new QLabel("RPM1: 0 | RPM2: 0");
    mainLayout->addWidget(rpmLabel);

    // ================= LAYOUT HORIZONTAL DOS DOIS MOTORES =================
    QHBoxLayout *motorsLayout = new QHBoxLayout();

    // ----------------- MOTOR 1 (lado esquerdo) -----------------
    QVBoxLayout *motor1Layout = new QVBoxLayout();

    QLabel *motor1Label = new QLabel(" MOTOR 1 ");
    motor1Label->setAlignment(Qt::AlignCenter);
    motor1Label->setStyleSheet("font-size: 20px; font-weight: bold; color: #222;");
    motor1Layout->addWidget(motor1Label);

    serial.Kp1 = 0.2f;
    serial.Ki1 = 0.11f;

    QInfoSlider *kp1Slider = new QInfoSlider("Kp1", 0, 10, serial.Kp1);
    kp1Slider->setEnabled(true);
    QInfoSlider *ki1Slider = new QInfoSlider("Ki1", 0, 1, serial.Ki1);
    ki1Slider->setEnabled(true);
    QInfoSlider *kd1Slider = new QInfoSlider("Kd1", 0, 1, 0);
    QInfoSlider *sp1Slider = new QInfoSlider("Setpoint1", 0, 500, 0);

    motor1Layout->addWidget(kp1Slider);
    motor1Layout->addWidget(ki1Slider);
    motor1Layout->addWidget(kd1Slider);
    motor1Layout->addWidget(sp1Slider);

    // Feedback visual motor 1
    WheelWidget *wheel1 = new WheelWidget();
    motor1Layout->addWidget(wheel1);

    // Gráfico motor 1
    QLineSeries *rpmSeries1 = new QLineSeries();
    QLineSeries *spSeries1 = new QLineSeries();
    rpmSeries1->setName("RPM1");
    spSeries1->setName("Setpoint1");

    QChart *chart1 = new QChart();
    chart1->addSeries(rpmSeries1);
    chart1->addSeries(spSeries1);
    chart1->createDefaultAxes();
    chart1->axisX()->setRange(0, 100);
    chart1->axisY()->setRange(0, 500);
    chart1->setTitle("Motor 1");

    motor1Layout->addWidget(new QChartView(chart1));

    // ----------------- MOTOR 2 (lado direito) -----------------
    QVBoxLayout *motor2Layout = new QVBoxLayout();

    QLabel *motor2Label = new QLabel(" MOTOR 2 ");
    motor2Label->setAlignment(Qt::AlignCenter);
    motor2Label->setStyleSheet("font-size: 20px; font-weight: bold; color: #222;");
    motor2Layout->addWidget(motor2Label);

    serial.Kp2 = 0.2f;
    serial.Ki2 = 0.11f;

    QInfoSlider *kp2Slider = new QInfoSlider("Kp2", 0, 10, serial.Kp2);
    kp2Slider->setEnabled(true);
    QInfoSlider *ki2Slider = new QInfoSlider("Ki2", 0, 1, serial.Ki2);
    ki2Slider->setEnabled(true);
    QInfoSlider *kd2Slider = new QInfoSlider("Kd2", 0, 1, 0);
    QInfoSlider *sp2Slider = new QInfoSlider("Setpoint2", 0, 500, 0);

    motor2Layout->addWidget(kp2Slider);
    motor2Layout->addWidget(ki2Slider);
    motor2Layout->addWidget(kd2Slider);
    motor2Layout->addWidget(sp2Slider);

    // Feedback visual motor 2
    WheelWidget *wheel2 = new WheelWidget();
    motor2Layout->addWidget(wheel2);

    // Gráfico motor 2
    QLineSeries *rpmSeries2 = new QLineSeries();
    QLineSeries *spSeries2 = new QLineSeries();
    rpmSeries2->setName("RPM2");
    spSeries2->setName("Setpoint2");

    QChart *chart2 = new QChart();
    chart2->addSeries(rpmSeries2);
    chart2->addSeries(spSeries2);
    chart2->createDefaultAxes();
    chart2->axisX()->setRange(0, 100);
    chart2->axisY()->setRange(0, 500);
    chart2->setTitle("Motor 2");

    motor2Layout->addWidget(new QChartView(chart2));

    // Adiciona ambos ao layout principal lado a lado
    motorsLayout->addLayout(motor1Layout);
    motorsLayout->addLayout(motor2Layout);
    mainLayout->addLayout(motorsLayout);

    // ================= Atualização em tempo real =================
    QObject::connect(&serial, &SerialHandler::rpmUpdated, [&](float r1, float r2) {
        static int t = 0;
        t++;

        rpmLabel->setText(QString("RPM1: %1 | RPM2: %2")
                          .arg(r1, 0, 'f', 1).arg(r2, 0, 'f', 1));
        wheel1->setSpeed(r1);
        wheel2->setSpeed(r2);

        rpmSeries1->append(t, r1);
        spSeries1->append(t, serial.setpoint1);
        rpmSeries2->append(t, r2);
        spSeries2->append(t, serial.setpoint2);

        if (t > 100) { // mantém 100 amostras na tela
            chart1->axisX()->setRange(t - 100, t);
            chart2->axisX()->setRange(t - 100, t);
        }
    });

    // Atualiza PID params
    QObject::connect(kp1Slider, &QInfoSlider::valueChanged, [&](double v){ serial.Kp1 = static_cast<float>(v); });
    QObject::connect(ki1Slider, &QInfoSlider::valueChanged, [&](double v){ serial.Ki1 = static_cast<float>(v); });
    QObject::connect(kd1Slider, &QInfoSlider::valueChanged, [&](double v){ serial.Kd1 = static_cast<float>(v); });
    QObject::connect(sp1Slider, &QInfoSlider::valueChanged, [&](double v){ serial.setpoint1 = static_cast<float>(v); });

    QObject::connect(kp2Slider, &QInfoSlider::valueChanged, [&](double v){ serial.Kp2 = static_cast<float>(v); });
    QObject::connect(ki2Slider, &QInfoSlider::valueChanged, [&](double v){ serial.Ki2 = static_cast<float>(v); });
    QObject::connect(kd2Slider, &QInfoSlider::valueChanged, [&](double v){ serial.Kd2 = static_cast<float>(v); });
    QObject::connect(sp2Slider, &QInfoSlider::valueChanged, [&](double v){ serial.setpoint2 = static_cast<float>(v); });

    // =================================================================
    //              CONTROLE PS4 — ENVIA APENAS SETPOINTS
    //      Stick Esquerdo → Motor 2 | Stick Direito → Motor 1
    // =================================================================
    QGamepad *gamepad = nullptr;

    for (int id : QGamepadManager::instance()->connectedGamepads()) {
        gamepad = new QGamepad(id);
        qDebug() << "🎮 Controle PS4 conectado:" << gamepad->name();
        break;
    }

    if (gamepad) {
        QTimer *joyTimer = new QTimer();

        // valores suavizados de setpoint
        static double smoothSetpoint1 = 0.0; // Motor 1 (R3)
        static double smoothSetpoint2 = 0.0; // Motor 2 (L3)

        QObject::connect(joyTimer, &QTimer::timeout, [&]() {
            double leftY  = -gamepad->axisLeftY();   // Motor 2
            double rightY = -gamepad->axisRightY();  // Motor 1

            const double DEADZONE = 0.25;
            if (std::fabs(leftY)  < DEADZONE) leftY  = 0;
            if (std::fabs(rightY) < DEADZONE) rightY = 0;

            // ==========================
            //   STICKS NEUTROS → PARAR
            // ==========================
            if (leftY == 0 && rightY == 0) {
                serial.setpoint1 = 0;
                serial.setpoint2 = 0;

                serial.resetIntegrals();

                smoothSetpoint1 = 0;
                smoothSetpoint2 = 0;

                // 🔥 ESSA ERA A PARTE QUE FALTAVA:
                sp1Slider->setValue(0);
                sp2Slider->setValue(0);

                return;
            }

            const double MAX_RPM = 100.0;
            const double ACCEL_STEP = 15.0;

            double target1 = rightY * MAX_RPM; // Motor 1
            double target2 = leftY  * MAX_RPM; // Motor 2

            target1 = std::clamp(target1, -MAX_RPM, MAX_RPM);
            target2 = std::clamp(target2, -MAX_RPM, MAX_RPM);

            auto smooth = [&](double current, double target) {
                double diff = target - current;
                if (std::fabs(diff) < ACCEL_STEP) return target;
                if (diff > 0) diff = ACCEL_STEP;
                if (diff < 0) diff = -ACCEL_STEP;
                return current + diff;
            };

            smoothSetpoint1 = smooth(smoothSetpoint1, target1);
            smoothSetpoint2 = smooth(smoothSetpoint2, target2);

            // Atualiza sliders com módulo
            sp1Slider->setValue(std::fabs(smoothSetpoint1));
            sp2Slider->setValue(std::fabs(smoothSetpoint2));

            // Envia setpoints reais (COM SINAL!) para o PID
            serial.setpoint1 = static_cast<float>(smoothSetpoint1);
            serial.setpoint2 = static_cast<float>(smoothSetpoint2);

            qDebug() << "🎮 LeftY(M2):" << leftY
                    << "RightY(M1):" << rightY
                    << "| SP1:" << smoothSetpoint1
                    << "| SP2:" << smoothSetpoint2;
        });

        joyTimer->start(50); // atualiza a cada 50ms
    } else {
        qWarning() << "⚠ Nenhum controle PS4 detectado!";
    }

    // =================================================================
    window.show();
    return app.exec();
}