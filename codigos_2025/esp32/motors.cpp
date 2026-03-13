//Esse codigo controla dois motores trifasicos por PWM e printa a velocidade em RPM de cada um
//Cada motor possui 3 halls, que alteram em valores binarios (0 ou 1) dependendo do grau de rotaçao atual
//A cada 60 graus, um dos halls muda de valor, cada mudança dessa foi chamada de "etapa"
//6 etapas correspondem a 1 rotaçao eletrica do motor
//Como o motor possui 15 pares de polos magneticos, 15 rotaçoes eletricas correspondem a 1 rotaçao mecanica (uma volta completa)

// ======= MOTOR 1 =======
#define ha1_pin 18 
#define hb1_pin 19 //pinos dos halls do motor 1
#define hc1_pin 21
#define pwm1 4     //pino de pwm do motor 1
#define dir1 5     //pino de direção do motor 1 (NOVO)

// ======= MOTOR 2 =======
#define ha2_pin 25
#define hb2_pin 26 //pinos dos halls do motor 2
#define hc2_pin 27 
#define pwm2 33    //pino de pwm do motor 2
#define dir2 23    //pino de direção do motor 2 (NOVO)

volatile long etapas_1 = 0;
volatile long etapas_2 = 0;

volatile bool mudou_hall_1 = false;
volatile bool mudou_hall_2 = false;

// ======== INTERRUPÇÕES ========
void IRAM_ATTR mudar_ha1() { mudou_hall_1 = true; }
void IRAM_ATTR mudar_hb1() { mudou_hall_1 = true; }
void IRAM_ATTR mudar_hc1() { mudou_hall_1 = true; }

void IRAM_ATTR mudar_ha2() { mudou_hall_2 = true; }
void IRAM_ATTR mudar_hb2() { mudou_hall_2 = true; }
void IRAM_ATTR mudar_hc2() { mudou_hall_2 = true; }

void setup() {
  Serial.begin(9600);

  // Entradas dos halls
  pinMode(ha1_pin, INPUT);
  pinMode(hb1_pin, INPUT);
  pinMode(hc1_pin, INPUT);
  pinMode(ha2_pin, INPUT);
  pinMode(hb2_pin, INPUT);
  pinMode(hc2_pin, INPUT);

  // Saídas PWM e direção
  pinMode(pwm1, OUTPUT);
  pinMode(pwm2, OUTPUT);
  pinMode(dir1, OUTPUT);
  pinMode(dir2, OUTPUT);

  // Direções iniciais
  digitalWrite(dir1, LOW);   // Motor 1 sentido normal
  digitalWrite(dir2, HIGH);  // Motor 2 sentido invertido  <--- ALTERA AQUI O SENTIDO

  // Interrupções dos halls
  attachInterrupt(digitalPinToInterrupt(ha1_pin), mudar_ha1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hb1_pin), mudar_hb1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hc1_pin), mudar_hc1, CHANGE);

  attachInterrupt(digitalPinToInterrupt(ha2_pin), mudar_ha2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hb2_pin), mudar_hb2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hc2_pin), mudar_hc2, CHANGE);
}

void loop() {
  // Controle de velocidade (0–255)
  analogWrite(pwm1, 70);
  analogWrite(pwm2, 60);

  unsigned long agora = millis();
  static unsigned long antes_1 = millis();
  static unsigned long antes_2 = millis();
  delay(3000);
  

  // ===== MOTOR 1 =====
  bool mudou_1;
  noInterrupts();
  mudou_1 = mudou_hall_1;
  mudou_hall_1 = false;
  interrupts();

  if (mudou_1) {
    noInterrupts();
    etapas_1++;
    interrupts();
  }

  // ===== MOTOR 2 =====
  bool mudou_2;
  noInterrupts();
  mudou_2 = mudou_hall_2;
  mudou_hall_2 = false;
  interrupts();

  if (mudou_2) {
    noInterrupts();
    etapas_2++;
    interrupts();
  }

  // ===== Cálculo de RPM motor 1 =====
  if (agora - antes_1 >= 1000) {
    antes_1 = agora;
    float rpm_eletrico_1 = (etapas_1 / 6.0) * 60.0;
    float rpm_mecanico_1 = rpm_eletrico_1 / 15.0;

    Serial.print("Velocidade Motor 1: ");
    Serial.print(rpm_mecanico_1);
    Serial.println(" RPM");

    etapas_1 = 0;
  }

  // ===== Cálculo de RPM motor 2 =====
  if (agora - antes_2 >= 1000) {
    antes_2 = agora;
    float rpm_eletrico_2 = (etapas_2 / 6.0) * 60.0;
    float rpm_mecanico_2 = rpm_eletrico_2 / 15.0;

    Serial.print("Velocidade Motor 2: ");
    Serial.print(rpm_mecanico_2);
    Serial.println(" RPM");

    etapas_2 = 0;
  }

  analogWrite(pwm1, 0);
  analogWrite(pwm2, 0);
  delay(3000);
}

int loop(){
    // Recebe as informacoes do serial controller -> /cmd_vel

    // Manda os dados do enconder e imu
    // Enconder m1, enconder m2, aceleracao x, aceleracao y, giroscopio z
}