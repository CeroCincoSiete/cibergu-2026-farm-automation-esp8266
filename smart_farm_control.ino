#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Servo.h>

// ============================================
//  CONFIGURACIÓN - CAMBIA SÓLO ESTOS DATOS
// ============================================
#define WLAN_SSID     "SSID"
#define WLAN_PASS     "PASSWORD"
#define IO_USERNAME   "USER ADAFRUIT IO"
#define IO_KEY        "KEY ADAFRUIT IO"

// ============================================
//  PINES Y CONSTANTES
// ============================================
#define SERVO_PIN     15
#define DHTPIN        4
#define DHTTYPE       DHT11
#define BOTON_PIN     5

// ============================================
//  PARÁMETROS DE SEGURIDAD Y TIEMPOS
// ============================================
const unsigned long TIEMPO_AUTO_APAGADO = 12000;     // 12 segundos (apagado por tiempo)
const unsigned long TIEMPO_ENFRIAMIENTO = 60000;    // 1 minuto (cooldown entre activaciones automáticas)
const unsigned long TIMEOUT_CONEXION = 30000;        // 30 segundos sin conexión = cierre
const unsigned long INTERVALO_TEMP = 5000;           // 5 segundos entre lecturas DHT11
const unsigned long LIMITE_COMANDOS_POR_SEGUNDO = 1;  // anti-DoS

// ============================================
//  OBJETOS
// ============================================
DHT dht(DHTPIN, DHTTYPE);
Servo myServo;
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ============================================
//  VARIABLES GLOBALES
// ============================================
float temperaturaActual = 0;
float limiteTemperatura = 30.0;
int anguloActual = 0;
unsigned long lastTempRead = 0;
unsigned long lastComandoTime = 0;
int comandosEnSegundo = 0;
unsigned long lastConexionCheck = 0;
bool alarmaConexion = false;
unsigned long tiempoApertura = 0;

// Variables para el cooldown de la activación automática
unsigned long ultimaActivacionAuto = 0;   // Momento de la última vez que se abrió por temperatura
bool esperandoCooldown = false;           // Flag para no reactivar durante el enfriamiento

// Variables botón
bool botonPresionado = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// Watchdog
unsigned long lastLoopTime = 0;

// ============================================
//  PROTOTIPOS
// ============================================
void conectarWiFi();
void conectarMQTT();
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void moverServo(int angulo, String origen);
void publicarMensaje(String texto);
void leerTemperatura();
void leerBoton();
void verificarConexion();
void verificarApagadoAutomatico();

// ============================================
//  SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("\n==========================================");
  Serial.println("  SMART FARM CONTROL v6 (COOLDOWN 60s)");
  Serial.println("==========================================\n");

  dht.begin();
  myServo.attach(SERVO_PIN);
  myServo.write(0);
  anguloActual = 0;
  Serial.println("[INFO] Servo inicializado en 0° (cerrado)");

  pinMode(BOTON_PIN, INPUT_PULLUP);
  Serial.println("[INFO] Botón configurado en GPIO5 (D1)");

  conectarWiFi();
  mqtt.setServer("io.adafruit.com", 1883);
  mqtt.setCallback(callbackMQTT);
  mqtt.setBufferSize(1024);
  conectarMQTT();

  lastLoopTime = millis();
  Serial.println("[INFO] Sistema listo. Apagado automático en 12s si se abre.");
  Serial.println("[INFO] Cooldown entre riegos automáticos: 60 segundos.");
}

void loop() {
  // Watchdog
  if (millis() - lastLoopTime > 5000) {
    Serial.println("[ERROR] Bucle principal bloqueado. Reiniciando...");
    ESP.restart();
  }
  lastLoopTime = millis();

  if (!mqtt.connected()) conectarMQTT();
  mqtt.loop();
  
  verificarConexion();
  verificarApagadoAutomatico();
  leerTemperatura();
  leerBoton();

  // Anti-DoS: reset contador de comandos por segundo
  if (millis() - lastComandoTime >= 1000) {
    comandosEnSegundo = 0;
    lastComandoTime = millis();
  }
}

