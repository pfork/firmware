/**
  ************************************************************************************
  * @file    buttons.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides reading and handling of buttons
  ************************************************************************************
  */

#include "stm32f.h"
#include "buttons.h"
#include "buttons_gpio_gh.h"
#include "delay.h"

/**
* @brief  enable_button
*         configures a gpio as a button
* @param  base : base address of gpio port
* @param  port : pin number
* @retval None
*/
static void enable_button(unsigned int base, unsigned int port) {
  GPIO_Regs * greg;
  greg = (GPIO_Regs *) base;
  greg->MODER |= (GPIO_Mode_IN << (port << 1));
  //greg->OTYPER |= (GPIO_OType_PP << port);
  greg->PUPDR |= (GPIO_PuPd_UP << (port << 1));
  greg->OSPEEDR |= (GPIO_Speed_100MHz << (port << 1));
}

/**
* @brief  button_pressed
*         checks if a button was pressed, handles debouncing naively
* @param  base : base address of gpio port
* @param  button : pin number
* @retval None
*/
static char button_pressed(unsigned int port, unsigned int button) {
  unsigned int buttonState;
  while(1) {
    buttonState = gpio_get(port, button);
    uDelay(4);
    if(buttonState == (gpio_get(port, button))) {
      return !buttonState;
    }
  }
}

/**
* @brief  buttons_pressed
*         returns a buttonmask of all buttons pressed
* @retval None
*/

unsigned char buttons_pressed(void) {
    return button_pressed(BTN_0) |
           button_pressed(BTN_1) << 1 |
           button_pressed(BTN_2) << 2 |
           button_pressed(BTN_3) << 3 |
           button_pressed(BTN_4) << 4;
}

/**
* @brief  buttons_init
*         enables gpios for all available buttons
* @param  None
* @retval None
*/
void buttons_init(void) {
  MMIO32(RCC_AHB1ENR) |= (KEYBOARD_CLK);

  enable_button(BTN_0_PORT,BTN_0_PINNO);
  enable_button(BTN_1_PORT,BTN_1_PINNO);
  enable_button(BTN_2_PORT,BTN_2_PINNO);
  enable_button(BTN_3_PORT,BTN_3_PINNO);
  enable_button(BTN_4_PORT,BTN_4_PINNO);
}

static unsigned char prevmask = 0;
/**
* @brief  button_handler
*         checks all buttons, returns a char with a mask of buttons
*         released since last invocation
* @param  None
* @retval None
*/
unsigned char button_handler(void) {
  unsigned char buttonmask = buttons_pressed();
  unsigned char res;
  //res =  prevmask ^ buttonmask; /* detect changes in button states */
  res = (prevmask ^ buttonmask) & ((~buttonmask) & prevmask); /* detect on button releases */
  prevmask=buttonmask;
  return res;
}
