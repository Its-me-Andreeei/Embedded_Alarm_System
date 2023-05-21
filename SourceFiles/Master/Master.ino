#include <Crypto.h>

#include <SHA256.h>

#include <avr/sleep.h>
#include <avr/power.h>

#include <Wire.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <Keypad.h>
#include <stdlib.h>
#include <string.h>

#define SLAVE_PRESENT_CF
#define DEBUG_CF
#define PIR_CALIB_TM_CF 20000U
#define TONE_DURATION_CF 3000
#define FIRST_SENSOR_FREQ_CF 1000
#define SECOND_SENSOR_FREQ_CF 220

#define PASS_LEN 4U +1U
const byte ROWS = 4; 
const byte COLS = 4; 

SHA256 sha256;
const byte password_encrypted[32]={0xB3, 0xD9, 0x1B, 0xF8, 0xA2, 0x3F, 0xA7, 0xEB, 0x3E, 0x26, 0xD5, 0x34, 0x40, 0xE9, 0x0A, 0x12, 0x73, 0x03, 0x0C, 0x6E, 0xB8, 0x69, 0xA0, 0x7B, 0x04, 0x65, 0x0E, 0xDD, 0x94, 0x97, 0xB3, 0x84};

//----------------pins---------------------
const uint8_t servo_PIN = 11;
const uint8_t PIR_sensor1_PIN = 2;
const uint8_t PIR_sensor2_PIN = 3;
const uint8_t buzzer_PIN = 12;
//-----------------------------------------

enum {
  NOT_PROTECTED,
  PROTECTED,
  ENABLED,
  NOT_ENABLED,
  MUST_INIT
};

struct systemState{
  uint8_t protectionState;
  uint8_t servoState;
  uint8_t detectionSensorState;
}state;


char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', '4'},
  {'5', '6', '7', '8'},
  {'9', 'A', 'B', 'C'},
  {'D', 'E', 'F', 'G'}
};

byte rowPins[ROWS] = {13, 10, 9, 8}; 
byte colPins[COLS] = {7, 6, 5, 4};

//index used for comparing entered password
uint8_t index;
byte pass_to_be_checked[32];
byte pass_to_be_checked_after_encryption[32];
Servo servoModule;

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

#ifdef SLAVE_PRESENT_CF
  void write_status(const char *str)
  {
    Wire.beginTransmission(0x1);
    Wire.write(str);
    Wire.endTransmission();
  }
#else
   void write_status(const char *str)
  {
    Serial.println(str);
  }
#endif
#ifdef DEBUG_CF
  #define Monitor_Write(x) Serial.println(x)
#else
  #define Monitor_Write(x) 
#endif

void setup()
{
  pinMode(servo_PIN, OUTPUT);
  pinMode(PIR_sensor1_PIN, INPUT);
  pinMode(PIR_sensor2_PIN, INPUT);

  Wire.begin();
  write_status(" ");
  servoModule.attach(servo_PIN);
  servoModule.write(0);
  delay(15);
  #ifdef DEBUG_CF
    Serial.begin(9600);
  #endif
  index = 0;
  
  state.protectionState = NOT_PROTECTED;
  state.detectionSensorState = MUST_INIT;
  state.servoState = NOT_ENABLED;

}
  
void loop(){
  if(state.protectionState == NOT_PROTECTED)
  {
    uint8_t index_pass = 0;
    byte readKey;
    while(index_pass < PASS_LEN -1)
    {
      readKey = (byte) customKeypad.getKey();
      if(readKey)
      {
        Monitor_Write((char) readKey);
        pass_to_be_checked[index_pass++] = readKey;
      }
    }

    sha256.update(pass_to_be_checked, sizeof(pass_to_be_checked));
    sha256.finalize(pass_to_be_checked_after_encryption, sizeof(pass_to_be_checked_after_encryption));
    sha256.clear();
    for (int i = 0; i < sizeof(pass_to_be_checked_after_encryption); i++) {
              Serial.print(pass_to_be_checked_after_encryption[i] < 0x10 ? "0" : "");  // Ensure leading zero for values less than 0x10
              Serial.print(pass_to_be_checked_after_encryption[i], HEX);  // Print the hash value as hexadecimal
  }
    boolean pass_OK = true;
    for(byte i = 0; i<sizeof(password_encrypted); i++)
    {
      if(pass_to_be_checked_after_encryption[i] != password_encrypted[i])
        pass_OK = false;
    }
    if(pass_OK)
    {
      state.protectionState = PROTECTED;
    }
    else
    {
        index_pass = 0;
        write_status("Wrong Password");
        delay(2000);
        write_status(" ");
    }
  }
  else if(state.protectionState == PROTECTED)
  {
    if(state.servoState == NOT_ENABLED)
    {
      write_status("alarm enabled");
      servoModule.write(179);
      //delay(15);
      state.servoState = ENABLED;
    }
    if(state.detectionSensorState == MUST_INIT)
    {
      //PIR sensors requirement wait for calibration
      if(millis() >= PIR_CALIB_TM_CF)
      {
        state.detectionSensorState = ENABLED;
        write_status("PIR INIT: done");
        delay(2000); //system sleep for lcd clean-up - no risk for using delay here - sensor not read
        write_status(" "); // workaround for lcd to clear screen
      }
    }
    if(state.detectionSensorState == ENABLED)
    {
      uint8_t sensor_value1 = digitalRead(PIR_sensor1_PIN);
      uint8_t sensor_value2 = digitalRead(PIR_sensor2_PIN);

      //Monitor_Write(sensor_value1);
      Monitor_Write(sensor_value2);

      if(sensor_value1==HIGH && sensor_value2 == HIGH)
      {
        write_status("Alert room 1.Alert room 2");
        for(char i=0; i<15; i++)
        {
          tone(buzzer_PIN, FIRST_SENSOR_FREQ_CF, TONE_DURATION_CF);
          delay(100);
          tone(buzzer_PIN, SECOND_SENSOR_FREQ_CF, TONE_DURATION_CF);
          delay(100);
        }
      }
      else if(sensor_value1 == HIGH)
      {
        write_status("Alert room 1");
        tone(buzzer_PIN, FIRST_SENSOR_FREQ_CF, TONE_DURATION_CF);
      }
      else if(sensor_value2 == HIGH)
      {
        write_status("Alert room 2");
        tone(buzzer_PIN, SECOND_SENSOR_FREQ_CF, TONE_DURATION_CF);
      }
      else {
        write_status(" "); //clear screen
      }
      delay(1000);
    }
  }
}