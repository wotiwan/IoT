#include <Wire.h>

#define LED_PIN 5
#define BLINK_INTERVAL 500

int ADXL345 = 0x53;
float X_out, Y_out, Z_out;
float X1, Y1, Z1;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  Wire.begin();
  Wire.beginTransmission(ADXL345);
  Wire.write(0x2D); 
  Wire.write(0x08); 
  Wire.endTransmission();
}

void blink_lights() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    
    long start_time = millis();
    while(millis() - start_time < BLINK_INTERVAL) {}
    digitalWrite(LED_PIN, LOW);
    while(millis() - start_time < BLINK_INTERVAL * 1.5) {}
  }
}

void loop() {
  Wire.beginTransmission(ADXL345);
  Wire.write(0x32);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345, 6, true);
  X_out = (Wire.read() | Wire.read() << 8);
  X1 = X_out / 256;
  Y_out = (Wire.read() | Wire.read() << 8);
  Y1 = Y_out / 256;
  Z_out = (Wire.read() | Wire.read() << 8);
  Z1 = Z_out / 256;
  Serial.print(X1);
  Serial.print("    ");
  Serial.print(Y1);
  Serial.print("    ");
  Serial.print(Z1);
  Serial.println("    ");

  if (abs(X1) <= 0.05 && abs(Y1) <= 0.05 && abs(Z1) <= 0.05) { // Если нет ускорения ни в одной плоскости => невесомость
    blink_lights();
  }
}
