/**
  ************************************************************************************
  * @file    dma.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides DMA reset and supplementary functions
  ************************************************************************************
  */
#include "stm32f.h"

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
