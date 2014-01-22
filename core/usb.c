/**
  ************************************************************************************
  * @file    usb.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides the usb low-level hw setup for PITCHFORK mode
  ************************************************************************************
  */

#include <stdlib.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include "stm32f.h"
#include "main.h"
#include "led.h"
#include "usb.h"
#include "pitchfork.h"

/**
  * @brief  defines a custom vendor usb device with a STM usb dev id
  *         initializes the SDIO, SDIO DMA and USB IRQs
  */
static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0xff, /* vendor class */
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5740,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/**
  * @brief  defines 4 endpoints 2 for ctl 2 for data
  */
static const struct usb_endpoint_descriptor data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_CRYPTO_EP_CTRL_IN,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_CRYPTO_EP_CTRL_OUT,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_CRYPTO_EP_DATA_IN,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_CRYPTO_EP_DATA_OUT,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 4,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = data_endp,
}};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = data_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"stf (c) 2013",
	"PITCHFORK " VERSION,
	"0x01729a",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

/**
  * @brief  usb set config callback as per libopencm3
  */
void usb_set_config(usbd_device *usbd_dev, uint16_t wValue) {
	(void)wValue;

	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_CTRL_IN, USB_ENDPOINT_ATTR_BULK, 64, handle_ctl);
	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_CTRL_OUT, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64, handle_data);
	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64, NULL);
}

/**
  * @brief  usb set config callback as per libopencm3
  * @param  None
  * @retval None
  */
void usb_start(void) {
	usbd_dev = usbd_init(&otgfs_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, usb_set_config);
}

/**
  * @brief  usb init - sets up stm32 hw
  * @param  None
  * @retval None
  */
void usb_init(void) {
   GPIO_Regs *greg;
   // enable gpioa
   MMIO32(RCC_AHB1ENR) |= RCC_AHB1Periph_GPIOA;
   // enable otgfsen
   MMIO32(RCC_AHB2ENR) |= RCC_AHB2ENR_OTGFSEN;

   greg = (GPIO_Regs *) GPIOA_BASE;
   // enable gpioa_[9, 11, 12]
   greg->MODER |=   (GPIO_Mode_AF << (9 << 1)) | (GPIO_Mode_AF << (11 << 1)) | (GPIO_Mode_AF << (12 << 1));
   greg->PUPDR |= (GPIO_PuPd_NOPULL << (9 << 1)) | (GPIO_PuPd_NOPULL << (11 << 1)) | (GPIO_PuPd_NOPULL << (12 << 1));
   greg->AFR[1] |=  (GPIO_AF_OTG_FS << (1 << 2)) | (GPIO_AF_OTG_FS << (3 << 2)) | (GPIO_AF_OTG_FS << (4 << 2));

   usb_start();
}

/**
  * @brief  usb_write: wraps usb_ep_write for led and retry handling
  * @param  src: pkt to send
  * @param  len: len of packet
  * @param  retries: number of retries, or 0 if send until succeed.
  * @param  ep: id of endpoint
  * @retval None
  */
void usb_write(const unsigned char* src, const char len, unsigned int retries, unsigned char ep) {
  set_write_led;
  if(retries == 0) {
    // blocking
    while(usbd_ep_write_packet(usbd_dev, ep, src, len) == 0);
  } else {
    for(;(usbd_ep_write_packet(usbd_dev, ep, src, len) == 0) && retries>0;retries--);
  }
  reset_write_led;
}

/**
  * @brief  data_read: wraps usb read ops with LED handling
  * @param  dst: receiving buffer (64bytes)
  * @retval lenght of bytes received in dst
  */
unsigned int data_read(unsigned char* dst) {
  unsigned int len;
  set_read_led;
  len = usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_DATA_IN, dst, 64);
  reset_read_led;
  return len;
}

