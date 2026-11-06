/*
 * =====================================================================
 *  Braço do robô – CBR categoria @Home
 *  ESP32 + 1x DS5160 (PWM) + 2x Feetech ST3215 (barramento serial TTL)
 *
 *  Ombro    : DS5160            – PWM 50 Hz, 500–2500 us (sem realimentação)
 *  Cotovelo : Feetech ST3215 30 kg – ID 1
 *  Pulso    : Feetech ST3215 14 kg – ID 2
 *
 *  Barramento Feetech: UART2 a 1 Mbps, half-duplex pelos buffers tri-state
 *  (74HC125 + inversor). A direção é feita pelo RTS da UART (PINO_DIR),
 *  igual ao código de teste.
 *
 *  Comandos pelo Monitor Serial (115200 baud, fim de linha "Nova linha"):
 *    o <graus>            move o ombro
 *    c <graus>            move o cotovelo
 *    p <graus>            move o pulso
 *    m <o> <c> <p>        move as três juntas; todas chegam juntas
 *    h                    vai para a pose inicial (home)
 *    v <graus/s>          velocidade máxima das juntas
 *    t                    telemetria
 *    x                    congela tudo na posição atual
 *    s / l                solta / liga o torque dos Feetech
 *    scan                 procura Feetech no barramento (IDs 0–20)
 *    id <atual> <novo>    troca o ID de um Feetech (deixe SÓ ele no barramento)
 *    zero <id>            grava a posição física atual como centro (2048)
 *    demo                 demonstração em loop (gestos); qualquer comando para
 *
 *  Ângulos em graus no referencial de cada junta: 0 = centro calibrado.
 *
 *  PRIMEIRO USO – definir os IDs (os dois Feetech saem de fábrica com ID 1):
 *    1. Ligue SÓ o servo do pulso no barramento e digite: scan   (deve achar 1)
 *    2. Digite: id 1 2
 *    3. Ligue o cotovelo também e digite: scan   (deve achar 1 e 2)
 *    Nunca deixe os dois no barramento com o mesmo ID: as respostas colidem.
 * =====================================================================
 */

#include <Arduino.h>
#include "esp_arduino_version.h"
#include "driver/uart.h"

// ───────────────────────── Pinos ─────────────────────────
#define PINO_RX      16
#define PINO_TX      17
#define PINO_DIR      4
#define PINO_OMBRO   25        // <<< CONFIRMAR NA PCB: sinal do DS5160

// ───────────────────────── IDs Feetech ─────────────────────────
#define ID_COTOVELO   1
#define ID_PULSO      2

// ───────────────────────── DS5160 (ombro) ─────────────────────────
#define OMBRO_FAIXA_GRAUS   180.0f   // DS5160 versão 180°: 500–2500 us = -90° a +90°
#define OMBRO_US_MIN        500
#define OMBRO_US_MAX        2500
#define OMBRO_SENTIDO       1        // -1 inverte o sentido
#define OMBRO_OFFSET_GRAUS  0.0f     // ajuste fino do zero mecânico
#define PWM_FREQ_HZ         50
#define PWM_RES_BITS        16
#define PWM_CANAL           0        // usado só no core ESP32 2.x

// ───────────────────────── Feetech ─────────────────────────
#define STS_CENTRO          2048
#define PASSOS_POR_GRAU     (4096.0f / 360.0f)
#define COTOVELO_SENTIDO    1
#define PULSO_SENTIDO       1
#define ACEL_STS            30       // unidade de 100 passos/s² (0 = máxima)

// PID interno (opcional). Mude para 1 para aplicar na inicialização.
#define APLICAR_PID         0
const uint8_t PID_COTOVELO[3] = { 32, 32, 0 };   // P, D, I
const uint8_t PID_PULSO[3]    = { 32, 32, 0 };

// ───────────────────────── Limites de software (CALIBRAR!) ─────────────────────────
struct Limite { float min, max; };
const Limite LIM_OMBRO    = {  -90.0f,  90.0f };
const Limite LIM_COTOVELO = { -120.0f, 120.0f };
const Limite LIM_PULSO    = {  -90.0f,  90.0f };

