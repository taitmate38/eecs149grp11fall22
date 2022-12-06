#include <Arduino.h>
#include <stdlib.h>
#include "wii_nunchuck_setup.h"

#define PIN_SDA 21
#define PIN_SCL 22
#define LaserPin 23
#define WII_I2C_PORT 0
#define AIM_THRESHOLD 150
#define STEER_THRESHOLD 200

#define MTR1_1 32
#define MTR1_2 33
#define STEER 4

unsigned int controller_type = 0;
static bool drive = false;
static int lastCycle = -1;

const int freq = 100;
const int steerChannel = 0;
const int resolution = 8;

void parse_nunchuck(const unsigned char *data, wii_i2c_nunchuk_state* state)
{
  wii_i2c_decode_nunchuk(data, state);

  Serial.printf("a = (%5d,%5d,%5d)\n", state->acc_x, state->acc_y, state->acc_z);
  Serial.printf("d = (%5d,%5d)\n", state->x, state->y);
  Serial.printf("c=%d, z=%d\n", state->c, state->z);
}

void handle_nunchuck(const unsigned char *data)
{
  wii_i2c_nunchuk_state state;
  parse_nunchuck(data, &state);
  drive = state.z == 1;
  if (drive) {
    // drive forward
    Serial.println("DRIVE FORWARD");
    digitalWrite(MTR1_1, HIGH);
    digitalWrite(MTR1_2, LOW);
  }

  int steerDutyCycle;
  int acc_x = state.acc_x;
  if (acc_x > 200) {
    acc_x = 200;
  }
  if (acc_x < -200) {
    acc_x = -200;
  }
  steerDutyCycle = (int)(30 * -acc_x / 200 + 40);
  Serial.println(steerDutyCycle);

  if (abs(steerDutyCycle - lastCycle) > 2) {
    ledcWrite(steerChannel, steerDutyCycle);
    lastCycle = steerDutyCycle;
  }
    
}

void setup()
{
  Serial.begin(115200);
  Serial.printf("Starting...\n");
  pinMode(MTR1_1, OUTPUT);
  pinMode(MTR1_2, OUTPUT);

  ledcSetup(steerChannel, freq, resolution);

  ledcAttachPin(STEER, steerChannel);

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
}

void loop()
{
  const unsigned char *data = wii_i2c_read_state();
  wii_i2c_request_state();
  if (data)
  {
    
    switch (controller_type)
    {
    case WII_I2C_IDENT_NUNCHUK:
      handle_nunchuck(data);
      break;
    default:
      Serial.printf("data: %02x %02x %02x %02x %02x %02x\n",
                    data[0], data[1], data[2], data[3], data[4], data[5]);
      break;
    }
  }
  else
  {
    Serial.printf("no data :(\n");
  }

  delay(50);
}