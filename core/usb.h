#ifndef usb_crypto_h
#define usb_crypto_h

#include <libopencm3/usb/usbd.h>

#define USB_CRYPTO_EP_CTRL_IN 0x01
#define USB_CRYPTO_EP_DATA_IN 0x02
#define USB_CRYPTO_EP_CTRL_OUT 0x81
#define USB_CRYPTO_EP_DATA_OUT 0x82

void usb_init(void);
void usb_start(void);
void usb_write(const unsigned char* src, const char len, unsigned int retries, unsigned char ep);
unsigned int data_read(unsigned char* dst);

void OTG_FS_IRQHandler(void);

#endif
