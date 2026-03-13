// HB_Motor_PID - Adaptado para ESP32 e motor 2 do hoverboard (ZS-X11H)
// Corrigido para evitar crash (uso seguro de micros() em ISR)

#define pb 34                // Botão de start/stop (opcional)
#define DIR_PIN_BT 35        // Botão de direção (opcional)
#define DIR_PIN_OUT 32       // Saída de direção (ZF/REV)
#define HALL_PIN 25          // Hall sensor (usando apenas HA)
#define PWM_PIN_OUT 33       // Saída PWM

volatile unsigned long tic = 0, tac = 0;
volatile float feedback = 0.0;     // feedback de velocidade
bool dir = 0;                      // direção, 0 = CW, 1 = CCW
float now, prvTime, dt;            // tempo
float P = 0.0, I = 0.0, D = 0.0;
float error = 0.0, prevErr = 0.0, maxSum = 50, errSum = 0.0, pid = 0.0;

float kp = 0.15, ki = 0.7, kd = 0.001;
float target = 50, trgt_min = 30, trgt_max = 60;
float fb_min = 4000, fb_max = 800; // micros entre pulsos (ajuste depois)

// Protótipo da ISR
void IRAM_ATTR intrupt();

void setup() {
  Serial.begin(115200);
  Serial.println("\n🔧 HB_Motor_PID adaptado para ESP32 (motor 2)");

  pinMode(pb, INPUT_PULLUP);
  pinMode(DIR_PIN_BT, INPUT_PULLUP);
  pinMode(DIR_PIN_OUT, OUTPUT);
  pinMode(PWM_PIN_OUT, OUTPUT);
  pinMode(HALL_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(HALL_PIN), intrupt, RISING);

  digitalWrite(DIR_PIN_OUT, HIGH); // frente
  delay(500);

  Serial.println("🟢 Pronto! O motor deve girar para frente automaticamente.");
}

void loop() {
  now = millis();
  dt = (now - prvTime) / 1000.0;
  prvTime = now;

  if (!digitalRead(DIR_PIN_BT)) {
    dir = !dir;
    delay(300);
    analogWrite(PWM_PIN_OUT, 0);
    digitalWrite(DIR_PIN_OUT, dir);
    Serial.print("↪ Direção alterada: ");
    Serial.println(dir ? "Reversa" : "Frente");
  }

  pid = PID();
  pid = constrain(pid, trgt_min, trgt_max);
  analogWrite(PWM_PIN_OUT, round(pid));

  Plotter();
}

// ISR segura
void IRAM_ATTR intrupt() {
  unsigned long nowMicros = micros();
  unsigned long diff = nowMicros - tac;
  tac = nowMicros;

  if (diff > 0 && diff < 100000) { // filtro básico contra ruído
    feedback = map(diff, fb_min, fb_max, trgt_min, trgt_max);
  }
}

// PID básico
float PID() {
  noInterrupts();
  float fb = feedback;
  interrupts();

  error = target - fb;

  P = kp * error;
  I += ki * error * dt;
  I = constrain(I, -maxSum, maxSum);
  D = kd * (error - prevErr) / dt;
  prevErr = error;

  return P + I + D;
}

// Plotter serial
void Plotter() {
  Serial.print(0);
  Serial.print("  ");
  Serial.print(feedback, 3);
  Serial.print("  ");
  Serial.print(pid, 3);
  Serial.print("  ");
  Serial.println(trgt_max, 0);
}
