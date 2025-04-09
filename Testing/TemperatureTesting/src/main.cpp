#include <Arduino.h>
#include <math.h>
 
int sensorPin = 3; // select the input pin for the potentiometer
 
double Thermistor(int RawADC) {
  if (RawADC == 0) return NAN; // avoid log(∞)

  double Temp;
  Temp = log(10000.0 * ((4095.0 / RawADC - 1)));  // <-- fix this
  Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp )) * Temp );
  Temp = Temp - 273.15;
  return Temp;
}

void setup() {
 Serial.begin(115200);
}
 
void loop() {
  Serial.print("Temp: ");
  Serial.println(Thermistor(analogRead(sensorPin)));
  delay(500);
}