// Pose inicial (o ombro recebe esse pulso logo ao ligar — veja ombroIniciar)
const float HOME_OMBRO    = 0.0f;
const float HOME_COTOVELO = 0.0f;
const float HOME_PULSO    = 0.0f;

// ───────────────────────── Segurança ─────────────────────────
#define CORRENTE_MAX_MA       2000.0f
#define TEMP_MAX_C            65
#define SOBRECARGA_SOLTAR     3      // leituras seguidas acima do limite até soltar o torque
#define FALHAS_COMUNIC_MAX    5
#define PERIODO_SEGURANCA_MS  200

float velMaxGrausS = 60.0f;

// Loop de teste: 1 = começa sozinho ao ligar; 0 = só com o comando "demo"
#define DEMO_AO_LIGAR       0
#define DEMO_AMPLITUDE      1.0f     // escala todos os ângulos do demo (0.5 = metade)
#define DEMO_RAPIDEZ        1.0f     // escala todas as velocidades do demo (0.5 = metade)

// ═════════════════════════════════════════════════════════════════════
//  1. PROTOCOLO FEETECH (STS)
// ═════════════════════════════════════════════════════════════════════
#define INST_PING        0x01
#define INST_READ        0x02
#define INST_WRITE       0x03
#define INST_SYNC_WRITE  0x83
#define ID_BROADCAST     0xFE

// Tabela de memória STS/ST3215
#define REG_ID             5
#define REG_P_COEF        21    // P, D, I em 21, 22, 23
#define REG_TORQUE        40    // 0 = solto, 1 = torque, 128 = calibra centro
#define REG_ACC           41
#define REG_GOAL_POS      42
#define REG_GOAL_TIME     44
#define REG_GOAL_SPEED    46
#define REG_LOCK          55    // 0 = EEPROM liberada, 1 = travada
#define REG_PRESENT_POS   56
#define REG_PRESENT_SPEED 58
#define REG_PRESENT_LOAD  60
#define REG_VOLTAGE       62
#define REG_TEMPERATURE   63
#define REG_MOVING        66
#define REG_CURRENT       69

uint8_t ultimoErroSts = 0;   // byte de erro do último pacote de status

void stsEnviar(uint8_t id, uint8_t instrucao, const uint8_t* params, uint8_t nParams) {
  if (nParams > 64) return;
  uint8_t pacote[6 + 64];
  pacote[0] = 0xFF;
  pacote[1] = 0xFF;
  pacote[2] = id;
  pacote[3] = nParams + 2;
  pacote[4] = instrucao;
  uint8_t soma = id + pacote[3] + instrucao;
  for (uint8_t i = 0; i < nParams; i++) {
    pacote[5 + i] = params[i];
    soma += params[i];
  }
  pacote[5 + nParams] = ~soma;

  while (Serial2.available()) Serial2.read();   // descarta lixo antigo
  Serial2.write(pacote, 6 + nParams);
  Serial2.flush();                              // espera sair tudo (RTS volta p/ RX)
}

// Lê um pacote de status: procura o cabeçalho, confere ID, tamanho e checksum.
bool stsReceber(uint8_t idEsperado, uint8_t* dados, uint8_t nDados, uint16_t timeoutMs = 10) {
  if (nDados > 16) return false;
  uint8_t estado = 0, id = 0, len = 0, idx = 0;
  uint8_t corpo[18];                            // erro + dados + checksum
  uint32_t t0 = millis();

  while (millis() - t0 < timeoutMs) {
    if (!Serial2.available()) continue;
    uint8_t b = Serial2.read();
    switch (estado) {
      case 0: if (b == 0xFF) estado = 1; break;
      case 1: estado = (b == 0xFF) ? 2 : 0; break;
      case 2:
        if (b == 0xFF) break;                   // 0xFF extra: continua no cabeçalho
        id = b; estado = 3; break;
      case 3:
        len = b;
        if (id != idEsperado || len != nDados + 2) return false;
        idx = 0; estado = 4; break;
      case 4:
        corpo[idx++] = b;
        if (idx == len) {
          uint8_t soma = id + len;
          for (uint8_t i = 0; i < len - 1; i++) soma += corpo[i];
          if ((uint8_t)~soma != corpo[len - 1]) return false;
          ultimoErroSts = corpo[0];
          for (uint8_t i = 0; i < nDados; i++) dados[i] = corpo[1 + i];
          return true;
        }
        break;
    }
  }
  return false;
}

