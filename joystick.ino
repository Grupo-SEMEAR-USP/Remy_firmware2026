#include <Arduino.h>
#include <Bluepad32.h>
#include <string>

// Usar a entrada Brake do ZS-X11H? -- X = 0x0001 e R2 = 0x0080

// Esse código controla dois motores BLDC por meio de um controle genérico (compatibilidade na documentação do Bluepad32), acelerando para frente e para trás
// por meio do analógico esquerdo e se move para direita e esquerda por meio do analógico direito. Além disso, calculando a RPM dos motores no processo.


// ************************ Variáveis **************************** //

// Ponteiro de configuração dos controles

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Pinos do motor 1 na ESP-32

// PWM
const int PIN_PWM_1 = 4;
// Encoder
const int PIN_Ha_1 = 18;
const int PIN_Hb_1 = 19;
const int PIN_Hc_1 = 21;
// Direção
const int PIN_DIR_1 = 23;

// Pinos do motor 2 na ESP-32

// PWM
const int PIN_PWM_2 = 32;
// Encoder
const int PIN_Ha_2 = 25;
const int PIN_Hb_2 = 26;
const int PIN_Hc_2 = 27;
// Direção
const int PIN_DIR_2 = 13;

// Variáveis de leitura do estado atual do sensor hall do motor 1

volatile bool Ha1_Val = false;
volatile bool Hb1_Val = false;
volatile bool Hc1_Val = false;

// Variáveis de leitura do estado atual do sensor hall do motor 2

volatile bool Ha2_Val = false;
volatile bool Hb2_Val = false;
volatile bool Hc2_Val = false;

// Variáveis de leitura do sensor encoder

// Sentido de rotação

const int H = 1; // Horário
const int AH = -1; // Anti-horário

std::string R1;
std::string R2;

// Variáveis de armazenamento do motor 1

volatile int sentido1 = 1;
volatile int Pulsos1 = 0;

// Variáveis de armazenamento do motor 2

volatile int sentido2 = 1;
volatile int Pulsos2 = 0;

// Variáveis de interrupções do motor 1

volatile unsigned long tempo_prev1 = 0; // Marca o tempo de início da interrupção prévia
volatile unsigned long tempo_Ha1 = 100; // Guarda o tempo de duração de um pulso do Hall A
volatile unsigned long tempo_Hb1 = 100; // Guarda o tempo de duração de um pulso do Hall B
volatile unsigned long tempo_Hc1 = 100; // Guarda o tempo de duração de um pulso do Hall C

// Variáveis de interrupções do motor 2

volatile unsigned long tempo_prev2 = 0; // Marca o tempo de início da interrupção prévia
volatile unsigned long tempo_Ha2 = 100; // Guarda o tempo de duração de um pulso do Hall A
volatile unsigned long tempo_Hb2 = 100; // Guarda o tempo de duração de um pulso do Hall B
volatile unsigned long tempo_Hc2 = 100; // Guarda o tempo de duração de um pulso do Hall C

// Variáveis de leitura do motor 1

float PPM1 = 0; // Guarda quantos pulsos por minuto foram registrados
float RPM1 = 0; // Guarda quantas rotações por minuto foram registradas 

// Variáveis de leitura do motor 2

float PPM2 = 0; // Guarda quantos pulsos por minuto foram registrados
float RPM2 = 0; // Guarda quantas rotações por minuto foram registradas

// Variável para watchdog do controle
unsigned long lastControllerUpdate = 0;

// ****************************** Funções de Interrupção (ISRs) ******************************* //


// Motor 1

void IRAM_ATTR HS_A1() {
  unsigned long tempo_ini = millis(); // Usa o contador interno da esp32 para cronometrar o tempo inicial da contagem
  Ha1_Val = digitalRead(PIN_Ha_1); // Lê o estado atual do Hall Sensor A (High ou Low) definido como bool
  Hb1_Val = digitalRead(PIN_Hb_1); // Lê o estado atual do Hall Sensor B (High ou Low) definido como bool
  sentido1 = (Ha1_Val == Hb1_Val) ? H : AH; // Determina o sentido de rotação (Horário ou Anti-Horário) por meio de uma sentencia ternária
  Pulsos1 = Pulsos1 + (1 * sentido1); // Define a quantidade de pulsos em relação ao sentido de giro

  unsigned long delta = tempo_ini - tempo_prev1; // Intervalo de segurança entre os pulsos
  if (delta > 5 && delta < 5000) {
    tempo_Ha1 = delta; // tempo do pulso do Hall Sensor A
  }
  tempo_prev1 = tempo_ini; // Marca o tempo atual como tempo prévio, preparando para a próxima iteração
}

