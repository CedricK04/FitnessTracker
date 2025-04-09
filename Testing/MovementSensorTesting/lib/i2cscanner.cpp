#include <Arduino.h>
#include <Wire.h>

void setup() {
  Wire.begin(4, 5); // ESP32-C3 I²C pins
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Scanning I2C bus...");
  byte error, address;
  int count = 0;

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println();
      count++;
    }
  }
  if (count == 0) Serial.println("No I2C devices found.");
  else Serial.println("Scan complete.");
}

void loop() {}
