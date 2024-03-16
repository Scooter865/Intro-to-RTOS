/** 
 * Demonstration of mutex protecting critical variable (LED flash rate).
 * This example doesn't work in current FreeRTOS because it won't allow a 
 * task to give a mutex if it is not the owner of it (i.e. the task that 
 * took it). It's improper use of a mutex anyways and the code works as 
 * intended even without the mutex.
*/

#include <stdio.h>
#include "led_strip.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "freertos/semphr.h"


// GPIO assignment
#define LED_STRIP_BLINK_GPIO  GPIO_NUM_8
// Numbers of the LED in the strip
#define LED_STRIP_LED_NUMBERS 1
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)
#define UART_PORT_NUM UART_NUM_0

static const char* TAG = ">";
static led_strip_handle_t led = NULL;
static SemaphoreHandle_t mutex;


led_strip_handle_t configure_led(void) {
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,   // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_NUMBERS,        // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .rmt_channel = 0,
#else
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .flags.with_dma = false,               // DMA feature is available on ESP target like ESP32-S3
#endif
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}

// Toggle the LED on/off for the specified amount of time
void blinkLED(void* param) {
    int num = *(int*)param;
    ESP_LOGI(TAG, "Received %d", num);
    //xSemaphoreGive(mutex);

    while (1) {
        ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, 5, 5, 5));
        ESP_ERROR_CHECK(led_strip_refresh(led));
        vTaskDelay(pdMS_TO_TICKS(num));
        ESP_ERROR_CHECK(led_strip_clear(led));
        vTaskDelay(pdMS_TO_TICKS(num));
    }
}

void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, 16, 17, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
}



void app_main(void) {
    led = configure_led();
    ESP_ERROR_CHECK(led_strip_clear(led));
    uart_init();
    //mutex = xSemaphoreCreateMutex();

    int delay_arg = 1000;

    // Program UART buffer
    uint8_t data[32];
    // Command parsed from UART
    char cmd[32];
    size_t idx = 0;

    printf("Enter a number for delay (ms):\n");
    //ESP_LOGI(TAG, "Enter a number for delay (ms):\n");

    while (data[0] != '\r' && data[0] != '\n') {
        size_t len = 0;
        // See how much data is in the UART Rx FIFO
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_PORT_NUM, &len));
        // Read that much data into the program buffer
        len = uart_read_bytes(UART_PORT_NUM, data, len, 0);

        if (len) {
            cmd[idx] = data[0];
            idx++;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    cmd[idx-1] = '\0';
    delay_arg = atoi(cmd);

    //xSemaphoreTake(mutex, portMAX_DELAY);
    xTaskCreate(blinkLED, "Blink LED", 2048, &delay_arg, 0, NULL);

    //xSemaphoreTake(mutex, portMAX_DELAY);
    ESP_LOGI(TAG, "Created Blink LED with %d ms delay", delay_arg);

}
