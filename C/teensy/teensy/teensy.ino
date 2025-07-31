#include <Arduino.h>

#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ODriveCAN.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 //OLED display height
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define CAN_BAUDRATE 250000
#define ODRV0_NODE_ID 0
#define IS_TEENSY_BUILTIN 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#include <FlexCAN_T4.h>
#include "ODriveFlexCAN.hpp"

#define SS_SWITCH   24
#define SEESAW_ADDR 0x36
Adafruit_seesaw ss;
uint8_t encoder_position;
float maxEncoderPos = 50;
float minEncoderPos = -50;
float torque;

#undef LED_BUILTIN
#define LED_BUILTIN 13
int LED_cooldown_timer;

float inverseLerp(float l, float u, float pos) {
  return ((float)pos - l) / (u - l);
}

float lerp(float l, float u, float t) {
  return (((u - l) * t) + l);
}

int MODULO(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

void Serial_clear() {
  while (Serial.available()) {
    Serial.read();
  }
}

#define STR(x) #x
#define XSTR(x) STR(x)
      
#define ASSERT(condition) \
  do { \
    if (!(condition)) { \
      display.setTextSize(8); \
      display.setTextColor(WHITE); \
      display.printf("%s", __DATE__);  \
      display.display(); \
      while(true) { \
      } \
    } \
  } while(0)

//////////////////////////////////////////////

struct ODriveStatus; // hack to prevent teensy compile error

//CAN
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can_intf;

void setupCan() {
  can_intf.begin();
  can_intf.setBaudRate(CAN_BAUDRATE);
  can_intf.setMaxMB(16);
  can_intf.enableFIFO();
  can_intf.enableFIFOInterrupt();
  can_intf.onReceive(onCanMessage);
}


//ODrive Objects
ODriveCAN odrv0(wrap_can_intf(can_intf), ODRV0_NODE_ID); // Standard CAN message ID
ODriveCAN* odrives[] = {&odrv0}; // Make sure all ODriveCAN instances are accounted for here

struct ODriveUserData {
  Heartbeat_msg_t last_heartbeat;
  bool received_heartbeat = false;
  Get_Encoder_Estimates_msg_t last_feedback;
  bool received_feedback = false;
};

// Keep some application-specific user data for every ODrive.
ODriveUserData odrv0_user_data;

// Called every time a Heartbeat message arrives from the ODrive
void onHeartbeat(Heartbeat_msg_t& msg, void* user_data) {
  ODriveUserData* odrv_user_data = static_cast<ODriveUserData*>(user_data);
  odrv_user_data->last_heartbeat = msg;
  odrv_user_data->received_heartbeat = true;
}

// Called every time a feedback message arrives from the ODrive
void onFeedback(Get_Encoder_Estimates_msg_t& msg, void* user_data) {
  ODriveUserData* odrv_user_data = static_cast<ODriveUserData*>(user_data);
  odrv_user_data->last_feedback = msg;
  odrv_user_data->received_feedback = true;
}

// Called for every message that arrives on the CAN bus
void onCanMessage(const CanMsg& msg) {
  for (auto odrive: odrives) {
    onReceive(msg, *odrive);
  }
}
/////////////

float current_position;
float newAngle;
char buffer[4];
void setup() {
  Serial.begin(2500000);

  pinMode(LED_BUILTIN, OUTPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }

  ASSERT(ss.begin(SEESAW_ADDR));
  ASSERT(((ss.getVersion() >> 16) & 0xFFFF)  == 4991);
  ss.pinMode(SS_SWITCH, INPUT_PULLUP);
  ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
  ss.enableEncoderInterrupt();

//  ODrive:

  odrv0.onFeedback(onFeedback, &odrv0_user_data);
  odrv0.onStatus(onHeartbeat, &odrv0_user_data);


  // Configure and initialize the CAN bus interface. This function depends on
  // your hardware and the CAN stack that you're using.

  setupCan();

  Serial.println("Waiting for ODrive...");
  while (!odrv0_user_data.received_heartbeat) {
    pumpEvents(can_intf);
  }

  while (odrv0_user_data.last_heartbeat.Axis_State != ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrv0.clearErrors();
    delay(1);
    odrv0.setState(ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL);

    // Pump events for 150ms
    for (int i = 0; i < 15; ++i) {
      delay(10);
      pumpEvents(can_intf);
    }
  }

//posgain normally set to 20
  Serial.println("ODrive running!"); 
  odrv0.setPosGain(15.0f);
 odrv0.setVelGains(0.167f, 1.0f);
}

unsigned long prev_refresh_timestamp;
unsigned long prev_frame_timestamp;
void loop() {
  //ODrive:
  pumpEvents(can_intf);
            
  unsigned long _millis = millis();
  //unsigned long delta_frame = (_millis - prev_frame_timestamp);
  prev_frame_timestamp = _millis;
  unsigned long delta_refresh = (_millis - prev_refresh_timestamp);
  if (delta_refresh > 1000 / 24) {
    prev_refresh_timestamp = millis();
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    char date_no_year[32];
    strcpy(date_no_year, __DATE__);
    date_no_year[strlen(date_no_year) - 4] = '\0';
    //int fps = (int) (1000 / delta_frame);    
    display.printf("%s\n%s\n%.2f\n%f ", date_no_year, __TIME__, current_position, torque);
    display.display();
  }
  
  Serial.write((byte *) &current_position, 4);

  // // receive
  if (Serial.available() >= 4) {
    Serial.readBytes(buffer, 4);  
    memcpy(&newAngle, buffer, 4);   
    odrv0.setPosition(newAngle);
    Serial_clear();
    LED_cooldown_timer = 100; 
  }
  
  current_position = odrv0_user_data.last_feedback.Pos_Estimate;
  
  // // send
  uint8_t new_position = MODULO(ss.getEncoderPosition(), 24);

  if (encoder_position != new_position) {
    float t = inverseLerp(0.0f, 23.0f, (float)encoder_position);
    torque = lerp(0.0f, 0.025f, t);
    encoder_position = new_position;    // and save for next round
  }

  // updates
  if (LED_cooldown_timer > 0) {
    digitalWrite(LED_BUILTIN, HIGH);
    --LED_cooldown_timer;
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
