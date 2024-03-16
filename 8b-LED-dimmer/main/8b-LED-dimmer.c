#include <stdio.h>
#include "common_configs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"


static led_strip_handle_t led;
static TimerHandle_t led_dim_timer = NULL;


// Callback to turn off LED after timer expires
void led_turnoff(TimerHandle_t xTimer) {
        led_strip_clear(led);
}

void uart_listener_task(void* param) {
    // Program UART buffer
    uint8_t* data = (uint8_t*)malloc(32 * sizeof(uint8_t));

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE, pdMS_TO_TICKS(20));

        if (len) {
            // Echo character back to terminal
            uart_write_bytes(UART_PORT_NUM, (const char*)data, len); //Should be outside the if?
            if (data[len-1] == '\n' || data[len-1] == '\r') {
                printf("\n");
            }

            // Restart LED dim timer
            xTimerStart(led_dim_timer, portMAX_DELAY);
            // Reset LED on - maybe check LED state here?
            led_strip_set_pixel(led, 0, 10, 10, 10);
            led_strip_refresh(led);
        }
    }
}

void app_main(void) {
    led = configure_led();
    led_strip_clear(led);
    configure_uart();
    led_dim_timer = xTimerCreate("LED Dimmer Timer", pdMS_TO_TICKS(5000), pdTRUE, (void*)0, led_turnoff);

    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("\n-----LED Dimmer-----\n");

    xTaskCreate(uart_listener_task, "UART Listener", 2048, NULL, 1, NULL);
}
