#include <Wire.h>
#include "MAX30105.h"  // SparkFun MAX30105 library (compatible with MAX30102)

MAX30105 particleSensor;

// Peak detection parameters
const int BUFFER_SIZE = 100;
uint32_t irBuffer[BUFFER_SIZE]; // IR LED sensor data
int bufferIndex = 0;
unsigned long lastBeatTime = 0;
float beatsPerMinute;
int beatCount = 0;

// --- Simple Peak Detection for HR Estimation ---
void detectHeartRate(uint32_t irValue) {
  static uint32_t lastIRValue = 0;
  static bool rising = false;
  static uint32_t peak = 0;
  static uint32_t threshold = 50000; // Adjust based on environment/signal

  // Detect peaks (simplified logic)
  if (irValue > lastIRValue) {
    rising = true;
    peak = irValue;
  }

  if (irValue < lastIRValue && rising && peak > threshold) {
    unsigned long currentTime = millis();
    unsigned long deltaTime = currentTime - lastBeatTime;
    lastBeatTime = currentTime;

    // Calculate BPM
    beatsPerMinute = 60000.0 / deltaTime;
    beatCount++;

    Serial.print("BPM: ");
    Serial.println(beatsPerMinute);

    rising = false;
  }

  lastIRValue = irValue;
}

void setup() {
  Serial.begin(115200);
  delay(500); // Wait for serial monitor

  // Initialize I2C for ESP32-C3 (GPIO8=SDA, GPIO9=SCL)
  Wire.begin();

  // Initialize MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found. Check wiring.");
    while (1);
  }

  // Sensor setup: SpO2 mode, LED power, sample rate, pulse width
  particleSensor.setup();  // Default config: 69 samples/s, 411uA, 1600us pulse width
  particleSensor.setPulseAmplitudeRed(0x0A); // Low power red LED
  particleSensor.setPulseAmplitudeIR(0x0A);  // Low power IR LED

  Serial.println("MAX30102 initialized.");
}

void loop() {
  uint32_t irValue = particleSensor.getIR();
  uint32_t redValue = particleSensor.getRed();

  // Store IR value for HR analysis
  irBuffer[bufferIndex] = irValue;
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;

  // --- Output only numeric data for plotting ---
  Serial.print(irValue);
  Serial.print(", ");
  Serial.println(redValue);

  // Heart rate detection (basic peak detection on IR)
  detectHeartRate(irValue);

  delay(20); // ~50 Hz sampling rate (adjust if needed)
}