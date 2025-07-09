#include <SPI.h>
#include <Wire.h>
#include <Servo.h>

void Serial_clear() {
  while (Serial.available()) {
    Serial.read();
  }
}

const int ledPin = 13;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
}

int frame;
int phaseCooldownTimer;

int unityPhase = 0;
void loop() {
  delay(17); // FORNOW

  // send
  Serial.println(frame);

  // receive
  if (Serial.available()) {
    Serial_clear();
    phaseCooldownTimer = 10;
  }

  // updates
  frame++;
  if (phaseCooldownTimer > 0) {
    digitalWrite(ledPin, HIGH);
    --phaseCooldownTimer;
  } else {
    digitalWrite(ledPin, LOW);
  }
 


}
