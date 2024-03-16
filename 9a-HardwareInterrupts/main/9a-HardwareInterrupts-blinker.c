/** 
 * Part 1: Use a hardware timer to drive a blinky program. I'm using FreeRTOS task
 * notifications to trigger the blinker task from the ISR.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/semphr.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "common_configs.h"


static led_strip_handle_t led_strip = NULL;         // LED strip handle
static bool led_strip_status = 0;                   // 1 = on, 0 = off
static const char* TAG = "";
static TaskHandle_t led_toggle_task_handle = NULL;  
//static SemaphoreHandle_t bin_sem = NULL;


void led_toggle_task(void* param) {
    ESP_LOGI(TAG, "Enter toggle task");
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        //xSemaphoreTake(bin_sem, portMAX_DELAY);
        ESP_LOGI(TAG, "Toggle");
        if (led_strip_status) {
            led_strip_clear(led_strip);
        }
        else {
            led_strip_set_pixel(led_strip, 0, 10, 10, 10);
            led_strip_refresh(led_strip);
        }
        led_strip_status = !led_strip_status;
    }
}

/* ISR that gives the semaphore/task notification to the LED toggle task. */
static bool IRAM_ATTR toggleLED(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void* user_ctx) {
    BaseType_t high_task_awoken = pdFALSE;
    
    vTaskNotifyGiveFromISR(led_toggle_task_handle, &high_task_awoken);
    //xSemaphoreGiveFromISR(bin_sem, &high_task_awoken);
    
    return (high_task_awoken == pdTRUE);
}


void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Configure LED");
    led_strip = configure_led();
    led_strip_clear(led_strip);
    led_strip_status = 0;
    //bin_sem = xSemaphoreCreateBinary();

    // Blinker task that takes the semaphore/task notification and toggles the LED
    xTaskCreate(led_toggle_task, "LED toggler", 2048, NULL, 10, &led_toggle_task_handle);

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
        .alarm_count = 1*1000*1000, // 1s
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
