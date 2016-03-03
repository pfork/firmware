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

void dmacpy_init(void);
void dmacpy32(void* dest, const void *buf, unsigned short len);
void dmaset32(void* dest, const int val, unsigned short len);
void dmawait(void);

#endif
