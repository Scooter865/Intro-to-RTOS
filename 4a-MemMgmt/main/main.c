#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Mundane task to show stack overflow
void memoryTask(void * param) {
    while (1) {
        // Stack usage
        int a = 1;
        int b[100]; //Increase array size to overflow stack

        for (size_t i = 0; i < 100; i++) {
            b[i] = a++;
        }
        printf("%d", b[0]);
        printf("\nHigh Water Mark in words: %d\n", uxTaskGetStackHighWaterMark(NULL));

        // Heap usage
        printf("Heap before: %d\n", xPortGetFreeHeapSize());
        int * ptr = (int*)pvPortMalloc(1024 * sizeof(int));
        if (ptr == NULL) { //Remove null check to crash when out of heap
            printf("out of heap\n");
        }
        else {
            for (size_t i = 0; i < 1024; i++) {
                ptr[i] = 3;
            }
        }
        printf("Heap after: %d\n", xPortGetFreeHeapSize());
        vPortFree(ptr); //Remove free to exhause heap

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    printf("\n-----Stack Overflow & Heap Depletion Example-----\n");

    // Decrease stack size to overflow stack
    xTaskCreate(memoryTask, "Stack overflow & heap depletion Task", 2048, NULL, 1, NULL);
}
