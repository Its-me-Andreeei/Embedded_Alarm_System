#include <Arduino_FreeRTOS.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <semphr.h>

//CONFIG params--------------------------------------------
#define Inactivity_Time_Before_LP 6000U
#define LP_Mng_Period_MS 100U
#define PIR_Polling_Period_MS 60U
#define Buzzer_Freq_HZ 440U
//---------------------------------------------------------

//Semaphore for lastDuration (MUTEX)-----------------------
SemaphoreHandle_t semaphore_for_lastDuration;
//---------------------------------------------------------

//Low Power Management task---------------------------------
TaskHandle_t LP_Mng_handler;
void LP_Mng_Task(void *pvParameters);
//----------------------------------------------------------

//PIR Sensor polling task-----------------------------------
TaskHandle_t PIR_Polling_handler;
void PIR_Polling_Task(void *pvParameters);
//----------------------------------------------------------

//Interrupt---------------
void Perform_WakeUp();
//------------------------

//Variables-------------------------------------------------
unsigned long lastDetectionTime_ms;
#define PIR_sensor_vout_pin 2U
#define buzzer_pin 4U
volatile uint8_t Low_Power_Reached = 0x00;
//----------------------------------------------------------

//Helpful Macros-------------------------------------------
#define getTimeNow() xTaskGetTickCount() / configTICK_RATE_HZ * 1000
//---------------------------------------------------------
void setup() 
{
  wdt_disable();
  power_adc_enable();
  power_spi_enable();
  power_usart0_enable();
  power_twi_enable();
  power_timer0_enable();
  power_timer1_enable();
  power_timer2_enable();
  power_adc_enable();

  Serial.begin(9600);
  Serial.println("RESET");
  //Semaphore configurations----------------------------------------
  semaphore_for_lastDuration = xSemaphoreCreateMutex();
  if(semaphore_for_lastDuration == NULL)
  {
    Serial.println("Error while creating semaphore");
    for(;;){}
  }
  lastDetectionTime_ms = getTimeNow();
  //Sleep configurations--------------------------------------------
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_bod_disable();
  sleep_enable();

  //Pin configurations-----------------------------------------------
  pinMode(PIR_sensor_vout_pin, INPUT);
  pinMode(buzzer_pin, OUTPUT);
  interrupts(); // -> Enables interrupts
  attachInterrupt(digitalPinToInterrupt(PIR_sensor_vout_pin), Perform_WakeUp, RISING);

  //Scheduler configuration------------------------------------------
  xTaskCreate(LP_Mng_Task, "LP_Mng", 350, NULL, 2, &LP_Mng_handler);
  xTaskCreate(PIR_Polling_Task, "PIR_Polling", 250, NULL, 1, &PIR_Polling_handler);
  vTaskStartScheduler();
}

void loop() {

}

void LP_Mng_Task(void *pvParameters)
{
  (void)pvParameters; //no params
  TickType_t getLastStartTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(LP_Mng_Period_MS);
  unsigned long getTimeNow_ms;
  for(;;)
  {
    //functional code start here---------------------------------------
    getTimeNow_ms =  getTimeNow();
    if(xSemaphoreTake(semaphore_for_lastDuration, portMAX_DELAY) == pdTRUE) 
    {
      unsigned long duration = getTimeNow_ms - lastDetectionTime_ms;
      if(duration >= Inactivity_Time_Before_LP)
      {
        xSemaphoreGive(semaphore_for_lastDuration);
        Low_Power_Reached = 0x01;
        power_adc_disable();
        power_spi_disable();
        power_usart0_disable();
        power_twi_disable();
        vTaskEndScheduler();
        power_timer0_disable();
        power_timer1_disable();
        power_timer2_disable();
        sleep_mode();
        power_adc_enable();
        power_spi_enable();
        power_usart0_enable();
        power_twi_enable();
        power_timer0_enable();
        power_timer1_enable();
        power_timer2_enable();
        power_adc_enable();
        Serial.println("After WakeUp");
        //infinite loop until watchdog triggers
        for(;;){}
      }
      else {
        xSemaphoreGive(semaphore_for_lastDuration);
      }
    }

    //code ends here----------------------------------------
    vTaskDelayUntil(&getLastStartTime, period);
  }
}

void PIR_Polling_Task(void *pvParameters)
{
  (void)pvParameters; //no params
  TickType_t getLastStartTime = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(PIR_Polling_Period_MS);
  uint8_t read_value;
  for(;;)
  {
    //functional code start here---------------------------------------
    
    read_value = digitalRead(PIR_sensor_vout_pin);
    Serial.println(read_value);
    if(read_value)
    {
      if(xSemaphoreTake(semaphore_for_lastDuration, portMAX_DELAY) == pdTRUE)
      {
        lastDetectionTime_ms = getTimeNow();
        xSemaphoreGive(semaphore_for_lastDuration);
      }
      //TBD if OK
      //tone(buzzer_pin, Buzzer_Freq_HZ, 2000);
    }

    //code ends here----------------------------------------
    vTaskDelayUntil(&getLastStartTime, period);
  }
}

void Perform_WakeUp()
{
  noInterrupts();
  if(Low_Power_Reached == 0x01)
  {
    sleep_disable();
    Low_Power_Reached = 0x00;
    Serial.println("Wake");
    wdt_enable(WDTO_120MS);
  }
  interrupts();
}