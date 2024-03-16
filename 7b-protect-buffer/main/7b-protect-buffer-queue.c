/**
 * FreeRTOS Counting Semaphore Challenge
 * 
 * Challenge: use a mutex and queue(s) to protect the shared buffer 
 * so that each number (0 through 4) is printed exactly 3 times to the Serial 
 * monitor (in any order).
 * 
 * The producer tasks should write to the queue, the consumers should read from the queue.
 * Keep the binary semaphore as is. Use the mutex to protect print operations.
 * 
 * Don't need to protect queue operations with the mutex. 
 */


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

enum {QUEUE_SIZE = 5};                  // Queue size
static const int num_prod_tasks = 5;    // Number of producer tasks
static const int num_cons_tasks = 2;    // Number of consumer tasks
static const int num_writes = 3;        // Number of times each producer writes to the queue

static SemaphoreHandle_t bin_sem;       // Binary semaphore that waits for param to be read
static SemaphoreHandle_t mutex;         // Mutex for protecting print operations
static QueueHandle_t queue;             // Queue for holding ints


// Producer task: writes to the buffer a given number of times
void producer_task(void* param) {
    int num = *(int*)param;
    // Give semaphore since the data has been copied here
    xSemaphoreGive(bin_sem);

    for (size_t i = 0; i < num_writes; i++) {
        xQueueSend(queue, &num, portMAX_DELAY);
    }

    vTaskDelete(NULL);
}

// Consumer task: continuously reads from the buffer
void consumer_task(void* param) {
    int val;

    while (true) {
        xQueueReceive(queue, &val, portMAX_DELAY);

        xSemaphoreTake(mutex, portMAX_DELAY);
        printf("%d\n", val);
        xSemaphoreGive(mutex);
    }
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("\n-----Circular Buffer Protection Challenge-----\n");
    char task_name[12];
    bin_sem = xSemaphoreCreateBinary();
    mutex = xSemaphoreCreateMutex();
    queue = xQueueCreate(QUEUE_SIZE, sizeof(int));

    for (size_t i = 0; i < num_prod_tasks; i++) {
        sprintf(task_name, "Producer %i", i);
        xTaskCreate(producer_task, task_name, 2048, &i, 1, NULL);
        // Each task must be created and get its param before execution proceeds
        xSemaphoreTake(bin_sem, portMAX_DELAY);
    }

    for (size_t i = 0; i < num_cons_tasks; i++) {
        sprintf(task_name, "Consumer %i", i);
        xTaskCreate(consumer_task, task_name, 2048, NULL, 1, NULL);
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    printf("All tasks created\n");
    xSemaphoreGive(mutex);
}
