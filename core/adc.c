/**
  ************************************************************************************
  * @file    adc.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   this module implements the eris ADC, it misreads to maximize entropy.
  ************************************************************************************
  */

#include "stm32f.h"

/**
* @brief  adc_init
*
*         configures ADC1 for sampling the internal temperature and
*         vref sensors, to achieve maximum entropy it misconfigures
*         the ADC timing
*
* @param  None
* @retval None
*/
void adc_init ( void ) {
  //enable adc1
  MMIO32(RCC_APB2ENR) |= 1<<8;

  // disable temperature/vref sensor
  ADC_CCR &= ~ADC_CCR_TSVREFE;
  // disable vbat sensor
  ADC_CCR &= ~ADC_CCR_VBATEN;
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
                     (ADC_SMPR_SMP_1DOT5CYC << ((ADC_CHANNEL17 - 10) * 3)) |
                     (ADC_SMPR_SMP_1DOT5CYC << ((ADC_CHANNEL18 - 10) * 3));
  // enable EOC irq
  ADC_CR1(ADC1) |= ADC_CR1_EOCIE;
  ADC_SQR1(ADC1) = 0; // only one entry
}

/**
* @brief  read_chan
*
*         reads from given chan abusing it for reading the most
*         inaccurate measurements possible
*
* @param  chan: channel to read
* @retval sampled value
*/
static unsigned short read_chan( unsigned char chan ) {
  unsigned int res = 0x800;

  // set chan
  ADC_SQR3(ADC1) = (chan << (0 * 5));

  while(res==0x800) {
    // start conversion
    if(chan==ADC_CHANNEL16 || chan==ADC_CHANNEL17) ADC_CCR |= ADC_CCR_TSVREFE;
    else if(chan==ADC_CHANNEL18) ADC_CCR |= ADC_CCR_VBATEN;
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

/**
* @brief  read_temp
*
*         read cpu temperature, wrapper for read_chan
*
* @param  None
* @retval sampled CPU temperature
*/
unsigned short read_temp( void ) {
  return read_chan(ADC_CHANNEL16);
}

/**
* @brief  read_vref
*
*         read vref, wrapper for read_chan
*
* @param  None
* @retval sampled CPU internal ref voltage
*/
unsigned short read_vref( void ) {
  return read_chan(ADC_CHANNEL17);
}

/**
* @brief  read_vbat
*
*         read vbat, wrapper for read_chan
*
* @param  None
* @retval sampled VBAT voltage
*/
unsigned short read_vbat( void ) {
  return read_chan(ADC_CHANNEL18);
}
