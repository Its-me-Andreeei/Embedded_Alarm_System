#include <Wire.h>
#include <LiquidCrystal.h>
#include <stdlib.h>

LiquidCrystal lcd(2,3, 4,5,6,7);

short acknowledge_flag = 0;
char str[100]; //used as buffer to display received characters from master
void onReceive_Handler(int bytes);
void onRequest_Handler(void);

void setup(){
  Wire.begin(0x1);
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  Wire.onReceive(onReceive_Handler);
  Wire.onRequest(onRequest_Handler);

}
void onReceive_Handler(int bytes)
{
  acknowledge_flag = 0;
  int index=0;
  char c;

  lcd.clear();

  if(bytes!= 0 && bytes <= 32) //only 32 characters can fill LCD and 1 char = 1 byte
  {
    while(Wire.available())
    {
      c = Wire.read();
      if( c == '.') //limit of characters per row achived
      {
        str[index] = '\0';
        lcd.write(str);
        lcd.setCursor(0, 1);
        index = 0;
        continue;
      }
      str[index] = c;
      //Serial.println(str[index]); //for debugging purpose
      index++;
    }
    str[index]='\0';
    lcd.write(str);

    acknowledge_flag = 1;
  }
}

void onRequest_Handler()
{
  Wire.write(acknowledge_flag);
}
void loop(){}