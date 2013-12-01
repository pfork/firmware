#include "stm32f.h"

void adc_init ( void ) {
  //enable adc1
  MMIO32(RCC_APB2ENR) |= 1<<8;

  // disable temperature/vref sensor
  ADC_CCR &= ~ADC_CCR_TSVREFE;
  // adc off
  ADC_CR2(ADC1) &= ~ADC_CR2_ADON;
  // scanmode off
  ADC_CR1(ADC1) &= ~ADC_CR1_SCAN;
  // single conversion mode
  ADC_CR2(ADC1) &= ~ADC_CR2_CONT;
  // disable external trigger
  ADC_CR2(ADC1) &= ~ADC_CR2_EXTTRIG;
  // right aligned
  ADC_CR2(ADC1) &= ~ADC_CR2_ALIGN;
  // set sampling time
  ADC_SMPR1(ADC1) |= (ADC_SMPR_SMP_1DOT5CYC << ((ADC_CHANNEL16 - 10) * 3)) |
                     (ADC_SMPR_SMP_1DOT5CYC << ((ADC_CHANNEL17 - 10) * 3));
  ADC_SQR1(ADC1) = 0; // only one entry
}

unsigned short read_chan( unsigned char chan ) {
  unsigned int res = 0x800;

  // set chan
  ADC_SQR3(ADC1) = (chan << (0 * 5));

  while(res==0x800) {
    // start conversion
    ADC_CCR |= ADC_CCR_TSVREFE;
    ADC_CR2(ADC1) |= ADC_CR2_ADON;
    ADC_CR2(ADC1) |= ADC_CR2_SWSTART;

    // wait till end of conversion
    while ((ADC_SR(ADC1) & ADC_SR_EOC) == 0) ;
    res = ADC_DR(ADC1);
    // stop adc
    ADC_CR2(ADC1) &= ~ADC_CR2_SWSTART;
    ADC_CR2(ADC1) &= ~ADC_CR2_ADON;
    ADC_CCR &= ~ADC_CCR_TSVREFE;
  }
  return res;
}

unsigned short read_temp( void ) {
  return read_chan(ADC_CHANNEL16);
}

unsigned short read_vref( void ) {
  return read_chan(ADC_CHANNEL17);
}
