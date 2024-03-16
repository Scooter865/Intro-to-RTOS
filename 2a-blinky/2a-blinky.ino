// Restrict usage to 1 core for demo
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#define LED_BUILTIN 2;

// Pins
static const int led_pin = LED_BUILTIN;

// Blink an LED
void toggleLED(void * param) {
  while(true) {
    digitalWrite(led_pin, HIGH); // Set LED high/on
    vTaskDelay(500 / portTICK_PERIOD_MS); // For 500 ticks (500ms / 1ms/tick = 500 ticks)
    digitalWrite(led_pin, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void setup() {
  // put your setup code here, to run once:

  pinMode(led_pin, OUTPUT);

  xTaskCreatePinnedToCore( // Create a task for a single core (ESP-IDF specific)
    toggleLED,      // Function to call
    "Toggle LED",   // Task name
    1024,           // Stack size - check config file for mininum stack size.
    NULL,           // Argument to pass for to the function
    1,              // Task priority (lowest = 0, highest = configMAX_PRIORITIES-1)
    NULL,           // Task handle to check status, memory usage, etc
    app_cpu         // Core to run on (not in vanilla FreeRTOS xTaskCreate())
  );

}

void loop() {
  // put your main code here, to run repeatedly:

}
