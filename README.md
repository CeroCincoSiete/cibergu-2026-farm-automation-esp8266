PROYECTO CIBERGU 2026 | EQUIPO V.A
-------------------------------------
Sistema de riego y dispensador inteligente controlado vía web con ESP8266. Permite activar el riego remotamente desde Adafruit IO, mediante botón físico o automáticamente por temperatura. Ideal para huertos y granjas. Bajo costo y fácil de implementar.
-------------------------------------
# 🌱 Farm Automation Esp8266 - Riego y Dispensador Inteligente

![Versión](https://img.shields.io/badge/versión-1.0.0-blue)
![ESP8266](https://img.shields.io/badge/Placa-ESP8266-orange)
![MQTT](https://img.shields.io/badge/Protocolo-MQTT-ff69b4)
![Adafruit IO](https://img.shields.io/badge/Cloud-Adafruit%20IO-green)

## 📖 Descripción del Proyecto

**farm-automation-esp8266** es una solución de automatización de bajo costo para el sector agrícola y doméstico. Permite controlar de forma remota un sistema de riego o un dispensador de pienso utilizando una placa **ESP8266 (NodeMCU)**, un servomotor, un sensor de temperatura DHT11 y un botón físico para control manual.

El proyecto nace de la necesidad de facilitar la vida a personas con huertos, jardines o pequeñas granjas que deben desplazarse diariamente para regar o alimentar a sus animales. Con este sistema, cualquier persona puede activar el riego o el dispensador desde cualquier lugar con conexión a Internet, o bien de forma automática según la temperatura ambiente.

> 🚀 **Ideal para**: Hackatones, proyectos universitarios, agricultura urbana, jardinería inteligente y domótica de bajo coste.

---

## 🎯 Características Principales

| Característica | Descripción |
|----------------|-------------|
| 🌍 **Control remoto** | Desde cualquier lugar mediante un dashboard en Adafruit IO |
| 🔘 **Control manual** | Botón físico para activar/desactivar el sistema localmente |
| 🌡️ **Automatización por temperatura** | Se activa automáticamente si la temperatura supera un umbral configurable |
| ⏰ **Programación horaria** | Permite programar acciones a horas específicas desde Adafruit IO |
| 📢 **Notificaciones visuales** | Mensajes de confirmación en tiempo real en el dashboard |
| 🔌 **Bajo costo** | Componentes económicos y fáciles de conseguir |
| 📡 **Conexión WiFi** | Utiliza la red doméstica para conectarse a Internet |

---

## 🧠 Lógica de Funcionamiento

1. La placa **ESP8266** se conecta a la red WiFi y al servidor MQTT de **Adafruit IO**.
2. Escucha constantemente el feed `servo-control` para recibir comandos `ON`/`OFF`.
3. Lee el **sensor DHT11** cada 5 segundos y publica la temperatura en el feed `temperatura`.
4. Si la temperatura supera los **30°C**, activa automáticamente el riego.
5. El **botón físico** permite alternar el estado del sistema localmente.
6. Cada acción (manual, remota o automática) publica un mensaje en el feed `estado-manual`.

---

## 🧰 Componentes Necesarios

| Componente | Cantidad | Aprox. Costo |
|------------|----------|---------------|
| NodeMCU ESP8266 (o ESP32) | 1 | 8-12 € |
| Servomotor (SG90 o MG995) | 1 | 3-8 € |
| Sensor DHT11 | 1 | 2-5 € |
| Pulsador / Botón | 1 | 0.5 € |
| Protoboard | 1 | 3-5 € |
| Cables Dupont (macho-macho) | 10 | 2 € |
| Resistencia 220Ω (para LED opcional) | 1 | 0.1 € |
| LED (opcional, para pruebas) | 1 | 0.1 € |

**Costo total aproximado**: 20-35 €

---

## 🔌 Diagrama de Conexiones

| Componente | Pin ESP8266 | Etiqueta |
|------------|-------------|----------|
| Servo (señal) | GPIO15 | D8 |
| Servo (VCC) | 5V | - |
| Servo (GND) | GND | - |
| DHT11 (datos) | GPIO4 | D2 |
| DHT11 (VCC) | 3.3V | - |
| DHT11 (GND) | GND | - |
| Botón (señal) | GPIO5 | D1 |
| Botón (GND) | GND | - |

---
**Nota:** El botón no necesita resistencia externa porque se usa la resistencia pull-up interna de la placa.

## 🚀 Configuración

### 1. Arduino IDE

Agrega la siguiente URL en Preferencias → Gestor de URLs adicionales: https://arduino.esp8266.com/stable/package_esp8266com_index.json
Instala el paquete **esp8266** desde el Gestor de Tarjetas.

### 2. Librerías necesarias

- `PubSubClient` (Nick O'Leary)
- `DHT sensor library` (Adafruit)
- `Servo` (incluida)

### 3. Adafruit IO

1. Crea cuenta en [io.adafruit.com](https://io.adafruit.com)
2. Anota tu **Username** y **AIO Key**
3. Crea los feeds:
   - `servo-control`
   - `temperatura`
   - `estado-manual`
4. Crea un dashboard con:
   - Toggle button (para servo-control)
   - Gauge o Text (para temperatura)
   - Text (para estado-manual)

### 4. Configurar el código

Edita estas líneas en el archivo `.ino`:

```cp
#define WLAN_SSID     "tu_wifi"
#define WLAN_PASS     "tu_contraseña"
#define IO_USERNAME   "tu_usuario_adafruit"
#define IO_KEY        "tu_aio_key"
````
---

### 5. Subir a la placa

**Placa:** NodeMCU 1.0 (ESP-12E Module)

**Puerto:** */dev/ttyUSB0 (Linux) o COMx (Windows)*

**Velocidad:** 115200

Si falla la subida: Mantén presionado **FLASH**, presiona y suelta **RST**, luego suelta **FLASH**.

🧪 **Pruebas**
---
Acción	Resultado esperado
Encender placa	Monitor serie muestra IP
Enviar ON desde dashboard	Servo gira a 90°
Enviar OFF desde dashboard	Servo vuelve a 0°
Presionar botón físico	Alterna estado del servo
Calentar DHT11 > 30°C	Servo se abre automáticamente

👨‍💻 **Autores**
---
@CeroCincoSiete | @alvaromendez

Proyecto desarrollado para Hackatón.


⭐ **Créditos**
---
Adafruit IO

Espressif

Comunidad open-source
