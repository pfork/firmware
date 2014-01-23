/**
  ************************************************************************************
  * @file    dma.h
  * @author  stf
  * @version V0.0.1
  * @date    01-January-2014
  ************************************************************************************
  */

#ifndef dma_h
#define dma_h
#include <stm32f.h>

void DMA_DeInit(DMA_Stream_Regs* stream);
FlagStatus DMA_GetFlagStatus(DMA_Stream_Regs* stream, unsigned int flag);
#endif
