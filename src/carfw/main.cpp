#include <Arduino.h>
#include <stdlib.h>
#include <LoRaLib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Driving
#define MTR1_1 33
#define MTR1_2 25
#define STEER 32
#define STEER_THRESHOLD 200

// Modules
#define IDT0 39
#define M0_0 5
#define M0_1 17
#define M0_2 16

// Modules
#define IDT1 36
#define M1_0 26
#define M1_1 27
#define M1_2 12
#define turret_x_min 820
#define turret_x_max 3400

#define turret_y_min 550
#define turret_y_max 2840

#define AIM_THRESHOLD 150


// drive parameters
unsigned int controller_type = 0;
static bool drive_fwd = false;
static bool drive_back = false;
static int lastCycle = -1;

// steering parameters
const int freq = 300;
const int steerChannel = 0;
const int resolution = 12;


static int MODULES[7] = {240, 600, 885, 2700, 1445, 1725, 4000};

static LiquidCrystal_I2C lcd(0x27, 16, 2);

static float turret_x = 1600;
static float turret_y = 1600;

int idt(int pin) {
  int identity = analogRead(pin);
  int idModule = -1;
  if (identity == 0) {
    return -1;
  }
  int closest = 9999;
  for (int i = 0; i < 7; i += 1)
  {
    int check = abs(identity - MODULES[i]);
    if (check < closest)
    {
      closest = check;
      idModule = i;
    }
  }
  Serial.print("ID MODULE: ");
  Serial.println(idModule);
  Serial.print("ANALOG READING: ");
  Serial.println(identity);
  Serial.printf("PIN NUMBER: %d \n", pin);
  return idModule;
}

void handle_modules(int tilt_x, int tilt_y, int joy_x, int joy_y, int c, int z, int idtPin, int gpio0, int gpio1, int gpio2) {
  int tilt_x_channel = 1;
  int tilt_y_channel = 2;
  float increment_x, increment_y;
  int bump;

  if (idtPin == IDT1) {
    lcd.clear();
    lcd.setCursor(0,0);
  } else {
    lcd.setCursor(0,1);
  }

  switch (idt(idtPin))
  {
  case 0:
    lcd.print("DIST");
    // DISTANCE MODULE
    float duration, cm, inches;
    pinMode(gpio1, OUTPUT);
    pinMode(gpio2, INPUT);
    digitalWrite(gpio1, LOW);
    delayMicroseconds(5);
    digitalWrite(gpio1, HIGH);
    delayMicroseconds(10);
    digitalWrite(gpio1, LOW);

    pinMode(gpio2, INPUT);
    duration = pulseIn(gpio2, HIGH);

    inches = (duration / 2) / 74;

    Serial.print("INCHES: ");
    Serial.println(inches);
    if (inches < 5 && inches > 0)
    {
      // idle for half a second
      digitalWrite(MTR1_1, LOW);
      digitalWrite(MTR1_2, LOW);

      delay(500);

      // back up for 1/2 second
      ledcWrite(steerChannel, 2000);
      digitalWrite(MTR1_1, LOW);
      digitalWrite(MTR1_2, HIGH);

      delay(500);

      // return to idle
      digitalWrite(MTR1_1, LOW);
      digitalWrite(MTR1_2, LOW);
      Serial.println("STOPPPPP");
    }

    break;
  case 1:
    lcd.print("BUMPER");
    pinMode(gpio1, INPUT);

    bump = digitalRead(gpio1);

    // Serial.printf("BUMP: %d \n\n", bump);
    if (bump) {
      // idle for half a second
      digitalWrite(MTR1_1, LOW);
      digitalWrite(MTR1_2, LOW);

      delay(500);

      // back up for 1/2 second
      ledcWrite(steerChannel, 2000);
      digitalWrite(MTR1_1, LOW);
      digitalWrite(MTR1_2, HIGH);

      delay(500);

      // return to idle
      digitalWrite(MTR1_1, LOW);
      digitalWrite(MTR1_2, LOW);
      Serial.println("STOPPPPP");
    }

    break;
  case 2:
    // code block
    break;
  case 3:
    lcd.print("TURRET");
    ledcSetup(tilt_x_channel, freq, resolution);
    ledcSetup(tilt_y_channel, freq, resolution);
    ledcAttachPin(gpio0, tilt_x_channel);
    ledcAttachPin(gpio1, tilt_y_channel);

    increment_x = 0;
    increment_y = 0;
    if (abs(joy_x) > 10)
    {
      increment_x = joy_x / 2;
    }
    if (abs(joy_y) > 10)
    {
      increment_y = joy_y / 2;
    }

    turret_x -= increment_x;
    turret_y += increment_y;

    if (turret_x > turret_x_max)
    {
      turret_x = turret_x_max;
    }
    if (turret_y > turret_y_max)
    {
      turret_y = turret_y_max;
    }
    if (turret_x < turret_x_min)
    {
      turret_x = turret_x_min;
    }
    if (turret_y < turret_y_min)
    {
      turret_y = turret_y_min;
    }

    Serial.printf("joy_x: %d \n", joy_x);
    Serial.printf("joy_y: %d \n", joy_y);

    Serial.printf("Turret_x: %f \n", turret_x);
    Serial.printf("Turret_y: %f \n", turret_y);
    ledcWrite(tilt_x_channel, turret_x);
    ledcWrite(tilt_y_channel, turret_y);
    break;
  case 4:
    lcd.print("LASER");
    // LASER
    pinMode(gpio0, OUTPUT);
    if (c)
    {
      digitalWrite(gpio0, HIGH);
    }
    else
    {
      digitalWrite(gpio0, LOW);
    }
    break;
  case 5:
    break;
  case 6:
    lcd.print("NOTHING");
    Serial.println("No Module plugged in");
    break;
  default:
    lcd.print("???");
    Serial.println("Unrecognized Module");
  }
}

