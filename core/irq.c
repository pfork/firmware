#include "sd.h"
#include "sdio.h"

void sdio_isr(void) {
  /* Process All SDIO Interrupt Sources */
  SD_ProcessIRQSrc();
}
void dma2_stream3_isr(void) {
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
