#include <Arduino.h>
#include <stdlib.h>
#include <LoRaLib.h>
#define PIN_SDA 21
#define PIN_SCL 22
#define LaserPin 23
#define WII_I2C_PORT 0
#define AIM_THRESHOLD 150
#define STEER_THRESHOLD 200

#define MTR1_1 33
#define MTR1_2 26
#define STEER 32

unsigned int controller_type = 0;
static bool drive = false;
static int lastCycle = -1;

const int freq = 300;
const int steerChannel = 0;
const int resolution = 12;



void handle_nunchuck(int tilt, int b0, int b1)
{
  drive = b0;
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
  int acc_x = tilt;
  if (acc_x > 200) {
    acc_x = 200;
  }
  if (acc_x < -200) {
    acc_x = -200;
  }
  steerDutyCycle = (int)((-1.5*tilt) + 2000);
  Serial.println(steerDutyCycle);

  if (abs(steerDutyCycle - lastCycle) > 2) {
    ledcWrite(steerChannel, steerDutyCycle);
    lastCycle = steerDutyCycle;
  }
    
}


SX1278 lora = new LoRa;
void setup()
{
  Serial.begin(115200);
  Serial.printf("Starting...\n");
  pinMode(MTR1_1, OUTPUT);
  pinMode(MTR1_2, OUTPUT);

  ledcSetup(steerChannel, freq, resolution);

  ledcAttachPin(STEER, steerChannel);

    Serial.print(F("Initializing ... "));
  // carrier frequency:           434.0 MHz
  // bandwidth:                   250.0 kHz
  // spreading factor:            10
  // coding rate:                 8
  // sync word:                   0x12
  // output power:                17 dBm
  // current limit:               240 mA
  // preamble length:             8 symbols
  // amplifier gain:              1 (maximum gain control)
  int state = lora.begin(434.0, 250, 7, 8, 0x12, 17, 240, 8, 1);
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

}

void loop()
{
  String str;
  
  int bigButton, smallButton, tilt = 0;

  int state = lora.receive(str);
  if (state == ERR_NONE) {
    // packet was successfully received
    Serial.println(F("success!"));

    // print data of the packet
    // Serial.print(F("Data:\t\t\t"));
    int ctr = 0;
    int index = 0;
    while (ctr < 6) {
      if (str[index] == ',') {
        ctr++;
      }

      if (ctr == 6) {
        // Serial.printf("buttonval: %d\n", str[index+1]);
        bigButton = str[index+1] == 48 ? 0 : 1;
      } else if (ctr == 5) {
        smallButton = str[index+1] == 48 ? 0 : 1;
      } else if ((ctr == 0) && str[index+1] == ',') {
        tilt = str.substring(0, index+1).toInt();
      }
      index++;
    }

    // Serial.printf("bb: %d, sb: %d, tilt: %d\n", bigButton, smallButton, tilt);
    Serial.println(str);

  } else if (state == ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));

  } else if (state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));

  }
  handle_nunchuck(tilt, bigButton, smallButton);
}