void handle_nunchuck(int tilt_x, int tilt_y, int z, int c)
{
  drive_fwd = z;
  drive_back = z && tilt_y < -125;
  if (drive_back) {
    // drive forward
    Serial.println("DRIVE BACKWARD");
    digitalWrite(MTR1_1, LOW);
    digitalWrite(MTR1_2, HIGH);
  } else if (drive_fwd) {
    Serial.println("DRIVE FORWARD");
    digitalWrite(MTR1_1, HIGH);
    digitalWrite(MTR1_2, LOW);
  } else {
    digitalWrite(MTR1_1, LOW);
    digitalWrite(MTR1_2, LOW);
  }

  // clip nunchuck output to STEER_THRESHOLD
  int steerDutyCycle;
  int acc_x = tilt_x;
  if (acc_x > STEER_THRESHOLD) {
    acc_x = STEER_THRESHOLD;
  }
  if (acc_x < -STEER_THRESHOLD) {
    acc_x = -STEER_THRESHOLD;
  }
  steerDutyCycle = (int)((-1.5*tilt_x) + 2000);
  Serial.println(steerDutyCycle);

  // only update steering servo position if there has been a significant enough change in commanded position  
  if (abs(steerDutyCycle - lastCycle) > 2) {
    ledcWrite(steerChannel, steerDutyCycle);
    lastCycle = steerDutyCycle;
  }
    
}


SX1278 lora = new LoRa;

void setup()
{
  Serial.begin(115200);
  Serial.printf("Setting motor & steering pins...\n");
  pinMode(MTR1_1, OUTPUT);
  pinMode(MTR1_2, OUTPUT);

  ledcSetup(steerChannel, freq, resolution);
  ledcAttachPin(STEER, steerChannel);

  // initialize the LCD
  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.print("INITIALIZED");

  lcd.setCursor(0, 1);
  lcd.print(":)");

  turret_x = 1600;
  turret_y = 1600;

  Serial.print(F("Initializing Radio... "));
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
  
  int bigButton = 0;
  int smallButton = 0;
  int tilt = 0;

  int state = lora.receive(str);
  if (state == ERR_NONE) {
    // packet was successfully received
    Serial.println(F("success!"));

    int str_len = str.length() + 1;
    char char_array[100];
    str.toCharArray(char_array, str.length());
    char* token;
    char* end;
    int tilt_x = (int)strtol((strtok(char_array, ",")), &end, 10);
    int tilt_y = (int)strtol((strtok(NULL, ",")), &end, 10);
    int tilt_z = (int)strtol((strtok(NULL, ",")), &end, 10);
    int joy_x = (int)strtol((strtok(NULL, ",")), &end, 10);
    int joy_y = (int)strtol((strtok(NULL, ",")), &end, 10);
    int c = (int)strtol((strtok(NULL, ",")), &end, 10);
    int z = (int)strtol((strtok(NULL, ",")), &end, 10);

    printf("%5d,%5d,%5d,%5d,%5d,%1d,%1d \n\n",
           tilt_x, tilt_y, tilt_z,
           joy_x, joy_y, c, z);

    handle_nunchuck(tilt_x, tilt_y, z, c);
    handle_modules(tilt_x, tilt_y, joy_x, joy_y, c, z, IDT1, M1_0, M1_1, M1_2);
    handle_modules(tilt_x, tilt_y, joy_x, joy_y, c, z, IDT0, M0_0, M0_1, M0_2);
  } else if (state == ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));

  } else if (state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));
  }
}
