#include "cdcacm.h"
#include "init.h"
#include "uart.h"
#include "main.h"

unsigned int state = OFF;

int main ( void ) {
  usbd_device *usbd_dev;

  init();
  usbd_dev = get_usbdev();

  while(1) {
    usbd_poll(usbd_dev);
    //Delay(1000);
  }
  return(0);
}
