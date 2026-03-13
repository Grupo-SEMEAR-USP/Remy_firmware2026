//Esse codigo controla dois motores trifasicos por PWM e printa a velocidade em RPM de cada um
//Cada motor possui 3 halls, que alteram em valores binarios (0 ou 1) dependendo do grau de rotaçao atual
//A cada 60 graus, um dos halls muda de valor, cada mudança dessa foi chamada de "etapa"
//6 etapas correspondem a 1 rotaçao eletrica do motor
//Como o motor possui 15 pares de polos magneticos, 15 rotaçoes eletricas correspondem a 1 rotaçao mecanica (uma volta completa)

//motor 1:
#define ha1_pin 18 
#define hb1_pin 19 //pinos dos halls do motor 1
#define hc1_pin 21
#define pwm1 4 //pino de pwm do motor 1

//motor 2:
#define ha2_pin 25
#define hb2_pin 26 //pinos dos halls do motor 2
#define hc2_pin 27 
#define pwm2 12 //pino de pwm do motor 2

volatile long etapas_1 = 0; //soma o total de mudanças de estados dos sensores halls (0 pra 1 ou 1 pra 0) do motor 1
volatile long etapas_2 = 0; //soma o total de mudanças de estados dos sensores halls (0 pra 1 ou 1 pra 0) do motor 2

volatile bool mudou_hall_1 = false; //flags que indicam que os sensores mudaram desde a ultima leitura
volatile bool mudou_hall_2 = false;

void IRAM_ATTR mudar_ha1() {
  mudou_hall_1 = true; }

void IRAM_ATTR mudar_hb1() {
  mudou_hall_1 = true; }

void IRAM_ATTR mudar_hc1() { //essas funcoes sao chamadas sempre que algum dos halls muda de estado 
  mudou_hall_1 = true; }     //sempre que sao chamadas, elas ativam a flag "mudou_hall_x"

void IRAM_ATTR mudar_ha2() {
  mudou_hall_2 = true; }

void IRAM_ATTR mudar_hb2() {
  mudou_hall_2 = true; }

void IRAM_ATTR mudar_hc2() {
  mudou_hall_2 = true; }

void setup() {
  
  Serial.begin(9600); //colocar o monitor serial em 9600 pra ver os dados

  pinMode(ha1_pin, INPUT);
  pinMode(hb1_pin, INPUT); //os halls sao entrada e os pwm sao saída
  pinMode(hc1_pin, INPUT);
  pinMode(pwm1, OUTPUT);

  pinMode(ha2_pin, INPUT);
  pinMode(hb2_pin, INPUT);
  pinMode(hc2_pin, INPUT);
  pinMode(pwm2, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(ha1_pin), mudar_ha1, CHANGE); //essas funcoes configuram as funcoes "void IRAM_ATTR mudar_hx"
  attachInterrupt(digitalPinToInterrupt(hb1_pin), mudar_hb1, CHANGE); //elas ligam cada pino com a sua funcao de interrupcao
  attachInterrupt(digitalPinToInterrupt(hc1_pin), mudar_hc1, CHANGE);

  attachInterrupt(digitalPinToInterrupt(ha2_pin), mudar_ha2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hb2_pin), mudar_hb2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(hc2_pin), mudar_hc2, CHANGE);

}

void loop() {

  analogWrite(pwm1, 20); //Controlar velocidade do motor 1      <--------------------- AQUI MUDAS AS VELOCIDADESSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS
  analogWrite(pwm2, 40); //Controlar velocidade do motor 2      (Vai de 0 a 255, mas pelo amor de deus nao passem de 80)

  unsigned long agora = millis(); //tempo em ms desde quando o codigo começou a rodar

  static unsigned long antes_1 = millis(); //ultima medicao de cada motor 
  static unsigned long antes_2 = millis();

  int ha1, hb1, hc1, ha2, hb2, hc2; //variaveis para os halls

  //Proteção contra condição de corrida para o motor 1
  bool mudou_1;
  noInterrupts();
  mudou_1 = mudou_hall_1; 
  if (mudou_hall_1) {
    mudou_hall_1 = false; //volta a flag pra false
  }
  interrupts();

  if (mudou_1) { 
    noInterrupts(); 
    etapas_1++; //soma +1 no total de mudanças de halls do motor 1
    interrupts();
  }

  // Proteção contra condição de corrida para o motor 2
  bool mudou_2;
  noInterrupts();
  mudou_2 = mudou_hall_2; 
  if (mudou_hall_2) {
    mudou_hall_2 = false; //volta a flag pra false
  }
  interrupts();

  if (mudou_2) {
    noInterrupts();
    etapas_2++; //soma +1 no total de mudanças de halls do motor 2
    interrupts();
  }

  // Cálculo de velocidade do motor 1 (a cada 1 segundo) em rpm
  if (agora - antes_1 >= 1000) { //a cada 1 segundo
    antes_1 = agora; //atualiza a variavel antes do motor 1
    float rpm_eletrico_1 = (etapas_1 / 6.0) * 60.0; // 6 etapas por rotação elétrica
    float rpm_mecanico_1 = rpm_eletrico_1 / 15.0;    // 15 pares de polos

    Serial.print("Velocidade Motor 1: ");
    Serial.print(rpm_mecanico_1); //printa a velocidade do motor 1 em rpm no monitor serial
    Serial.println(" RPM");

    etapas_1 = 0; //reseta a soma de mudanças dos halls do motor 1
  }

  // Cálculo de velocidade do motor 2 (a cada 1 segundo)
  if (agora - antes_2 >= 1000) { //a cada um segundo
    antes_2 = agora; //atualiza a variavel antes do motor 2
    float rpm_eletrico_2 = (etapas_2 / 6.0) * 60.0; // 6 etapas por rotação elétrica
    float rpm_mecanico_2 = rpm_eletrico_2 / 15.0;    // 15 pares de polos

    Serial.print("Velocidade Motor 2: ");
    Serial.print(rpm_mecanico_2); //printa a velocidade do motor 2 em rpm no monitor serial
    Serial.println(" RPM");

    etapas_2 = 0; //reseta a soma de mudanças dos halls do motor 2
  }

  
}