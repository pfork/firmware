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

//used gpios for keys: A(PA04)  B(PA06)  C(PB15)  D(PC06)  PRESS(PC07)

#define JOYSTICK_0_CLK       RCC_AHB1Periph_GPIOA
#define JOYSTICK_0_PORT      GPIOA_BASE
#define JOYSTICK_0_PIN       GPIO_Pin_4
#define JOYSTICK_0_PINNO     4
#define JOYSTICK_0           JOYSTICK_0_PORT, JOYSTICK_0_PIN

#define JOYSTICK_1_CLK       RCC_AHB1Periph_GPIOA
#define JOYSTICK_1_PORT      GPIOA_BASE
#define JOYSTICK_1_PIN       GPIO_Pin_6
#define JOYSTICK_1_PINNO     6
#define JOYSTICK_1           JOYSTICK_1_PORT, JOYSTICK_1_PIN

#define JOYSTICK_2_CLK       RCC_AHB1Periph_GPIOB
#define JOYSTICK_2_PORT      GPIOB_BASE
#define JOYSTICK_2_PIN       GPIO_Pin_15
#define JOYSTICK_2_PINNO     15
#define JOYSTICK_2           JOYSTICK_2_PORT, JOYSTICK_2_PIN

#define JOYSTICK_3_CLK       RCC_AHB1Periph_GPIOC
#define JOYSTICK_3_PORT      GPIOC_BASE
#define JOYSTICK_3_PIN       GPIO_Pin_6
#define JOYSTICK_3_PINNO     6
#define JOYSTICK_3           JOYSTICK_3_PORT, JOYSTICK_3_PIN

#define JOYSTICK_PRESS_CLK   RCC_AHB1Periph_GPIOC
#define JOYSTICK_PRESS_PORT  GPIOC_BASE
#define JOYSTICK_PRESS_PIN   GPIO_Pin_7
#define JOYSTICK_PRESS_PINNO 7
#define JOYSTICK_PRESS       JOYSTICK_PRESS_PORT, JOYSTICK_PRESS_PIN

// 2 extra keys
//used gpios for keys: USER(PB01)  WAKEUP(PA00)

#define USER_CLK             RCC_AHB1Periph_GPIOB
#define USER_PORT            GPIOB_BASE
#define USER_KEY_PIN         GPIO_Pin_1
#define USER_KEY_PINNO       1
#define USER_KEY             USER_PORT,USER_KEY_PIN

#define WAKEUP_CLK           RCC_AHB1Periph_GPIOA
#define WAKEUP_PORT          GPIOA_BASE
#define WAKEUP_KEY_PIN       GPIO_Pin_0
#define WAKEUP_KEY_PINNO     0
#define WAKEUP_KEY           WAKEUP_PORT,WAKEUP_KEY_PIN

#define KEYBOARD_CLK         JOYSTICK_0_CLK | JOYSTICK_1_CLK | JOYSTICK_2_CLK | JOYSTICK_3_CLK | JOYSTICK_PRESS_CLK | USER_CLK | WAKEUP_CLK

void keys_init(void);
char key_pressed(unsigned int port, unsigned int key);
unsigned char key_handler(void);

#endif
