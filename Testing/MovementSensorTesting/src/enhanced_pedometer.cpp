#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#define MPU9250_ADDR 0x68
int16_t ax, ay, az;

// Filtered acceleration values
float axf = 0, ayf = 0, azf = 0;

int16_t gx, gy, gz;
float gxf = 0, gyf = 0, gzf = 0;

// DC-removal buffer
const int windowSize = 25; // ~1.25s at 20Hz
float magHistory[windowSize] = {0};
int historyIndex = 0;

unsigned long lastStepTime = 0;
int stepCount = 0;
float stepLengthTotal = 0.0;

const float stepThreshold = 0.25; // Adjust for sensitivity
const unsigned long stepInterval = 300; // ms
const float stepLengthFactor = 0.7; // Tune based on user height etc.

// Simple low-pass filter weight
const float alpha = 0.3;

void readAccelData() {
    Wire.beginTransmission(MPU9250_ADDR);
    Wire.write(0x3B); // Start from accel data
    Wire.endTransmission(false);
    Wire.requestFrom(MPU9250_ADDR, 14); // Accel (6) + Temp (2) + Gyro (6)
  
    if (Wire.available() == 14) {
      ax = (Wire.read() << 8) | Wire.read();
      ay = (Wire.read() << 8) | Wire.read();
      az = (Wire.read() << 8) | Wire.read();
      Wire.read(); Wire.read(); // skip temperature
      gx = (Wire.read() << 8) | Wire.read();
      gy = (Wire.read() << 8) | Wire.read();
      gz = (Wire.read() << 8) | Wire.read();
    } else {
      Serial.println("Failed to read sensor data");
    }
  }
  

void setup() {
  Wire.begin(4, 5); // ESP32-C3 I2C pins
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting MPU-9250 filtered pedometer...");

  // Wake up MPU-9250
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(100);
}

void loop() {
  readAccelData();

  // Convert to g's
  float axg = ax / 16384.0;
  float ayg = ay / 16384.0;
  float azg = az / 16384.0;

  // IIR Low-pass filter
  axf = alpha * axg + (1 - alpha) * axf;
  ayf = alpha * ayg + (1 - alpha) * ayf;
  azf = alpha * azg + (1 - alpha) * azf;

  // Convert gyro to deg/sec (or rad/s)
  float gx_dps = gx / 131.0;
  float gy_dps = gy / 131.0;
  float gz_dps = gz / 131.0;

  // Low-pass filter gyro data
  gxf = alpha * gx_dps + (1 - alpha) * gxf;
  gyf = alpha * gy_dps + (1 - alpha) * gyf;
  gzf = alpha * gz_dps + (1 - alpha) * gzf;

  float gMag = sqrt(gxf * gxf + gyf * gyf + gzf * gzf);

  // Accel magnitude
  float aMag = sqrt(axf * axf + ayf * ayf + azf * azf);

  // Moving average DC offset
  magHistory[historyIndex] = aMag;
  historyIndex = (historyIndex + 1) % windowSize;

  float magSum = 0;
  for (int i = 0; i < windowSize; i++) {
    magSum += magHistory[i];
  }
  float aMagMean = magSum / windowSize;
  float aMagNoDC = aMag - aMagMean;

  const float gyroThreshold = 15.0; // deg/s — adjust as needed

  if (aMagNoDC > stepThreshold && gMag > gyroThreshold && millis() - lastStepTime > stepInterval) {
    stepCount++;
    lastStepTime = millis();
    float stepLength = stepLengthFactor * sqrt(aMagNoDC);
    stepLengthTotal += stepLength;

    Serial.print("Step ");
    Serial.print(stepCount);
    Serial.print(" | Len: ");
    Serial.print(stepLength, 2);
    Serial.print(" m | GyroMag: ");
    Serial.print(gMag, 1);
    Serial.print(" | Total: ");
    Serial.println(stepLengthTotal, 2);
  }

  // Debug
  Serial.print("aMag: ");
  Serial.print(aMag, 3);
  Serial.print(" | aMagNoDC: ");
  Serial.println(aMagNoDC, 3);

  delay(50); // ~20 Hz sample rate
}
