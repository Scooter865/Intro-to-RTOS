#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

TaskHandle_t printerTaskHandle = NULL;
TaskHandle_t printerTaskHandle2 = NULL;
TaskHandle_t blinkerHandle = NULL;
TaskHandle_t blinkerHandle2 = NULL;

// Begin copied LED strip config code
#define BLINK_GPIO 8

led_strip_handle_t led_strip;

void led_strip_init(void) {
    /* LED strip initialization with the GPIO and pixels number*/
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
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}
// End LED strip config code
/*
void printer(void * param) {
    while(true) {
        printf("demo printer printing\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void printer2(void * param) {
    while(true) {
        printf("demo printer 2 printing\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
*/
void blinker(void * param) {
    while(true) {
        printf("White on!\n");
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        led_strip_refresh(led_strip);
        vTaskDelay(323 / portTICK_PERIOD_MS);
        printf("White off!\n");
        led_strip_clear(led_strip);
        vTaskDelay(323 / portTICK_PERIOD_MS);
    }
}

void blinker2(void * param) {
    while(true) {
        printf("Red on!\n");
        led_strip_set_pixel(led_strip, 0, 16, 0, 0);
        led_strip_refresh(led_strip);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        printf("Red off!\n");
        led_strip_clear(led_strip);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    led_strip_init();

    //xTaskCreatePinnedToCore(printer, "printer", 1024, NULL, 1, &printerTaskHandle, 0);
    //xTaskCreatePinnedToCore(printer2, "printer2", 1024, NULL, 1, &printerTaskHandle2, 0);
    xTaskCreatePinnedToCore(blinker, "blinker", 1024, NULL, 1, &blinkerHandle, 0);
    xTaskCreatePinnedToCore(blinker2, "blinker2", 2048, NULL, 1, &blinkerHandle2, 0);
}