bool stsPing(uint8_t id) {
  stsEnviar(id, INST_PING, nullptr, 0);
  return stsReceber(id, nullptr, 0);
}

// Escrita com espera da confirmação (evita colisão com a resposta do servo)
bool stsEscrever(uint8_t id, uint8_t endereco, const uint8_t* dados, uint8_t n) {
  if (n > 16) return false;
  uint8_t p[17];
  p[0] = endereco;
  memcpy(p + 1, dados, n);
  stsEnviar(id, INST_WRITE, p, n + 1);
  if (id == ID_BROADCAST) return true;
  return stsReceber(id, nullptr, 0);
}

bool stsEscrever1(uint8_t id, uint8_t endereco, uint8_t v) {
  return stsEscrever(id, endereco, &v, 1);
}

bool stsEscrever2(uint8_t id, uint8_t endereco, uint16_t v) {
  uint8_t d[2] = { (uint8_t)(v & 0xFF), (uint8_t)(v >> 8) };
  return stsEscrever(id, endereco, d, 2);
}

// Retorna o valor bruto (1 ou 2 bytes) ou -1 em caso de falha
int stsLer(uint8_t id, uint8_t endereco, uint8_t n) {
  uint8_t p[2] = { endereco, n };
  uint8_t d[2] = { 0, 0 };
  stsEnviar(id, INST_READ, p, 2);
  if (!stsReceber(id, d, n)) return -1;
  return (n == 1) ? d[0] : (d[0] | (d[1] << 8));
}

// Valores com sinal-magnitude (bit de sinal varia por registrador)
int sinalMagnitude(int bruto, uint8_t bitSinal) {
  int mag = bruto & ((1 << bitSinal) - 1);
  return (bruto & (1 << bitSinal)) ? -mag : mag;
}

// SYNC WRITE: ACC(1) POS(2) TEMPO(2) VEL(2) para vários servos num só pacote
void stsMoverSync(const uint8_t* ids, const uint16_t* pos, const uint16_t* vel, uint8_t n) {
  const uint8_t L = 7;
  uint8_t p[2 + 4 * (L + 1)];
  if (n > 4) return;
  uint8_t k = 0;
  p[k++] = REG_ACC;
  p[k++] = L;
  for (uint8_t i = 0; i < n; i++) {
    p[k++] = ids[i];
    p[k++] = ACEL_STS;
    p[k++] = pos[i] & 0xFF;  p[k++] = pos[i] >> 8;
    p[k++] = 0;              p[k++] = 0;            // tempo = 0 (usa velocidade)
    p[k++] = vel[i] & 0xFF;  p[k++] = vel[i] >> 8;
  }
  stsEnviar(ID_BROADCAST, INST_SYNC_WRITE, p, k);  // broadcast não tem resposta
}

void stsTorque(uint8_t id, bool ligar) {
  if (ligar) {
    // Antes de ligar, alvo = posição atual, para o servo não dar um tranco
    int pos = stsLer(id, REG_PRESENT_POS, 2);
    if (pos >= 0) stsEscrever2(id, REG_GOAL_POS, pos);
  }
  stsEscrever1(id, REG_TORQUE, ligar ? 1 : 0);
}

void stsAjustarPID(uint8_t id, const uint8_t pdi[3]) {
  stsEscrever(id, REG_P_COEF, pdi, 3);
}

void stsTrocarId(uint8_t atual, uint8_t novo) {
  if (!stsPing(atual)) { Serial.printf("ID %u não respondeu.\n", atual); return; }
  stsEscrever1(atual, REG_LOCK, 0);
  stsEscrever1(atual, REG_ID, novo);   // a resposta pode vir com o ID novo; ignorada
  delay(20);
  stsEscrever1(novo, REG_LOCK, 1);
  Serial.printf("ID %u -> %u : %s\n", atual, novo, stsPing(novo) ? "OK" : "FALHOU");
}

void stsCalibrarCentro(uint8_t id) {
  stsEscrever1(id, REG_LOCK, 0);
  stsEscrever1(id, REG_TORQUE, 128);   // posição atual passa a valer 2048
  delay(20);
  stsEscrever1(id, REG_LOCK, 1);
  Serial.printf("ID %u: centro gravado. Posição agora = %d\n", id, stsLer(id, REG_PRESENT_POS, 2));
}