void IRAM_ATTR HS_B1() {
  unsigned long tempo_ini = millis(); // Usa o contador interno da esp32 para cronometrar o tempo inicial da contagem
  Hb1_Val = digitalRead(PIN_Hb_1); // Lê o estado atual do Hall Sensor B (High ou Low) definido como bool
  Hc1_Val = digitalRead(PIN_Hc_1); // Lê o estado atual do Hall Sensor C (High ou Low) definido como bool
  sentido1 = (Hb1_Val == Hc1_Val) ? H : AH; // Determina o sentido de rotação (Horário ou Anti-Horário) por meio de uma sentencia ternária
  Pulsos1 = Pulsos1 + (1 * sentido1); // Define a quantidade de pulsos em relação ao sentido de giro

  unsigned long delta = tempo_ini - tempo_prev1; // Intervalo de segurança entre os pulsos
  if (delta > 5 && delta < 5000) {
    tempo_Hb1 = delta; // tempo do pulso do Hall Sensor A
  }
  tempo_prev1 = tempo_ini; // Marca o tempo atual como tempo prévio, preparando para a próxima iteração
}

void IRAM_ATTR HS_C1() {
  unsigned long tempo_ini = millis(); // Usa o contador interno da esp32 para cronometrar o tempo inicial da contagem
  Hc1_Val = digitalRead(PIN_Hc_1); // Lê o estado atual do Hall Sensor C (High ou Low) definido como bool
  Ha1_Val = digitalRead(PIN_Ha_1); // Lê o estado atual do Hall Sensor A (High ou Low) definido como bool
  sentido1 = (Hc1_Val == Ha1_Val) ? H : AH; // Determina o sentido de rotação (Horário ou Anti-Horário) por meio de uma sentencia ternária
  Pulsos1 = Pulsos1 + (1 * sentido1); // Define a quantidade de pulsos em relação ao sentido de giro

  unsigned long delta = tempo_ini - tempo_prev1; // Intervalo de segurança entre os pulsos
  if (delta > 5 && delta < 5000) {
    tempo_Hc1 = delta; // tempo do pulso do Hall Sensor A
  }
  tempo_prev1 = tempo_ini; // Marca o tempo atual como tempo prévio, preparando para a próxima iteração
}

// Motor 2

void IRAM_ATTR HS_A2() {
  unsigned long tempo_ini = millis(); // Usa o contador interno da esp32 para cronometrar o tempo inicial da contagem
  Ha2_Val = digitalRead(PIN_Ha_2); // Lê o estado atual do Hall Sensor A (High ou Low) definido como bool
  Hb2_Val = digitalRead(PIN_Hb_2); // Lê o estado atual do Hall Sensor B (High ou Low) definido como bool
  sentido2 = (Ha2_Val == Hb2_Val) ? H : AH; // Determina o sentido de rotação (Horário ou Anti-Horário) por meio de uma sentencia ternária
  Pulsos2 = Pulsos2 + (1 * sentido2); // Define a quantidade de pulsos em relação ao sentido de giro

  unsigned long delta = tempo_ini - tempo_prev2; // Intervalo de segurança entre os pulsos
  if (delta > 5 && delta < 5000) {
    tempo_Ha2 = delta; // tempo do pulso do Hall Sensor A
  }
  tempo_prev2 = tempo_ini; // Marca o tempo atual como tempo prévio, preparando para a próxima iteração
}

