#include "stm32f.h"

volatile unsigned short adcval[2] = {0,0};

void adc_init ( void )
{
    unsigned int ra;

    // Enable DMA2
    ra=GET32(RCC_AHB1ENR);
    ra |= 1<<22;
    PUT32(RCC_AHB1ENR,ra);
    //enable ADC1
    ra=GET32(RCC_APB2ENR);
    ra |= 1<<8;
    PUT32(RCC_APB2ENR,ra);

    // setup DMA2 for ADC1
    DMA2_LIFCR = DMA_Stream0_IT_MASK; // reset interrupt flags
    DMA2_S0PAR = &(ADC1_DR); // peripheral (source) address
    DMA2_S0M0AR = adcval; // memory (desination) address
    DMA2_S0NDTR = 2; // 2 transfers
    DMA2_S0CR = DMA_SxCR_CIRC | DMA_SxCR_MINC | DMA_SxCR_PSIZE_16BIT | DMA_SxCR_MSIZE_16BIT;
    // enable dma2 stream 0
    DMA2_S0CR |= DMA_SxCR_EN;

    // enable temperature sensor
    ADC_CCR = ADC_CCR_TSVREFE;

    ADC_CR2(ADC1) = ADC_CR2_CONT;
    ADC_CR1(ADC1) = ADC_CR1_SCAN;

    // set sampling time
    ADC_SMPR1(ADC1) = (ADC_SMPR_SMP_239DOT5CYC << ((ADC_CHANNEL16 - 10) * 3)) |
                      (ADC_SMPR_SMP_239DOT5CYC << ((ADC_CHANNEL17 - 10) * 3));
    // set queue
    ADC_SQR1(ADC1) = 1 << ADC_SQR1_L_LSB; // 2 entries
    ADC_SQR3(ADC1) = (ADC_CHANNEL16 << (0 * 5)) | (ADC_CHANNEL17 << (1 * 5));

    ADC_CR2(ADC1) |= ADC_CR2_DDS;
    ADC_CR2(ADC1) |= ADC_CR2_DMA;
    ADC_CR2(ADC1) |= ADC_CR2_ADON; // Turn on conversion
    ADC_CR2(ADC1) |= ADC_CR2_SWSTART;
}

unsigned short read_temp( void )
{
  return adcval[0];
}

unsigned short read_volt( void )
{
  return adcval[1];
}
