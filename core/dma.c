/**
  ************************************************************************************
  * @file    dma.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013 - 03-March-2016
  * @brief   This file provides DMA reset and supplementary functions
  ************************************************************************************
  */
#include "stm32f.h"
#include <stdint.h>

// dma2, stream0, channel1
#define DMACPY_CHANNEL DMA_SxCR_CHSEL_1
#define DMACPY_FLAG_TCIF DMA_LISR_TCIF0
#define DMACPY_STREAM_REGS ((DMA_Stream_Regs *) DMA2_Stream0_BASE)

/**
  * @brief  Deinitialize the DMAy Streamx registers to their default reset values.
  * @param  stream: a pointer to the DMA_Stream_Regs struct representing the stream
  * @retval None
  */
void DMA_DeInit(DMA_Stream_Regs* stream) {
  /* Disable the selected DMA Stream */
  stream->CR &= ~((unsigned int)DMA_SxCR_EN);
  stream->CR  = 0;
  stream->NDTR = 0;
  stream->PAR  = 0;
  stream->M0AR = 0;
  stream->M1AR = 0;
  stream->FCR = (unsigned int)0x00000021;

  // calculate stream number from stream offset
  // -0x10 for dma common dma regs, /0x18 is size of one stream reg
  unsigned int mask = DMA_ISR_MASK(((((unsigned int) stream && 0xff) - 0x10) / 0x18));
  unsigned int dma = (unsigned int) stream & ~((unsigned int) 0xff);

  // deduce upper/lower half of fcr based on stream offset
  if(((unsigned int) stream & 0xff) < 0x70 ) {
    // deduce dma base address from stream
    DMA_LIFCR(dma) = mask;
  } else {
    DMA_HIFCR(dma) = mask;
  }
}

/**
  * @brief  Checks whether the specified DMAy Streamx flag is set or not.
  * @param  stream: a dma_stram_regs struct pointer
  * @param  flag: specifies the flag to check.
  *          This parameter can be one of the following values:
  *            @arg DMA_FLAG_TCIF:  Stream transfer complete flag
  *            @arg DMA_FLAG_HTIF:  Stream half transfer complete flag
  *            @arg DMA_FLAG_TEIF:  Stream transfer error flag
  *            @arg DMA_FLAG_DMEIF: Stream direct mode error flag
  *            @arg DMA_FLAG_FEIF:  Stream FIFO error flag
  * @retval The state of flag
  */
FlagStatus DMA_GetFlagStatus(DMA_Stream_Regs* stream, unsigned int flag) {
  // deduce upper/lower half of fcr based on stream offset
  unsigned char chan = ((((unsigned int) stream && 0xff) - 0x10) / 0x18);
  unsigned int dma = (unsigned int) stream & ~((unsigned int) 0xff);
  unsigned int mask = ((flag << DMA_ISR_OFFSET(chan)) | (unsigned int)DMA_ISR_RESERVED_MASK);
  if(chan < 4) {
    return DMA_LIFCR(dma) & mask;
  } else {
    return DMA_HIFCR(dma) & mask;
  }
}

void dmacpy_init(void) {
  RCC->AHB1ENR |= RCC_AHB1Periph_DMA2;

  irq_disable(NVIC_DMA2_STREAM0_IRQ);
  DMA_Stream_Regs *regs = DMACPY_STREAM_REGS;
  /* DMA disable */
  regs->CR &= ~((unsigned int) DMA_SxCR_EN);
  /* wait till cleared */
  while(regs->CR & ((unsigned int) DMA_SxCR_EN));
  /* DMA deConfig */
  DMA_DeInit(DMACPY_STREAM_REGS);
}

void dmawait(void) {
  while((DMA2_LISR & DMACPY_FLAG_TCIF) == 0);

  DMA2_LIFCR |= DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 |
                DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 |
                DMA_LIFCR_CTCIF0;
}

static void dma(void* dest, const void *buf, unsigned short len, unsigned int cfg) {
  DMA_Stream_Regs *regs = DMACPY_STREAM_REGS;
  /* DMA Config */
  regs->CR = cfg;
  // setup DMA
  /* memory source address */
  regs->PAR = (uint32_t) buf;
  regs->NDTR = len;
  /* dest address */
  regs->M0AR = (uint32_t) dest;
  /* DMA enable */
  regs->CR |= DMA_SxCR_EN;
}

void dmacpy32(void* dest, const void *buf, unsigned short len) {
  dma(dest, buf, len, (DMACPY_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM |
                          DMA_SxCR_MINC | DMA_SxCR_PINC |
                          DMA_SxCR_PSIZE_32BIT | DMA_SxCR_MSIZE_32BIT |
                          //DMA_SxCR_TCIE | DMA_SxCR_HTIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE |/* enable tx complete interrupt */
                          DMA_SxCR_TCIE |
                          DMA_SxCR_PL_VERY_HIGH ));
}

void dmaset32(void* dest, const int val, unsigned short len) {
  dma(dest, &val, len, (DMACPY_CHANNEL | DMA_SxCR_DIR_MEM_TO_MEM |
                        DMA_SxCR_MINC |
                        DMA_SxCR_PSIZE_32BIT | DMA_SxCR_MSIZE_32BIT |
                        DMA_SxCR_TCIE |
                        DMA_SxCR_PL_VERY_HIGH ));
}
