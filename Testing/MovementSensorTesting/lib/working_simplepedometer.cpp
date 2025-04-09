#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#define MPU9250_ADDR 0x68
int16_t ax, ay, az;

unsigned long lastStepTime = 0;
int stepCount = 0;
const float stepThreshold = 1.2; // g threshold
const unsigned long stepInterval = 300; // ms between steps to avoid double-counting

void readAccelData() {
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(0x3B); // Starting register for accel data
  Wire.endTransmission(false);
  Wire.requestFrom(MPU9250_ADDR, 6);

  if (Wire.available() == 6) {
    ax = (Wire.read() << 8) | Wire.read();
    ay = (Wire.read() << 8) | Wire.read();
    az = (Wire.read() << 8) | Wire.read();
  } else {
    Serial.println("Failed to read accel data");
  }
}

void setup() {
  Wire.begin(4, 5); // ESP32 I2C pins
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting MPU-9250 pedometer...");

  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0x00); // Wake up
  Wire.endTransmission();
  delay(100);
}

void loop() {
  readAccelData();

  float axg = ax / 16384.0;
  float ayg = ay / 16384.0;
  float azg = az / 16384.0;

  float aMagnitude = sqrt(axg * axg + ayg * ayg + azg * azg);

  Serial.print("Accel (g): ");
  Serial.print(axg, 2); Serial.print(", ");
  Serial.print(ayg, 2); Serial.print(", ");
  Serial.print(azg, 2); Serial.print(" | Mag: ");
  Serial.println(aMagnitude, 2);

  if (aMagnitude > stepThreshold && millis() - lastStepTime > stepInterval) {
    stepCount++;
    lastStepTime = millis();
    Serial.print("Step detected! Total steps: ");
    Serial.println(stepCount);
  }

  delay(50);
}