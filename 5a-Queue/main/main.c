#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Declare queue as global so all tasks can use it
static QueueHandle_t myQueue;
static const uint8_t queueLength = 5;

// Task to pop a value off the queue and print it
void printerTask(void * param) {
    int item;

    while (1) {
        if (xQueueReceive(myQueue, &item, 0) == pdTRUE) {
            //printf("%d\n", item);
        }
        printf("%d\n", item);

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

// Task to push a value onto the queue
void pushTask(void * param) {
    static int num = 0;

    while (1) {
        if (xQueueSend(myQueue, &num, 10) != pdTRUE) {
            printf("Queue full\n");
        }
        num++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    printf("\n-----Queue Example-----\n");

    myQueue = xQueueCreate(queueLength, sizeof(int));

    xTaskCreate(printerTask, "Pop from queue", 2048, NULL, 1, NULL);
    xTaskCreate(pushTask, "Push to queue", 2048, NULL, 1, NULL);
}
