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
//  PINES
// ============================================
#define SERVO_PIN     15
#define DHTPIN        4
#define DHTTYPE       DHT11
#define BOTON_PIN     5

// ============================================
//  OBJETOS
// ============================================
DHT dht(DHTPIN, DHTTYPE);
Servo myServo;
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ============================================
//  VARIABLES
// ============================================
float temperaturaActual = 0;
float limiteTemperatura = 30.0;
int anguloActual = 0;
unsigned long lastTempRead = 0;
const long tempReadInterval = 5000;

// Variables para el botón
bool botonPresionado = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// Contador de reintentos para mensajes de error
int wifiReintentos = 0;
int mqttReintentos = 0;

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

// ============================================
//  SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("\n==========================================");
  Serial.println("  SMART FARM CONTROL v3 (CON ERRORES)");
  Serial.println("==========================================\n");

  dht.begin();
  myServo.attach(SERVO_PIN);
  myServo.write(0);
  anguloActual = 0;
  Serial.println("[INFO] Servo inicializado en posición 0°");

  pinMode(BOTON_PIN, INPUT_PULLUP);
  Serial.println("[INFO] Botón configurado en GPIO5 (D1) con pull-up interno");

  conectarWiFi();
  mqtt.setServer("io.adafruit.com", 1883);
  mqtt.setCallback(callbackMQTT);
  mqtt.setBufferSize(1024);
  conectarMQTT();
}

void loop() {
  if (!mqtt.connected()) {
    conectarMQTT();
  }
  mqtt.loop();

  leerTemperatura();
  leerBoton();
}

// ============================================
//  CONEXIÓN WiFi CON MENSAJES DE ERROR
// ============================================
void conectarWiFi() {
  Serial.print("[WiFi] Conectando a SSID: ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  wifiReintentos = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiReintentos++;
    if (wifiReintentos > 40) { // 20 segundos sin conexión
      Serial.println("\n[ERROR] No se pudo conectar a WiFi. Verifica SSID/contraseña.");
      // Opcional: reiniciar o seguir intentando
    }
  }
  Serial.println("\n[WiFi] Conectado exitosamente!");
  Serial.print("[WiFi] Dirección IP: ");
  Serial.println(WiFi.localIP().toString());
}

// ============================================
//  CONEXIÓN MQTT CON MENSAJES DE ERROR
// ============================================
void conectarMQTT() {
  mqttReintentos = 0;
  while (!mqtt.connected()) {
    Serial.print("[MQTT] Conectando a Adafruit IO... ");
    if (mqtt.connect(IO_USERNAME, IO_USERNAME, IO_KEY)) {
      Serial.println("OK");
      String topicControl = String(IO_USERNAME) + "/feeds/servo-control";
      if (mqtt.subscribe(topicControl.c_str())) {
        Serial.print("[MQTT] Suscrito a: ");
        Serial.println(topicControl);
      } else {
        Serial.println("[ERROR] No se pudo suscribir al feed servo-control");
      }
      publicarMensaje("Sistema iniciado (con manejo de errores)");
    } else {
      Serial.print("FALLÓ (rc=");
      Serial.print(mqtt.state());
      Serial.println(")");
      mqttReintentos++;
      if (mqttReintentos >= 5) {
        Serial.println("[ERROR] Múltiples fallos de conexión MQTT. Verifica usuario/clave y red.");
      }
      Serial.println("[MQTT] Reintentando en 5 segundos...");
      delay(5000);
    }
  }
}

// ============================================
//  CALLBACK MQTT (recibe comandos remotos)
// ============================================
void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";
  for (unsigned int i = 0; i < length; i++) mensaje += (char)payload[i];
  Serial.print("[MQTT] Comando remoto recibido en [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(mensaje);

  int angulo = mensaje.toInt();
  if (angulo >= 0 && angulo <= 180) {
    moverServo(angulo, "remoto");
  } else {
    Serial.println("[ERROR] Comando no válido (no es un ángulo entre 0 y 180): " + mensaje);
  }
}

