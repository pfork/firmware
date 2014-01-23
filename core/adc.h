/**
  ************************************************************************************
  * @file    adc.h
  * @author  stf
  * @version V0.0.1
  * @date    01-January-2014
  ************************************************************************************
  */

#ifndef adc_h
#define adc_h
void adc_init ( void );
unsigned short read_temp( void );
unsigned short read_vref( void );
#endif
