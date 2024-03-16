/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include "led_strip.h"

/**
 * This is an example which echos any data it receives on configured UART back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define TASK_STACK_SIZE         (CONFIG_EXAMPLE_TASK_STACK_SIZE)
#define BLINK_GPIO GPIO_NUM_8
#define BUF_SIZE (1024)

int delay = 1000;
static const char *TAG = "UART TEST";
led_strip_handle_t led;


void led_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO, // The GPIO that connected to the LED strip's data line
        .max_leds = 1, // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812, // LED strip model
        .flags.invert_out = false, // whether to invert the output signal (useful when your hardware has a level inverter)
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false, // whether to enable the DMA feature
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led));
    // Set all LED off to clear all pixels
    led_strip_clear(led);
}

void led_blink_task(void* param) {
    while(1) {
        //printf("White on!\n");
        led_strip_set_pixel(led, 0, 16, 16, 16);
        led_strip_refresh(led);
        vTaskDelay(delay / portTICK_PERIOD_MS);
        //printf("White off!\n");
        led_strip_clear(led);
        vTaskDelay(delay / portTICK_PERIOD_MS);
    }
}

void uart_init(void) {
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
}

static void uart_listen_task(void *arg)
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    char buf[5];
    memset(buf, 0, 5);
    int idx = 0;

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 10 / portTICK_PERIOD_MS);
        // Write data back to the UART
        //uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) data, len);
        
        if (len) {
            //ESP_LOGI(TAG, "data[0] = %d", data[0]);

            if (((data[0] == '\n') || (data[0] == '\r')) && (idx > 0)) {
                buf[idx] = '\0';
                delay = atoi(buf);
                idx = 0;
            }
            else {
                buf[idx] = data[0];
                idx++;
            }
            ESP_LOGI(TAG, "delay = %d", delay);
        }
    }
}

void app_main(void)
{
    led_init();
    uart_init();
    xTaskCreate(uart_listen_task, "uart_listen_task", TASK_STACK_SIZE, NULL, 10, NULL);
    xTaskCreate(led_blink_task, "led_blink_task", TASK_STACK_SIZE, NULL, 10, NULL);
}
