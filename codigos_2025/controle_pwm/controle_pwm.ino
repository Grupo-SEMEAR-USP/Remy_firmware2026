// ===============================================================
// Controle de 2 motores BLDC via PWM e feedback de Halls
// Contagem precisa de transições HALL (conta cada transição no ISR)
// ===============================================================

#define ha1_pin 18
#define hb1_pin 19
#define hc1_pin 21
#define pwm1_pin 4
#define dir1_pin 23

#define ha2_pin 25
#define hb2_pin 26
#define hc2_pin 27
#define pwm2_pin 32
#define dir2_pin 13

const int pares_de_polos = 15;
const int etapas_por_rotacao_eletrica = 6;
const unsigned long isr_deadtime_us = 200; // deadtime para evitar contagens muito próximas

unsigned long ultimo_comando_valido = 0; 
const unsigned long TIMEOUT_SEGURANCA = 500;

volatile unsigned long etapas_1 = 0;
volatile unsigned long etapas_2 = 0;

// armazenar último tempo (micros) e último estado por motor para filtrar e contar mudanças reais
volatile unsigned long last_isr_us_1 = 0;
volatile unsigned long last_isr_us_2 = 0;
volatile uint8_t last_state_1 = 0;
volatile uint8_t last_state_2 = 0;

// lê sinais HALL em forma de 3-bit (100..111 -> 0..7)
static inline uint8_t read_hall_state_1() {
  return (digitalRead(ha1_pin) << 2) | (digitalRead(hb1_pin) << 1) | digitalRead(hc1_pin);
}
static inline uint8_t read_hall_state_2() {
  return (digitalRead(ha2_pin) << 2) | (digitalRead(hb2_pin) << 1) | digitalRead(hc2_pin);
}

// === Interrupções Motor 1 ===
void IRAM_ATTR isr_ha1() {
  unsigned long now = micros();
  if ((now - last_isr_us_1) < isr_deadtime_us) return; // filtro de deadtime
  uint8_t s = (digitalRead(ha1_pin) << 2) | (digitalRead(hb1_pin) << 1) | digitalRead(hc1_pin);
  if (s != last_state_1) {
    last_state_1 = s;
    etapas_1++;
    last_isr_us_1 = now;
  }
}
void IRAM_ATTR isr_hb1() { isr_ha1(); } // reusa a mesma lógica
void IRAM_ATTR isr_hc1() { isr_ha1(); }

// === Interrupções Motor 2 ===
void IRAM_ATTR isr_ha2() {
  unsigned long now = micros();
  if ((now - last_isr_us_2) < isr_deadtime_us) return;
  uint8_t s = (digitalRead(ha2_pin) << 2) | (digitalRead(hb2_pin) << 1) | digitalRead(hc2_pin);
  if (s != last_state_2) {
    last_state_2 = s;
    etapas_2++;
    last_isr_us_2 = now;
  }
}
void IRAM_ATTR isr_hb2() { isr_ha2(); }
void IRAM_ATTR isr_hc2() { isr_ha2(); }

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);

  pinMode(ha1_pin, INPUT);
  pinMode(hb1_pin, INPUT);
  pinMode(hc1_pin, INPUT);
  pinMode(pwm1_pin, OUTPUT);
  pinMode(dir1_pin, OUTPUT);

  pinMode(ha2_pin, INPUT);
  pinMode(hb2_pin, INPUT);
  pinMode(hc2_pin, INPUT);
  pinMode(pwm2_pin, OUTPUT);
  pinMode(dir2_pin, OUTPUT);

  // ler e inicializar último estado antes de ativar as ISRs
  last_state_1 = read_hall_state_1();
  last_state_2 = read_hall_state_2();

  attachInterrupt(digitalPinToInterrupt(ha1_pin), isr_ha1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hb1_pin), isr_hb1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hc1_pin), isr_hc1, CHANGE);

  attachInterrupt(digitalPinToInterrupt(ha2_pin), isr_ha2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hb2_pin), isr_hb2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hc2_pin), isr_hc2, CHANGE);

  analogWrite(pwm1_pin, 0);
  analogWrite(pwm2_pin, 0);
  digitalWrite(dir1_pin, HIGH);
  digitalWrite(dir2_pin, HIGH);

  ultimo_comando_valido = millis();

  Serial.println("READY");
}

void loop() {
  // === Recebe comandos da GUI (mantive seu parser intacto) ===
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("PWM")) {
      int sep1 = cmd.indexOf(',');
      int sep2 = cmd.indexOf(',', sep1 + 1);
      int sep3 = cmd.indexOf(',', sep2 + 1);
      int sep4 = cmd.indexOf(',', sep3 + 1);
      if (sep4 > 0) {
        int pwm1 = cmd.substring(sep1 + 1, sep2).toInt();
        int dir1 = cmd.substring(sep2 + 1, sep3).toInt();
        int pwm2 = cmd.substring(sep3 + 1, sep4).toInt();
        int dir2 = cmd.substring(sep4 + 1).toInt();
        pwm1 = constrain(pwm1, 0, 255);
        pwm2 = constrain(pwm2, 0, 255);
        digitalWrite(dir1_pin, dir1 ? LOW : HIGH);
        digitalWrite(dir2_pin, dir2 ? HIGH : LOW);
        analogWrite(pwm1_pin, pwm1);
        analogWrite(pwm2_pin, pwm2);

        ultimo_comando_valido = millis();
      }
    }
  }

  if (millis() - ultimo_comando_valido > TIMEOUT_SEGURANCA) {
      analogWrite(pwm1_pin, 0);
      analogWrite(pwm2_pin, 0);
  }

  // === PRINT DAS LEITURAS DOS SENSORES HALL (a cada 50 ms) ===
  static unsigned long ultimo_print_hall = 0;
  if (millis() - ultimo_print_hall >= 50) {
    ultimo_print_hall = millis();

    int ha1 = digitalRead(ha1_pin);
    int hb1 = digitalRead(hb1_pin);
    int hc1 = digitalRead(hc1_pin);

    int ha2 = digitalRead(ha2_pin);
    int hb2 = digitalRead(hb2_pin);
    int hc2 = digitalRead(hc2_pin);

  //  Serial.printf("HALL1,%d,%d,%d | HALL2,%d,%d,%d\n",
  //                ha1, hb1, hc1,
  //                ha2, hb2, hc2);
  }

  // === Envia RPM a cada 100 ms (contagem correta feita nas ISRs) ===
  static unsigned long ultimo_envio = 0;
  unsigned long agora = millis();
  if (agora - ultimo_envio >= 100) {
    ultimo_envio = agora;

    noInterrupts();
    unsigned long e1 = etapas_1;
    unsigned long e2 = etapas_2;
    etapas_1 = 0;
    etapas_2 = 0;
    interrupts();

    // cada "etapa" aqui = 1 transição de estado HALL
    // rpm = (transicoes_por_100ms) * (600 / (etapas_por_rotacao_eletrica * pares_de_polos))
    float rpm_1 = e1 * (600.0f / (etapas_por_rotacao_eletrica * pares_de_polos));
    float rpm_2 = e2 * (600.0f / (etapas_por_rotacao_eletrica * pares_de_polos));

    Serial.printf("RPM,%.2f,%.2f\n", rpm_1, rpm_2);
  }

  delay(5);
}