// ============================================
//  CONEXIONES
// ============================================
void conectarWiFi() {
  Serial.print("[WiFi] Conectando a ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  int reintentos = 0;
  while (WiFi.status() != WL_CONNECTED && reintentos < 20) {
    delay(500);
    Serial.print(".");
    reintentos++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Conectado. IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[ERROR] No se pudo conectar a WiFi.");
  }
}

void conectarMQTT() {
  int reintentos = 0;
  while (!mqtt.connected() && reintentos < 10) {
    Serial.print("[MQTT] Conectando a Adafruit IO... ");
    if (mqtt.connect(IO_USERNAME, IO_USERNAME, IO_KEY)) {
      Serial.println("OK");
      String topic = String(IO_USERNAME) + "/feeds/servo-control";
      mqtt.subscribe(topic.c_str());
      Serial.print("[MQTT] Suscrito a: ");
      Serial.println(topic);
      publicarMensaje("Sistema seguro iniciado (cooldown 60s)");
      alarmaConexion = false;
    } else {
      Serial.print("FALLÓ (rc=");
      Serial.print(mqtt.state());
      Serial.println(")");
      reintentos++;
      delay(5000);
    }
  }
}

// ============================================
//  VERIFICAR CAÍDA DE CONEXIÓN
// ============================================
void verificarConexion() {
  bool conectado = (WiFi.status() == WL_CONNECTED) && mqtt.connected();
  if (!conectado) {
    if (!alarmaConexion) {
      alarmaConexion = true;
      Serial.println("[ALERTA] Pérdida de conexión. Cerrando servo.");
      moverServo(0, "seguridad_conexion");
    }
  } else {
    if (alarmaConexion) {
      alarmaConexion = false;
      Serial.println("[INFO] Conexión restablecida.");
    }
  }
}

// ============================================
//  APAGADO AUTOMÁTICO POR TIEMPO (12 segundos)
// ============================================
void verificarApagadoAutomatico() {
  if (anguloActual > 0 && (millis() - tiempoApertura >= TIEMPO_AUTO_APAGADO)) {
    Serial.println("[TIMER] Tiempo de apertura superado (12s). Cerrando servo.");
    moverServo(0, "temporizador");
  }
}

// ============================================
//  CALLBACK MQTT (anti-DoS)
// ============================================
void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  if (comandosEnSegundo >= LIMITE_COMANDOS_POR_SEGUNDO) {
    Serial.println("[SEGURIDAD] Demasiados comandos por segundo. Ignorando.");
    return;
  }
  comandosEnSegundo++;

  String mensaje = "";
  for (unsigned int i = 0; i < length; i++) mensaje += (char)payload[i];
  Serial.print("[MQTT] Comando recibido: ");
  Serial.println(mensaje);

  int angulo = mensaje.toInt();
  if (angulo >= 0 && angulo <= 180) {
    moverServo(angulo, "remoto");
  } else {
    Serial.println("[ERROR] Comando no válido: " + mensaje);
  }
}

// ============================================
//  MOVER SERVO (con sincronización y gestión de tiempos)
// ============================================
void moverServo(int angulo, String origen) {
  if (angulo == anguloActual) {
    Serial.println("[INFO] El servo ya está en esa posición.");
    return;
  }
  if (angulo < 0) angulo = 0;
  if (angulo > 180) angulo = 180;

  myServo.write(angulo);
  anguloActual = angulo;
  Serial.print("[SERVO] Movido a ");
  Serial.print(angulo);
  Serial.print("° desde ");
  Serial.println(origen);
  publicarMensaje("Servo movido a " + String(angulo) + "° desde " + origen);

  if (angulo > 0) {
    tiempoApertura = millis();
    Serial.println("[TIMER] Apagado automático programado en 12 segundos.");
  } else {
    Serial.println("[TIMER] Servo cerrado. Se canceló el apagado pendiente.");
  }

  // Sincronización: solo publicamos si el origen NO es remoto
  if (origen != "remoto") {
    String topicServo = String(IO_USERNAME) + "/feeds/servo-control";
    if (mqtt.publish(topicServo.c_str(), String(angulo).c_str())) {
      Serial.println("[MQTT] Sincronización enviada: " + String(angulo) + "°");
    } else {
      Serial.println("[ERROR] Fallo al publicar sincronización");
    }
  } else {
    Serial.println("[MQTT] Origen remoto: no se publica de vuelta.");
  }
}

