#include "cdcacm.h"
#include "haveged.h"
#include "init.h"
#include "uart.h"
#include "cmd.h"
#include "randombytes_salsa20_random.h"
#include "main.h"

unsigned int state = OFF;

int main ( void ) {
  unsigned int cnt = 0;
  usbd_device *usbd_dev;

  init();
  usbd_dev = get_usbdev();

  while(1) {
    haveged_collect();
    usbd_poll(usbd_dev);
    if(state == RNG) {
      stream_rnd();
      if(cnt & (1 << 10)) uart_putc('>');
    }
    // stir occasionally
    if (cnt++ & (1<<15) ) {
      uart_putc('S');
      randombytes_salsa20_random_stir();
      cnt = 0;
    }
    //Delay(1000);
  }
  return(0);
}