uint16_t grausParaSts(float graus, int sentido) {
  long bruto = STS_CENTRO + lroundf(graus * sentido * PASSOS_POR_GRAU);
  return (uint16_t)constrain(bruto, 0L, 4095L);
}

float stsParaGraus(int bruto, int sentido) {
  return (bruto - STS_CENTRO) / PASSOS_POR_GRAU * sentido;
}

// ═════════════════════════════════════════════════════════════════════
//  2. OMBRO – DS5160 (PWM com rampa de velocidade por software)
// ═════════════════════════════════════════════════════════════════════
float ombroAtual = HOME_OMBRO;     // posição comandada neste instante
float ombroAlvo  = HOME_OMBRO;
float ombroVel   = 30.0f;          // graus/s da rampa em curso
uint32_t ombroUltimoMs = 0;

uint32_t ombroGrausParaUs(float graus) {
  float meio = (OMBRO_US_MIN + OMBRO_US_MAX) / 2.0f;
  float usPorGrau = (OMBRO_US_MAX - OMBRO_US_MIN) / OMBRO_FAIXA_GRAUS;
  float us = meio + (graus * OMBRO_SENTIDO + OMBRO_OFFSET_GRAUS) * usPorGrau;
  return (uint32_t)constrain(lroundf(us), (long)OMBRO_US_MIN, (long)OMBRO_US_MAX);
}

void ombroEscreverUs(uint32_t us) {
  const uint32_t periodoUs = 1000000UL / PWM_FREQ_HZ;
  uint32_t duty = (uint64_t)us * ((1UL << PWM_RES_BITS) - 1) / periodoUs;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PINO_OMBRO, duty);
#else
  ledcWrite(PWM_CANAL, duty);
#endif
}

void ombroIniciar() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PINO_OMBRO, PWM_FREQ_HZ, PWM_RES_BITS);
#else
  ledcSetup(PWM_CANAL, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(PINO_OMBRO, PWM_CANAL);
#endif
  // Servo PWM não informa onde está: o primeiro pulso o leva direto (e rápido)
  // para HOME_OMBRO. Ligue o robô com o ombro perto dessa pose.
  ombroEscreverUs(ombroGrausParaUs(ombroAtual));
  ombroUltimoMs = millis();
}

void ombroAtualizar() {
  uint32_t agora = millis();
  uint32_t dtMs = agora - ombroUltimoMs;
  if (dtMs < 20) return;                         // um passo por período do PWM
  ombroUltimoMs = agora;

  float passo = ombroVel * dtMs / 1000.0f;
  float erro  = ombroAlvo - ombroAtual;
  if (fabsf(erro) <= passo) ombroAtual = ombroAlvo;
  else                      ombroAtual += (erro > 0) ? passo : -passo;
  ombroEscreverUs(ombroGrausParaUs(ombroAtual));
}

// ═════════════════════════════════════════════════════════════════════
//  3. MOVIMENTO COORDENADO DAS TRÊS JUNTAS
// ═════════════════════════════════════════════════════════════════════
float cotoveloAlvo = HOME_COTOVELO;
float pulsoAlvo    = HOME_PULSO;
bool  torqueLigado[2] = { false, false };       // [0] cotovelo, [1] pulso
bool  demoAtivo = false;
void  pararDemo();                               // definida na seção 5

float lerGrausSts(uint8_t id, int sentido, float padrao) {
  int bruto = stsLer(id, REG_PRESENT_POS, 2);
  return (bruto >= 0) ? stsParaGraus(bruto, sentido) : padrao;
}

