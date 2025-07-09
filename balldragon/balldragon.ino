#include <SPI.h>
#include <Wire.h>
#include <Servo.h>

void setup() {
  Serial.begin(115200);
}

int frame;
void loop() {
 Serial.println(frame);
 frame++;
 frame++;
 frame++;
 delay(17); // FORNOW
}

void readSerial() {
  // if (Serial.available()) {
    // String unityData = Serial.readStringUntil('\n');
    // ... = unityData.toFloat(); //float between 0 and 1
  // }
}
