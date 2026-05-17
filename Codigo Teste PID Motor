#include <Arduino.h>
#include <HardwareSerial.h>
#include "driver/uart.h"

// --- Definição dos Pinos (ESP32) ---
#define PINO_RX  16
#define PINO_TX  17
#define PINO_DIR 4
#define ID_MOTOR 1

// ─────────────────────────────────────────
// 1. ESCRITA NO MOTOR (Movimento e PID)
// ─────────────────────────────────────────
void moverMotor(uint8_t idMotor, uint16_t posicao) {
  if (posicao > 4095) posicao = 4095;

  uint8_t pos_L = posicao & 0xFF;
  uint8_t pos_H = (posicao >> 8) & 0xFF;

  uint8_t length      = 5;
  uint8_t instruction = 0x03; // WRITE
  uint8_t address     = 0x2A; // Endereço da Posição Alvo

  uint32_t soma = idMotor + length + instruction + address + pos_L + pos_H;
  uint8_t checksum = (~soma) & 0xFF;

  byte pacote[] = { 0xFF, 0xFF, idMotor, length, instruction, address, pos_L, pos_H, checksum };
  Serial2.write(pacote, sizeof(pacote));
}

void ajustarPIDInterno(uint8_t id, uint8_t p, uint8_t d, uint8_t i) {
  uint8_t instruction = 0x03; // WRITE
  uint8_t startAddress = 21;  // Endereço inicial (Proporcional)
  uint8_t length = 6;         // Instrução(1) + Endereço(1) + P(1) + D(1) + I(1)

  uint32_t soma = id + length + instruction + startAddress + p + d + i;
  uint8_t checksum = (~soma) & 0xFF;

  byte pacote[] = { 0xFF, 0xFF, id, length, instruction, startAddress, p, d, i, checksum };

  Serial2.write(pacote, sizeof(pacote));
  Serial2.flush();
  delay(20); // Tempo para o motor salvar na RAM
}

// ─────────────────────────────────────────
// 2. LEITURA DO MOTOR (Telemetria)
// ─────────────────────────────────────────
int lerRegistrador(uint8_t idMotor, uint8_t address, uint8_t numBytes) {
  uint8_t length      = 4;
  uint8_t instruction = 0x02; // READ

  uint32_t soma = idMotor + length + instruction + address + numBytes;
  uint8_t checksum = (~soma) & 0xFF;

  byte pacote[] = { 0xFF, 0xFF, idMotor, length, instruction, address, numBytes, checksum };

  while (Serial2.available()) Serial2.read(); // Limpa buffer
  Serial2.write(pacote, sizeof(pacote));
  Serial2.flush();

  uint8_t totalEsperado = 6 + numBytes;
  uint32_t timeout = millis() + 50; 
  while (Serial2.available() < totalEsperado) {
    if (millis() > timeout) return -1;
  }

  uint8_t resposta[10];
  for (int i = 0; i < totalEsperado; i++) resposta[i] = Serial2.read();

  if (resposta[0] != 0xFF || resposta[1] != 0xFF) return -1;

  if (numBytes == 1) return resposta[5];
  else return resposta[5] | (resposta[6] << 8);
}