// Calcula as velocidades para que as três juntas terminem ao mesmo tempo
// Retorna a duração estimada do movimento em segundos (0 se já está na pose)
float moverJuntas(float o, float c, float p) {
  o = constrain(o, LIM_OMBRO.min,    LIM_OMBRO.max);
  c = constrain(c, LIM_COTOVELO.min, LIM_COTOVELO.max);
  p = constrain(p, LIM_PULSO.min,    LIM_PULSO.max);

  float c0 = lerGrausSts(ID_COTOVELO, COTOVELO_SENTIDO, cotoveloAlvo);
  float p0 = lerGrausSts(ID_PULSO,    PULSO_SENTIDO,    pulsoAlvo);

  float dO = fabsf(o - ombroAtual);
  float dC = fabsf(c - c0);
  float dP = fabsf(p - p0);
  float dMax = max(dO, max(dC, dP));
  if (dMax < 0.1f) return 0.0f;

  float T = dMax / velMaxGrausS;                 // duração do movimento (s)

  ombroVel  = max(dO / T, 1.0f);
  ombroAlvo = o;

  const uint8_t  ids[2] = { ID_COTOVELO, ID_PULSO };
  const uint16_t pos[2] = { grausParaSts(c, COTOVELO_SENTIDO), grausParaSts(p, PULSO_SENTIDO) };
  // velocidade 0 = máxima no STS, por isso o mínimo é 1
  const uint16_t vel[2] = { (uint16_t)max(1L, lroundf(dC / T * PASSOS_POR_GRAU)),
                            (uint16_t)max(1L, lroundf(dP / T * PASSOS_POR_GRAU)) };
  stsMoverSync(ids, pos, vel, 2);

  cotoveloAlvo = c;
  pulsoAlvo    = p;
  Serial.printf("-> ombro %.1f  cotovelo %.1f  pulso %.1f  (~%.2f s)\n", o, c, p, T);
  return T;
}

void congelar() {
  pararDemo();
  ombroAlvo = ombroAtual;
  const uint8_t ids[2] = { ID_COTOVELO, ID_PULSO };
  for (uint8_t i = 0; i < 2; i++) {
    int pos = stsLer(ids[i], REG_PRESENT_POS, 2);
    if (pos >= 0) stsEscrever2(ids[i], REG_GOAL_POS, pos);
  }
  cotoveloAlvo = lerGrausSts(ID_COTOVELO, COTOVELO_SENTIDO, cotoveloAlvo);
  pulsoAlvo    = lerGrausSts(ID_PULSO,    PULSO_SENTIDO,    pulsoAlvo);
  Serial.println("Congelado na posição atual.");
}

void torqueFeetech(bool ligar) {
  if (!ligar) pararDemo();
  stsTorque(ID_COTOVELO, ligar);
  stsTorque(ID_PULSO,    ligar);
  torqueLigado[0] = torqueLigado[1] = ligar;
  Serial.println(ligar ? "Torque Feetech LIGADO." : "Torque Feetech SOLTO.");
}

// ═════════════════════════════════════════════════════════════════════
//  4. TELEMETRIA E SEGURANÇA
// ═════════════════════════════════════════════════════════════════════
void telemetriaSts(const char* nome, uint8_t id, int sentido) {
  int pos   = stsLer(id, REG_PRESENT_POS, 2);
  int vel   = stsLer(id, REG_PRESENT_SPEED, 2);
  int carga = stsLer(id, REG_PRESENT_LOAD, 2);
  int tens  = stsLer(id, REG_VOLTAGE, 1);
  int temp  = stsLer(id, REG_TEMPERATURE, 1);
  int corr  = stsLer(id, REG_CURRENT, 2);

  Serial.printf("  %-9s (ID %u)", nome, id);
  if (pos < 0) { Serial.println("  SEM RESPOSTA"); return; }
  Serial.printf("  pos %4d (%6.1f°)", pos, stsParaGraus(pos, sentido));
  if (vel   >= 0) Serial.printf("  vel %5d p/s", sinalMagnitude(vel, 15));
  if (carga >= 0) Serial.printf("  carga %5.1f%%", sinalMagnitude(carga, 10) / 10.0f);
  if (tens  >= 0) Serial.printf("  %4.1f V", tens / 10.0f);
  if (temp  >= 0) Serial.printf("  %d °C", temp);
  if (corr  >= 0) Serial.printf("  %6.1f mA", (corr & 0x7FFF) * 6.5f);
  Serial.printf("  erro 0x%02X\n", ultimoErroSts);
}