void IRAM_ATTR HS_B2() {
  unsigned long tempo_ini = millis(); // Usa o contador interno da esp32 para cronometrar o tempo inicial da contagem
  Hb2_Val = digitalRead(PIN_Hb_2); // Lê o estado atual do Hall Sensor B (High ou Low) definido como bool
  Hc2_Val = digitalRead(PIN_Hc_2); // Lê o estado atual do Hall Sensor C (High ou Low) definido como bool
  sentido2 = (Hb2_Val == Hc2_Val) ? H : AH; // Determina o sentido de rotação (Horário ou Anti-Horário) por meio de uma sentencia ternária
  Pulsos2 = Pulsos2 + (1 * sentido2); // Define a quantidade de pulsos em relação ao sentido de giro

  unsigned long delta = tempo_ini - tempo_prev2; // Intervalo de segurança entre os pulsos
  if (delta > 5 && delta < 5000) {
    tempo_Hb2 = delta; // tempo do pulso do Hall Sensor A
  }
  tempo_prev2 = tempo_ini; // Marca o tempo atual como tempo prévio, preparando para a próxima iteração
}

void IRAM_ATTR HS_C2() {
  unsigned long tempo_ini = millis(); // Usa o contador interno da esp32 para cronometrar o tempo inicial da contagem
  Hc2_Val = digitalRead(PIN_Hc_2); // Lê o estado atual do Hall Sensor C (High ou Low) definido como bool
  Ha2_Val = digitalRead(PIN_Ha_2); // Lê o estado atual do Hall Sensor A (High ou Low) definido como bool
  sentido2 = (Hc2_Val == Ha2_Val) ? H : AH; // Determina o sentido de rotação (Horário ou Anti-Horário) por meio de uma sentencia ternária
  Pulsos2 = Pulsos2 + (1 * sentido2); // Define a quantidade de pulsos em relação ao sentido de giro

  unsigned long delta = tempo_ini - tempo_prev2; // Intervalo de segurança entre os pulsos
  if (delta > 5 && delta < 5000) {
    tempo_Hc2 = delta; // tempo do pulso do Hall Sensor A
  }
  tempo_prev2 = tempo_ini; // Marca o tempo atual como tempo prévio, preparando para a próxima iteração
}

// Variáveis de controle da Manobra em "S"
bool executandoManobraS = false;
unsigned long tempoInicioManobra = 0;
int estadoManobra = 0; // 0: Inativo, 1: Curva Direita, 2: Curva Esquerda

// ************************************ Função de Setup ************************************ //


void setup() {
  Serial.begin(115200); // 9600????????????????????
  Serial.println("Iniciando o programa...");

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();

  // Setup pinos sensor hall

  pinMode(PIN_Ha_1, INPUT);
  pinMode(PIN_Hb_1, INPUT);
  pinMode(PIN_Hc_1, INPUT);
  pinMode(PIN_Ha_2, INPUT);
  pinMode(PIN_Hb_2, INPUT);
  pinMode(PIN_Hc_2, INPUT);

  // Setup pinos PWM e DIR

  pinMode(PIN_PWM_1, OUTPUT);
  pinMode(PIN_PWM_2, OUTPUT);
  pinMode(PIN_DIR_1, OUTPUT);
  pinMode(PIN_DIR_2, OUTPUT);

  // Motores parados no início

  analogWrite(PIN_PWM_1, 0);
  analogWrite(PIN_PWM_2, 0);
  digitalWrite(PIN_DIR_1, LOW);
  digitalWrite(PIN_DIR_2, LOW);

  // Configuração de interrupção por bordas de subida e descida do motor 1

  attachInterrupt(digitalPinToInterrupt(PIN_Ha_1), HS_A1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_Hb_1), HS_B1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_Hc_1), HS_C1, CHANGE);

  // Configuração de interrupção por bordas de subida e descida do motor 2

  attachInterrupt(digitalPinToInterrupt(PIN_Ha_2), HS_A2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_Hb_2), HS_B2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_Hc_2), HS_C2, CHANGE);

  Serial.println("Programa iniciado!");
}


// ************************** Gerenciamento de controles ************************** //


// Procura se tem controles conectados, se tiver, uma flag é liberada com a mensagem informando a conexão e o modelo do controle

void onConnectedController(ControllerPtr ctl) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.printf("Controle está conectado, Player = %d \n", i);
      Serial.printf("Modelo do controle: %s \n", ctl -> getModelName().c_str());
      myControllers[i] = ctl;
      ctl->setColorLED(0,255,0); // LED verde
      foundEmptySlot = true;
      break;
    }
  }

  if (!foundEmptySlot) {
    Serial.printf("Controle desconectado, sem espaço para controles!");
  }
}

