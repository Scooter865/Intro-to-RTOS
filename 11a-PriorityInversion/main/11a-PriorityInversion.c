/**
 * Unbounded priority inversion example
*/


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"


#define CS_WAIT pdMS_TO_TICKS(250)      // Time spent in critical section
#define MED_WAIT pdMS_TO_TICKS(2000)    // Time medium priority task spends working


static SemaphoreHandle_t lock;
static const char* TAG = "";


void HighPriorityTask(void* param) {
    TickType_t timestamp;
    while (true) {
        ESP_LOGI(TAG, "High priority task taking lock");
        timestamp = xTaskGetTickCount();
        xSemaphoreTake(lock, portMAX_DELAY);
        ESP_LOGI(TAG, "High priority task took %li ms to get lock", pdTICKS_TO_MS(xTaskGetTickCount() - timestamp));

        ESP_LOGI(TAG, "High priority task doing work");
        timestamp = xTaskGetTickCount();
        while (xTaskGetTickCount() - timestamp < CS_WAIT); // Do nothing for a while

        ESP_LOGI(TAG, "High priority task releasing lock");
        xSemaphoreGive(lock);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void MedPriorityTask(void* param) {
    TickType_t timestamp;
    while (true) {
        ESP_LOGI(TAG, "Medium priority task doing work");
        timestamp = xTaskGetTickCount();
        while (xTaskGetTickCount() - timestamp < MED_WAIT); // Do nothing for a while
    
        ESP_LOGI(TAG, "Medium priority task done");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void LowPriorityTask(void* param) {
    TickType_t timestamp;
    while (true) {
        ESP_LOGI(TAG, "Low priority task taking lock");
        timestamp = xTaskGetTickCount();
        xSemaphoreTake(lock, portMAX_DELAY);
        ESP_LOGI(TAG, "Low priority task took %li ms to get lock", pdTICKS_TO_MS(xTaskGetTickCount() - timestamp));

        ESP_LOGI(TAG, "Low priority task doing work");
        timestamp = xTaskGetTickCount();
        while (xTaskGetTickCount() - timestamp < CS_WAIT) {
            // Do nothing for a while
        }

        ESP_LOGI(TAG, "Low priority task releasing lock");
        xSemaphoreGive(lock);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "-----Priority Inversion Example-----");

    /* Using a binary semaphore causes unbounded priority inversion 
    lock = xSemaphoreCreateBinary();
    xSemaphoreGive(lock);
    */

    /* Mutexes implement priority inheritance in FreeRTOS */
    lock = xSemaphoreCreateMutex();
    

    xTaskCreatePinnedToCore(LowPriorityTask, "Low Priority Task", 2048, NULL, 1, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(1)); // Need a short delay to make priority inversion happen
    xTaskCreatePinnedToCore(HighPriorityTask, "High Priority Task", 2048, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(MedPriorityTask, "Medium Priority Task", 2048, NULL, 2, NULL, 0);
}
