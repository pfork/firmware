/**
  ************************************************************************************
  * @file    led.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides functions and intializers for the leds
  ************************************************************************************
  */

#include "stm32f.h"
#include "led.h"
#include "main.h"

/**
  * @brief  configuration for leds
  * @param  None
  * @retval None
  */
LED_CFG leds[4] = {
  { // status 1 for usb status
    .mode = 0,
    .port = LED_BASE,
    .pin = GPIO_Pin_13,
    .period = 0,
    .counter = 0,
    .fadedir = 0,
    .fadeidx = 0,
  },
  { // status 2
    .mode = SOFTBLINK,
    .port = LED_BASE,
    .pin = GPIO_Pin_12,
    .period = 300,
    .counter = 0,
    .fadedir = 5,
    .fadeidx = 3000,
  },
  { // used for read led
    .mode = 0,
    .port = LED_BASE,
    .pin = GPIO_Pin_15,
    .period = 0,
    .counter = 0,
    .fadedir = 0,
    .fadeidx = 0,
  },
  { // used for write led
    .mode = 0,
    .port = LED_BASE,
    .pin = GPIO_Pin_14,
    .period = 0,
    .counter = 0,
    .fadedir = 0,
    .fadeidx = 0,
  }
};

/**
* @brief  led_init
*         inits gpios for leds
* @param  None
* @retval None
*/
void led_init(void) {
  GPIO_Regs * greg;

  /* GPIOF Periph clock enable */
  MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOD;

  greg = (GPIO_Regs *) GPIOD_BASE;
  /* Configure Pd15 Pd14 Pd13 Pd12 in output pushpull mode */
  greg->MODER |= ((GPIO_Mode_OUT << (15 << 1)) |
                  (GPIO_Mode_OUT << (14 << 1)) |
                  (GPIO_Mode_OUT << (13 << 1)) |
                  (GPIO_Mode_OUT << (12 << 1)));
  //greg->OTYPER |= ((GPIO_OType_PP << 15) |
  //                 (GPIO_OType_PP << 14) |
  //                 (GPIO_OType_PP << 13) |
  //                 (GPIO_OType_PP << 12));
  greg->PUPDR |= ((GPIO_PuPd_UP << (15 << 1)) |
                 (GPIO_PuPd_UP << (14 << 1)) |
                  (GPIO_PuPd_UP << (13 << 1)) |
                  (GPIO_PuPd_UP << (12 << 1)));
  greg->OSPEEDR |= ((GPIO_Speed_100MHz << (15 << 1)) |
                    (GPIO_Speed_100MHz << (14 << 1)) |
                    (GPIO_Speed_100MHz << (13 << 1)) |
                    (GPIO_Speed_100MHz << (12 << 1)));
}

/**
* @brief  handle_led
*         handles led status
* @param  led: pointer to led context
* @retval None
*/
void handle_led(LED_CFG* led) {
  if(led->period>0) {
    if(led->counter>0) {
      led->counter=led->counter-1;
    } else {
      gpio_toggle(led->port, led->pin);
      if(led->fadedir==0) {
        // we reached bottom, restart
        led->counter=led->period;
      } else {
        if((led->fadedir+led->fadeidx>=led->period) &&
           (led->fadedir+led->fadeidx<=0)) {
          // this is the end, reverse fading
          led->fadedir *= -1;
        }
        led->fadeidx+=led->fadedir;
        led->counter=led->fadeidx;
      }
    }
  }
}

/**
* @brief  led_handler
*         handles "all" configured leds
* @param  None
* @retval None
*/
void led_handler(void) {
  //handle_led(&leds[0]); // status 1
  handle_led(&leds[1]); // status 2
  //handle_led(&leds[2]); // status r
  //handle_led(&leds[3]); // status w
}
