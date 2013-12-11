#include <libopencm3/usb/usbd.h>
#include "sd.h"
#include "sdio.h"
#include "usb_core.h"
#include "usbd_core.h"
#include "main.h"

extern USB_OTG_CORE_HANDLE USB_OTG_dev;
extern usbd_device *usbd_dev;
extern unsigned int USBD_OTG_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
extern unsigned int state;

void OTG_FS_IRQHandler(void) {
  if (state == RNG) {
    usbd_poll(usbd_dev);
  } else if(state == DISK) {
    USBD_OTG_ISR_Handler(&USB_OTG_dev);
  }
}

void SDIO_IRQHandler(void) {
  /* Process All SDIO Interrupt Sources */
  SD_ProcessIRQSrc();
}
void DMA2_Stream3_IRQHandler(void) {
  /* Process All SDIO DMA Interrupt Sources */
  SD_ProcessDMAIRQ();
}

void irq_init(void) {
  // enable SDIO IRQ
  NVIC_IPR(NVIC_SDIO_IRQn) = 0;
  irq_enable(NVIC_SDIO_IRQn);

  NVIC_IPR(SD_SDIO_DMA_IRQn) = 0;
  irq_enable(SD_SDIO_DMA_IRQn);

  // enable USB IRQ
  NVIC_IPR(NVIC_OTG_FS_IRQ) = 0;
  irq_enable(NVIC_OTG_FS_IRQ);
}
