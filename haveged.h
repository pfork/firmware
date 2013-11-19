#ifndef haveged_h
#define haveged_h
#define SIZEENTROPY 32 // also set in .c m(

extern unsigned int Entropy[SIZEENTROPY];

void haveged_init( void );
void haveged_collect(void);
#endif
