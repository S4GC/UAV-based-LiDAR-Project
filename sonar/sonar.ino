#include <ESP32Servo.h>

Servo myservo;
const int pinServo = 23;
const int trigPin = 5;
const int echoPin = 18;

void setup() {
  Serial.begin(115200);
  myservo.attach(pinServo);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

long readUltrasonic() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  long distance = duration * 0.034 / 2; // sqrt(gamma*R*T) / 2
  return distance;
}



void loop() {
  for (int angle = 0; angle <= 180; angle += 1) {
    long dist = readUltrasonic();
    //Serial.print(angle);
    //Serial.print(",");
    Serial.println(dist);
    myservo.write(angle);
    delay(500);
  }

  for (int angle = 180; angle >= 0; angle -= 1) {
    long dist = readUltrasonic();
    //Serial.print(angle);
    //Serial.print(",");
    Serial.println(dist);
    myservo.write(angle);
    delay(500);
  }
}
