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
  keyState = 0;

  keyState = gpio_get(port, key);
  uDelay(4);
  if(keyState == (gpio_get(port, key))) {
    return !keyState;
  }
  return -1;
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
  unsigned char keymask = 0;
  unsigned char res;
  if(key_pressed(JOYSTICK_0) == 1)     keymask = 1;
  if(key_pressed(JOYSTICK_1) == 1)     keymask |= 1 << 1;
  if(key_pressed(JOYSTICK_2) == 1)     keymask |= 1 << 2;
  if(key_pressed(JOYSTICK_3) == 1)     keymask |= 1 << 3;
  if(key_pressed(JOYSTICK_PRESS) == 1) keymask |= 1 << 4;
  if(key_pressed(USER_KEY) == 1)       keymask |= 1 << 5;
  if(key_pressed(WAKEUP_KEY) == 1)     keymask |= 1 << 6;
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

  enable_key(JOYSTICK_0_PORT,JOYSTICK_0_PINNO);
  enable_key(JOYSTICK_1_PORT,JOYSTICK_1_PINNO);
  enable_key(JOYSTICK_2_PORT,JOYSTICK_2_PINNO);
  enable_key(JOYSTICK_3_PORT,JOYSTICK_3_PINNO);
  enable_key(JOYSTICK_PRESS_PORT,JOYSTICK_PRESS_PINNO);
  enable_key(USER_PORT,USER_KEY_PINNO);
  enable_key(WAKEUP_PORT,WAKEUP_KEY_PINNO);
}
