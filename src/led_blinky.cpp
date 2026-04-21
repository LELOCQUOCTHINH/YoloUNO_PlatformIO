#include "led_blinky.h"

void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);
  
  while(1) {                        
    digitalWrite(LED_GPIO, led1_state);  // turn the LED ON
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}