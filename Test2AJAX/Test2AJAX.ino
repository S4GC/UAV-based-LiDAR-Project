#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TFLI2C.h>
#include <MPU6050.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// --- Configuración del Access Point ---
const char* apSSID     = "ESP32-SensorHub";   // Nombre de la red
const char* apPassword = "12345678";          // Clave (mínimo 8 caracteres)

// --- Sensor TF-Luna ---
TFLI2C tflI2C;
int16_t tfDist;
int16_t tfAddr = TFL_DEF_ADR;   // Dirección I2C por defecto (0x10)

// --- Sensor MPU6050 ---
MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;

// --- Sensor GPS ---
TinyGPSPlus gps;
HardwareSerial SerialGPS(2); // Creamos Serial2 para el GPS
const uint32_t GPSBaud = 9600; // Baudrate por defecto del NEO6M

// --- Pines ---
const int BAUD = 115200;
const int SDAPin = 21; // I2C SDA -> GPIO 21 
const int SCLPin = 22; // I2C SCL -> GPIO 22 
const int RXPin  = 16; // UART TX -> RX2 GPIO 16
const int TXPin  = 17; // UART RX -> TX2 GPIO 17

// --- Servidor web ---
WebServer server(80);

// --- Página principal ---
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset='utf-8'>
    <title>ESP32 Sensor Hub</title>
  </head>
  <body>
    <h1>Lecturas de Sensores</h1>

    <h2>TF-Luna</h2>
    <p><b>Distancia:</b> <span id="dist">--</span> cm</p>

    <h2>MPU6050</h2>
    <p>Aceleración: X=<span id="ax">--</span> m/s^2 | Y=<span id="ay">--</span> m/s^2 | Z=<span id="az">--</span> m/s^2</p>
    <p>Giroscopio: X=<span id="gx">--</span> °/s | Y=<span id="gy">--</span> °/s | Z=<span id="gz">--</span> °/s</p>

    <h2>GPS</h2>
    <p>Latitud: <span id="lat">--</span></p>
    <p>Longitud: <span id="lng">--</span></p>
    <p>Satélites: <span id="sats">--</span></p>
    <p>Altitud: <span id="alt">--</span> m</p>
    <p>Hora UTC: <span id="hora">--</span></p>

    <script>
    async function updateData() {
      try {
        const response = await fetch("/data");
        const data = await response.json();

        document.getElementById("dist").textContent = data.distancia;
        document.getElementById("ax").textContent   = data.ax;
        document.getElementById("ay").textContent   = data.ay;
        document.getElementById("az").textContent   = data.az;
        document.getElementById("gx").textContent   = data.gx;
        document.getElementById("gy").textContent   = data.gy;
        document.getElementById("gz").textContent   = data.gz;

        document.getElementById("lat").textContent  = data.lat;
        document.getElementById("lng").textContent  = data.lng;
        document.getElementById("sats").textContent = data.sats;
        document.getElementById("alt").textContent  = data.alt;
        document.getElementById("hora").textContent = data.hora;
      } catch (e) {
        console.error("Error obteniendo datos:", e);
      }
    }
    setInterval(updateData, 500); // cada 500 ms (2 Hz)
    updateData();
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// --- Endpoint con datos JSON ---
void handleData() {
  // Lecturas de MPU6050 y TF-Luna
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  tflI2C.getData(tfDist, tfAddr);

  // Crear JSON con ArduinoJson
  StaticJsonDocument<256> doc;
  doc["distancia"] = tfDist;
  doc["ax"] = ax * 9.80665 / 16384;
  doc["ay"] = ay * 9.80665 / 16384;
  doc["az"] = az * 9.80665 / 16384;
  doc["gx"] = gx * PI / 180 / 131;
  doc["gy"] = gy * PI / 180 / 131;
  doc["gz"] = gz * PI / 180 / 131;

  if (gps.location.isValid()) {
    doc["lat"] = gps.location.lat();
    doc["lng"] = gps.location.lng();
  } else {
    doc["lat"] = 0;
    doc["lng"] = 0;
  }
  doc["sats"] = gps.satellites.value();
  doc["alt"] = gps.altitude.meters();

  char timeStr[16];
  sprintf(timeStr, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
  doc["hora"] = timeStr;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(BAUD);

  Wire.begin(SDAPin, SCLPin);

  // --- Iniciar MPU6050 ---
  Serial.println("Inicializando MPU6050...");
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 conectado correctamente");
  } else {
    Serial.println("Error: no se detecta el MPU6050");
  }

  // --- Iniciar GY-NEO6MV2 ---
  SerialGPS.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);
  Serial.println("Iniciando GPS...");

  // --- Configurar Access Point ---
  Serial.println("Creando Access Point...");
  WiFi.softAP(apSSID, apPassword);
  Serial.print("Conéctate a la red: ");
  Serial.println(apSSID);
  Serial.print("Clave: ");
  Serial.println(apPassword);

  Serial.print("IP del servidor: ");
  Serial.println(WiFi.softAPIP()); // normalmente 192.168.4.1

  // --- Servidor web ---
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Servidor web iniciado.");
}

void loop() {
  // Procesar GPS continuamente
  while (SerialGPS.available()) {
    gps.encode(SerialGPS.read());
  }

  // Servidor web
  server.handleClient();
}
