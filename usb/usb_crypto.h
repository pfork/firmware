#ifndef usb_crypto_h
#define usb_crypto_h

#include <libopencm3/usb/usbd.h>

void usb_init(void);
usbd_device* get_usbdev(void);
void usb_start(void);

void usb_putc(const unsigned char c);
void usb_puts(const char *c);
void usb_hexstring(const unsigned int d, const unsigned int cr);
void usb_string(const char *s);

void OTG_FS_IRQHandler(void);

#endif
