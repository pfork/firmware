#include "cdcacm.h"
#include "init.h"
#include "uart.h"
#include "cmd.h"
#include "randombytes_salsa20_random.h"
#include "mixer.h"
#include "main.h"

unsigned int state = OFF;

void randombytes_salsa20_random_init(struct entropy_store* pool);
struct entropy_store* pool;

int main ( void ) {
  unsigned int cnt = 0;
  usbd_device *usbd_dev;

  init();
  pool = init_pool();
  randombytes_salsa20_random_init(pool);
  usbd_dev = get_usbdev();

  while(1) {
    usbd_poll(usbd_dev);
    if(state == RNG) {
      stream_rnd();
    }
    if (cnt++ & (1<<(22 - (~~state)*7)) ) {
      cnt = 0;
      if(state!=RNG) {
        // stir occasionally
        uart_putc('S');
        randombytes_salsa20_random_stir();
      } else {
        uart_putc('>');
      }
    }
    //Delay(1000);
  }
  return(0);
}
