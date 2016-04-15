/**
  ************************************************************************************
  * @file    keys.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides reading and handling of buttons
  ************************************************************************************
  */

#include "stm32f.h"
#include "keys.h"
#include "delay.h"

/**
* @brief  enable_key
*         configures a gpio as a button
* @param  base : base address of gpio port
* @param  port : pin number
* @retval None
*/
void enable_key(unsigned int base, unsigned int port) {
  GPIO_Regs * greg;
  greg = (GPIO_Regs *) base;
  greg->MODER |= (GPIO_Mode_IN << (port << 1));
  //greg->OTYPER |= (GPIO_OType_PP << port);
  greg->PUPDR |= (GPIO_PuPd_UP << (port << 1));
  greg->OSPEEDR |= (GPIO_Speed_100MHz << (port << 1));
}

/**
* @brief  key_pressed
*         checks if a button was pressed, handles debouncing naively
* @param  base : base address of gpio port
* @param  key : pin number
* @retval None
*/
char key_pressed(unsigned int port, unsigned int key) {
  unsigned int keyState;
  while(1) {
    keyState = gpio_get(port, key);
    uDelay(4);
    if(keyState == (gpio_get(port, key))) {
      return !keyState;
    }
  }
}

/**
* @brief  keys_pressed
*         returns a keymask of all buttons pressed
* @retval None
*/

unsigned char keys_pressed(void) {
    return key_pressed(BTN_0) |
           key_pressed(BTN_1) << 1 |
           key_pressed(BTN_2) << 2 |
           key_pressed(BTN_3) << 3;
}

unsigned char prevmask = 0;
/**
* @brief  key_handler
*         checks all keys, returns a char with a mask of keys
*         released since last invocation
* @param  None
* @retval None
*/
unsigned char key_handler(void) {
  unsigned char keymask = keys_pressed();
  unsigned char res;
  //res =  prevmask ^ keymask; /* detect changes in key states */
  res = (prevmask ^ keymask) & ((~keymask) & prevmask); /* detect on key releases */
  prevmask=keymask;
  return res;
}

/**
* @brief  keys_init
*         enables gpios for all available buttons
* @param  None
* @retval None
*/
void keys_init(void) {
  MMIO32(RCC_AHB1ENR) |= (KEYBOARD_CLK);

  enable_key(BTN_0_PORT,BTN_0_PINNO);
  enable_key(BTN_1_PORT,BTN_1_PINNO);
  enable_key(BTN_2_PORT,BTN_2_PINNO);
  enable_key(BTN_3_PORT,BTN_3_PINNO);
}
