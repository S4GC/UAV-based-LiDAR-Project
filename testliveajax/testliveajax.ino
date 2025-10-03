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
    <p>Aceleración: X=<span id="ax">--</span> | Y=<span id="ay">--</span> | Z=<span id="az">--</span></p>
    <p>Giroscopio: X=<span id="gx">--</span> | Y=<span id="gy">--</span> | Z=<span id="gz">--</span></p>

    <script>
    async function updateData() {
      try {
        const response = await fetch("/data");
        const data = await response.json();

        document.getElementById("dist").textContent = data.distancia;
        document.getElementById("ax").textContent = data.ax;
        document.getElementById("ay").textContent = data.ay;
        document.getElementById("az").textContent = data.az;
        document.getElementById("gx").textContent = data.gx;
        document.getElementById("gy").textContent = data.gy;
        document.getElementById("gz").textContent = data.gz;
      } catch (e) {
        console.error("Error obteniendo datos:", e);
      }
    }
    setInterval(updateData, 10); // cada 10 ms (~100 Hz)
    updateData();
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// --- Endpoint con datos JSON ---
void handleData() {
  // obtener lecturas actualizadas
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  tflI2C.getData(tfDist, tfAddr);

  String json = "{";
  json += "\"distancia\":" + String(tfDist) + ",";
  json += "\"ax\":" + String(ax) + ",";
  json += "\"ay\":" + String(ay) + ",";
  json += "\"az\":" + String(az) + ",";
  json += "\"gx\":" + String(gx) + ",";
  json += "\"gy\":" + String(gy) + ",";
  json += "\"gz\":" + String(gz);
  json += "}";

  server.send(200, "application/json", json);
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

  // --- Servidor web ---
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Servidor web iniciado.");
}

void loop() {
  server.handleClient();
}

