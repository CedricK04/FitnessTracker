#include <Wire.h>
#include "MAX30105.h"  // SparkFun MAX30102 library (sometimes named MAX30105)

MAX30105 particleSensor;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found. Check connections.");
    while (1);
  }

  particleSensor.setup();  // Default settings: 69 samples/sec, 411uA, 1600us pulse
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A);
}

void loop() {
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  Serial.print("IR: ");
  Serial.print(irValue);
  Serial.print(" RED: ");
  Serial.println(redValue);

  delay(100);
}
