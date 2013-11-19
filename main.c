#include "haveged.h"
#include "uart.h"
#include "init.h"
#include "randombytes_salsa20_random.h"
#include "crypto_scalarmult_curve25519.h"
#include "utils.h"

int notmain ( void )
{
  unsigned char e1[crypto_scalarmult_curve25519_BYTES];
  unsigned char e2[crypto_scalarmult_curve25519_BYTES];
  unsigned char pub1[crypto_scalarmult_curve25519_BYTES];
  unsigned char pub2[crypto_scalarmult_curve25519_BYTES];
  unsigned char s1[crypto_scalarmult_curve25519_BYTES];
  unsigned char s2[crypto_scalarmult_curve25519_BYTES];
  char out[100];

  init();

  //temp =(unsigned int) (25 + ((read_temp()-760)*10)/25);
  while(1) {
    haveged_collect();

    // gen e1
    randombytes_salsa20_random_buf((void *) e1, (size_t) crypto_scalarmult_curve25519_BYTES);
    //uart_string(sodium_bin2hex(out,100,e1,(size_t) crypto_scalarmult_curve25519_BYTES));
    //uart_string("\r\n");
    // gen e2
    randombytes_salsa20_random_buf((void *) e2, (size_t) crypto_scalarmult_curve25519_BYTES);
    //uart_string(sodium_bin2hex(out,100,e2,(size_t) crypto_scalarmult_curve25519_BYTES));
    //uart_string("\r\n");
    // calc pub1
    crypto_scalarmult_curve25519_base(pub1, e1);
    //uart_string(sodium_bin2hex(out,100,pub1,(size_t) crypto_scalarmult_curve25519_BYTES));
    //uart_string("\r\n");
    // calc pub2
    crypto_scalarmult_curve25519_base(pub2, e2);
    //uart_string(sodium_bin2hex(out,100,pub2,(size_t) crypto_scalarmult_curve25519_BYTES));
    //uart_string("\r\n");
    // calc s1
    crypto_scalarmult_curve25519(s1, e1, pub2);
    uart_string(sodium_bin2hex(out,100,s1,(size_t) crypto_scalarmult_curve25519_BYTES));
    uart_string("\r\n");
    // calc s2
    crypto_scalarmult_curve25519(s2, e2, pub1);
    uart_string(sodium_bin2hex(out,100,s2,(size_t) crypto_scalarmult_curve25519_BYTES));
    uart_string("\r\n");
    uart_string("-----\n\r");

    //Delay(1000);
  }
  return(0);
}
