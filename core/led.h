#ifndef led_h
#define led_h
#include "stm32f.h"

#define LED_BASE GPIOD_BASE

#define set_read_led gpio_set(LED_BASE, GPIO_Pin_15)
#define reset_read_led gpio_reset(LED_BASE, GPIO_Pin_15)
#define toggle_read_led gpio_toggle(LED_BASE, GPIO_Pin_15)

#define set_write_led gpio_set(LED_BASE, GPIO_Pin_14)
#define reset_write_led gpio_reset(LED_BASE, GPIO_Pin_14)
#define toggle_write_led gpio_toggle(LED_BASE, GPIO_Pin_14)

#define set_status1_led gpio_set(LED_BASE, GPIO_Pin_13)
#define reset_status1_led gpio_reset(LED_BASE, GPIO_Pin_13)
#define toggle_status1_led gpio_toggle(LED_BASE, GPIO_Pin_13)

#define set_status2_led gpio_set(LED_BASE, GPIO_Pin_12)
#define reset_status2_led gpio_reset(LED_BASE, GPIO_Pin_12)
#define toggle_status2_led gpio_toggle(LED_BASE, GPIO_Pin_12)

void led_init(void);
void led_handler(void);

#endif // led_h
