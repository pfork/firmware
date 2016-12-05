#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "crypto_generichash.h"
#include "crypto_sign.h"

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/stm32/flash.h>

#include "pitchfork.h"
#include "stm32f.h"
#include "usb.h"
#include "delay.h"
#include "oled.h"

// important note usbd_dev must be in a region from the firmware which is not overwritten by iap!!!
// otherwise strange usb errors, when switching modes, and no usb device working at the end.

#define ALIGN(x) __attribute__((aligned(x)))
ALIGN(128) uint32_t nvic_table[128];

usbd_device *usbd_dev;

static volatile char state=0;
static int sector=0;
static unsigned char sectorhashes[8][64];
static unsigned char* bufstart;
static unsigned char *ptr;
static crypto_generichash_state hashstate;

static const uint8_t cont[]="go";

static char itos(char *d, uint32_t x) {
  uint8_t t[11], *p=t;
  p += 11;
  *--p = 0;
  do {
    *--p = '0' + x % 10;
    x /= 10;
  } while (x);
  memcpy(d,p,11-(p-t));
  return 11-(p-t);
}

static void invalid_sig(void) {
  // failed to verify master signature: aborting
  oled_clear();
  oled_print(0,9,"Invalid",Font_8x8);
  oled_print(0,18,"master",Font_8x8);
  oled_print(0,27,"signature",Font_8x8);
  oled_print(0,36,"pls reset",Font_8x8);
  while(1);
}

static void disable_irqs(void) {
  SYSTICK_CTRL &= ~((unsigned int) 2); /* disable interrupts */
#ifdef HAVE_MSC
  irq_disable(NVIC_SDIO_IRQn);
  irq_disable(SD_SDIO_DMA_IRQn);
#endif // HAVE_MSC
  irq_disable(NVIC_OTG_FS_IRQ);
}

static void enable_irqs(void) {
  irq_enable(NVIC_OTG_FS_IRQ);
  // enable systick irq
  //SYSTICK_CTRL |= 2; /* Enable interrupts */
}

/*
 *
 *  Vector table. If the program changes an entry in the vector table, and then
 *  enables the corresponding exception, use a DMB instruction between the
 *  operations. This ensures that if the exception is taken immediately after
 *  being enabled the processor uses the new exception vector.
 *
 */

static void move_nvic_table(uint32_t* dest) {
  disable_irqs();
  if(((unsigned long)dest & 127) != 0) {
    return; // must be 128 aligned
  }
  size_t i;
  uint32_t *src = (uint32_t*) 0x08000000; //SCB->VTOR;
  if(src == dest) return;
  for(i=0;i<128;i++) {
    dest[i] = src[i];
  }
  SCB->VTOR = (uint32_t) dest;
  enable_irqs();
}

static void write_block(uint32_t dst, unsigned char *data, size_t len) {
  disable_irqs();
  flash_unlock();
  flash_program(dst, data, len);
  flash_lock();
  enable_irqs();
}

static void clear_flash(unsigned int sector_id) {
  disable_irqs();
  flash_unlock();
  /* Erasing page*/
  flash_erase_sector(sector_id, FLASH_CR_PROGRAM_X64);
  flash_lock();
  enable_irqs();
}

void new_handle_data(usbd_device *usbd_dev, uint8_t ep) {
  int plen, total, i;
  crypto_generichash_state tmpstate;

  if(sector>=8) return;

  switch(state) {
  case 0: { // start 1st reading
    ptr=bufstart; // reset output ptr and hashstate
    crypto_generichash_init(&hashstate, NULL, 0, 64);
    state++;
    oled_print(0,9,"verifying sig...", Font_8x8);
  }
  case 1: { // collect hashes and verify signature
    plen=usb_read(ptr);
    if(plen<64) { // should never happen
      oled_clear();
      oled_print(0,9,"short usb packet.",Font_8x8);
      oled_print(0,18,"abort.",Font_8x8);
      while(1);
    }
    ptr+=plen;
    total=ptr - bufstart;
    if(total>=32*1024) {
      if(sector==7) {
        usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
        oled_print(0,18,"hashing block 7",Font_8x8);
        crypto_generichash_update(&hashstate, bufstart, (total-crypto_sign_BYTES));
        crypto_generichash_final(&hashstate, sectorhashes[sector], 64);

        oled_print(0,27,"verifying sig",Font_8x8);
        if ((crypto_sign_verify_detached((void*) bufstart+total-crypto_sign_BYTES,
                                         sectorhashes[sector],
                                         64,
                                         (uint8_t*) (OTP_START_ADDR)) != 0) &&
            (crypto_sign_verify_detached((void*) bufstart+total-crypto_sign_BYTES,
                                         sectorhashes[sector],
                                         64,
                                         (uint8_t*) (OTP_START_ADDR + 1 * OTP_BYTES_IN_BLOCK)) != 0)) {
          invalid_sig();
        }
        oled_print(0,27,"correct sig     ",Font_8x8);
        oled_print(0,36,"erasing 256KB   ",Font_8x8);
        // todo erase 256KB of flash for the new firmware
        for(i=0;i<6;i++) {
          clear_flash(i);
        }
        oled_print(0,36,"erased 256KB    ",Font_8x8);
        state++;
        ptr=bufstart; // reset output ptr
        crypto_generichash_init(&hashstate, NULL, 0, 64); // reset hash state
        sector=0;
        usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
      } else {
        usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
        oled_print(0,18,"hashing block",Font_8x8);
        char tmp[16];
        itos(tmp,sector);
        oled_print(14*8, 18, tmp, Font_8x8);
        crypto_generichash_update(&hashstate, bufstart, total);
        memcpy(&tmpstate,&hashstate,sizeof(hashstate));
        crypto_generichash_final(&tmpstate, sectorhashes[sector], 64);
        sector++;
        ptr=bufstart; // reset output ptr
        usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
        usb_write(cont, sizeof(cont), 32, USB_CRYPTO_EP_DATA_OUT);
      }
    }
    break;
  }
  case 2: { // 2nd run, now flash until hashes are the same
    plen=usb_read(ptr);
    if(plen<64) { // should never happen
      oled_clear();
      oled_print(0,45,"short usb packet.",Font_8x8);
      oled_print(0,54,"abort.",Font_8x8);
      while(1);
    }
    ptr+=plen;
    total=ptr - bufstart;
    if(total>=32*1024) {
      unsigned char hash[64];
      if(sector==7) {
        usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
        oled_print(0,45,"hashing block 7",Font_8x8);
        crypto_generichash_update(&hashstate, bufstart, (total-crypto_sign_BYTES));
        crypto_generichash_final(&hashstate, hash, 64);
        if(memcmp(hash,sectorhashes[sector],sizeof(hash))!=0 ) {
          invalid_sig();
        }
        oled_print(0,45,"writing block 7",Font_8x8);
        // todo write block
        write_block(0x08000000 + 32*1024*sector, bufstart, total);
        oled_print(0,45,"writing done    ",Font_8x8);
        state++;
        ptr=bufstart; // reset output ptr
        crypto_generichash_init(&hashstate, NULL, 0, 64); // reset hash state
        sector=0;
        usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
      } else {
        usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 1);
        char tmp[16];
        itos(tmp,sector);
        oled_print(0,45,"hashing block",Font_8x8);
        oled_print(14*8, 45, tmp, Font_8x8);
        crypto_generichash_update(&hashstate, bufstart, total);
        memcpy(&tmpstate,&hashstate,sizeof(hashstate));
        crypto_generichash_final(&tmpstate, hash, 64);
        if(memcmp(hash,sectorhashes[sector],sizeof(hash))!=0 ) {
          invalid_sig();
        }
        oled_print(0,45,"writing block",Font_8x8);
        oled_print(14*8, 45, tmp, Font_8x8);
        // todo write block
        write_block(0x08000000 + 32*1024*sector, bufstart, total);
        sector++;
        ptr=bufstart; // reset output ptr
        usbd_ep_nak_set(usbd_dev, USB_CRYPTO_EP_DATA_IN, 0);
        usb_write(cont, sizeof(cont), 32, USB_CRYPTO_EP_DATA_OUT);
      }
    }
    break;
  }
  }
}

