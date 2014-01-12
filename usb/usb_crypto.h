#ifndef usb_crypto_h
#define usb_crypto_h

#include <libopencm3/usb/usbd.h>

#define USB_CRYPTO_EP_CTRL_IN 0x01
#define USB_CRYPTO_EP_DATA_IN 0x02
#define USB_CRYPTO_EP_CTRL_OUT 0x81
#define USB_CRYPTO_EP_DATA_OUT 0x82

void usb_init(void);
void usb_start(void);

void usb_putc(const unsigned char c);
/* void usb_puts(const char *c); */
void usb_hexstring(const unsigned int d, const unsigned int cr);
void usb_string(const char *s);

void OTG_FS_IRQHandler(void);

#endif
