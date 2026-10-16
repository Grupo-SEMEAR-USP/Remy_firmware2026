#include <Arduino.h>
#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

// Importando os tipos de mensagens do ROS 2
#include <geometry_msgs/msg/twist.h>
#include <geometry_msgs/msg/point.h>

rcl_node_t node;
rclc_support_t support;
rcl_allocator_t allocator;
rclc_executor_t executor;
rcl_timer_t timer;

// Publisher e Subscriber
rcl_publisher_t odom_publisher;
rcl_subscription_t cmd_vel_subscriber;

// Mensagens
geometry_msgs__msg__Point ticks_msg;
geometry_msgs__msg__Twist twist_msg;

// --- Pinos e Variáveis dos Motores (Mantido igual) ---
const int PIN_PWM_1 = 4, PIN_DIR_1 = 23;
const int PIN_Ha_1 = 18, PIN_Hb_1 = 19, PIN_Hc_1 = 21;

const int PIN_PWM_2 = 32, PIN_DIR_2 = 13;
const int PIN_Ha_2 = 25, PIN_Hb_2 = 26, PIN_Hc_2 = 27;

volatile bool Ha1_Val, Hb1_Val, Hc1_Val;
volatile bool Ha2_Val, Hb2_Val, Hc2_Val;

const int H = 1, AH = -1;
volatile int sentido1 = 1, sentido2 = 1;
volatile long Pulsos1 = 0, Pulsos2 = 0;   

int motorLeft = 0, motorRight = 0;
unsigned long lastCmdVelTime = 0;

// --- ISRs motor 1 ---
void IRAM_ATTR HS_A1() {
  Ha1_Val = digitalRead(PIN_Ha_1); Hb1_Val = digitalRead(PIN_Hb_1);
  sentido1 = (Ha1_Val == Hb1_Val) ? H : AH;
  Pulsos1 += sentido1;
}
void IRAM_ATTR HS_B1() {
  Hb1_Val = digitalRead(PIN_Hb_1); Hc1_Val = digitalRead(PIN_Hc_1);
  sentido1 = (Hb1_Val == Hc1_Val) ? H : AH;
  Pulsos1 += sentido1;
}
void IRAM_ATTR HS_C1() {
  Hc1_Val = digitalRead(PIN_Hc_1); Ha1_Val = digitalRead(PIN_Ha_1);
  sentido1 = (Hc1_Val == Ha1_Val) ? H : AH;
  Pulsos1 += sentido1;
}

// --- ISRs motor 2 ---
void IRAM_ATTR HS_A2() {
  Ha2_Val = digitalRead(PIN_Ha_2); Hb2_Val = digitalRead(PIN_Hb_2);
  sentido2 = (Ha2_Val == Hb2_Val) ? H : AH;
  Pulsos2 += sentido2;
}
void IRAM_ATTR HS_B2() {
  Hb2_Val = digitalRead(PIN_Hb_2); Hc2_Val = digitalRead(PIN_Hc_2);
  sentido2 = (Hb2_Val == Hc2_Val) ? H : AH;
  Pulsos2 += sentido2;
}
void IRAM_ATTR HS_C2() {
  Hc2_Val = digitalRead(PIN_Hc_2); Ha2_Val = digitalRead(PIN_Ha_2);
  sentido2 = (Hc2_Val == Ha2_Val) ? H : AH;
  Pulsos2 += sentido2;
}

// Função de acionamento
void applyMotors(int L, int R) {
  if (L > 0) { digitalWrite(PIN_DIR_1, LOW); analogWrite(PIN_PWM_1, L); }
  else if (L < 0) { digitalWrite(PIN_DIR_1, HIGH); analogWrite(PIN_PWM_1, -L); }
  else analogWrite(PIN_PWM_1, 0);

  if (R > 0) { digitalWrite(PIN_DIR_2, HIGH); analogWrite(PIN_PWM_2, R); }
  else if (R < 0) { digitalWrite(PIN_DIR_2, LOW); analogWrite(PIN_PWM_2, -R); }
  else analogWrite(PIN_PWM_2, 0);
}

// =========================================================
// CALLBACK DO SUBSCRIBER (Recebe comando de velocidade)
// =========================================================
void cmd_vel_callback(const void * msgin) {
  const geometry_msgs__msg__Twist * msg = (const geometry_msgs__msg__Twist *)msgin;
  
  float linear = msg->linear.x;
  float angular = msg->angular.z;

  // Cinemática simples: converte m/s e rad/s para escala de PWM (0 a 255)
  // Depois, quando o controle PID estiver implementado, isso enviará a meta (setpoint) em RPM
  float left  = (linear - angular) * 255.0; 
  float right = (linear + angular) * 255.0;

  motorLeft = constrain((int)left, -255, 255);
  motorRight = constrain((int)right, -255, 255);
  
  lastCmdVelTime = millis();
}

// =========================================================
// CALLBACK DO TIMER (Publica a odometria a cada 50ms / 20Hz)
// =========================================================
void timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
  if (timer != NULL) {
    ticks_msg.x = (double)Pulsos1;
    ticks_msg.y = (double)(-Pulsos2);
    ticks_msg.z = 0.0;
    
    // Publica a mensagem no tópico /encoder_ticks
    rcl_publish(&odom_publisher, &ticks_msg, NULL);
  }
}

void setup() {
  Serial.begin(115200);
  set_microros_serial_transports(Serial);
  delay(2000);

  // Pinos
  pinMode(PIN_Ha_1, INPUT); pinMode(PIN_Hb_1, INPUT); pinMode(PIN_Hc_1, INPUT);
  pinMode(PIN_Ha_2, INPUT); pinMode(PIN_Hb_2, INPUT); pinMode(PIN_Hc_2, INPUT);
  pinMode(PIN_PWM_1, OUTPUT); pinMode(PIN_PWM_2, OUTPUT);
  pinMode(PIN_DIR_1, OUTPUT); pinMode(PIN_DIR_2, OUTPUT);
  analogWrite(PIN_PWM_1, 0); analogWrite(PIN_PWM_2, 0);

  // Interrupções
  attachInterrupt(digitalPinToInterrupt(PIN_Ha_1), HS_A1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_Hb_1), HS_B1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_Hc_1), HS_C1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_Ha_2), HS_A2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_Hb_2), HS_B2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_Hc_2), HS_C2, CHANGE);

  // --- INICIALIZAÇÃO MICRO-ROS ---
  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "esp32_base_node", "", &support);

  // 1. Cria o Publisher (/encoder_ticks)
  rclc_publisher_init_default(
    &odom_publisher, &node, 
    ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Point), 
    "encoder_ticks");

  // 2. Cria o Subscriber (/cmd_vel)
  rclc_subscription_init_default(
    &cmd_vel_subscriber, &node, 
    ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), 
    "cmd_vel");

  // 3. Cria o Timer (20Hz = 50ms)
  rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(50), timer_callback);

  // 4. Cria o Executor (gerencia 1 timer + 1 subscriber)
  rclc_executor_init(&executor, &support.context, 2, &allocator);
  rclc_executor_add_timer(&executor, &timer);
  rclc_executor_add_subscription(&executor, &cmd_vel_subscriber, &twist_msg, &cmd_vel_callback, ON_NEW_DATA);
}

void loop() {
  // Gira o executor para processar envios e recebimentos pendentes do ROS
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));

  // Trava de segurança: para os motores se o ROS parar de mandar comandos por 0.5s
  if (millis() - lastCmdVelTime > 500) { 
    motorLeft = 0; 
    motorRight = 0; 
  }
  
  applyMotors(motorLeft, motorRight);
}