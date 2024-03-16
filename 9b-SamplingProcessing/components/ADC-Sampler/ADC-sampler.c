#include "freertos/FreeRTOS.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char* TAG = "";
adc_oneshot_unit_handle_t adc_handle;

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
}


// ISR to sample from the ADC every 100ms when the timer expires
static bool IRAM_ATTR ADC_sample_ISR(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {
    BaseType_t high_task_awoken = pdFALSE;
    // Sample from the ADC and write into double buffer
    // May need to do this through an ISR-safe FreeRTOS call like a task notification

    // Documentation says not to put this in an ISR but let's see if it works
    adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &buffer);

    return (high_task_awoken == pdTRUE);
}

// Function to configure a timer for this project and start it.
// This should be called after other inits to start the program
void ADC_sample_timer_config_start(void) {
    //ESP_LOGI(TAG, "Create timer handle");
    gptimer_handle_t ADC_sample_timer = NULL;
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
    ESP_ERROR_CHECK(gptimer_start(ADC_sample_timer));
    // Not including any stop functionality
}