void telemetria() {
  Serial.println("\n┌─── Telemetria ─────────────────────────────────────────────");
  Serial.printf("  Ombro     (PWM)   comandado %6.1f°  alvo %6.1f°  pulso %lu us\n",
                ombroAtual, ombroAlvo, (unsigned long)ombroGrausParaUs(ombroAtual));
  telemetriaSts("Cotovelo", ID_COTOVELO, COTOVELO_SENTIDO);
  telemetriaSts("Pulso",    ID_PULSO,    PULSO_SENTIDO);
  Serial.println("└────────────────────────────────────────────────────────────\n");
}

// Sobrecarga: 1ª vez congela (para de forçar); se persistir, solta o torque.
void verificarSeguranca() {
  static uint32_t ultimo = 0;
  static uint8_t falhas[2] = { 0, 0 };
  static uint8_t sobrecarga[2] = { 0, 0 };
  if (millis() - ultimo < PERIODO_SEGURANCA_MS) return;
  ultimo = millis();

  const uint8_t ids[2] = { ID_COTOVELO, ID_PULSO };
  const char* nomes[2] = { "Cotovelo", "Pulso" };

  for (uint8_t i = 0; i < 2; i++) {
    if (!torqueLigado[i]) continue;
    int corr = stsLer(ids[i], REG_CURRENT, 2);
    int temp = stsLer(ids[i], REG_TEMPERATURE, 1);

    if (corr < 0 || temp < 0) {
      if (++falhas[i] == FALHAS_COMUNIC_MAX)
        Serial.printf("AVISO: %s (ID %u) não responde.\n", nomes[i], ids[i]);
      continue;
    }
    falhas[i] = 0;

    float mA = (corr & 0x7FFF) * 6.5f;
    if (mA > CORRENTE_MAX_MA || temp > TEMP_MAX_C) {
      sobrecarga[i]++;
      Serial.printf("ALERTA: %s %.0f mA, %d °C\n", nomes[i], mA, temp);
      if (sobrecarga[i] == 1) {
        congelar();
      } else if (sobrecarga[i] >= SOBRECARGA_SOLTAR) {
        stsTorque(ids[i], false);
        torqueLigado[i] = false;
        sobrecarga[i] = 0;
        Serial.printf("%s: torque SOLTO por segurança. Use 'l' para religar.\n", nomes[i]);
      }
    } else {
      sobrecarga[i] = 0;
    }
  }
}

// ═════════════════════════════════════════════════════════════════════
//  5. DEMONSTRAÇÃO EM LOOP (comando "demo")
//     Cada pose tem seus ângulos, sua velocidade e a pausa depois de chegar.
//     A próxima pose só é enviada quando o braço termina a anterior.
//     Confira os ângulos com os limites reais do braço antes de rodar!
// ═════════════════════════════════════════════════════════════════════
struct Pose { float o, c, p; float vel; uint16_t pausaMs; const char* nome; };

const Pose SEQUENCIA_TESTE[] = {
  // ombro cotov. pulso   vel   pausa   nome
  {    0,     0,    0,    30,   800, "home"             },

  // 1. Acordando: levanta devagar e se espreguiça
  {   30,    45,    0,    25,   400, "acordando"        },
  {   60,    90,   60,    35,   500, "espreguicando"    },
  {   60,    90,  -60,    60,   400, "espreguicando"    },

  // 2. Tchau: braço levantado, pulso abanando rápido
  {   45,   100,   45,    60,   150, "tchau: posicao"   },
  {   45,   100,  -45,   120,     0, "tchau"            },
  {   45,   100,   45,   120,     0, "tchau"            },
  {   45,   100,  -45,   120,     0, "tchau"            },
  {   45,   100,   45,   120,     0, "tchau"            },
  {   45,   100,    0,    90,   500, "tchau: fim"       },

  // 3. Olhando em volta: o ombro varre de um lado ao outro
  {  -60,    30,   30,    60,   400, "olhando: lado A"  },
  {   60,    30,  -30,    60,   400, "olhando: lado B"  },
  {    0,    60,    0,    60,   400, "olhando: frente"  },

  // 4. Onda: cotovelo e pulso em fases opostas
  {   20,    80,  -70,    90,     0, "onda"             },
  {   20,   -80,   70,    90,     0, "onda"             },
  {   20,    80,  -70,    90,     0, "onda"             },
  {   20,   -80,   70,    90,   300, "onda"             },

  // 5. Alcançar: estica o braço devagar e recolhe rápido
  {   75,     0,    0,    40,   700, "alcancando"       },
  {   20,   110,   80,   100,   500, "recolhendo"       },

  // 6. Reverência e volta para casa
  {  -40,   -60,  -50,    30,  1200, "reverencia"       },
  {    0,     0,    0,    40,  2000, "home"             },
};
const uint8_t N_POSES = sizeof(SEQUENCIA_TESTE) / sizeof(SEQUENCIA_TESTE[0]);

