#include <Wire.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <stdlib.h>
#include <string.h>

#define SLAVE_PRESENT_CF
#define DEBUG_CF
#define PIR_CALIB_TM_CF 20000U
#define TONE_DURATION_CF 3000
#define FIRST_SENSOR_FREQ_CF 1000
#define SECOND_SENSOR_FREQ_CF 220

//----------------pins---------------------
const uint8_t servo_PIN = 11;
const uint8_t PIR_sensor1_PIN = 2;
const uint8_t Smoke_sensor_PIN = 3;
const uint8_t buzzer_PIN = 12;
const uint8_t button_PIN = 8;
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

Servo servoModule;

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
  pinMode(Smoke_sensor_PIN, INPUT);
  pinMode(button_PIN, INPUT);

  Wire.begin();
  write_status(" ");
  servoModule.attach(servo_PIN);
  servoModule.write(0);
  delay(15);
  #ifdef DEBUG_CF
    Serial.begin(9600);
  #endif
  
  state.protectionState = NOT_PROTECTED;
  state.detectionSensorState = MUST_INIT;
  state.servoState = NOT_ENABLED;

}
  
void loop(){
  if(state.protectionState == NOT_PROTECTED)
  {
    uint8_t index_pass = 0;
    byte readButtonStatus;      
    readButtonStatus = (byte) digitalRead(button_PIN);
    if(readButtonStatus == HIGH)
    {
      delay(1);
      if(readButtonStatus == HIGH)
      {
        state.protectionState = PROTECTED;
      }
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
      uint8_t sensor_value = digitalRead(PIR_sensor1_PIN);
      uint8_t smoke_sensor_value = digitalRead(Smoke_sensor_PIN);

      //Monitor_Write(sensor_value);
      //Monitor_Write(smoke_sensor_value);

      if(sensor_value==HIGH && smoke_sensor_value == LOW)
      {
        write_status("Alert in room.Smoke in room");
        for(char i=0; i<15; i++)
        {
          tone(buzzer_PIN, FIRST_SENSOR_FREQ_CF, TONE_DURATION_CF);
          delay(100);
          tone(buzzer_PIN, SECOND_SENSOR_FREQ_CF, TONE_DURATION_CF);
          delay(100);
        }
      }
      else if(sensor_value == HIGH)
      {
        write_status("Alert in room");
        tone(buzzer_PIN, FIRST_SENSOR_FREQ_CF, TONE_DURATION_CF);
      }
      else if(smoke_sensor_value == LOW)
      {
        write_status("Smoke in room");
        tone(buzzer_PIN, SECOND_SENSOR_FREQ_CF, TONE_DURATION_CF);
      }
      else {
        write_status(" "); //clear screen
      }
      delay(1000);
    }
  }
}