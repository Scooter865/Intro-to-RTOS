#include "led_strip.h"
#include "driver/uart.h"


// GPIO assignment
#define LED_STRIP_BLINK_GPIO  GPIO_NUM_8
// Numbers of the LED in the strip
#define LED_STRIP_LED_NUMBERS 1
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)
// UART parameters
#define UART_PORT_NUM UART_NUM_0
#define UART_TX_PIN 16
#define UART_RX_PIN 17
#define UART_BUF_SIZE 1024


led_strip_handle_t configure_led(void);
void configure_uart(void);