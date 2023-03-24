#include <Wire.h>
#include <LiquidCrystal.h>
#include <stdlib.h>

LiquidCrystal lcd(2,3, 4,5,6,7);

short acknowledge_flag;

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
  char *str = NULL;
  int index=0;

  if(bytes!= 0)
  {
    str = (char *)malloc((bytes+1) * sizeof(char));
    if(str == NULL) //not enough memory !
    {
      abort();
    }

    
    while(Wire.available())
    {
      str[index]=Wire.read();
      Serial.println(str[index]);
      index++;
    }
    str[index]='\0';

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(str);

    free(str);
    acknowledge_flag = 1;
  }
}
void onRequest_Handler()
{
  Wire.write(acknowledge_flag);
}
void loop(){}