enum EstadoDemo : uint8_t { DEMO_ENVIAR, DEMO_MOVENDO, DEMO_PAUSA };
EstadoDemo demoEstado = DEMO_ENVIAR;
uint8_t  demoIdx = 0;
uint32_t demoCiclo = 0;
uint32_t demoMarcoMs = 0;     // próximo instante de checagem / fim da pausa
uint32_t demoLimiteMs = 0;    // tempo máximo esperando a pose (evita travar)
float    demoVelSalva = 0;

void iniciarDemo() {
  if (!torqueLigado[0] || !torqueLigado[1]) {
    Serial.println("Torque dos Feetech desligado. Use 'l' antes do demo.");
    return;
  }
  demoVelSalva = velMaxGrausS;
  demoAtivo    = true;
  demoEstado   = DEMO_ENVIAR;
  demoIdx      = 0;
  demoCiclo    = 0;
  Serial.println(">>> Demo iniciado. Qualquer comando para; 'x' congela.");
}

void pararDemo() {
  if (!demoAtivo) return;
  demoAtivo = false;
  velMaxGrausS = demoVelSalva;
  Serial.println(">>> Demo parado.");
}

// true quando o ombro terminou a rampa e os dois Feetech pararam
bool bracoParado() {
  if (ombroAtual != ombroAlvo) return false;
  if (stsLer(ID_COTOVELO, REG_MOVING, 1) != 0) return false;
  if (stsLer(ID_PULSO,    REG_MOVING, 1) != 0) return false;
  return true;
}

void atualizarDemo() {
  if (!demoAtivo) return;
  uint32_t agora = millis();

  switch (demoEstado) {
    case DEMO_ENVIAR: {
      if (demoIdx == 0) {
        demoCiclo++;
        Serial.printf("\n=========== Ciclo %lu ===========", (unsigned long)demoCiclo);
        telemetria();
      }
      const Pose& alvo = SEQUENCIA_TESTE[demoIdx];
      velMaxGrausS = constrain(alvo.vel * DEMO_RAPIDEZ, 1.0f, 360.0f);
      Serial.printf("[%2u/%u] %-16s ", demoIdx + 1, N_POSES, alvo.nome);
      float T = moverJuntas(alvo.o * DEMO_AMPLITUDE,
                            alvo.c * DEMO_AMPLITUDE,
                            alvo.p * DEMO_AMPLITUDE);
      if (T == 0.0f) Serial.println("(ja esta na pose)");
      demoMarcoMs  = agora + 60;                              // tempo para o Feetech começar
      demoLimiteMs = agora + (uint32_t)(T * 3000.0f) + 1500;  // segurança contra travamento
      demoEstado   = DEMO_MOVENDO;
      break;
    }

    case DEMO_MOVENDO:
      if ((int32_t)(agora - demoMarcoMs) < 0) return;
      demoMarcoMs = agora + 20;                               // checa a cada 20 ms
      if (bracoParado() || (int32_t)(agora - demoLimiteMs) >= 0) {
        demoMarcoMs = agora + SEQUENCIA_TESTE[demoIdx].pausaMs;
        demoEstado  = DEMO_PAUSA;
      }
      break;

    case DEMO_PAUSA:
      if ((int32_t)(agora - demoMarcoMs) < 0) return;
      demoIdx    = (demoIdx + 1) % N_POSES;
      demoEstado = DEMO_ENVIAR;
      break;
  }
}

// ═════════════════════════════════════════════════════════════════════
//  6. COMANDOS PELA SERIAL
// ═════════════════════════════════════════════════════════════════════
void ajuda() {
  Serial.println(
    "Comandos: o/c/p <graus> | m <o> <c> <p> | h | v <graus/s> | t | x | s | l\n"
    "          scan | id <atual> <novo> | zero <id> | demo");
}

