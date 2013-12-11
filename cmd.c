#include "cdcacm.h"
#include "main.h"
#include "uart.h"

#ifndef LITE
#include "randombytes_salsa20_random.h"

void stream_rnd ( void ) {
  usbd_device *usbd_dev = get_usbdev();
  char buf[64];

  randombytes_salsa20_random_buf((void *) buf, sizeof(buf));
  while (usbd_ep_write_packet(usbd_dev, 0x82, buf, sizeof(buf)) == 0) ;
}

void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
  (void)ep;

  char buf[64];
  int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);
  if(len>0) {
    switch(buf[0]) {
    case 'r': {
      if (state != RNG) {
        state = RNG; uart_putc('R');
      } else {
        state = OFF; uart_putc('r');
      }
    }
    }
  }
}
#else
void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
  (void)ep;
  char buf[64];
  buf[0]=0;
  int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);
  if(len>0) {
    switch(buf[0]) {
    case 'r': {
      state = RNG;
      break;
    }
    case 'd': {
      state = DISK;
      break;
    }
    default: {
#ifdef USE_CDC_UART
      cdc_putc('?');
#else
      uart_putc('?');
#endif
    }
    }
  }
  //echo stuff
  //cdc_putc(buf[0]);
}
#endif
