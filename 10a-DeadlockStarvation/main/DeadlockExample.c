/**
 * Deadlock example.
 * There are 2 tasks that take 2 mutexes, do some work in a critical section for some time 
 * (500ms for this demo), then give the mutexes back. This will result in a deadlock.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define MUTEX_TAKE_TIMEOUT pdMS_TO_TICKS(1000)

static SemaphoreHandle_t mutex1 = NULL;
static SemaphoreHandle_t mutex2 = NULL;
static const char* TAG = "";

void deadlock_task_A(void* param) {
    while (true) {
        xSemaphoreTake(mutex1, portMAX_DELAY);
        ESP_LOGI(TAG, "Task A took mutex 1");
        vTaskDelay(pdMS_TO_TICKS(10));

        xSemaphoreTake(mutex2, portMAX_DELAY);
        ESP_LOGI(TAG, "Task A took mutex 2");

        ESP_LOGI(TAG, "Task A doing work");
        vTaskDelay(pdMS_TO_TICKS(500));
        
        xSemaphoreGive(mutex2);
        xSemaphoreGive(mutex1);

        ESP_LOGI(TAG, "Task A going to sleep");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void deadlock_task_B(void* param) {
    while (true) {
        xSemaphoreTake(mutex2, portMAX_DELAY);
        ESP_LOGI(TAG, "Task B took mutex 2");
        vTaskDelay(pdMS_TO_TICKS(10));

        xSemaphoreTake(mutex1, portMAX_DELAY);
        ESP_LOGI(TAG, "Task B took mutex 1");

        ESP_LOGI(TAG, "Task B doing work");
        vTaskDelay(pdMS_TO_TICKS(500));

        xSemaphoreGive(mutex1);
        xSemaphoreGive(mutex2);

        ESP_LOGI(TAG, "Task B going to sleep");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "-----Deadlock Example-----");
    mutex1 = xSemaphoreCreateMutex();
    mutex2 = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(deadlock_task_A, "Deadlock Demo task A", 2048, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(deadlock_task_B, "Deadlock Demo task B", 2048, NULL, 1, NULL, 0);
}
