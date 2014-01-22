#ifdef HAVE_MSC
#  include "dual.h"
#else 
#  include "usb.h"
#endif // HAVE_MSC
#include "init.h"
#include "led.h"
#include "keys.h"
#include "randombytes_salsa20_random.h"
#include "mixer.h"
#include "pitchfork.h"
#include "systimer.h"

void randombytes_salsa20_random_init(struct entropy_store* pool);
struct entropy_store* pool;

int main ( void ) {
#ifdef HAVE_MSC
  unsigned char kmask;
#endif // HAVE_MSC
  init();
  pool = init_pool();
  randombytes_salsa20_random_init(pool);

  bufs[0].start =  bufs[0].buf + crypto_secretbox_ZEROBYTES;
  bufs[1].start =  bufs[1].buf + crypto_secretbox_ZEROBYTES;

  while(1) {
    //usbd_poll(usbd_dev);
#ifdef HAVE_MSC
    if(dual_usb_mode == CRYPTO) handle_buf();
    kmask = key_handler();
    if(kmask & (1<<6)) {
      switch(dual_usb_mode) {
      case CRYPTO: { storage_mode(); break; }
      case DISK: { crypto_mode(); break; }
      }
    }
#endif // HAVE_MSC
    led_handler();
    if(!(sysctr & 1023)) { // 1/1000 chance ;)
      randombytes_salsa20_random_stir();
    }
  }
  return(0);
}
