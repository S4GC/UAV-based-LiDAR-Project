#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// Objeto GPS
TinyGPSPlus gps;

// Creamos Serial2 para el GPS
HardwareSerial SerialGPS(2);

// Pines para UART2
const int RXPin = 16;  // GPS TX -> ESP32 RX2
const int TXPin = 17;  // GPS RX -> ESP32 TX2
const uint32_t GPSBaud = 9600; // Baudrate por defecto del NEO6M

void setup() {
  Serial.begin(115200);       
  SerialGPS.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);

  Serial.println("Iniciando GPS...");
}

void loop() {
  while (SerialGPS.available()) { // SerialGPS.available() > 0
    char c = SerialGPS.read();    // lee un carácter
    Serial.print(c);              // muestra NMEA crudo
    gps.encode(c);                // pasa el mismo char al parser

    // Si TinyGPS++ detecta que hay datos nuevos de posición
    if (gps.location.isUpdated()) {
      Serial.print("Latitud: ");
      Serial.println(gps.location.lat(), 6);
      Serial.print("Longitud: ");
      Serial.println(gps.location.lng(), 6);
      Serial.print("Satélites: ");
      Serial.println(gps.satellites.value());
      Serial.print("Altitud: ");
      Serial.print(gps.altitude.meters());
      Serial.println(" m");
      Serial.print("Hora UTC: ");
      Serial.printf("%02d:%02d:%02d\n",
                    gps.time.hour(),
                    gps.time.minute(),
                    gps.time.second());
      Serial.println("-------------------------");
    }
  }
}