// ============================================
//  FUNCIÓN PRINCIPAL: mover servo y publicar SOLO si es local
// ============================================
void moverServo(int angulo, String origen) {
  if (angulo == anguloActual) {
    Serial.println("[INFO] El servo ya está en " + String(angulo) + "°. No se mueve.");
    return;
  }

  // Validar rango por si acaso
  if (angulo < 0) angulo = 0;
  if (angulo > 180) angulo = 180;

  myServo.write(angulo);
  anguloActual = angulo;
  Serial.print("[SERVO] Movido a ");
  Serial.print(angulo);
  Serial.print("° desde ");
  Serial.println(origen);

  publicarMensaje("Servo movido a " + String(angulo) + "° desde " + origen);

  // Solo publicar de vuelta si el origen es local (para evitar bucles)
  if (origen == "boton" || origen == "automatico") {
    String topicServo = String(IO_USERNAME) + "/feeds/servo-control";
    if (mqtt.publish(topicServo.c_str(), String(angulo).c_str())) {
      Serial.println("[MQTT] Sincronización enviada: " + String(angulo) + "°");
    } else {
      Serial.println("[ERROR] Fallo al publicar sincronización");
    }
  } else {
    Serial.println("[MQTT] Origen remoto: no se publica de vuelta para evitar bucle");
  }
}

// ============================================
//  PUBLICAR MENSAJE EN FEED "estado-manual"
// ============================================
void publicarMensaje(String texto) {
  String topic = String(IO_USERNAME) + "/feeds/estado-manual";
  if (mqtt.publish(topic.c_str(), texto.c_str())) {
    Serial.println("[MQTT] Mensaje publicado: " + texto);
  } else {
    Serial.println("[ERROR] Fallo al publicar mensaje en estado-manual");
  }
}

// ============================================
//  LECTURA DE TEMPERATURA CON MANEJO DE ERRORES
// ============================================
void leerTemperatura() {
  if (millis() - lastTempRead >= tempReadInterval) {
    lastTempRead = millis();
    float t = dht.readTemperature();
    if (isnan(t)) {
      Serial.println("[ERROR] DHT11: No se pudo leer la temperatura (sensor desconectado o fallo)");
      return;
    }
    temperaturaActual = t;
    Serial.print("[DHT11] Temperatura: ");
    Serial.print(temperaturaActual);
    Serial.println(" °C");

    String topicTemp = String(IO_USERNAME) + "/feeds/temperatura";
    if (!mqtt.publish(topicTemp.c_str(), String(temperaturaActual).c_str())) {
      Serial.println("[ERROR] Fallo al publicar temperatura en Adafruit IO");
    }

    if (temperaturaActual > limiteTemperatura && anguloActual == 0) {
      Serial.println("[AUTO] Temperatura supera el límite (30°C). Activando riego automático.");
      moverServo(90, "automatico");
    } else if (temperaturaActual > limiteTemperatura && anguloActual != 0) {
      Serial.println("[AUTO] Temperatura alta pero el riego ya está activo (ángulo actual " + String(anguloActual) + "°)");
    }
  }
}

// ============================================
//  BOTÓN FÍSICO (alterna entre 0 y 90 grados)
// ============================================
void leerBoton() {
  int lectura = digitalRead(BOTON_PIN);

  if (lectura == LOW && !botonPresionado) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      botonPresionado = true;
      lastDebounceTime = millis();
      Serial.println("[BOTÓN] Pulsación detectada (flanco de bajada). Alternando servo.");
      if (anguloActual == 0) {
        moverServo(90, "boton");
      } else {
        moverServo(0, "boton");
      }
    }
  }
  else if (lectura == HIGH && botonPresionado) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
      botonPresionado = false;
      lastDebounceTime = millis();
      // No se necesita acción adicional al soltar
    }
  }
}