// Informa que o controle está desconectado ou não foi encontrado

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("Controle desconectado do Player = %d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController) {
        Serial.printf("Controle desconectado, mas não encontrado entre os espaços livres!");
    }
}


// **************************** Loop do serial monitor ****************************** //


void loop(){

  // Se a esp32 não receber dados do bluetooth do controle por 500ms ele força a parada do motor

  bool dataUpdated = BP32.update();

  if (dataUpdated) {
    lastControllerUpdate = millis();
  }

  if(millis() - lastControllerUpdate > 500) {
  analogWrite(PIN_PWM_1, 0);
  analogWrite(PIN_PWM_2, 0);
  }

  // Processa controles conectados
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    ControllerPtr ctl = myControllers[i];
    if (ctl && ctl->isConnected()) {
      processGamepad(ctl);
    }
  }

  // Cálculo do RPM

  unsigned long now = millis();
  if (now - tempo_prev1 > 600) {
    RPM1 = 0;
  } else{
    float media = (tempo_Ha1 + tempo_Hb1 + tempo_Hc1) / 3.0;
    if(media > 0) {
      float PPM1 = (60000.0 / media); // Pulsos por minuto, calculamos para 1s (1000ms) e multiplicamos por 60 (1min)
      RPM1 = PPM1 / 90.0; // 90 = PPR (pulsos por rotação) = n° de pares de polos do motor * 6, o motor do hover tem 15 pares de polos
    }
  }

  if (now - tempo_prev2 > 600) {
    RPM2 = 0;
  } else{
    float media = (tempo_Ha2 + tempo_Hb2 + tempo_Hc2) / 3.0;
    if(media > 0) {
      float PPM2 = (60000.0 / media); // Pulsos por minuto, calculamos para 1s (1000ms) e multiplicamos por 60 (1min)
      RPM2 = PPM2 / 90.0; // 90 = PPR (pulsos por rotação) = n° de pares de polos do motor * 6, o motor do hover tem 15 pares de polos
    }
  }

  // Print do sentido de rotação dos motores, sentido = 1 ==> Horário, sentido = -1 ==> Anti-horário (Para o motor 2 é invertido)

  if(sentido1 == 0) {R1 = "Horário";}
  else {R1 = "Anti-Horário";}

  if(sentido2 == 1) {R2 = "Horário";}
  else {R2 = "Anti-Horário";}

  // Print do RPM dos motores

  static unsigned long ultimo_print = 0;
  if(now - ultimo_print > 1000) {
    Serial.printf("RPM Motor 1: %.2f  |  RPM Motor 2: %.2f  |  Sentido Motor 1: %s  |  Sentido Motor 2: %s \n", RPM1, RPM2, R1.c_str(), R2.c_str());
    ultimo_print = now;
  }

  delay(10);
}

