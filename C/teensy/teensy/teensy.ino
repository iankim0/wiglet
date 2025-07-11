float inverseLerp(float l, float u, int32_t pos) {
  return ((float)pos - l) / (u - l);
}

uint8_t lerp(int l, int u, float t) {
  return (uint8_t)(((u - l) * t) + l);
}

void Serial_clear() {
  while (Serial.available()) {
    Serial.read();
  }
}

// TODO: Serial2 to send to C; monitor Serial for debug
// (failing this, connect a screen)
      //display.print("ASSERT(" #condition "); failed // line %d\n", __LINE__);

#define STR(x) #x
#define XSTR(x) STR(x)
      
#define ASSERT(condition) \
  do { \
    if (!(condition)) { \
      display.printf("%d", __LINE__);  \
      display.display(); \
      while(true) { \
      } \
    } \
  } while(0)

//////////////////////////////////////////////

#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>
#define SS_SWITCH   24
#define SEESAW_ADDR 0x36
Adafruit_seesaw ss;
int32_t encoder_position;
float maxEncoderPos = 50;
float minEncoderPos = -50;

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 //OLED display height
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int LED_cooldown_timer;

//////////////////////////////////////////////

#undef LED_BUILTIN
#define LED_BUILTIN 13

void setup() {
  Serial.begin(115200);
  // while (!Serial) delay(100);

  pinMode(LED_BUILTIN, OUTPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
  display.clearDisplay();
  display.setTextSize(8);
  display.setTextColor(WHITE);
  display.setCursor(0, 5);

  ASSERT(2 + 2 == 5);

  ASSERT(ss.begin(SEESAW_ADDR));
  ASSERT(((ss.getVersion() >> 16) & 0xFFFF)  == 4991);
  ss.pinMode(SS_SWITCH, INPUT_PULLUP);
  ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
  ss.enableEncoderInterrupt();
}

void loop() {

  // // receive
  if (Serial.available()) {
    Serial_clear();
    LED_cooldown_timer = 100;
  }



  // // send

  //get and clamp encoder position
  int32_t new_position = ss.getEncoderPosition();

  //clamp
  if (new_position < minEncoderPos) {
    ss.setEncoderPosition(minEncoderPos);
    new_position = minEncoderPos;
  } else if (new_position > maxEncoderPos) {
    ss.setEncoderPosition(maxEncoderPos);
    new_position = maxEncoderPos;
  }

  if (encoder_position != new_position) {
    float t = inverseLerp(minEncoderPos, maxEncoderPos, new_position);
    Serial.write(lerp(0, 255, t));         // display new position
    encoder_position = new_position;      // and save for next round
  }

  // updates
  if (LED_cooldown_timer > 0) {
    digitalWrite(LED_BUILTIN, HIGH);
    --LED_cooldown_timer;
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

}
