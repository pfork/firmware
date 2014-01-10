/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include "stm32f.h"
#include "dual.h"
#include "main.h"
#include "usb_crypto.h"
#include "crypto/usb_handler.h"

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
	"PITCHFORK "VERSION,
	"0x01729a",
};

/**
* @brief  usb_putc
*         outputs one char to the usb debug endpoint
* @param  c: char to output
* @retval None
*/
void usb_putc(const unsigned char c) {
  if(dual_usb_mode != CRYPTO) return;
  while (usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_CTRL_OUT, (void*) &c, 1) == 0) ;
}

/**
* @brief  usb_puts
*         outputs a string to the usb debug endpoint
* @param  s: string to output
* @retval None
*/
void usb_puts(const char *c) {
  if(dual_usb_mode != CRYPTO) return;
  char *p = (char*) c;
  unsigned int i=0;
  while(p[i]) {
    for(;p[i] && i<64;i++);
    if(i==0) break;
    while (usbd_ep_write_packet(usbd_dev, USB_CRYPTO_EP_CTRL_OUT, (void*) p, i) == 0) ;
    p+=i;
    i=0;
  }
}

/**
* @brief  usb_hexstring
*         converts an unsigned int to hex and
*         outputs it to the usb debug endpoint
* @param  d: int to output
* @param  cr: append newline?
* @retval None
*/
void usb_hexstring(unsigned int d, unsigned int cr) {
  if(dual_usb_mode != CRYPTO) return;
  //unsigned int ra;
  unsigned int rb;
  unsigned int rc;

  rb=32;
  while(1) {
    rb-=4;
    rc=(d>>rb)&0xF;
    if(rc>9) rc+=0x37; else rc+=0x30;
    usb_putc(rc);
    if(rb==0) break;
  }
  if(cr) {
    usb_putc(0x0D);
    usb_putc(0x0A);
  } else {
    usb_putc(0x20);
  }
}

/**
* @brief  usb_puts
*         outputs a string to the usb debug endpoint
* @param  s: string to output
* @retval None
*/
void usb_string(const char *s) {
  if(dual_usb_mode != CRYPTO) return;
  for(;*s;s++) {
    if(*s==0x0A) usb_putc(0x0D);
    usb_putc(*s);
  }
}

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static int usb_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
  (void)complete;
  (void)buf;
  (void)usbd_dev;

  /* switch (req->bRequest) { */
  /* case some_value: */
  /*   return 1; */
  /* } */
  return 0;
}

void usb_data_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
  switch(ep) {
  case USB_CRYPTO_EP_CTRL_IN: {
    handle_ctl();
    break;
  }
  case USB_CRYPTO_EP_DATA_IN: {
    handle_data();
    break;
  }
  }
}

void usb_set_config(usbd_device *usbd_dev, uint16_t wValue) {
	(void)wValue;

	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_CTRL_IN, USB_ENDPOINT_ATTR_BULK, 64, usb_data_rx_cb);
	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_CTRL_OUT, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64, usb_data_rx_cb);
	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64, NULL);

	usbd_register_control_callback(usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				usb_control_request);
}

void usb_start(void) {
	usbd_dev = usbd_init(&otgfs_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, usb_set_config);
}

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
