#include "cdcacm.h"
#include "init.h"
#include "uart.h"
#include "main.h"
#include "systimer.h"
#include "sd.h"
#include "led.h"
#include "keys.h"

typedef struct {
    unsigned char first_byte;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char end_chs[3];
    unsigned long start_sector;
    unsigned long length_sectors;
} __attribute((packed)) PartitionTable;

unsigned int state = OFF;

void time(void) {
  cdc_string("\n\rMode: time ");
  cdc_hexstring(sysctr, 0);
}

void rng(void) {
  cdc_string("\n\rMode: rng");
}

void disk(void) {
  SD_CardInfo cardinfo;
  unsigned int sector_buf[512/sizeof(unsigned int)];
  int i;
  for(i=0;i<(512/sizeof(unsigned int));i++) sector_buf[i]=0;

  if(SD_GetCardInfo(&cardinfo) != SD_OK) {
    cdc_string("\n\rerr: getstatus error");
  } else {
    cdc_string("\n\r sd size: ");
    cdc_hexstring(cardinfo.CardCapacity / cardinfo.CardBlockSize,0);
    cdc_string("\n\r sd type: ");
    if (SDIO_STD_CAPACITY_SD_CARD_V1_1 == cardinfo.CardType)
      cdc_string("v1.1");
    if (SDIO_STD_CAPACITY_SD_CARD_V2_0 == cardinfo.CardType)
      cdc_string("v2.0");
    if (SDIO_HIGH_CAPACITY_SD_CARD == cardinfo.CardType)
      cdc_string("SDHC");

    // read partition table
    if(SD_ReadMultiBlocks((unsigned char*) sector_buf, 0x0, 512, 1) != SD_OK) {
      cdc_string("\n\rerr: readblock error");
    } else {
      SD_WaitReadOperation();
      while(SD_GetStatus() != SD_TRANSFER_OK);

      //PartitionTable *pt = (void*) &sector_buf[0x1BE];
      PartitionTable *pt = ((PartitionTable*) &sector_buf[0x1BE/4]);
      for(i=0; i<4; i++) {
        cdc_string("\n\rpartition type: ");
        cdc_hexstring(pt[i].partition_type,0);
        cdc_string("\n\rstart sector: ");
        cdc_hexstring(pt[i].start_sector,0);
        cdc_string("\n\rblocks: ");
        cdc_hexstring(pt[i].length_sectors,0);
      }
    }
  }
}

int main ( void ) {
  unsigned char kmask;
  init();

  toggle_status1_led;
  toggle_write_led;
  while(1) {
    switch(state) {
    case RNG: { rng(); break; }
    case TIME: { time(); break; }
    case DISK: { disk(); break; }
    }
    state = OFF;
    kmask = key_handler();
    if(kmask!=0) cdc_hexstring(kmask,1);
    toggle_status1_led;
    toggle_status2_led;
    toggle_write_led;
    toggle_read_led;
    Delay(25);
  }
  return(0);
}
