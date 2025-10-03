#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TFLI2C.h>
#include <MPU6050.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <ESP32Servo.h>

// --- Configuración del Access Point ---
const char* apSSID     = "ESP32-SensorHub";
const char* apPassword = "12345678";

// --- Sensor TF-Luna ---
TFLI2C tflI2C;
int16_t tfDist;
int16_t tfAddr = TFL_DEF_ADR;

// --- Sensor MPU6050 ---
MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;

// --- Sensor GPS ---
TinyGPSPlus gps;
HardwareSerial SerialGPS(2);
const uint32_t GPSBaud = 9600;

// --- Sensor DHT ---
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- Servo + HC-SR04 ---
Servo myservo;
const int pinServo = 23;
const int trigPin = 5;
const int echoPin = 18;
int currentAngle = 0;
bool servoForward = true;
unsigned long lastServoMove = 0;
const int servoInterval = 200; // ms entre pasos
long lastUltrasonic = 0;

// --- Pines ---
const int BAUD = 115200;
const int SDAPin = 21;
const int SCLPin = 22;
const int RXPin  = 16;
const int TXPin  = 17;

// --- Servidor web ---
WebServer server(80);

// --- Variables de lectura diferenciada ---
unsigned long lastFastUpdate = 0; // TF-Luna, MPU, GPS
unsigned long lastSlowUpdate = 0; // DHT11, Servo, HC-SR04
const int fastInterval = 500;     // 500 ms
const int dhtInterval  = 2000;    // 2 s para DHT11
const int servoIntervalGlobal = 250; // servo/ultrasonido cada 250 ms

// --- Variables globales con últimos datos ---
float g_ax, g_ay, g_az, g_gx, g_gy, g_gz;
float g_lat, g_lng, g_alt;
int   g_sats;
int   g_tfDist;
float g_temp, g_tempF, g_hum;
int   g_angle;
long  g_ultra;
String g_hora;

// --- Función para medir ultrasonido ---
long readUltrasonic() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  return distance;
}

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
    <p>Aceleración: X=<span id="ax">--</span> | Y=<span id="ay">--</span> | Z=<span id="az">--</span> m/s^2</p>
    <p>Giroscopio: X=<span id="gx">--</span> | Y=<span id="gy">--</span> | Z=<span id="gz">--</span> °/s</p>

    <h2>GPS</h2>
    <p>Latitud: <span id="lat">--</span></p>
    <p>Longitud: <span id="lng">--</span></p>
    <p>Satélites: <span id="sats">--</span></p>
    <p>Altitud: <span id="alt">--</span> m</p>
    <p>Hora UTC: <span id="hora">--</span></p>

    <h2>DHT11</h2>
    <p>Temperatura: <span id="temp">--</span> °C | <span id="tempF">--</span> °F</p>
    <p>Humedad: <span id="hum">--</span> %</p>

    <h2>Ultrasonido + Servo</h2>
    <p>Ángulo: <span id="angle">--</span> ° | Distancia: <span id="ultra">--</span> cm</p>

    <script>
    async function updateData() {
      try {
        const response = await fetch("/data");
        const data = await response.json();

        document.getElementById("dist").textContent   = data.distancia;
        document.getElementById("ax").textContent     = data.ax.toFixed(2);
        document.getElementById("ay").textContent     = data.ay.toFixed(2);
        document.getElementById("az").textContent     = data.az.toFixed(2);
        document.getElementById("gx").textContent     = data.gx.toFixed(2);
        document.getElementById("gy").textContent     = data.gy.toFixed(2);
        document.getElementById("gz").textContent     = data.gz.toFixed(2);

        document.getElementById("lat").textContent    = data.lat;
        document.getElementById("lng").textContent    = data.lng;
        document.getElementById("sats").textContent   = data.sats;
        document.getElementById("alt").textContent    = data.alt;
        document.getElementById("hora").textContent   = data.hora;

        document.getElementById("temp").textContent   = data.temp;
        document.getElementById("tempF").textContent  = data.tempF;
        document.getElementById("hum").textContent    = data.hum;

        document.getElementById("angle").textContent  = data.angle;
        document.getElementById("ultra").textContent  = data.ultra;
      } catch (e) {
        console.error("Error obteniendo datos:", e);
      }
    }
    setInterval(updateData, 500); // actualización rápida
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// --- Endpoint con datos JSON ---
void handleData() {
  StaticJsonDocument<512> doc;

  doc["distancia"] = g_tfDist;
  doc["ax"] = g_ax;
  doc["ay"] = g_ay;
  doc["az"] = g_az;
  doc["gx"] = g_gx;
  doc["gy"] = g_gy;
  doc["gz"] = g_gz;

  doc["lat"]  = g_lat;
  doc["lng"]  = g_lng;
  doc["sats"] = g_sats;
  doc["alt"]  = g_alt;
  doc["hora"] = g_hora;

  doc["hum"]   = g_hum;
  doc["temp"]  = g_temp;
  doc["tempF"] = g_tempF;

  doc["angle"] = g_angle;
  doc["ultra"] = g_ultra;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(BAUD);
  Wire.begin(SDAPin, SCLPin);

  mpu.initialize();
  if (mpu.testConnection()) Serial.println("MPU6050 OK");
  else Serial.println("Error: MPU6050 no detectado");

  SerialGPS.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);

  dht.begin();
  myservo.attach(pinServo);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  WiFi.softAP(apSSID, apPassword);
  Serial.print("IP servidor: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  // GPS
  while (SerialGPS.available()) gps.encode(SerialGPS.read());

  unsigned long now = millis();

  // --- Sensores rápidos (TF-Luna, MPU, GPS) cada 500 ms ---
  if (now - lastFastUpdate > fastInterval) {
    lastFastUpdate = now;

    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    tflI2C.getData(tfDist, tfAddr);

    g_ax = ax * 9.80665 / 16384;
    g_ay = ay * 9.80665 / 16384;
    g_az = az * 9.80665 / 16384;
    g_gx = gx / 131.0;
    g_gy = gy / 131.0;
    g_gz = gz / 131.0;
    g_tfDist = tfDist;

    g_lat  = gps.location.isValid() ? gps.location.lat() : 0;
    g_lng  = gps.location.isValid() ? gps.location.lng() : 0;
    g_sats = gps.satellites.value();
    g_alt  = gps.altitude.meters();

    char timeStr[16];
    sprintf(timeStr, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
    g_hora = timeStr;
  }

  // --- Sensores lentos (DHT11) cada 2 s ---
  if (now - lastSlowUpdate > dhtInterval) {
    lastSlowUpdate = now;

    g_hum   = dht.readHumidity();
    g_temp  = dht.readTemperature();
    g_tempF = dht.readTemperature(true);
    if (isnan(g_hum) || isnan(g_temp)) {
      g_hum = g_temp = g_tempF = -1;
    }
  }

  // --- Servo + HC-SR04 cada 250 ms ---
  if (now - lastServoMove > servoIntervalGlobal) {
    lastServoMove = now;

    myservo.write(currentAngle);
    g_ultra = readUltrasonic();
    g_angle = currentAngle;

    if (servoForward) {
      currentAngle++;
      if (currentAngle >= 180) servoForward = false;
    } else {
      currentAngle--;
      if (currentAngle <= 0) servoForward = true;
    }
  }

  server.handleClient();
}

