#ifndef cdcacm_h
#define cdcacm_h

#include <libopencm3/usb/usbd.h>

void usb_init(void);
usbd_device* get_usbdev(void);

void cdc_putc(const unsigned char c);
void cdc_puts(const char *c);
void cdc_hexstring(const unsigned int d, const unsigned int cr);
void cdc_string(const char *s);

void otg_fs_isr(void);

#endif
