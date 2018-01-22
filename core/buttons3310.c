#include "stm32f.h"
#include "delay.h"
#include "buttons.h"
#include "buttons_gpio.h"

/** @brief driver for the charlieplexed keyboard of the nokia3310 board
 * @description the keyboard on the nokia3310 layout is a 4 pin
   charlieplexed button matrix with 16 keys the pins associated with
   this matrix are btn[0-3]

   to read out a button press, we need to configure each line (acting
   as a row) as output(High) while we poll all the other lines (acting
   as columns) as inputs if they are pulled high. The below matrix
   shows the setup:

   btn0   btn1   btn2   btn3   high
   {       {d,4}, {m,7}, {c,a}}, // btn0 active
   {{u,3},        {m,9}, {c,s}}, // btn1 active
   {{2,3}, {5,6},        {s,0}}, // btn2 active
   {{2,1}, {5,4}, {8,7}       }, // btn3 active

   e.g. we set btn2 as high, and if btn0 is high, then either the
   button for 2 or 3 are pressed, which it is we can find out by
   checking btn0 while setting btn1 and btn3 to high and checking btn2
   in which case it is high, deciding if 2 or 3 was pressed.
 */

#define KEYBOARD_CLK         BTN_0_CLK | BTN_1_CLK | BTN_2_CLK | BTN_3_CLK

/**
* @brief  gpio_init
*         configures a gpio as a button line - pullup/100MHz, these don't change over time.
* @param  base : base address of gpio port
* @param  pin : pin number
* @retval None
*/
static void gpio_init(unsigned int base, unsigned int pin) {
  GPIO_Regs * greg;
  greg = (GPIO_Regs *) base;
  greg->PUPDR &= ~(3 << (pin << 1));
  greg->OSPEEDR &= ~(GPIO_Speed_100MHz << (pin << 1));
  greg->PUPDR |= (GPIO_PuPd_DOWN << (pin << 1));
  greg->OSPEEDR |= (GPIO_Speed_100MHz << (pin << 1));
}

/**
* @brief  buttonrow_input
*         configures a gpio as a buttonrow input line
* @param  base : base address of gpio port
* @param  pin : pin number
* @retval None
*/
static void buttonrow_input(unsigned int base, unsigned int pin) {
  GPIO_Regs * greg;
  greg = (GPIO_Regs *) base;
  greg->MODER &= ~(3 << (pin << 1));
  greg->MODER |= (GPIO_Mode_IN << (pin << 1));
}

/**
* @brief  buttonrow_active
*         configures a gpio as an active (hi) buttonrow
* @param  base : base address of gpio port
* @param  pin : pin number
* @retval None
*/
static void buttonrow_active(unsigned int base, unsigned int pin) {
  GPIO_Regs * greg;
  greg = (GPIO_Regs *) base;
  greg->MODER &= ~(3 << (pin << 1));
  greg->MODER |= (GPIO_Mode_OUT << (pin << 1));
}

/**
* @brief  button_matrix
*         defines a private array of gpio port and gpio pin number
*         tuples. This is used to iterate over the GPIOs to read out
*         the keyboard state.
*/
const static struct {
  uint32_t port;
  uint32_t pin;
} button_matrix[] = {
  {BTN_0_PORT, BTN_0_PINNO},
  {BTN_1_PORT, BTN_1_PINNO},
  {BTN_2_PORT, BTN_2_PINNO},
  {BTN_3_PORT, BTN_3_PINNO},
};

/**
* @brief  line_active
*         checks if a buttoncolumn is active, handles debouncing naively
* @param  port : base address of gpio port
* @param  pin : pin number
* @retval None
*/
static int line_active(unsigned int port, unsigned int pin) {
  pin = (1 << pin); // gpio_get needs bit mask
  unsigned int line_state = gpio_get(port, pin), tmp;
  while(1) {
    uDelay(4);
    if(line_state == (tmp=gpio_get(port, pin))) {
      return line_state!=0;
    }
    line_state = tmp;
  }
}

/**
* @brief  _get_buttons
*         cycles through buttonrows, and samples the columns, converts it to buttons pressed
* @retval the pressed button as a char
*/
static unsigned char _buttons_pressed(void) {
  int i,j;
  uint32_t state=0;
  // set all rows to input
  for(i=0;i<4;i++) {
      buttonrow_input(button_matrix[i].port,button_matrix[i].pin);
  }

  // for each button row
  for(i=0;i<4;i++) {
    // set row active, read out the others
    buttonrow_active(button_matrix[i].port,button_matrix[i].pin);
    gpio_set(button_matrix[i].port,1<<button_matrix[i].pin);
    for(j=0;j<4;j++) {
      if(i==j) continue;
      // read out the line
      if(line_active(button_matrix[j].port,button_matrix[j].pin)) {
        state|=((1<<j)<<(i*4));
      }
    }
    // put the row back to input
    buttonrow_input(button_matrix[i].port,button_matrix[i].pin);
  }
  switch(state) {
  case 0x0088: return 'c';
  case 0x0044: return 'm';
  case 0x0002: return 'd';
  case 0x0010: return 'u';
  case 0x1000: return '1';
  case 0x1100: return '2';
  case 0x0110: return '3';
  case 0x2002: return '4';
  case 0x2200: return '5';
  case 0x0200: return '6';
  case 0x4004: return '7';
  case 0x4000: return '8';
  case 0x0040: return '9';
  case 0x0008: return '*';
  case 0x0800: return '0';
  case 0x0880: return '#';
  default: return 0;
  }
}

/**
* @brief  get_buttons
*         wrapper around _get_buttons to "debounce" buttons which have only one bit set
* @retval the pressed button as a char
*/
unsigned char buttons_pressed(void) {
  unsigned char b=_buttons_pressed();
  switch(b) {
  case 'u':
  case 'd':
  case '1':
  case '6':
  case '8':
  case '9':
  case '*':
  case '0': {
    if(b==_buttons_pressed()) return b;
    return 0;
  }
  default: return b;
  }
}

/**
* @brief  button_init
*         initializes the gpios
* @retval None
*/
void buttons_init(void) {
  MMIO32(RCC_AHB1ENR) |= (KEYBOARD_CLK);

  int i;
  // init button gpios
  for(i=0;i<4;i++) {
    gpio_init(button_matrix[i].port,button_matrix[i].pin);
  }
}

static unsigned char prevbutt = 0;
/**
* @brief  button_handler
*         checks all buttons, returns a char with a mask of buttons
*         released since last invocation
* @param  None
* @retval None
*/
unsigned char button_handler(void) {
  unsigned char button = buttons_pressed();
  if(prevbutt && button==0) {
    button=prevbutt;
    prevbutt = 0;
    return button;
  }
  prevbutt=button;
  return 0;
}
