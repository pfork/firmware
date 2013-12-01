#include "cdcacm.h"
#include "randombytes_salsa20_random.h"

void stream_rnd ( void ) {
  usbd_device *usbd_dev = get_usbdev();
  char buf[64];

  randombytes_salsa20_random_buf((void *) buf, sizeof(buf));
  while (usbd_ep_write_packet(usbd_dev, 0x82, buf, sizeof(buf)) == 0) ;
}