void lerDados() {
  int posicao     = lerRegistrador(ID_MOTOR, 56, 2);
  int velocidade  = lerRegistrador(ID_MOTOR, 58, 2);
  int carga       = lerRegistrador(ID_MOTOR, 60, 2);
  int tensao      = lerRegistrador(ID_MOTOR, 62, 1);
  int temperatura = lerRegistrador(ID_MOTOR, 63, 1);
  int corrente    = lerRegistrador(ID_MOTOR, 69, 2);

  Serial.println("\n  ┌─── Telemetria ───────────────────┐");

  if (posicao != -1) {
    Serial.print("  │ Posicao:     "); Serial.print(posicao);
    Serial.print(" raw  ("); Serial.print((posicao / 4095.0f) * 360.0f, 1); Serial.println(" graus)");
  } else Serial.println("  │ Posicao:     ERRO");

  if (velocidade != -1) {
    Serial.print("  │ Velocidade:  ");
    if (velocidade & 0x8000) Serial.print("-");
    Serial.print(velocidade & 0x7FFF); Serial.println(" steps/s");
  } else Serial.println("  │ Velocidade:  ERRO");

  if (carga != -1) {
    int cargaVal = carga & 0x3FF;
    Serial.print("  │ Carga:       ");
    if (carga & 0x8000) Serial.print("-");
    Serial.print(cargaVal); Serial.print(" raw (");
    Serial.print((cargaVal / 1000.0f) * 100.0f, 1); Serial.println("%)");
  } else Serial.println("  │ Carga:       ERRO");

  if (tensao != -1) {
    Serial.print("  │ Tensao:      ");
    Serial.print(tensao / 10.0f, 1); Serial.println(" V");
  } else Serial.println("  │ Tensao:      ERRO");

  if (temperatura != -1) {
    Serial.print("  │ Temperatura: ");
    Serial.print(temperatura); Serial.println(" C");
  } else Serial.println("  │ Temperatura: ERRO");

  if (corrente != -1) {
    Serial.print("  │ Corrente:    ");
    Serial.print(corrente * 6.5f, 1); Serial.println(" mA");
  } else Serial.println("  │ Corrente:    ERRO");

  Serial.println("  └──────────────────────────────────┘\n");
}

// ─────────────────────────────────────────
// 3. FUNÇÃO DE TESTE GRÁFICO (Degrau PID)
// ─────────────────────────────────────────
void testeDegrauPID(uint16_t posicaoAlvo, const char* label) {
  // Dá um tempo para o usuário ler o label no Monitor Serial (se estiver usando)
  Serial.print(">>> Iniciando Teste: "); Serial.println(label);
  delay(1000); 

  moverMotor(ID_MOTOR, posicaoAlvo);

  // Fica lendo a posição loucamente por 1.5 segundos para desenhar o gráfico
  uint32_t tempoInicio = millis();
  while (millis() - tempoInicio < 1500) {
    int posicaoAtual = lerRegistrador(ID_MOTOR, 56, 2);
    
    // Se der erro na leitura, repete o alvo só pra não quebrar a linha do gráfico
    if (posicaoAtual == -1) posicaoAtual = posicaoAlvo; 

    // Imprime no formato exato que o Plotter Serial da Arduino IDE exige:
    Serial.print("Alvo:"); Serial.print(posicaoAlvo);
    Serial.print(",");
    Serial.print("Posicao_Real:"); Serial.println(posicaoAtual);
    
    delay(15); // Evita travar a porta serial com excesso de dados
  }
}

// ─────────────────────────────────────────
// SETUP E LOOP PRINCIPAL
// ─────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while(!Serial) { delay(10); }

  Serial2.begin(1000000, SERIAL_8N1, PINO_RX, PINO_TX);
  uart_set_pin(UART_NUM_2, PINO_TX, PINO_RX, PINO_DIR, UART_PIN_NO_CHANGE);
  uart_set_mode(UART_NUM_2, UART_MODE_RS485_HALF_DUPLEX);

  Serial.println("\n--- Sistema Iniciado ---");
  delay(1000);

  // Lê como o motor está agora antes de começar a se mexer
  Serial.println("Estado inicial do motor:");
  lerDados();
  delay(2000);
}

void loop() {
  // --- TESTE 1: Movimento SUAVE (P baixo) ---
  ajustarPIDInterno(ID_MOTOR, 15, 0, 0); 
  testeDegrauPID(3000, "Movimento Suave (P=10, D=0.1, I=1)");
  delay(1000); // Pausa antes de voltar
  testeDegrauPID(1000, "Voltando (Suave)");
  delay(1000); 


  
  // Imprime os dados atuais para ver como estão a temperatura/tensão após o esforço
  Serial.println("\n>>> Fim do Ciclo de Testes. Verificando Saúde do Motor:");
  lerDados();
  delay(5000); // Espera 5 segundos antes de repetir todo o loop
}