/**
  * @brief  defines a custom vendor usb device with a STM usb dev id
  */
ALIGN(4) static const struct usb_device_descriptor dev = {
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
ALIGN(4) static const struct usb_endpoint_descriptor data_endp[] = {{
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

ALIGN(4) static const struct usb_interface_descriptor data_iface[] = {{
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

ALIGN(4) static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = data_iface,
}};

ALIGN(4) static const struct usb_config_descriptor config = {
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

ALIGN(4) static const char *usb_strings[] = {
	"stf (c) 2016",
	"PITCHFORK IAP",
	"0x01729a",
};

void * __stack_chk_guard = NULL;

void __stack_chk_guard_setup() {
  __stack_chk_guard = (void*) 0xdeadb33f;
}

void __attribute__((noreturn)) __stack_chk_fail() {
  for(;;);
}

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[128];

void usb_write(const unsigned char* src, const char len, unsigned int retries, unsigned char ep) {
  if(retries == 0) {
    // blocking
    while(usbd_ep_write_packet(usbd_dev, ep, src, len) == 0);
  } else {
    for(;(usbd_ep_write_packet(usbd_dev, ep, src, len) == 0) && retries>0;retries--);
  }
}

/**
  * @brief  data_read: wraps usb read ops with LED handling
  * @param  dst: receiving buffer (64bytes)
  * @retval lenght of bytes received in dst
  */
unsigned int usb_read(unsigned char* dst) {
  return usbd_ep_read_packet(usbd_dev, USB_CRYPTO_EP_DATA_IN, dst, 64);
}

/**
  * @brief  usb set config callback as per libopencm3
  */
static void new_usb_set_config(usbd_device *usbd_dev, uint16_t wValue) {
	(void)wValue;

	//usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_CTRL_IN, USB_ENDPOINT_ATTR_BULK, 64, new_handle_ctl);
	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_CTRL_IN, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_CTRL_OUT, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64, new_handle_data);
	usbd_ep_setup(usbd_dev, USB_CRYPTO_EP_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64, NULL);
}

static void iap_start(void) {
	usbd_dev = usbd_init(&otgfs_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, new_usb_set_config);
   usbd_disconnect(usbd_dev, true);
   mDelay(1);
   usbd_disconnect(usbd_dev, false);
}

void usb_irqhandler(void) {
  usbd_poll(usbd_dev);
}

extern unsigned char* _load_addr;

void firmware_updater(usbd_device *usbddev) {
  usbd_dev=usbddev;
  state=0;
  sector=0;
  bufstart=((void*) &_load_addr+(48*1024)); //bufs[0].buf;
  oled_clear();
  oled_print(0,0,"Firmware updater", Font_8x8);
  // move nvic
  move_nvic_table(nvic_table);
  // disable mpu
  MPU->CTRL = 0;
  iap_start();
  nvic_table[83]=(uint32_t) usb_irqhandler;
  while(1) {
    if(state==3) { // successfully verified full image
      oled_clear();
      oled_print(0,0,"Firmware update", Font_8x8);
      oled_print(0,18,"successful \\o/", Font_8x8);
      oled_print(0,36,"pls reboot", Font_8x8);
      while(1); //softreset();
    }
  }
}
