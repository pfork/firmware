/**
  ******************************************************************************
  * @file    sd.c
  * @date    04-December-2013
  * @brief   This file provides a set of functions needed to manage the SDIO SD
  ******************************************************************************
  */

#include "usbd_msc_data.h"

/* USB Mass storage Page 0 Inquiry Data */
const unsigned char  MSC_Page00_Inquiry_Data[] = {//7
	0x00,
	0x00,
	0x00,
	(LENGTH_INQUIRY_PAGE00 - 4),
	0x00,
	0x80,
	0x83
};
/* USB Mass storage sense 6  Data */
const unsigned char  MSC_Mode_Sense6_data[] = {
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00
};
/* USB Mass storage sense 10  Data */
const unsigned char  MSC_Mode_Sense10_data[] = {
	0x00,
	0x06,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00
};
