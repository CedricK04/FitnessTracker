#include <Wire.h>
#include <Arduino.h>
#include <math.h>
#include "MAX30105.h"

// === I2C Setup ===
TwoWire I2C_MPU = TwoWire(0);
TwoWire I2C_MAX = TwoWire(1);

// === MPU9250 ===
#define MPU9250_ADDR 0x68
int16_t ax, ay, az;
float axf = 0, ayf = 0, azf = 0;
int16_t gx, gy, gz;
float gxf = 0, gyf = 0, gzf = 0;

const int windowSize = 25;
float magHistory[windowSize] = {0};
int historyIndex = 0;

unsigned long lastStepTime = 0;
int stepCount = 0;
float stepLengthTotal = 0.0;

const float stepThreshold = 0.25;
const unsigned long stepInterval = 300;
const float stepLengthFactor = 0.7;
const float alpha = 0.3;

void readAccelData() {
  I2C_MPU.beginTransmission(MPU9250_ADDR);
  I2C_MPU.write(0x3B);
  I2C_MPU.endTransmission(false);
  I2C_MPU.requestFrom(MPU9250_ADDR, 14);

  if (I2C_MPU.available() == 14) {
    ax = (I2C_MPU.read() << 8) | I2C_MPU.read();
    ay = (I2C_MPU.read() << 8) | I2C_MPU.read();
    az = (I2C_MPU.read() << 8) | I2C_MPU.read();
    I2C_MPU.read(); I2C_MPU.read(); // skip temp
    gx = (I2C_MPU.read() << 8) | I2C_MPU.read();
    gy = (I2C_MPU.read() << 8) | I2C_MPU.read();
    gz = (I2C_MPU.read() << 8) | I2C_MPU.read();
  } else {
    Serial.println("Failed to read MPU9250");
  }
}

// === MAX30105 ===
MAX30105 particleSensor;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- MPU9250 Init ---
  I2C_MPU.begin(4, 5); // SDA, SCL
  I2C_MPU.beginTransmission(MPU9250_ADDR);
  I2C_MPU.write(0x6B);
  I2C_MPU.write(0x00);
  I2C_MPU.endTransmission();
  Serial.println("MPU9250 initialized");

  // --- MAX30105 Init ---
  I2C_MAX.begin(6, 7); // SDA, SCL
  if (!particleSensor.begin(I2C_MAX, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30105 not found. Check wiring.");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A);

  Serial.println("MAX30105 initialized");
}

void loop() {
  // === MPU9250 Logic ===
  readAccelData();

  float axg = ax / 16384.0;
  float ayg = ay / 16384.0;
  float azg = az / 16384.0;

  axf = alpha * axg + (1 - alpha) * axf;
  ayf = alpha * ayg + (1 - alpha) * ayf;
  azf = alpha * azg + (1 - alpha) * azf;

  float gx_dps = gx / 131.0;
  float gy_dps = gy / 131.0;
  float gz_dps = gz / 131.0;

  gxf = alpha * gx_dps + (1 - alpha) * gxf;
  gyf = alpha * gy_dps + (1 - alpha) * gyf;
  gzf = alpha * gz_dps + (1 - alpha) * gzf;

  float gMag = sqrt(gxf * gxf + gyf * gyf + gzf * gzf);
  float aMag = sqrt(axf * axf + ayf * ayf + azf * azf);

  magHistory[historyIndex] = aMag;
  historyIndex = (historyIndex + 1) % windowSize;

  float magSum = 0;
  for (int i = 0; i < windowSize; i++) magSum += magHistory[i];
  float aMagMean = magSum / windowSize;
  float aMagNoDC = aMag - aMagMean;

  const float gyroThreshold = 15.0;

  if (aMagNoDC > stepThreshold && gMag > gyroThreshold && millis() - lastStepTime > stepInterval) {
    stepCount++;
    lastStepTime = millis();
    float stepLength = stepLengthFactor * sqrt(aMagNoDC);
    stepLengthTotal += stepLength;

    Serial.print("Step ");
    Serial.print(stepCount);
    Serial.print(" | Len: ");
    Serial.print(stepLength, 2);
    Serial.print(" m | Total: ");
    Serial.println(stepLengthTotal, 2);
  }

  // === MAX30105 Logic ===
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  Serial.print("IR: ");
  Serial.print(irValue);
  Serial.print(" RED: ");
  Serial.println(redValue);

  delay(50); // ~20Hz sample rate
}