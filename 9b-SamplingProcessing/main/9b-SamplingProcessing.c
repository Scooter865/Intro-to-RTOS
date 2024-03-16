/**
 * Create a sampling, processing, and interface system using hardware interrupts and 
 * whatever kernel objects (e.g. queues, mutexes, and semaphores) you might need.
 * 
 * Use a hardware timer to sample the ADC pin at 10hz - DONE
 * Copy the sampled data into a double buffer - DONE
 *  Consider what should or shouldn't be written when the buffer's full
 * Write an ISR to notify the processing task when there are 10 samples in the buffer - DONE (used task notification)
 * The processing task shall update a global float variable with the average of the last 10 samples - DONE
 *  So I may not need to use every sample in the calculation
 *  Writing to this variable may take more than 1 instruction cycle so protect it
 * Write a repl task that echoes input other than "avg" which should return the average variable
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freeRTOS/semphr.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_system.h"
#include "esp_log.h"


#define BUF_SIZE 10
#define DEFAULT_DELAY 10


static gptimer_handle_t ADC_sample_timer = NULL;
static adc_oneshot_unit_handle_t adc_handle = NULL;
static TaskHandle_t ADC_sample_task_handle = NULL;
static TaskHandle_t calc_avg_task_handle = NULL;
static int buf1[BUF_SIZE];
static int buf2[BUF_SIZE];
static SemaphoreHandle_t buf1CntSem = NULL;
static SemaphoreHandle_t buf2CntSem = NULL;
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
static float adc_avg;

static const char* TAG = "";


// ISR that notifies ADC sample task it should run
static bool IRAM_ATTR ADC_sample_ISR(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {
    BaseType_t high_task_awoken = pdFALSE;
    vTaskNotifyGiveFromISR(ADC_sample_task_handle, &high_task_awoken);
    return (high_task_awoken == pdTRUE);
}

// ADC Configuration
void ADC_config(void) {
    // May have to mess with ADC unit and channel to get this to work
    adc_oneshot_unit_init_cfg_t adc_init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &config));

    /* Calibration code skeleton. Don't think this is necessary 
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_0, ADC_ATTEN_DB_12, &adc1_cali_chan0_handle);
    */
}

// Timer Configuration
void ADC_sample_timer_config(void) {
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &ADC_sample_timer));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = ADC_sample_ISR,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(ADC_sample_timer, &cbs, NULL));

    //ESP_LOGI(TAG, "Enable timer");
    ESP_ERROR_CHECK(gptimer_enable(ADC_sample_timer));

    //ESP_LOGI(TAG, "Start timer, stop it at alarm event");
    gptimer_alarm_config_t alarm_config1 = {
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
        .alarm_count = 100*1000, // period = 100ms
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(ADC_sample_timer, &alarm_config1));
}

// Task to write the ADC data into the buffer
void ADC_sample_task(void* param) {
    int* wBuf = buf1;
    size_t wIdx = 0;

    while (true) {
        // Write to buffer when notified from ISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Switch buffer if you just filled it
        if (wIdx >= BUF_SIZE) {
            if (wBuf == buf1) {
                wBuf = buf2;
                wIdx = 0;
                xTaskNotifyGive(calc_avg_task_handle);
            }
            else if (wBuf == buf2) {
                wBuf = buf1;
                wIdx = 0;
                xTaskNotifyGive(calc_avg_task_handle);
            }
            else {
                ESP_LOGE(TAG, "wBuf is undefined");
            }
        }

        // Write to the appropriate buffer if there's space
        // Otherwise drop the value
        // The read task should give the semaphore 10 times (lock it?)
        if (wBuf == buf1) {
            if (xSemaphoreTake(buf1CntSem, pdMS_TO_TICKS(DEFAULT_DELAY)) == pdTRUE) {
                ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &wBuf[wIdx]));
                wIdx++;
            }
            else {
                ESP_LOGI(TAG, "Dropped Value");
            }
        }
        else if (wBuf == buf2) {
            if (xSemaphoreTake(buf2CntSem, pdMS_TO_TICKS(DEFAULT_DELAY)) == pdTRUE) {
                ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &wBuf[wIdx]));
                wIdx++;
            }
            else {
                ESP_LOGI(TAG, "Dropped Value");
            }
        }
        else {
            ESP_LOGE(TAG, "wBuf is undefined");
        }
    }
}

// Task to read 10 samples out of a buffer and write their average to global
// This is triggered when a buffer is full
void calc_avg_task(void* param) {
    int* rBuf = buf1;
    float avg = 0.0;
    while (true) {
        // Take notification from ADC task that a buffer is full
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Read data from the buffer, average it, write it
        if (uxSemaphoreGetCount(buf1CntSem) == 0) {
            rBuf = buf1;
        }
        else if (uxSemaphoreGetCount(buf2CntSem) == 0) {
            rBuf = buf2;
        }
        else {
            ESP_LOGE(TAG, "Counting semaphore problem.");
        }

        for (size_t i = 0; i < BUF_SIZE; i++) {
            avg += (float)rBuf[i];
        }
        avg /= BUF_SIZE;
        portENTER_CRITICAL(&spinlock);
        adc_avg = avg;
        portEXIT_CRITICAL(&spinlock);

        // Reset semaphore now that the data has been read to indicate the buffer can be written to
        if (rBuf == buf1) {
            portENTER_CRITICAL(&spinlock);
            for (size_t i = 0; i < BUF_SIZE; i++) {
                xSemaphoreGive(buf1CntSem);
            }
            portEXIT_CRITICAL(&spinlock);
        }
        else if (rBuf == buf2) {
            portENTER_CRITICAL(&spinlock);
            for (size_t i = 0; i < BUF_SIZE; i++) {
                xSemaphoreGive(buf2CntSem);
            }
            portEXIT_CRITICAL(&spinlock);
        }
        ESP_LOGI(TAG, "Average = %f", adc_avg);
    }
}

void app_main(void) {
    ADC_config();
    ADC_sample_timer_config();

    // Init semaphores saying all slots in the buffers are available
    buf1CntSem = xSemaphoreCreateCounting(BUF_SIZE, BUF_SIZE);
    buf2CntSem = xSemaphoreCreateCounting(BUF_SIZE, BUF_SIZE);

    xTaskCreate(ADC_sample_task, "ADC sampler task", 1024, NULL, 1, &ADC_sample_task_handle);
    xTaskCreate(calc_avg_task, "Calculator task", 2048, NULL, 1, &calc_avg_task_handle);
    
    ESP_ERROR_CHECK(gptimer_start(ADC_sample_timer));
}