void scanBarramento() {
  Serial.print("Servos encontrados:");
  bool algum = false;
  for (uint8_t id = 0; id <= 20; id++) {
    if (stsPing(id)) { Serial.printf(" %u", id); algum = true; }
  }
  Serial.println(algum ? "" : " nenhum");
}

void processarComando(char* linha) {
  char* cmd = strtok(linha, " \t");
  if (!cmd) return;
  char* a1 = strtok(nullptr, " \t");
  char* a2 = strtok(nullptr, " \t");
  char* a3 = strtok(nullptr, " \t");

  if (demoAtivo && strcmp(cmd, "demo")) pararDemo();   // qualquer comando interrompe o demo

  if      (!strcmp(cmd, "o") && a1) moverJuntas(atof(a1), cotoveloAlvo, pulsoAlvo);
  else if (!strcmp(cmd, "c") && a1) moverJuntas(ombroAlvo, atof(a1), pulsoAlvo);
  else if (!strcmp(cmd, "p") && a1) moverJuntas(ombroAlvo, cotoveloAlvo, atof(a1));
  else if (!strcmp(cmd, "m") && a3) moverJuntas(atof(a1), atof(a2), atof(a3));
  else if (!strcmp(cmd, "h"))       moverJuntas(HOME_OMBRO, HOME_COTOVELO, HOME_PULSO);
  else if (!strcmp(cmd, "v") && a1) {
    velMaxGrausS = constrain(atof(a1), 1.0f, 360.0f);
    Serial.printf("Velocidade máxima: %.0f graus/s\n", velMaxGrausS);
  }
  else if (!strcmp(cmd, "t"))       telemetria();
  else if (!strcmp(cmd, "x"))       congelar();
  else if (!strcmp(cmd, "s"))       torqueFeetech(false);
  else if (!strcmp(cmd, "l"))       torqueFeetech(true);
  else if (!strcmp(cmd, "scan"))    scanBarramento();
  else if (!strcmp(cmd, "id") && a2) stsTrocarId(atoi(a1), atoi(a2));
  else if (!strcmp(cmd, "zero") && a1) stsCalibrarCentro(atoi(a1));
  else if (!strcmp(cmd, "demo"))    iniciarDemo();
  else ajuda();
}

void lerSerial() {
  static char buf[64];
  static uint8_t n = 0;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      buf[n] = '\0';
      if (n) processarComando(buf);
      n = 0;
    } else if (n < sizeof(buf) - 1) {
      buf[n++] = ch;
    }
  }
}

// ═════════════════════════════════════════════════════════════════════
//  SETUP E LOOP
// ═════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(300);

  ombroIniciar();   // primeiro de tudo: o ombro passa a segurar a pose inicial

  Serial2.begin(1000000, SERIAL_8N1, PINO_RX, PINO_TX);
  uart_set_pin(UART_NUM_2, PINO_TX, PINO_RX, PINO_DIR, UART_PIN_NO_CHANGE);
  uart_set_mode(UART_NUM_2, UART_MODE_RS485_HALF_DUPLEX);
  delay(100);

  Serial.println("\n=== Braço @Home iniciado ===");
  Serial.printf("Cotovelo (ID %u): %s\n", ID_COTOVELO, stsPing(ID_COTOVELO) ? "OK" : "NÃO RESPONDE");
  Serial.printf("Pulso    (ID %u): %s\n", ID_PULSO,    stsPing(ID_PULSO)    ? "OK" : "NÃO RESPONDE");

#if APLICAR_PID
  stsAjustarPID(ID_COTOVELO, PID_COTOVELO);
  stsAjustarPID(ID_PULSO,    PID_PULSO);
#endif

  torqueFeetech(true);
  telemetria();

  // Vai para home devagar
  float velSalva = velMaxGrausS;
  velMaxGrausS = 20.0f;
  moverJuntas(HOME_OMBRO, HOME_COTOVELO, HOME_PULSO);
  velMaxGrausS = velSalva;

  ajuda();

#if DEMO_AO_LIGAR
  delay(3000);   // tempo para o braço chegar em home
  iniciarDemo();
#endif
}

void loop() {
  lerSerial();
  ombroAtualizar();
  verificarSeguranca();
  atualizarDemo();
}
