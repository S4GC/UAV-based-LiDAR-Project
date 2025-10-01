#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TFLI2C.h>
#include <MPU6050.h>

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

// --- Servidor web ---
WebServer server(80);

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta http-equiv='refresh' content='1'>"; // refrescar cada 1 seg
  html += "<title>ESP32 Sensor Hub</title></head><body>";
  html += "<h1>Lecturas de Sensores</h1>";

  // --- Distancia TF-Luna ---
  if (tflI2C.getData(tfDist, tfAddr)) {
    html += "<h2>TF-Luna</h2>";
    html += "<p><b>Distancia:</b> " + String(tfDist) + " cm</p>";
    html += "<p><b>Distancia:</b> " + String(tfDist / 2.54, 1) + " in</p>";
  } else {
    html += "<h2>TF-Luna</h2><p>Error leyendo</p>";
  }

  // --- MPU6050 ---
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  html += "<h2>MPU6050</h2>";
  html += "<p>Aceleración: X=" + String(ax) + " | Y=" + String(ay) + " | Z=" + String(az) + "</p>";
  html += "<p>Giroscopio: X=" + String(gx) + " | Y=" + String(gy) + " | Z=" + String(gz) + "</p>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  // Inicializar I2C (SDA=21, SCL=22 en ESP32)
  Wire.begin(21, 22);

  // --- Iniciar MPU6050 ---
  Serial.println("Inicializando MPU6050...");
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("✅ MPU6050 conectado correctamente");
  } else {
    Serial.println("❌ Error: no se detecta el MPU6050");
  }

  // --- Configurar Access Point ---
  Serial.println("Creando Access Point...");
  WiFi.softAP(apSSID, apPassword);
  Serial.print("✅ Conéctate a la red: ");
  Serial.println(apSSID);
  Serial.print("Clave: ");
  Serial.println(apPassword);

  Serial.print("IP del servidor: ");
  Serial.println(WiFi.softAPIP()); // normalmente 192.168.4.1

  // --- Servidor web ---
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Servidor web iniciado.");
}

void loop() {
  server.handleClient();
}

