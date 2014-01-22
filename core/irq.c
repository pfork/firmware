#include "dual.h"
#include "sd.h"
#include "sdio.h"
#include "systimer.h"
#include "usbd_core.h"
#include "usb_dcd_int.h"

/**
  * @brief  USB IRQ handler
  *         depending on dual_usb_mode it executes either in PITCHFORK
  *         or in MSC mode
  * @param  None
  * @retval None
  */
void OTG_FS_IRQHandler(void) {
  if (dual_usb_mode == CRYPTO) {
    usbd_poll(usbd_dev);
  } else if(dual_usb_mode == DISK) {
    USBD_OTG_ISR_Handler(&USB_OTG_dev);
  }
    usbd_poll(usbd_dev);
}

/**
  * @brief  SDIO IRQ handler
  * @param  None
  * @retval None
  */
void SDIO_IRQHandler(void) {
  /* Process All SDIO Interrupt Sources */
  SD_ProcessIRQSrc();
}
/**
  * @brief  SDIO DMA IRQ handler
  * @param  None
  * @retval None
  */
void DMA2_Stream3_IRQHandler(void) {
  /* Process All SDIO DMA Interrupt Sources */
  SD_ProcessDMAIRQ();
}

/**
  * @brief  IRQ initializer
  *         initializes the SDIO, SDIO DMA and USB IRQs
  * @param  None
  * @retval None
  */
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
/**
  * @brief  SysTick IRQ handler maintains syctr
  * @param  None
  * @retval None
  */
void SysTick_Handler(void) {
  sysctr++;
}

/**
  * @brief  disables all irqs
  * @param  None
  * @retval None
  */
void disable_irqs(void) {
  SYSTICK_CTRL &= ~((unsigned int) 2); /* disable interrupts */
  irq_disable(NVIC_SDIO_IRQn);
  irq_disable(SD_SDIO_DMA_IRQn);
  irq_disable(NVIC_OTG_FS_IRQ);
}

/**
  * @brief  enables all irqs
  * @param  None
  * @retval None
  */
void enable_irqs(void) {
  irq_enable(NVIC_SDIO_IRQn);
  irq_enable(SD_SDIO_DMA_IRQn);
  irq_enable(NVIC_OTG_FS_IRQ);
  // enable systick irq
  SYSTICK_CTRL |= 2; /* Enable interrupts */
}
