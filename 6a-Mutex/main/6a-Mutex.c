#include <stdio.h>
#include <time.h>
#include "freertos/freeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static int shared_var = 0;
static SemaphoreHandle_t mutex;

/* Extra verbose and slow incrementer to illustrate race condition. */
void increment_task(void* param) {
    int local_var;
    while (1) {
        if (xSemaphoreTake(mutex, 10) == pdTRUE) {
            local_var = shared_var;
            local_var++;
            vTaskDelay((rand() % ((10+1-0)+0)) / portTICK_PERIOD_MS);
            shared_var = local_var;
            printf("%d\n",shared_var);
            xSemaphoreGive(mutex);
        }
        else {
            printf("Coudn't take mutex :(\n");
        }
    }
}

void app_main(void) {
    srand(time(NULL));
    mutex = xSemaphoreCreateMutex();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("-----Starting Race Condition Example-----\n");
    xTaskCreate(increment_task, "increment 1", 2048, NULL, 10, NULL);
    xTaskCreate(increment_task, "increment 2", 2048, NULL, 10, NULL);
}
