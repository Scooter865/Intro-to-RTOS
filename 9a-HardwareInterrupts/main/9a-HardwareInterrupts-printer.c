/** 
 * Part 2: Set the timer to trigger the ISR every 100ms.
 * Increment a counter protected by a spinlock inside the ISR.
 * Write a separate task to decrement and print the counter every 2s.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "esp_log.h"


static const char* TAG = "";
//static SemaphoreHandle_t bin_sem = NULL;
static const TickType_t task_delay = pdMS_TO_TICKS(2000);
static volatile int isr_counter;
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;


/* ISR that gives the semaphore/task notification to the LED toggle task. */
static bool IRAM_ATTR toggleLED(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void* user_ctx) {
    BaseType_t high_task_awoken = pdFALSE;
    
    portENTER_CRITICAL_ISR(&spinlock);
    isr_counter++;
    portEXIT_CRITICAL_ISR(&spinlock);

    return (high_task_awoken == pdTRUE);
}


void print_value_task(void* param) {
    while (true) {
        while (isr_counter > 0) {
            ESP_LOGI(TAG, "ISR Counter: %u", isr_counter);
            portENTER_CRITICAL(&spinlock);
            isr_counter--;
            portEXIT_CRITICAL(&spinlock);
        }
        vTaskDelay(task_delay);
    }
}

void app_main(void) {
    vTaskDelay(task_delay);

    // Blinker task that takes the semaphore/task notification and toggles the LED
    xTaskCreate(print_value_task, "ISR counter printer", 2048, NULL, 10, NULL);

    /* Basic timer config code. I don't think it matters if you do the alarm or callback config first.
       The alarm and callback are both associated with the timer but kind of independent of one another.
       I wonder if you can configure different callbacks for different alarms. */
    ESP_LOGI(TAG, "Create timer handle");
    gptimer_handle_t timer = NULL;

    ESP_LOGI(TAG, "Create timer configuration");
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1*1000*1000    // 1MHz
    };

    ESP_LOGI(TAG, "Create timer");
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));

    ESP_LOGI(TAG, "Register callback");
    gptimer_event_callbacks_t cbs = {
        .on_alarm = toggleLED
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));

    ESP_LOGI(TAG, "Enable timer");
    ESP_ERROR_CHECK(gptimer_enable(timer));

    ESP_LOGI(TAG, "Configure alarm");
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 100*1000, // 100ms
        .flags.auto_reload_on_alarm = true
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));

    ESP_LOGI(TAG, "Start timer");
    ESP_ERROR_CHECK(gptimer_start(timer));
    ESP_LOGI(TAG, "Timer Started");

    vTaskDelay(pdMS_TO_TICKS(10*1000));

    ESP_LOGI(TAG, "Stop timer");
    ESP_ERROR_CHECK(gptimer_stop(timer));

    ESP_LOGI(TAG, "Disable timer");
    ESP_ERROR_CHECK(gptimer_disable(timer));

    ESP_LOGI(TAG, "Delete timer");
    ESP_ERROR_CHECK(gptimer_del_timer(timer));
}
