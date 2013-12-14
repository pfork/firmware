#include "dual.h"
#include "init.h"
#include "led.h"
#include "keys.h"
#include "randombytes_salsa20_random.h"
#include "mixer.h"
#include "crypto_handlers.h"
#include "systimer.h"

void randombytes_salsa20_random_init(struct entropy_store* pool);
struct entropy_store* pool;
void (*op_cb)(void) = 0;

int main ( void ) {
  unsigned char kmask;
  init();
  pool = init_pool();
  randombytes_salsa20_random_init(pool);

  while(1) {
    //if(dual_usb_mode == CRYPTO && cmd_fn)
      // we are in polling mode
      //usbd_poll(usbd_dev);
    if(dual_usb_mode == CRYPTO && op_cb)
      op_cb();
    kmask = key_handler();
    if(kmask & (1<<6)) {
      switch(dual_usb_mode) {
      case CRYPTO: { storage_mode(); break; }
      case DISK: { crypto_mode(); break; }
      }
    }
    led_handler();
    if(!(sysctr & 1023)) { // app once / sec
      randombytes_salsa20_random_stir();
    }
  }
  return(0);
}
