#ifndef buttons_gpio_h
#define buttons_gpio_h

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

#endif // buttons_gpio_h
