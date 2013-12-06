#ifndef cdcacm_h
#define cdcacm_h

#include <libopencm3/usb/usbd.h>

void usb_init(void);
usbd_device* get_usbdev(void);

void cdc_putc(unsigned char c);
void cdc_put_block(unsigned char *c, unsigned char len);
void cdc_hexstring(unsigned int d, unsigned int cr);
void cdc_string(const char *s);

//void otg_fs_isr(void);

#endif
