#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#define MPU9250_ADDR 0x68
int16_t ax, ay, az;

unsigned long lastStepTime = 0;
int stepCount = 0;
float totalDistance = 0.0; // meters

const float stepThreshold = 1.2; // g threshold
const unsigned long stepInterval = 300; // ms
const float stepLengthFactor = 0.5; // Tunable: step length = k * sqrt(peakMag - 1.0)

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
  Wire.begin(4, 5); // ESP32-C3 I2C pins
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting MPU-9250 pedometer...");

  // Wake up MPU-9250
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0x00); // Wake up
  Wire.endTransmission();
  delay(100);
}

void loop() {
  readAccelData();

  float axg = ax / 16384.0;
  float ayg = ay / 16384.0;
  float azg = az / 16384.0;

  float aMag = sqrt(axg * axg + ayg * ayg + azg * azg);

  Serial.print("Accel (g): ");
  Serial.print(axg, 2); Serial.print(", ");
  Serial.print(ayg, 2); Serial.print(", ");
  Serial.print(azg, 2); Serial.print(" | Mag: ");
  Serial.print(aMag, 2);

  if (aMag > stepThreshold && millis() - lastStepTime > stepInterval) {
    stepCount++;
    lastStepTime = millis();

    // Estimate step length
    float stepLength = stepLengthFactor * sqrt(aMag - 1.0);
    totalDistance += stepLength;

    Serial.print(" -> Step! Count: ");
    Serial.print(stepCount);
    Serial.print(" | Step Length: ");
    Serial.print(stepLength, 2);
    Serial.print(" m | Total Distance: ");
    Serial.print(totalDistance, 2);
    Serial.println(" m");
  } else {
    Serial.println();
  }

  delay(50);
}
