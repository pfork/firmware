#include "dual.h"
#include "sd.h"
#include "sdio.h"
#include "usbd_core.h"
#include "usb_dcd_int.h"

void OTG_FS_IRQHandler(void) {
  if (dual_usb_mode == CRYPTO) {
    usbd_poll(usbd_dev);
  } else if(dual_usb_mode == DISK) {
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

void disable_irqs(void) {
  SYSTICK_CTRL &= ~((unsigned int) 2); /* disable interrupts */
  irq_disable(NVIC_SDIO_IRQn);
  irq_disable(SD_SDIO_DMA_IRQn);
  irq_disable(NVIC_OTG_FS_IRQ);
}

void enable_irqs(void) {
  irq_enable(NVIC_SDIO_IRQn);
  irq_enable(SD_SDIO_DMA_IRQn);
  irq_enable(NVIC_OTG_FS_IRQ);
  // enable systick irq
  SYSTICK_CTRL |= 2; /* Enable interrupts */
}
