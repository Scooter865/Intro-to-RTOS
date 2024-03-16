/** 
 * This example shows what a counting semaphore is. 
 * app_main creates 5 tasks then has to wait for all of them to be completed before it proceeds.
 * It moves fast enough that you can't really see a difference with and without the semaphore.
 * You will notice the delay if you move the task delay in print_msg_task before  xSemaphoreGive.
 * 
 * I think putting the printf statement in the print_msg_task in a mutex would protect from one 
 * task printing over another as seen in the video. I don't see this behavior on my system 
 * though so I'm not doing it at this time.
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


// Size of message body (so 19 characters + \0)
#define MAX_MSG_LEN 20

// The number of concurrent tasks
static const int num_tasks = 5;

// Message struct to pass into tasks
typedef struct Message {
    char body[MAX_MSG_LEN];
    uint8_t len;
} Message;

// Our counting semaphore
static SemaphoreHandle_t counter_sem;


// Task to print a message contained in a Message struct
void print_msg_task(void* param) {
    Message _msg = *(Message*)param;

    xSemaphoreGive(counter_sem);

    printf("Received: %s | length: %d\n", _msg.body, _msg.len);

    vTaskDelay(pdMS_TO_TICKS(2000));
    vTaskDelete(NULL);
}

void app_main(void) {
    char task_name[8];
    Message msg;
    char text[MAX_MSG_LEN] = "Me Mambo Dogface";

    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("\n-----Counting Semaphore Demo-----\n");

    strcpy(msg.body, text);
    msg.len = strlen(text);

    // Create counting semaphore that counts up to num_tasks (5)
    counter_sem = xSemaphoreCreateCounting(num_tasks, 0);

    for (size_t i = 0; i < num_tasks; i++) {
        printf("creating task %i\n", i);
        // Give each task a unique name (Task 1, Task 2, etc)
        sprintf(task_name, "Task %i", i);
        xTaskCreate(print_msg_task, task_name, 2048, &msg, 1, NULL);
    }

    // Execution can't go past this loop w/o the tasks getting the param
    for (size_t i = 0; i < num_tasks; i++) {
        xSemaphoreTake(counter_sem, portMAX_DELAY);
    }

    printf("All tasks complete\n");
}
