/**
  ******************************************************************************
  * @file    usbd_msc_data.h
  * @author  stf
  * @version V0.0.1
  * @date    06-December-2013
  * @brief   header for the usbd_msc_data.c file
  ******************************************************************************
  */

#ifndef usbd_msc_data_h
#define usbd_msc_data_h

#include "usbd_conf.h"

#define MODE_SENSE6_LEN			 8
#define MODE_SENSE10_LEN		 8
#define LENGTH_INQUIRY_PAGE00		 7
#define LENGTH_FORMAT_CAPACITIES    	20

extern const unsigned char MSC_Page00_Inquiry_Data[];
extern const unsigned char MSC_Mode_Sense6_data[];
extern const unsigned char MSC_Mode_Sense10_data[] ;

#endif /* usbd_msc_data_h */
