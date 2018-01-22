/**
  ************************************************************************************
  * @file    buttons.h
  * @author  stf
  * @version V0.0.1
  * @date    01-January-2014
  ************************************************************************************
  */

#ifndef buttons_h
#define buttons_h

#ifdef DEVICE_3310
  #include "buttons3310.h"
#else // defined(DEVICE_GH)
  #include "buttons_gh.h"
#endif

void buttons_init(void);
unsigned char buttons_pressed(void);
unsigned char button_handler(void);

#endif
