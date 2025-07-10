/* Pushbutton, Teensyduino Tutorial #3
   http://www.pjrc.com/teensy/tutorial3.html

   This example code is in the public domain.
*/

void setup() {                
  Serial.begin(115200);
  pinMode(7, INPUT);
}

void loop()                     
{
  if (digitalRead(7) == HIGH) {
  } else {
    Serial.print("1234");
  }
  delay(150);
}
