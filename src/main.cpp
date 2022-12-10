#include <Arduino.h>
#include <stdlib.h>
#include "wii_nunchuck_setup.h"

#define PIN_SDA 23
#define PIN_SCL 22
#define LaserPin 2
#define WII_I2C_PORT 0
#define AIM_THRESHOLD 150
#define STEER_THRESHOLD 200
#define TURRET_X 16
#define TURRET_Y 19

#define MTR1_1 2
#define MTR1_2 33
#define STEER 4

unsigned int controller_type = 0;
static bool drive = false;
static int lastCycle = -1;

static float x_aim_position = 30;
static float y_aim_position = 30;

const int freq = 300;
const int steerChannel = 0;
const int resolution = 12;

const int aim_freq = 100;
const int aim_resolution = 8;

const int xaimChannel = 1;
const int yaimChannel = 2;

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
  } else {
    digitalWrite(MTR1_1, LOW);
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
  steerDutyCycle = (int)(4 * -acc_x / 200 + 45);
  steerDutyCycle = 41;
  steerDutyCycle = 45*16;
  Serial.println(steerDutyCycle);

  if (abs(steerDutyCycle - lastCycle) > 2) {
    ledcWrite(steerChannel, steerDutyCycle);
    lastCycle = steerDutyCycle;
  }

  

  float x = state.x;
  if (abs(x) > 10) {
    if (x > 100)
    {
      x = 100;
    }
    if (x < -100)
    {
      x = -100;
    }
    float increment = x / 100;
    Serial.println("INCREMENT");
    Serial.println(increment);
    x_aim_position -= x / 100;
    if (x_aim_position < 10)
    {
      x_aim_position = 10;
    }

    if (x_aim_position > 70)
    {
      x_aim_position = 70;
    }
    ledcWrite(xaimChannel, x_aim_position);
    Serial.println("X AIM POSITION");
    Serial.println(x_aim_position);
  }

  float y = state.y;
  if (abs(y) > 10)
  {
    if (y > 100)
    {
      y = 100;
    }
    if (y < -100)
    {
      y = -100;
    }
    float increment = y / 100;
    Serial.println("INCREMENT");
    Serial.println(increment);
    y_aim_position -= y / 100;
    if (y_aim_position < 10)
    {
      y_aim_position = 10;
    }

    if (y_aim_position > 70)
    {
      y_aim_position = 70;
    }
    ledcWrite(yaimChannel, y_aim_position);
    Serial.println("Y AIM POSITION");
    Serial.println(y_aim_position);
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

  // ledcSetup(xaimChannel, aim_freq, aim_resolution);

  // ledcAttachPin(TURRET_X, xaimChannel);

  // ledcSetup(yaimChannel, aim_freq, aim_resolution);

  // ledcAttachPin(TURRET_Y, yaimChannel);

  x_aim_position = 30;
  y_aim_position = 30;
  ledcWrite(xaimChannel, x_aim_position);
  ledcWrite(yaimChannel, y_aim_position);

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
  // if (data)
  // {
  //   switch (controller_type)
  //   {
  //   case WII_I2C_IDENT_NUNCHUK:
  //     handle_nunchuck(data);
  //     break;
  //   default:
  //     Serial.printf("data: %02x %02x %02x %02x %02x %02x\n",
  //                   data[0], data[1], data[2], data[3], data[4], data[5]);
  //     break;
  //   }
  // }
  // else
  // {
  //   Serial.printf("no data :(\n");
  // }

  for (int sweep = 1700; sweep < 2450; sweep += 1) {
    digitalWrite(MTR1_1, sweep % 5);
   ledcWrite(steerChannel, sweep);
    Serial.println(sweep);
    delay(5);
  }

  delay(50);
}

// int trigPin = 2; // Trigger
// int echoPin = 17; // Echo
// long duration, cm, inches;

// void setup()
// {
//   // Serial Port begin
//   Serial.begin(115200);
//   // Define inputs and outputs
//   pinMode(trigPin, OUTPUT);
//   pinMode(echoPin, INPUT);
// }

// void loop()
// {
//   // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
//   // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
//   digitalWrite(trigPin, LOW);
//   delayMicroseconds(5);
//   digitalWrite(trigPin, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(trigPin, LOW);

//   // Read the signal from the sensor: a HIGH pulse whose
//   // duration is the time (in microseconds) from the sending
//   // of the ping to the reception of its echo off of an object.
//   pinMode(echoPin, INPUT);
//   duration = pulseIn(echoPin, HIGH);

//   // Convert the time into a distance
//   cm = (duration / 2) / 29.1;   // Divide by 29.1 or multiply by 0.0343
//   inches = (duration / 2) / 74; // Divide by 74 or multiply by 0.0135

//   Serial.print(inches);
//   Serial.print("in, ");
//   Serial.print(cm);
//   Serial.print("cm");
//   Serial.println();

//   // delay(250);
// }