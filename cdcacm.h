#ifndef cdcacm_h
#define cdcacm_h

#include <libopencm3/usb/usbd.h>

void usb_init(void);
usbd_device* get_usbdev(void);
extern unsigned char usb_active;

//void otg_fs_isr(void);

#endif
