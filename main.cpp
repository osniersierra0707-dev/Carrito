#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* ssid = "INTALNET_MELO";
const char* password = "Martin2020";

// ===== L298N: dirección =====
const int IN1 = 16;  // Motor izquierdo
const int IN2 = 17;
const int IN3 = 21;  // Motor derecho
const int IN4 = 22;

// ===== L298N: velocidad (PWM) =====
const int ENA = 25;  // Motor izquierdo
const int ENB = 26;  // Motor derecho
const int CANAL_A = 4;   // solo se usan en core 2.x (0-3 los deja libres para el servo)
const int CANAL_B = 5;
const int FRECUENCIA = 20000;
const int RESOLUCION = 8;  // 0-255

// ===== Sensores y servo =====
const int sensorIzq = 4;
const int sensorDer = 19;
const int NIVEL_NEGRO = HIGH;
const bool INVERTIR_GIRO = false;

const int TRIG_PIN = 23;
const int ECHO_PIN = 34;   // ECHO con divisor de voltaje (5V -> 3.3V)
const int SERVO_PIN = 13;

// ===== Ajustes del modo automático =====
const int DIST_OBSTACULO = 10;   // cm: distancia que activa el escaneo
const int TIEMPO_GIRO = 600;     // ms de giro (ajústalo a tu robot y velocidad)
const int TIEMPO_ATRAS = 400;    // ms de retroceso antes de girar si ambos lados están obstruidos

WebServer server(80);
Servo miServo;

String modoActual = "manual";
int velocidadPct = 60;  // 0-100, se cambia desde el HTML

// ---------- PWM compatible con core 2.x y 3.x ----------
void pwmInicio(int pin, int canal) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, FRECUENCIA, RESOLUCION);
#else
  ledcSetup(canal, FRECUENCIA, RESOLUCION);
  ledcAttachPin(pin, canal);
#endif
}

void pwmEscribir(int pin, int canal, int valor) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, valor);
#else
  ledcWrite(canal, valor);
#endif
}

void aplicarVelocidad() {
  int duty = velocidadPct * 255 / 100;
  pwmEscribir(ENA, CANAL_A, duty);
  pwmEscribir(ENB, CANAL_B, duty);
}

// ---------- Movimiento ----------
// La velocidad la fija el PWM de ENA/ENB; la dirección, los IN.
void detenerMotores() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void moverAdelante() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void moverAtras() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void girarIzquierda() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void girarDerecha() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

// ---------- Ultrasónico ----------
long medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracion = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duracion == 0) return 200;  // sin eco = libre
  return duracion * 0.034 / 2;
}

// ---------- Escaneo del modo automático ----------
void escanearYDecidir() {
  detenerMotores();
  delay(300);

  miServo.write(0);    // Derecha
  delay(500);
  long distDer = medirDistancia();

  miServo.write(180);  // Izquierda
  delay(600);
  long distIzq = medirDistancia();

  miServo.write(90);   // Centro
  delay(400);

  bool libreDer = distDer > DIST_OBSTACULO;
  bool libreIzq = distIzq > DIST_OBSTACULO;

  Serial.print("Der: "); Serial.print(distDer);
  Serial.print(" cm | Izq: "); Serial.print(distIzq);
  Serial.print(" cm -> ");

  if (libreDer && libreIzq) {
    // Ambos libres: gira hacia el que tenga más espacio
    if (distDer >= distIzq) { Serial.println("DERECHA"); girarDerecha(); }
    else                    { Serial.println("IZQUIERDA"); girarIzquierda(); }
    delay(TIEMPO_GIRO);
  } else if (libreDer) {
    Serial.println("DERECHA");
    girarDerecha();
    delay(TIEMPO_GIRO);
  } else if (libreIzq) {
    Serial.println("IZQUIERDA");
    girarIzquierda();
    delay(TIEMPO_GIRO);
  } else {
    // Ambos obstruidos: retrocede un poco, gira al azar y vuelve a escanear
    Serial.println("AMBOS OBSTRUIDOS: giro aleatorio");
    moverAtras();
    delay(TIEMPO_ATRAS);
    if (random(2) == 0) girarDerecha(); else girarIzquierda();
    delay(TIEMPO_GIRO);
  }

  detenerMotores();
  delay(200);
  // Al volver al loop mide el frente: si sigue obstruido, escanea de nuevo
}

// ---------- Rutas HTTP ----------
void responder(const char* msg) {
  server.send(200, "text/plain", msg);
}

void configurarRutas() {
  server.on("/", HTTP_GET, []() { responder("ESP32 Multimodo con PWM lista"); });

  server.on("/modo/manual", HTTP_GET, []() {
    modoActual = "manual";
    detenerMotores();
    miServo.write(90);
    responder("Modo manual activado");
  });

  server.on("/modo/automatico", HTTP_GET, []() {
    modoActual = "automatico";
    detenerMotores();
    miServo.write(90);
    responder("Modo automatico activado");
  });

  server.on("/modo/seguidor", HTTP_GET, []() {
    modoActual = "seguidor";
    detenerMotores();
    miServo.write(90);
    responder("Modo seguidor de linea activado");
  });

  // Velocidad 0-100 (%), válida en cualquier modo
  server.on("/velocidad", HTTP_GET, []() {
    if (!server.hasArg("valor")) {
      server.send(400, "text/plain", "Falta el parametro valor");
      return;
    }
    velocidadPct = constrain(server.arg("valor").toInt(), 0, 100);
    aplicarVelocidad();
    Serial.print("Velocidad: "); Serial.print(velocidadPct); Serial.println("%");
    responder("Velocidad actualizada");
  });

  // Comandos manuales (siempre responden, aunque no estés en modo manual)
  server.on("/adelante",  HTTP_GET, []() { if (modoActual == "manual") moverAdelante();  responder("OK"); });
  server.on("/atras",     HTTP_GET, []() { if (modoActual == "manual") moverAtras();     responder("OK"); });
  server.on("/izquierda", HTTP_GET, []() { if (modoActual == "manual") girarIzquierda(); responder("OK"); });
  server.on("/derecha",   HTTP_GET, []() { if (modoActual == "manual") girarDerecha();   responder("OK"); });
  server.on("/detener",   HTTP_GET, []() { if (modoActual == "manual") detenerMotores(); responder("OK"); });
}

void setup() {
  Serial.begin(115200);