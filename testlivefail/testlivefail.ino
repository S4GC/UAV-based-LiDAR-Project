#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
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

// --- Servidor Web + WebSocket ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");  // WebSocket en /ws

// --- Página HTML que el ESP32 sirve ---
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32 Sensor Hub</title>
  <style>
    body { font-family: Arial; background: #f2f2f2; padding: 20px; }
    h1 { color: #333; }
    .box { background: #fff; padding: 15px; margin: 10px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.2); }
  </style>
</head>
<body>
  <h1>Lecturas en tiempo real</h1>
  <div class="box">
    <h2>TF-Luna</h2>
    <p><b>Distancia:</b> <span id="tf">--</span> cm</p>
  </div>
  <div class="box">
    <h2>MPU6050</h2>
    <p><b>Aceleración:</b> <span id="accel">--</span></p>
    <p><b>Giroscopio:</b> <span id="gyro">--</span></p>
  </div>

  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;
    window.addEventListener('load', onLoad);

    function onLoad() {
      initWebSocket();
    }

    function initWebSocket() {
      websocket = new WebSocket(gateway);
      websocket.onmessage = function(event) {
        let data = JSON.parse(event.data);
        document.getElementById("tf").innerText = data.tf;
        document.getElementById("accel").innerText =
          "X=" + data.ax + " Y=" + data.ay + " Z=" + data.az;
        document.getElementById("gyro").innerText =
          "X=" + data.gx + " Y=" + data.gy + " Z=" + data.gz;
      };
    }
  </script>
</body>
</html>
)rawliteral";

// --- Función para enviar datos vía WebSocket ---
void notifyClients() {
  String json = "{";
  json += "\"tf\":" + String(tfDist) + ",";
  json += "\"ax\":" + String(ax) + ",";
  json += "\"ay\":" + String(ay) + ",";
  json += "\"az\":" + String(az) + ",";
  json += "\"gx\":" + String(gx) + ",";
  json += "\"gy\":" + String(gy) + ",";
  json += "\"gz\":" + String(gz);
  json += "}";
  ws.textAll(json); // envía a todos los clientes conectados
}

void setup() {
  Serial.begin(115200);

  // Inicializar I2C (SDA=21, SCL=22 en ESP32)
  Wire.begin(21, 22);

  // --- Iniciar MPU6050 ---
  Serial.println("Inicializando MPU6050...");
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 conectado correctamente");
  } else {
    Serial.println("Error: no se detecta el MPU6050");
  }

  // --- Configurar Access Point ---
  Serial.println("Creando Access Point...");
  WiFi.softAP(apSSID, apPassword);
  Serial.print("Conéctate a la red: ");
  Serial.println(apSSID);
  Serial.print("Clave: ");
  Serial.println(apPassword);

  Serial.print("IP del servidor: ");
  Serial.println(WiFi.softAPIP()); // normalmente 192.168.4.1

  // --- Configurar servidor ---
  ws.onEvent([](AsyncWebSocket * server, AsyncWebSocketClient * client, 
                AwsEventType type, void * arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      Serial.println("Cliente conectado al WebSocket");
    }
  });
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", htmlPage);
  });

  server.begin();
  Serial.println("Servidor Web iniciado.");
}

void loop() {
  // Leer sensores
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  tflI2C.getData(tfDist, tfAddr);

  // Enviar datos a clientes WebSocket
  notifyClients();

  delay(50); // ~20Hz de actualización
}
