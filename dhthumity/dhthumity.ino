#include "DHT.h"

// --- Configuración ---
#define DHTPIN 15        // Aquí pones el pin: 15, 2 o 4
#define DHTTYPE DHT11    // Tipo de sensor (DHT11 o DHT22)

// --- Objeto del sensor ---
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("Iniciando sensor DHT11...");
}

void loop() {
  // Lecturas del DHT11
  float humedad = dht.readHumidity();
  float temp = dht.readTemperature(); // °C
  float tempF = dht.readTemperature(true); // °F

  // Validar que los datos son correctos
  if (isnan(humedad) || isnan(temp)) {
    Serial.println("Error al leer el DHT11");
    return;
  }

  // Mostrar en el monitor serial
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.print(" %  |  Temperatura: ");
  Serial.print(temp);
  Serial.print(" °C  |  ");
  Serial.print(tempF);
  Serial.println(" °F");

  delay(2000); // el DHT11 es lento (~1 lectura cada 2 s)
}
