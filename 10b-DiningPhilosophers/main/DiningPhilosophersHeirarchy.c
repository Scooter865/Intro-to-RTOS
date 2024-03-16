/**
 * ESP32 Dining Philosophers
 * 
 * The classic "Dining Philosophers" problem in FreeRTOS form.
 * Implement a heirarchy solution to prevent deadlock.
 */


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
enum { NUM_TASKS = 5 };           // Number of tasks (philosophers)
enum { TASK_STACK_SIZE = 2048 };  // Bytes in ESP32, words in vanilla FreeRTOS

// Globals
static SemaphoreHandle_t bin_sem;   // Wait for parameters to be read
static SemaphoreHandle_t done_sem;  // Notifies main task when done
static SemaphoreHandle_t chopstick[NUM_TASKS];
static const char* TAG = "";

//*****************************************************************************
// Tasks

// The only task: eating
void eat(void *parameters) {
  // num is task number and ranges from 0 to NUM_TASKS-1
  int num;
  int first;
  int second;

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);

  // Take whichever chopstick is lower, num or (num+1)%NUM_TASKS
  // This will prevent the last task from picking up any chopsticks at first
  if (num < (num+1) % NUM_TASKS) {
    first = num;
    second = (num+1) % NUM_TASKS;
  }
  else {
    first = (num+1) % NUM_TASKS;
    second = num;
  }

  // Take left chopstick
  xSemaphoreTake(chopstick[first], portMAX_DELAY);
  ESP_LOGI(TAG, "Philosopher %i took chopstick %i", num, first);

  // Add some delay to force deadlock
  // Changed this from 1 to 10ms delay to induce deadlock
  vTaskDelay(10 / portTICK_PERIOD_MS);

  // Take right chopstick
  xSemaphoreTake(chopstick[second], portMAX_DELAY);
  ESP_LOGI(TAG, "Philosopher %i took chopstick %i", num, second);

  // Do some eating
  ESP_LOGI(TAG, "Philosopher %i is eating", num);
  vTaskDelay(10 / portTICK_PERIOD_MS);

  // Put down right chopstick
  xSemaphoreGive(chopstick[second]);
  ESP_LOGI(TAG, "Philosopher %i returned chopstick %i", num, second);

  // Put down left chopstick
  xSemaphoreGive(chopstick[num]);
  ESP_LOGI(TAG, "Philosopher %i returned chopstick %i", num, first);

  // Notify main task and delete self
  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void app_main(void) {

  char task_name[20];

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  ESP_LOGI(TAG, "---FreeRTOS Dining Philosophers Challenge---");

  // Create kernel objects before starting tasks
  bin_sem = xSemaphoreCreateBinary();
  done_sem = xSemaphoreCreateCounting(NUM_TASKS, 0);
  for (int i = 0; i < NUM_TASKS; i++) {
    chopstick[i] = xSemaphoreCreateMutex();
  }

  // Have the philosphers start eating
  for (size_t i = 0; i < NUM_TASKS; i++) {
    sprintf(task_name, "Philosopher %i", i);
    xTaskCreatePinnedToCore(eat,
                            task_name,
                            TASK_STACK_SIZE,
                            (void *)&i,
                            1,
                            NULL,
                            app_cpu);
    xSemaphoreTake(bin_sem, portMAX_DELAY);
  }


  // Wait until all the philosophers are done
  for (int i = 0; i < NUM_TASKS; i++) {
    xSemaphoreTake(done_sem, portMAX_DELAY);
  }

  // Say that we made it through without deadlock
  ESP_LOGI(TAG, "Done! No deadlock occurred!");
}
