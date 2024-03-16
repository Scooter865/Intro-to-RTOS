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
#include "freertos/queue.h"

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

#define ECHO_TEST_TXD           (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD           (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS           (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS           (UART_PIN_NO_CHANGE)
#define ECHO_UART_PORT_NUM      (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define TASK_STACK_SIZE         (CONFIG_EXAMPLE_TASK_STACK_SIZE)
#define BLINK_GPIO              GPIO_NUM_8
#define BUF_SIZE                (1024)
#define DELAY_QUEUE_SIZE        5
#define MSG_QUEUE_SIZE          5
#define MSG_SIZE                32

static const char *TAG = ">";
static led_strip_handle_t led;
static QueueHandle_t delay_queue;
static QueueHandle_t msg_queue;


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
    int delay = 1000;
    size_t blink_counter = 0;

    char delayUpdateMsg[MSG_SIZE];
    memset(delayUpdateMsg, 0, MSG_SIZE);

    while(1) {
        // Rx new delay if available
        if (xQueueReceive(delay_queue, &delay, 10) == pdTRUE) {
            // Indicate new blink rate (through message queue)
            sprintf(delayUpdateMsg, "Updated delay to %d", delay);
            xQueueSend(msg_queue, &delayUpdateMsg, 10);
        }

        // Blink the LED
        led_strip_set_pixel(led, 0, 16, 16, 16);
        led_strip_refresh(led);
        vTaskDelay(delay / portTICK_PERIOD_MS);
        led_strip_clear(led);
        vTaskDelay(delay / portTICK_PERIOD_MS);

        // Count the number of blinks and Tx blinked message at 100
        blink_counter++;
        if (blink_counter >= 100) {
            sprintf(delayUpdateMsg, "blinked %d times", blink_counter);
            xQueueSend(msg_queue, &delayUpdateMsg, 10);
            blink_counter = 0;
        }
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

/**
 * Prints messages from queue 2.
 * Then reads serial input. Echoes a line and evaluates "delay" command upon newline or carriage return if present.
 */
static void serial_com_task(void *arg) {
    // Message queue buffer to read messages into
    char msgBuf[MSG_SIZE];
    memset(msgBuf, 0, MSG_SIZE);

    // This program's UART buffer
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    // Buffer to capture a line of input
    char line[BUF_SIZE];
    memset(line, 0, BUF_SIZE);
    int idx = 0;

    // Delay to pass into delay_queue
    int delay = 1000;

    while (1) {
        // First print a message from queue if ready
        if (xQueueReceive(msg_queue, &msgBuf, 10) == pdTRUE) {
            ESP_LOGI(TAG, "%s", msgBuf);
        }

        // Read a character from UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 10 / portTICK_PERIOD_MS);
        
        if (len) {
            line[idx] = data[0];

            // Process new line
            if (line[idx] == '\n' || line[idx] == '\r') {
                // Process delay command
                if (memcmp(line, "delay ", 6) == 0) {
                    char* textualDelay = line + 6;
                    delay = atoi(textualDelay);

                    if (xQueueSend(delay_queue, &delay, 10) != pdTRUE) {
                        ESP_LOGI(TAG, "Delay Queue push error");
                    }
                }

                // ESP_LOG Echo
                //line[idx] = '\0';
                //ESP_LOGI(TAG, "%s", line);
                
                /* UART Echo */
                uart_write_bytes(ECHO_UART_PORT_NUM, line, BUF_SIZE);
                memset(line, 0, BUF_SIZE);
                
                
                idx = 0;
            }
            // Wait for newline or carriage return
            else {
                idx++;
            }
        }
    }
}


void app_main(void)
{
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    led_init();
    uart_init();

    // Queue 1 to send delay time from serial_com_task to led_blink_task
    delay_queue = xQueueCreate(DELAY_QUEUE_SIZE, sizeof(int));
    // Queue 2 to send blinked message from led_blink_task to serial_com_task
    msg_queue = xQueueCreate(MSG_QUEUE_SIZE, MSG_SIZE);

    xTaskCreate(serial_com_task, "uart_listen_task", 2 * TASK_STACK_SIZE, NULL, 10, NULL);
    xTaskCreate(led_blink_task, "led_blink_task", TASK_STACK_SIZE, NULL, 10, NULL);
}

