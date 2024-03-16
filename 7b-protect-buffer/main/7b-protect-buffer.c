/**
 * FreeRTOS Counting Semaphore Challenge
 * 
 * Challenge: use a mutex and counting semaphores to protect the shared buffer 
 * so that each number (0 through 4) is printed exactly 3 times to the Serial 
 * monitor (in any order). Do not use queues to do this!
 * 
 * Hint: you will need 2 counting semaphores in addition to the mutex, one for 
 * remembering number of filled slots in the buffer and another for 
 * remembering the number of empty slots in the buffer.
 */

/** Filled slot semaphore tells the consumer tasks if it's ok to read from the buffer.
    empty slot semaphore tells the producer tasks it it's ok to write to the buffer

    Filled slot: Consumer tasks take until semaphore == 0.
    Producer tasks give when they successfully write to buffer. 
    Empty slot: Producer tasks take until semaphore == 0.
    Consumer tasks give when they successfully read from buffer.

    Think about this literally. The producer task takes an empty slot, fills it, and gives a filled slot.
    The consumer task takes a filled slot, empties it, and gives an empty slot.

    The mutex doesn't show any new behavior because things move fast enough but wrap the head & tail 
    manipulation and print statements.
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

enum {BUF_SIZE = 5};                    // Buffer size
static const int num_prod_tasks = 5;    // Number of producer tasks
static const int num_cons_tasks = 2;    // Number of consumer tasks
static const int num_writes = 3;        // Number of times each producer writes to the buffer

static int buf[BUF_SIZE];               // Shared buffer
static int head = 0;                    // Write to buffer index
static int tail = 0;                    // Read from buffer index
static SemaphoreHandle_t bin_sem;       // Binary semaphore that waits for param to be read
static SemaphoreHandle_t filled_slots;  // Filled slots in the buffer
static SemaphoreHandle_t empty_slots;   // Empty slots in the buffer
static SemaphoreHandle_t mutex; // Mutex for protecting print operations


// Producer task: writes to the buffer a given number of times
void producer_task(void* param) {
    int num = *(int*)param;
    // Give semaphore since the data has been copied here
    xSemaphoreGive(bin_sem);

    for (size_t i = 0; i < num_writes; i++) {
        xSemaphoreTake(empty_slots, portMAX_DELAY);

        xSemaphoreTake(mutex, portMAX_DELAY);
        buf[head] = num;
        // Increment head around circular buffer
        head = (head+1) % BUF_SIZE;
        xSemaphoreGive(mutex);

        xSemaphoreGive(filled_slots);
    }

    vTaskDelete(NULL);
}

// Consumer task: continuously reads from the buffer
void consumer_task(void* param) {
    int val;

    while (true) {
        xSemaphoreTake(filled_slots, portMAX_DELAY);

        xSemaphoreTake(mutex, portMAX_DELAY);
        val = buf[tail];
        tail = (tail+1) % BUF_SIZE;
        printf("%d\n", val);
        xSemaphoreGive(mutex);

        xSemaphoreGive(empty_slots);
    }
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("\n-----Circular Buffer Protection Challenge-----\n");
    char task_name[12];
    bin_sem = xSemaphoreCreateBinary();
    filled_slots = xSemaphoreCreateCounting(BUF_SIZE, 0);
    empty_slots = xSemaphoreCreateCounting(BUF_SIZE, BUF_SIZE);
    mutex = xSemaphoreCreateMutex();

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
