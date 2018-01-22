#ifndef buttons_gpio_gh_h
#define buttons_gpio_gh_h

#include "buttons_gpio.h"

// gh has a 5th button:
// btn4 -> pa15

#define BTN_4_CLK       RCC_AHB1Periph_GPIOA
#define BTN_4_PORT      GPIOA_BASE
#define BTN_4_PIN       GPIO_Pin_15
#define BTN_4_PINNO     15
#define BTN_4           BTN_4_PORT, BTN_4_PIN

#define KEYBOARD_CLK         BTN_0_CLK | BTN_1_CLK | BTN_2_CLK | BTN_3_CLK | BTN_4_CLK

#endif // buttons_gpio_gh_h
