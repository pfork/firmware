/**
  ************************************************************************************
  * @file    delay.c
  * @author  stf
  * @version V0.0.1
  * @date    05-December-2013
  * @brief   This file provides incredibly naive usec and msec delay functions.
  ************************************************************************************
  */

/**
* @brief  uDelay
*         This function provides delay time in micro sec
* @param  usec : Value of delay required in micro sec
*/
void uDelay (const unsigned int usec) {
  unsigned int count = 0;
  const unsigned int utime = (120 * usec);
  do {
    if ( ++count > utime ) {
      return ;
    } } while (1);
}

/**
* @brief  mDelay
*          This function provides delay time in milli sec
* @param  msec : Value of delay required in milli sec
* @retval None
*/
void mDelay (const unsigned int msec) {
  uDelay(msec * 1000);
}
