/**
  ************************************************************************************
  * @file    buttons.c
  * @author  stf
  * @version V0.0.2
  * @date    15-January-2018
  * @brief   This file provides reading and handling of buttons
  ************************************************************************************
  */

#ifdef DEVICE_3310
  #include "buttons3310.c"
#else // defined(DEVICE_GH)
  #include "buttons_gh.c"
#endif
