#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freeRTOS/timers.h"

static TimerHandle_t one_shot_timer = NULL;
static TimerHandle_t auto_reload_timer = NULL;

void myTimerCallback(TimerHandle_t xTimer) {
    if((uint32_t)pvTimerGetTimerID(xTimer) == 0) {
        printf("One shot timer expired\n");
    }

    if((uint32_t)pvTimerGetTimerID(xTimer) == 1) {
        printf("Auto reload timer expired\n");
    }
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("-----Software Timer Example-----\n");
    one_shot_timer = xTimerCreate("one shot timer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, myTimerCallback);
    auto_reload_timer = xTimerCreate("auto reload timer", pdMS_TO_TICKS(1000), pdTRUE, (void*)1, myTimerCallback);

    if (one_shot_timer == NULL || auto_reload_timer == NULL) {
        printf("Could not create timer\n");
    }
    else {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("starting timer\n");
        xTimerStart(one_shot_timer, portMAX_DELAY);
        xTimerStart(auto_reload_timer, portMAX_DELAY);
    }
}
