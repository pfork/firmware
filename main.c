#include "cdcacm.h"
#include "haveged.h"
#include "init.h"
#include "randombytes_salsa20_random.h"
#include "uart.h"

int main ( void ) {
  unsigned int cnt = 0;
  usbd_device *usbd_dev;
  init();
  usbd_dev = get_usbdev();
  char buf[64];

  while(1) {
    haveged_collect();
    usbd_poll(usbd_dev);
    if(usb_active) {
      randombytes_salsa20_random_buf((void *) buf, sizeof(buf));
      while (usbd_ep_write_packet(usbd_dev, 0x82, buf, sizeof(buf)) == 0) ;
    }
    // stir occasionally
    if (cnt++ & (1<<13) ) {
      uart_putc('S');
      randombytes_salsa20_random_stir();
      cnt = 0;
    }
    //Delay(1000);
  }
  return(0);
}
