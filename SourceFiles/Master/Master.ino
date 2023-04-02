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
const char password[PASS_LEN] = {"1234"};
//----------------pins---------------------
const uint8_t servo_PIN = 10;
const uint8_t PIR_sensor1_PIN = A3;
const uint8_t PIR_sensor2_PIN = A2;
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

byte rowPins[ROWS] = {6, 7, 8, 9}; 
byte colPins[COLS] = {2, 3, 4, 5};

//index used for comparing entered password
uint8_t index;
char *pass_to_be_checked;
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

  pass_to_be_checked = (char *)malloc(PASS_LEN * (sizeof(char)));
  if(pass_to_be_checked == NULL)
    abort();
}
  
void loop(){
  if(state.protectionState == NOT_PROTECTED)
  {
    uint8_t index_pass = 0;
    char readKey;
    while(index_pass < PASS_LEN -1)
    {
      readKey = customKeypad.getKey();
      if(readKey)
      {
        Monitor_Write(readKey);
        pass_to_be_checked[index_pass++] = readKey;
      }
    }
    pass_to_be_checked[strlen(pass_to_be_checked)] = '\0';

    if(strncmp(password, pass_to_be_checked, PASS_LEN-1) == 0)
    {
      free(pass_to_be_checked);
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