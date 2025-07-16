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
      display.setTextSize(8); \
      display.setTextColor(WHITE); \
      display.printf("%s", __DATE__);  \
      display.display(); \
      while(true) { \
      } \
    } \
  } while(0)

//////////////////////////////////////////////


#define SS_SWITCH   24
#define SEESAW_ADDR 0x36
Adafruit_seesaw ss;
uint8_t encoder_position;
float maxEncoderPos = 50;
float minEncoderPos = -50;


int LED_cooldown_timer;


struct ODriveStatus; // hack to prevent teensy compile error


//////////////////////////////////////////////

#undef LED_BUILTIN
#define LED_BUILTIN 13

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
  display.setTextSize(2);
  display.setTextColor(WHITE);
  char *tmp = __DATE__;
  tmp[strlen(tmp) - 4] = '\0';
  display.printf("%s\n%s", __DATE__, __TIME__);  \
  display.display();

//  ASSERT(2 + 2 == 5);

  ASSERT(ss.begin(SEESAW_ADDR));
  ASSERT(((ss.getVersion() >> 16) & 0xFFFF)  == 4991);
  ss.pinMode(SS_SWITCH, INPUT_PULLUP);
  ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
  ss.enableEncoderInterrupt();

  //ODrive:

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

    // Pump events for 150ms. This delay is needed for two reasons;
    // 1. If there is an error condition, such as missing DC power, the ODrive might
    //    briefly attempt to enter CLOSED_LOOP_CONTROL state, so we can't rely
    //    on the first heartbeat response, so we want to receive at least two
    //    heartbeats (100ms default interval).
    // 2. If the bus is congested, the setState command won't get through
    //    immediately but can be delayed.
    for (int i = 0; i < 15; ++i) {
      delay(10);
      pumpEvents(can_intf);
    }
  }

  Serial.println("ODrive running!");
  
}

int MODULO(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

void loop() {

  //ODrive:
  pumpEvents(can_intf);
  float SINE_PERIOD = 2.0f; // Period of the position command sine wave in seconds

  float t = 0.001 * millis();
  
  float phase = t * (TWO_PI / SINE_PERIOD);

  odrv0.setPosition(
    sin(phase), // position
    cos(phase) * (TWO_PI / SINE_PERIOD) // velocity feedforward (optional)
  );

  // print position and velocity for Serial Plotter
  if (odrv0_user_data.received_feedback) {
    Get_Encoder_Estimates_msg_t feedback = odrv0_user_data.last_feedback;
    odrv0_user_data.received_feedback = false;
    Serial.print("odrv0-pos:");
    Serial.print(feedback.Pos_Estimate);
    Serial.print(",");
    Serial.print("odrv0-vel:");
    Serial.println(feedback.Vel_Estimate);
  }
  //


  // // receive
  if (Serial.available()) {
    Serial_clear();
    LED_cooldown_timer = 100;
  }
  
  // // send

  //get and clamp encoder position
  uint8_t new_position = MODULO(ss.getEncoderPosition(), 24);


  if (encoder_position != new_position) {
    Serial.write(new_position);         // display new position
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
