#include "keys.h"
#include "stm32f.h"

void enable_key(unsigned int base, unsigned int port) {
  GPIO_Regs * greg;
  greg = (GPIO_Regs *) base;
  greg->MODER |= (GPIO_Mode_IN << (port << 1));
  //greg->OTYPER |= (GPIO_OType_PP << port);
  greg->PUPDR |= (GPIO_PuPd_UP << (port << 1));
  greg->OSPEEDR |= (GPIO_Speed_100MHz << (port << 1));
}

char key_pressed(unsigned int port, unsigned int key) {
  unsigned int keyState, keyState2;
  keyState = keyState2 = 0;

  keyState = gpio_get(port, key);
  ASM_DELAY(1);
  keyState2 = (gpio_get(port, key));
  if(keyState == keyState2) {
    return !!keyState;
  }
  return -1;
}

/* unsigned int keyrepeat = 0; */

unsigned char key_handler(void) {
  unsigned char keymask = 0;
  /* if(keyrepeat>0) { */
  /*   keyrepeat--; */
  /* } else { */
    if(key_pressed(JOYSTICK_0) == 0)     keymask = 1;
    if(key_pressed(JOYSTICK_1) == 0)     keymask |= 1 << 1;
    if(key_pressed(JOYSTICK_2) == 0)     keymask |= 1 << 2;
    if(key_pressed(JOYSTICK_3) == 0)     keymask |= 1 << 3;
    if(key_pressed(JOYSTICK_PRESS) == 0) keymask |= 1 << 4;
    if(key_pressed(USER_KEY) == 0)       keymask |= 1 << 5;
    if(key_pressed(WAKEUP_KEY) == 0)     keymask |= 1 << 6;
    //if(keymask>0) keyrepeat = 200;
  /* } */
  return keymask;
}

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
