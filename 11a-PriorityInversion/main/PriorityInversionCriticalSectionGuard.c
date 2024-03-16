/**
 * Unbounded priority inversion example
 * 
 * Use critical section guards to prevent interruption during the critical section.
 * I don't think I want any sort of semaphore.
 * 
 * This is currently crashing when it tries to get the spinlock for the first 
 * time with the low priority task. Try commenting out ESP_LOGI lines and anything 
 * that doesn't need to be there.
*/


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"


#define CS_WAIT pdMS_TO_TICKS(250)      // Time spent in critical section
#define MED_WAIT pdMS_TO_TICKS(2000)    // Time medium priority task spends working
#define TASK_STACK 1024*3


static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
static const char* TAG = "";


void HighPriorityTask(void* param) {
    TickType_t timestamp;
    while (true) {
        ESP_LOGI(TAG, "High priority task taking lock");
        timestamp = xTaskGetTickCount();
        
        portENTER_CRITICAL(&spinlock);
        for (size_t i = 0; i < 1000*1000; i++) {
            // Do nothing for a while
        }
        portEXIT_CRITICAL(&spinlock);


        ESP_LOGI(TAG, "High priority task released lock, spent %li ms in critical section", \
            pdTICKS_TO_MS(xTaskGetTickCount() - timestamp));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void MedPriorityTask(void* param) {
    TickType_t timestamp;
    while (true) {
        ESP_LOGI(TAG, "Medium priority task doing work");
        timestamp = xTaskGetTickCount();
        while (xTaskGetTickCount() - timestamp < MED_WAIT) {
            // Do nothing for a while
        }
        ESP_LOGI(TAG, "Medium priority task done");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void LowPriorityTask(void* param) {
    TickType_t timestamp;
    while (true) {
        ESP_LOGI(TAG, "Low priority task taking lock");
        timestamp = xTaskGetTickCount();

        portENTER_CRITICAL(&spinlock);
        for (size_t i = 0; i < 1000*1000; i++) {
            // Do nothing for a while
        }
        portEXIT_CRITICAL(&spinlock);

        ESP_LOGI(TAG, "Low priority task released lock, spent %li ms in critical section", \
            pdTICKS_TO_MS(xTaskGetTickCount() - timestamp));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "-----Priority Inversion Example-----");
   
    xTaskCreatePinnedToCore(LowPriorityTask, "Low Priority Task", TASK_STACK, NULL, 1, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(1)); // Need a short delay to make priority inversion happen
    xTaskCreatePinnedToCore(HighPriorityTask, "High Priority Task", TASK_STACK, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(MedPriorityTask, "Medium Priority Task", TASK_STACK, NULL, 2, NULL, 0);
}