// ============================================
//  PUBLICAR MENSAJE EN FEED estado-manual
// ============================================
void publicarMensaje(String texto) {
  String topic = String(IO_USERNAME) + "/feeds/estado-manual";
  if (mqtt.publish(topic.c_str(), texto.c_str())) {
    Serial.println("[MQTT] Mensaje publicado: " + texto);
  } else {
    Serial.println("[ERROR] Fallo al publicar mensaje");
  }
}

// ============================================
//  LECTURA DE TEMPERATURA CON COOLDOWN
// ============================================
void leerTemperatura() {
  if (millis() - lastTempRead >= INTERVALO_TEMP) {
    lastTempRead = millis();
    float t = dht.readTemperature();
    if (isnan(t)) {
      Serial.println("[ERROR] DHT11: No se puede leer temperatura.");
      return;
    }
    temperaturaActual = t;
    Serial.print("[DHT11] Temperatura: ");
    Serial.print(temperaturaActual);
    Serial.println(" °C");

    // Publicar temperatura
    String topic = String(IO_USERNAME) + "/feeds/temperatura";
    if (!mqtt.publish(topic.c_str(), String(temperaturaActual).c_str())) {
      Serial.println("[ERROR] Fallo al publicar temperatura");
    }

    // ============================================
    //  LÓGICA DE ACTIVACIÓN AUTOMÁTICA CON COOLDOWN
    // ============================================
    // 1. Verificar si la temperatura supera el umbral
    // 2. Verificar que el servo esté cerrado (ángulo == 0)
    // 3. Verificar que NO esté en periodo de cooldown
    //    (cooldown = TIEMPO_ENFRIAMIENTO después de la última activación automática)
    // ============================================
    bool temperaturaAlta = (temperaturaActual > limiteTemperatura);
    bool servoCerrado = (anguloActual == 0);
    bool enCooldown = (millis() - ultimaActivacionAuto < TIEMPO_ENFRIAMIENTO);

    if (temperaturaAlta && servoCerrado && !enCooldown) {
      Serial.println("[AUTO] Temperatura >30°C y servo cerrado. Abriendo riego.");
      ultimaActivacionAuto = millis();   // Registrar momento de activación
      moverServo(90, "automatico");
    } else if (temperaturaAlta && servoCerrado && enCooldown) {
      // No se activa porque está en cooldown
      unsigned long tiempoRestante = (TIEMPO_ENFRIAMIENTO - (millis() - ultimaActivacionAuto)) / 1000;
      Serial.print("[AUTO] Temperatura alta pero en cooldown. Faltan ");
      Serial.print(tiempoRestante);
      Serial.println(" segundos para poder activar de nuevo.");
    } else if (temperaturaAlta && !servoCerrado) {
      Serial.println("[AUTO] Temperatura alta pero el servo ya está abierto. No se hace nada.");
    }
  }
}

// ============================================
//  BOTÓN FÍSICO (alterna entre 0 y 90)
// ============================================
void leerBoton() {
  int lectura = digitalRead(BOTON_PIN);
  if (lectura == LOW && !botonPresionado) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      botonPresionado = true;
      lastDebounceTime = millis();
      Serial.println("[BOTÓN] Pulsación detectada. Alternando servo.");
      if (anguloActual == 0) {
        moverServo(90, "boton");
      } else {
        moverServo(0, "boton");
      }
    }
  } else if (lectura == HIGH && botonPresionado) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      botonPresionado = false;
      lastDebounceTime = millis();
    }
  }
}
