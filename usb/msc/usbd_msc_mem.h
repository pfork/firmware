#ifndef usbd_mem_h
#define usbd_mem_h
#include "usbd_def.h"

#define USBD_STD_INQUIRY_LENGTH		36

typedef struct _USBD_STORAGE {
  char (* Init) (void);
  char (* GetCapacity) (unsigned int *block_num, unsigned int *block_size);
  char (* IsReady) (void);
  char (* IsWriteProtected) (void);
  char (* Read) (unsigned char *buf, unsigned int blk_addr, unsigned int blk_len);
  char (* Write)(unsigned char *buf, unsigned int blk_addr, unsigned int blk_len);
  char (* GetMaxLun)(void);
  char *pInquiry;
}USBD_STORAGE_cb_TypeDef;

extern USBD_STORAGE_cb_TypeDef *USBD_STORAGE_fops;
#endif
