/**
  ************************************************************************************
  * @file    master.h
  * @author  stf
  * @version V0.0.1
  * @date    01-January-2014
  ************************************************************************************
  */

#ifndef master_h
#define master_h
extern uint8_t pitchfork_hot;
unsigned char* get_master_key(char* msg);
void erase_master_key(void);
void expire_master_key(void);
#endif // master_h
