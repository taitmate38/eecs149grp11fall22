#include <Arduino.h>
#include <stdlib.h>
#include "wii_nunchuck_setup.h"
#include <LoRaLib.h>

// Nunchuck
#define WII_I2C_PORT 0
#define PIN_SDA 21
#define PIN_SCL 22

// Driving
#define AIM_THRESHOLD 150
#define STEER_THRESHOLD 200

#define MTR1_1 32
#define MTR1_2 33
#define STEER 4

// Modules
#define LaserPin 23

#define DEBUG false


unsigned int controller_type = 0;
static bool drive = false;
static int lastCycle = -1;

const int freq = 100;
const int steerChannel = 0;
const int resolution = 8;

int cycleTimer;

SX1278 lora = new LoRa;
int valu;


void handle_nunchuck(const unsigned char *data, wii_i2c_nunchuk_state* state);
void transmit_val(String to_transmit);


void setup() {
  Serial.begin(115200);

  Serial.printf("Starting...\n");

  // driving setup
  pinMode(MTR1_1, OUTPUT);
  pinMode(MTR1_2, OUTPUT);

  ledcSetup(steerChannel, freq, resolution);
  ledcAttachPin(STEER, steerChannel);


  // Nunchuk setup
  if (wii_i2c_init(WII_I2C_PORT, PIN_SDA, PIN_SCL) != 0)
  {
    Serial.printf("ERROR initializing wii i2c controller\n");
    return;
  }
  const unsigned char *ident = wii_i2c_read_ident();
  if (!ident)
  {
    Serial.printf("no ident :(\n");
    return;
  }

  controller_type = wii_i2c_decode_ident(ident);
  switch (controller_type)
  {
  case WII_I2C_IDENT_NUNCHUK:
    Serial.printf("-> nunchuk detected\n");
    break;
  default:
    Serial.printf("-> unknown controller detected: 0x%06x\n", controller_type);
    break;
  }
  wii_i2c_request_state();

  Serial.print(F("Initializing Radio ... "));
  // carrier frequency:           434.0 MHz
  // bandwidth:                   250.0 kHz
  // spreading factor:            9
  // coding rate:                 8
  // sync word:                   0x12
  // output power:                17 dBmdel
  // current limit:               240 mA
  // preamble length:             8 symbols
  // amplifier gain:              1 (maximum gain control)
  int state = lora.begin(434.0, 250, 6, 8, 0x12, 17, 240, 8, 1);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  cycleTimer = millis();
}

void loop() {

  const unsigned char *data = wii_i2c_read_state();
  wii_i2c_nunchuk_state state;

  Serial.print("1:"); // 1
  Serial.print(millis()-cycleTimer);

  wii_i2c_request_state();

  Serial.print(", 2:"); // 2
  Serial.print(millis()-cycleTimer);
  if (data)
  {
    
    switch (controller_type)
    {
    case WII_I2C_IDENT_NUNCHUK:
      handle_nunchuck(data, &state);
      break;
    default:
      // Serial.printf("data: %02x %02x %02x %02x %02x %02x\n",
      //               data[0], data[1], data[2], data[3], data[4], data[5]);
                    ;
      break;
    }
  }
  else
  {
    Serial.printf("no data :(\n");
  }

  //valu += 1;

  char buffer[35];

  int j = snprintf(buffer, 35, "%5d,%5d,%5d,%5d,%5d,%1d,%1d", 
                  state.acc_x, state.acc_y, state.acc_z, 
                  state.x, state.y, state.c, state.z);

  // Serial.println(j);
  // Serial.println(buffer);
  
  Serial.print(", 3:"); // 3
  Serial.print(millis()-cycleTimer);

  transmit_val(buffer);

  Serial.print(", total:");

  Serial.print(millis()-cycleTimer);
  cycleTimer = millis();
  Serial.printf(", hi = (%5d,%5d,%5d)\n", state.acc_x, state.acc_y, state.acc_z);
//  Serial.printf("d = (%5d,%5d)\n", state->x, state->y);
//  Serial.printf("c=%d, z=%d\n", state->c, state->z);

  
}

void transmit_val(String to_transmit) {
  String transmitted_value = to_transmit;
//  transmitted_value += "Datarate:\t\t";
//  transmitted_value += lora.getDataRate();
//  transmitted_value += " bps";
  
  int state = lora.transmit(transmitted_value);
  if (state == ERR_PACKET_TOO_LONG) {
    Serial.println(F(" too long!"));
  } else if (state == ERR_TX_TIMEOUT) {
    Serial.println(F(" timeout!"));
  }
}

void parse_nunchuck(const unsigned char *data, wii_i2c_nunchuk_state* state)
{
    wii_i2c_decode_nunchuk(data, state);

  // Serial.printf("a = (%5d,%5d,%5d)\n", state->acc_x, state->acc_y, state->acc_z);
  // Serial.printf("d = (%5d,%5d)\n", state->x, state->y);
  // Serial.printf("c=%d, z=%d\n", state->c, state->z);

}

void handle_nunchuck(const unsigned char *data, wii_i2c_nunchuk_state* state)
{
  parse_nunchuck(data, state);
  drive = state->z == 1;
  if (drive) {
    // drive forward
    Serial.println("DRIVE FORWARD");
    digitalWrite(MTR1_1, HIGH);
    digitalWrite(MTR1_2, LOW);
  }

  int steerDutyCycle;
  int acc_x = state->acc_x;
  if (acc_x > 200) {
    acc_x = 200;
  }
  if (acc_x < -200) {
    acc_x = -200;
  }
  steerDutyCycle = (int)(30 * -acc_x / 200 + 40);
  // Serial.println(steerDutyCycle);

  if (abs(steerDutyCycle - lastCycle) > 2) {
    ledcWrite(steerChannel, steerDutyCycle);
    lastCycle = steerDutyCycle;
  }
    
}