#include <Arduino.h>
#include "wii_nunchuck_setup.h"

// void setup() {
//   // put your setup code here, to run once:
//   Serial.begin(115200);
//   while (!Serial)
//   {
//     ; // wait for serial port to connect. Needed for native USB port only
//   }
// }

// void loop() {
//   // put your main code here, to run repeatedly:
//   Serial.println("bruh");
// }

// void establishContact()
// {

//   while (Serial.available() <= 0)
//   {
//     // 
//     Serial.print(0xF0);

//     delay(300);
//   }
// }

#define PIN_SDA 21
#define PIN_SCL 22
#define WII_I2C_PORT 0

unsigned int controller_type = 0;

void show_nunchuk(const unsigned char *data)
{
  wii_i2c_nunchuk_state state;
  wii_i2c_decode_nunchuk(data, &state);

  Serial.printf("a = (%5d,%5d,%5d)\n", state.acc_x, state.acc_y, state.acc_z);
  Serial.printf("d = (%5d,%5d)\n", state.x, state.y);
  Serial.printf("c=%d, z=%d\n", state.c, state.z);
}

void setup()
{
  Serial.begin(115200);
  Serial.printf("Starting...\n");

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
      show_nunchuk(data);
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

  delay(500);
}