void processGamepad(ControllerPtr ctl) {

  // *** BOTÃO DE SEGURANÇA: R2 deve estar pressionado ***
  bool r2Pressionado = ctl->throttle() > 50;

  if (!r2Pressionado) {
    analogWrite(PIN_PWM_1, 0);
    analogWrite(PIN_PWM_2, 0);
    executandoManobraS = false;
    estadoManobra = 0;
    static unsigned long ultimo_aviso = 0;
    if (millis() - ultimo_aviso > 1000) {
        Serial.println("Segurança ativa: segure R2 para mover!");
        ultimo_aviso = millis();
    }        // ← fecha o if do aviso
    return;  // ← fora do if do aviso, mas dentro do if (!r2Pressionado)
  }  

  // Lê valores dos analógicos
  int axisY = ctl->axisY();  // Analógico esquerdo Y
  int axisRX = ctl->axisRX();  // Analógico direito X

  // 1. Verifica se o Triângulo foi apertado e inicia a manobra se estiver inativa
  if (ctl->y() && !executandoManobraS) {
    executandoManobraS = true;
    tempoInicioManobra = millis();
    estadoManobra = 1; // Inicia a primeira curva
    Serial.println("Iniciando Manobra em S!");
  }

  // 2. Se a manobra estiver rodando, executa ela e IGNORA o resto da função
  if (executandoManobraS) {
    executarManobraS();
    return; // O comando 'return' força a saída da função aqui, ignorando os analógicos abaixo
  }
  
  // Analógico esquerdo para cima

  int motorLeft = 0;
  int motorRight = 0;
  
  // Calcula velocidades base (frente/trás)

  if (axisY < -25) {
    // Para FRENTE (analógico para cima)
    int speed = map(axisY, -25, -508, 15, 100);
    motorLeft = -speed;
    motorRight = -speed;
  } 
  else if (axisY > 25) {
    // Para TRÁS (analógico para baixo)
    int speed = map(axisY, 25, 512, 15, 100);
    motorLeft = speed;
    motorRight = speed;
  }
  
  // Aplica curvas
  if (axisRX < -25) {
    // Curva ESQUERDA
    int turn = map(axisRX, -25, -508, 10, 60);
    motorLeft += turn;
    motorRight -= turn;
  } 
  else if (axisRX > 25) {
    // Curva DIREITA
    int turn = map(axisRX, 25, 512, 10, 60);
    motorLeft -= turn;
    motorRight += turn;
  }
  
  // Limita valores
  motorLeft = constrain(motorLeft, -255, 255);
  motorRight = constrain(motorRight, -255, 255);
  
  // Aplica ao MOTOR 1 (esquerdo)
  if (motorLeft > 0) {
    digitalWrite(PIN_DIR_1, LOW);
    analogWrite(PIN_PWM_1, motorLeft);
  } else if (motorLeft < 0) {
    digitalWrite(PIN_DIR_1, HIGH);
    analogWrite(PIN_PWM_1, -motorLeft);
  } else {
    analogWrite(PIN_PWM_1, 0);
  }
  
  // Aplica ao MOTOR 2 (direito)
  if (motorRight > 0) {
    digitalWrite(PIN_DIR_2, HIGH);
    analogWrite(PIN_PWM_2, motorRight);
  } else if (motorRight < 0) {
    digitalWrite(PIN_DIR_2, LOW);
    analogWrite(PIN_PWM_2, -motorRight);
  } else {
    analogWrite(PIN_PWM_2, 0);
  }
}

// ***************************************** Manobra em S ********************************** //

void executarManobraS() {
  unsigned long tempoAtual = millis();
  unsigned long tempoDecorrido = tempoAtual - tempoInicioManobra;

  // Tempos da manobra em milissegundos (você precisará ajustar testando no chão)
  const unsigned long TEMPO_CURVA_1 = 2000; // Duração da primeira perna do "S"
  const unsigned long TEMPO_CURVA_2 = 4000; // Tempo TOTAL para o fim do "S"

  int motorLeft = 0;
  int motorRight = 0;

  if (estadoManobra == 1) {
    // Fase 1: Curva para a Direita (Motor Esquerdo mais rápido que o Direito)
    motorLeft = 40;
    motorRight = 10; 

    if (tempoDecorrido > TEMPO_CURVA_1) {
      estadoManobra = 2; // Avança para a próxima curva
    }
  } 
  else if (estadoManobra == 2) {
    // Fase 2: Curva para a Esquerda (Motor Direito mais rápido que o Esquerdo)
    motorLeft = 10;
    motorRight = 40;

    if (tempoDecorrido > TEMPO_CURVA_2) {
      // Fim da manobra
      estadoManobra = 0;
      executandoManobraS = false; 
      motorLeft = 0;
      motorRight = 0;
    }
  }

  // Aplica as velocidades físicas aos motores usando a mesma lógica do seu código original
  
  // Motor 1 (Esquerdo) indo para FRENTE
  if (motorLeft > 0) {
    digitalWrite(PIN_DIR_1, LOW); 
    analogWrite(PIN_PWM_1, motorLeft);
  } else {
    analogWrite(PIN_PWM_1, 0);
  }

  // Motor 2 (Direito) indo para FRENTE
  if (motorRight > 0) {
    digitalWrite(PIN_DIR_2, HIGH); 
    analogWrite(PIN_PWM_2, motorRight);
  } else {
    analogWrite(PIN_PWM_2, 0);
  }
}
