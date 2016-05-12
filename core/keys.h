/**
  ************************************************************************************
  * @file    keys.h
  * @author  stf
  * @version V0.0.1
  * @date    01-January-2014
  ************************************************************************************
  */

#ifndef keys_h
#define keys_h

// btn0 -> pb14
// btn1 -> pc6
// btn2 -> pc7
// btn3 -> pa8

#define BTN_0_CLK       RCC_AHB1Periph_GPIOB
#define BTN_0_PORT      GPIOB_BASE
#define BTN_0_PIN       GPIO_Pin_14
#define BTN_0_PINNO     14
#define BTN_0           BTN_0_PORT, BTN_0_PIN

#define BTN_1_CLK       RCC_AHB1Periph_GPIOC
#define BTN_1_PORT      GPIOC_BASE
#define BTN_1_PIN       GPIO_Pin_6
#define BTN_1_PINNO     6
#define BTN_1           BTN_1_PORT, BTN_1_PIN

#define BTN_2_CLK       RCC_AHB1Periph_GPIOC
#define BTN_2_PORT      GPIOC_BASE
#define BTN_2_PIN       GPIO_Pin_7
#define BTN_2_PINNO     7
#define BTN_2           BTN_2_PORT, BTN_2_PIN

#define BTN_3_CLK       RCC_AHB1Periph_GPIOA
#define BTN_3_PORT      GPIOA_BASE
#define BTN_3_PIN       GPIO_Pin_8
#define BTN_3_PINNO     8
#define BTN_3           BTN_3_PORT, BTN_3_PIN

#define BTN_4_CLK       RCC_AHB1Periph_GPIOA
#define BTN_4_PORT      GPIOA_BASE
#define BTN_4_PIN       GPIO_Pin_15
#define BTN_4_PINNO     15
#define BTN_4           BTN_4_PORT, BTN_4_PIN

#define KEYBOARD_CLK         BTN_0_CLK | BTN_1_CLK | BTN_2_CLK | BTN_3_CLK | BTN_4_CLK

#define BUTTON_ENTER 16
#define BUTTON_LEFT 8
#define BUTTON_DOWN 4
#define BUTTON_UP 2
#define BUTTON_RIGHT 1

void keys_init(void);
char key_pressed(unsigned int port, unsigned int key);
unsigned char keys_pressed(void);
unsigned char key_handler(void);

#endif
