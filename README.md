PROYECTO CIBERGU 2026 | EQUIPO V.A
-------------------------------------
Sistema de riego y dispensador inteligente controlado vía web con ESP8266.  
Permite activar el riego remotamente desde Adafruit IO, mediante botón físico o automáticamente por temperatura.  
Incluye apagado automático tras 12 segundos, protección contra ciclos repetitivos, sincronización en tiempo real y robustez frente a caídas de red.  
Ideal para huertos y granjas. Bajo costo y fácil de implementar.
-------------------------------------

# 🌱 Farm Automation ESP8266 - Riego y Dispensador Inteligente

![Versión](https://img.shields.io/badge/versión-2.0.0-blue)
![ESP8266](https://img.shields.io/badge/Placa-ESP8266-orange)
![MQTT](https://img.shields.io/badge/Protocolo-MQTT-ff69b4)
![Adafruit IO](https://img.shields.io/badge/Cloud-Adafruit%20IO-green)
![Seguridad](https://img.shields.io/badge/Seguridad-Anti--DoS%20%7C%20Watchdog-red)

## 📖 Descripción del Proyecto

**Farm Automation ESP8266** es una solución de automatización de bajo costo para el sector agrícola y doméstico. Controla un sistema de riego o dispensador de pienso mediante:

- **Control remoto** desde cualquier lugar (Adafruit IO).
- **Botón físico** para actuación local.
- **Automático por temperatura** (umbral 30°C).
- **Apagado automático de seguridad** a los 12 segundos de apertura (evita olvidos).
- **Sincronización total** entre el dashboard web (toggle + slider) y el estado real del servo.
- **Protección anti‑rebote y anti‑DoS** (límite de comandos por segundo).
- **Watchdog de software** que reinicia la placa si el programa se bloquea.

El sistema está pensado para **ahorrar tiempo, agua y energía**, ofreciendo flexibilidad tanto en campo como desde casa.

> 🚀 **Ideal para**: Agricultura urbana, jardinería inteligente, domótica de bajo coste y hackatones.

---

## 🎯 Características Principales

| Característica | Descripción |
|----------------|-------------|
| 🌍 **Control remoto** | Desde cualquier lugar mediante dashboard en Adafruit IO (toggle y slider sincronizados). |
| 🔘 **Control manual local** | Botón físico (D1) que alterna el estado del servo (0° ↔ 90°). |
| 🌡️ **Automatización por temperatura** | Al superar 30°C, el servo se abre automáticamente (si no lo está ya). |
| ⏱️ **Apagado automático de seguridad** | El servo se cierra automáticamente tras **12 segundos** abierto (evita riego o dispensado continuo). |
| 🔄 **Protección contra reactivación inmediata** | Una vez que el sistema se abre por calor y se cierra por tiempo, no se vuelve a abrir hasta que la temperatura baje del umbral y vuelva a superarlo (evita ciclos infinitos). |
| 📢 **Notificaciones en tiempo real** | Cada acción publica un mensaje en el feed `estado-manual` (ej: "Riego activado desde botón"). |
| 🔄 **Sincronización bidireccional** | Cualquier cambio local (botón, temporizador, temperatura) actualiza el slider y el toggle en Adafruit IO. |
| 🛡️ **Medidas de seguridad** | Anti‑DoS (1 comando/segundo), watchdog de 5 segundos, cierre automático por pérdida de conexión (>30s sin WiFi/MQTT). |
| 🔌 **Bajo costo** | Componentes económicos (< 35 €) y fácilmente disponibles. |

---

## 🧠 Lógica de Funcionamiento (versión actual)

1. **Conexión** → La ESP8266 se conecta al WiFi y al broker MQTT de Adafruit IO.
2. **Escucha remota** → Suscrita al feed `servo-control` para recibir ángulos (0‑180) desde el toggle o slider.
3. **Lectura de temperatura** → Cada 5 segundos lee el DHT11 y publica el valor en el feed `temperatura`.
4. **Automatización por calor** → Si la temperatura > 30°C **y** el servo está cerrado **y** no se ha acabado de activar hace menos de 60 segundos (cooldown), se abre el servo (90°).
5. **Apagado automático** → Cuando el servo está abierto, se comprueba continuamente si han pasado **12 segundos**. Si es así, se cierra (0°).
6. **Sincronización** → Solo los movimientos **locales** (botón, temporizador, automático por calor, pérdida de conexión) publican el nuevo ángulo al feed `servo-control`, actualizando el dashboard. Los comandos remotos no publican de vuelta, evitando bucles.
7. **Protecciones**:
   - **Anti‑DoS**: máximo 1 comando MQTT por segundo.
   - **Watchdog**: si el bucle principal se bloquea más de 5 segundos, la placa se reinicia.
   - **Caída de conexión**: si se pierde WiFi o MQTT durante más de 30 segundos, se cierra el servo.
   - **Cooldown térmico**: después de un cierre por temporizador, se evita una nueva activación por calor durante 60 segundos.

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

> **Nota:** El botón no necesita resistencia externa porque se usa la resistencia pull‑up interna de la ESP8266.

---

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

## 🐛 Solución de problemas de subida: Instalación de drivers (para placas no originales)

Si tu placa **no es original** (por ejemplo, un clon de NodeMCU), es muy probable que utilice el chip **CH340** para la comunicación USB. A veces, Windows no la reconoce automáticamente y necesitarás instalar el driver manualmente. Aquí te explicamos cómo hacerlo según tu sistema operativo.

### 💻 Windows 10 / 11

1. **Descarga el driver oficial** desde la página del fabricante (WCH) o desde este [enlace de respaldo](https://sparks.gogo.co.nz/ch340.html).
2. **Descomprime** el archivo `.zip` descargado.
3. **Ejecuta** el instalador (normalmente `CH341SER.EXE` o `SETUP_64.EXE`) como **Administrador** (clic derecho → "Ejecutar como administrador").
4. Sigue las instrucciones del asistente.
5. **Reinicia tu ordenador** y vuelve a conectar la placa.

> **Nota para Windows 11:** Si el driver no funciona, prueba a instalar una **versión anterior** (por ejemplo, la v3.4 o v3.5), ya que a veces las más nuevas pueden ser incompatibles.

### 🐧 Linux (Ubuntu / Linux Mint)

¡Buenas noticias! El driver para el chip **CH340 ya viene incluido en el kernel de la mayoría de las distribuciones Linux actuales**.

1. Conecta tu placa y el sistema debería reconocerla automáticamente.
2. Para verificarlo, abre una terminal y ejecuta `lsusb`. Deberías ver una línea como `QinHeng Electronics HL-340 USB-Serial adapter`.
3. Si por algún motivo no funciona, instala el driver manualmente desde el [repositorio oficial de WCH en GitHub](https://github.com/WCHSoftGroup/ch341ser_linux).

### 🍎 macOS

1. **Prueba primero sin instalar nada**: Algunas versiones de macOS ya incluyen un driver genérico. Conecta la placa y comprueba si aparece un nuevo puerto en el Arduino IDE.
2. **Si no funciona, instala el driver oficial**:
   - La forma más sencilla es usando **Homebrew**. Si no lo tienes, instálalo desde [brew.sh](https://brew.sh/).
   - Abre la terminal y ejecuta:  
     ```bash
     brew install --cask wch-ch34x-usb-serial-driver
   - Sigue las instrucciones en pantalla y reinicia tu Mac.

   - Si el problema persiste, descarga el driver oficial del fabricante desde la web de WCH. Dentro del archivo .zip encontrarás un PDF con instrucciones detalladas para tu versión de macOS.

Si falla la subida: Mantén presionado **FLASH**, presiona y suelta **RST**, luego suelta **FLASH**.

🧪 Pruebas y verificación
---
| Acción | Resultado esperado |
|--------|--------------------|
| Encender la placa | El monitor serie muestra la IP y "Sistema listo". |
| Enviar `ON` (toggle) desde dashboard | Servo gira a 90° y el slider se mueve a 90°. |
| Enviar `OFF` desde dashboard | Servo vuelve a 0° y el slider se mueve a 0°. |
| Mover el slider a un ángulo (ej. 45°) | Servo se mueve a 45°, toggle se pone en ON. |
| Presionar el botón físico (D1) | Alterna entre 0° y 90°, actualizando toggle y slider. |
| Calentar el sensor DHT11 > 30°C | El servo se abre (90°). Si sigue caliente, tras 12 segundos se cierra y no se reabre inmediatamente (cooldown de 60s). |
| Desconectar el WiFi (o perder conexión MQTT) | El servo se cierra por seguridad. Al reconectar, el dashboard se sincroniza. |
| Mantener el servo abierto más de 12 segundos | Se cierra automáticamente y el dashboard se actualiza a OFF/0°. |
| Enviar comandos muy rápido (más de 1 por segundo) | El sistema ignora los comandos extra (protección anti-DoS). |
| Bloquear el programa (simulación) | El watchdog reinicia la placa automáticamente. |

👨‍💻 Autores
---
- @CeroCincoSiete
- @martinezmendezalv-hue

Proyecto desarrollado para la Hackatón CIBERGU 2026.

⭐ Créditos
---
  -  Adafruit IO – plataforma MQTT y dashboards.

  - Espressif – por el ESP8266.

  - Comunidad open‑source por las librerías utilizadas.

📄 Licencia
---

MIT License – ver archivo LICENSE.
