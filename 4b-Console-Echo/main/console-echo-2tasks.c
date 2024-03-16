/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

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
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)
#define BUF_SIZE (1024)

static char* message = NULL;
static volatile uint8_t message_ready = 0;
static const char *TAG = "UART TEST";


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

    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
}

void uart_listen_task(void* param) {
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    char buf[BUF_SIZE];
    size_t idx = 0;

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);

        if (len) {
            if (data[0] == '\n' || data[0] == '\r') {
                if (idx == 0) {
                    ESP_LOGI(TAG, "Enter some text to echo.");
                }
                else if (!message_ready) {
                    buf[idx] = '\0';
                    message = (char*)pvPortMalloc((idx+1) * sizeof(char));
                    memcpy(message, buf, idx+1);
                    idx = 0;
                    message_ready = 1;
                }
                else {
                    ESP_LOGI(TAG, "Wait for echo. Try again.");
                }
            }
            else {
                buf[idx] = data[0];
                idx++;
            }
        }
    }
}

void uart_speak_task(void* param) {
    while (1) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
        if (message_ready) {
            ESP_LOGI(TAG, "%s", message);
            vPortFree(message);
            message = NULL;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            message_ready = 0;
        }
    }
}

void app_main(void)
{
    uart_init();
    xTaskCreate(uart_listen_task, "uart_listen_task", 1024 * 3, NULL, 10, NULL);
    xTaskCreate(uart_speak_task, "uart_speak_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
}
