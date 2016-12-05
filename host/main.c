#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pitchfork.h"

void usage(char **argv, int ret) {
  printf("%s: PITCHFORK front-end\n", argv[0]);
  printf("%s stop ; resets PITCHFORK mode\n", argv[0]);
  printf("%s rng [size] | random [sized] byte-stream\n", argv[0]);
  printf("%s encrypt peer | encrypts to peer using shared keys\n", argv[0]);
  printf("%s decrypt | decrypts using shared keys\n", argv[0]);
  printf("%s ancrypt | encrypts to peer using anonymous keys (prefix plaintext with pubkey)\n", argv[0]);
  printf("%s andecrypt | decrypts from anonymous keys\n", argv[0]);
  printf("%s send peer | sends message to peer using axolotl\n", argv[0]);
  printf("%s recv peer | recv message from peer using axolotl\n", argv[0]);
  printf("%s kex | starts a pq-x3dh\n", argv[0]);
  printf("%s respond peer | responds to a pq-x3dh from peer\n", argv[0]);
  printf("%s end peer | finishes a pq-x3dh from peer\n", argv[0]);
  printf("%s sign | sign message using xeddsa\n", argv[0]);
  printf("%s verify | verify message using xeddsa/blake (prefix msg with signature)\n", argv[0]);
  printf("%s pqsign | sign message using sphincs/blake\n", argv[0]);
  printf("%s pqverify | verify message using sphincs/blake (prefix msg with signature)\n", argv[0]);
  printf("%s list type [peer] | lists keys according to type\n\t[axolotl, sphincs, shared, longterm, prekey, pub], optionally filters only for peer\n", argv[0]);
  printf("%s getpub [sphincs] | returns either longterm, or sphincs pubkey\n", argv[0]);
  exit(ret);
}

int main(int argc, char **argv) {
  if(argc<2) {
    usage(argv, 0);
  }

  libusb_context *ctx = NULL; //a libusb session
  libusb_device_handle *dev_handle;
  int r; //for return values
  if(0!=open_pitchfork(&ctx, &dev_handle)) {
    return -1;
  }

  pf_reset(dev_handle);
  if(memcmp(argv[1],"rng",4)==0) {
    long int size=0;
    if(argc>2) {
      char *ptr;
      size = strtol(argv[2], &ptr, 0);
      if(*ptr!=0) {
        fprintf(stderr,"[!] rng <size> bad :/\nabort\n");
        pf_close(ctx, dev_handle);
        return 1;
      }
    }
    if(size>0) pf_rng(dev_handle, size);
    else while(1) pf_rng(dev_handle, 32768);
  } else if(memcmp(argv[1],"stop",5)==0) {
    pf_stop(dev_handle);
  } else if(memcmp(argv[1],"encrypt",8)==0) {
    if(argc<2) {
      fprintf(stderr,"encrypt needs a recipient name as param :/\nabort\n");
      pf_close(ctx, dev_handle);
      return 1;
    }
    pf_encrypt(dev_handle, argv[2]);
  } else if(memcmp(argv[1],"decrypt",8)==0) {
    pf_decrypt(dev_handle);
  } else if(memcmp(argv[1],"ancrypt",8)==0) {
    pf_encrypt_anon();
  } else if(memcmp(argv[1],"andecrypt",10)==0) {
    pf_decrypt_anon(dev_handle);
  } else if(memcmp(argv[1],"send",5)==0) {
    if(argc<2) {
      fprintf(stderr,"send needs a recipient name as param :/\nabort\n");
      pf_close(ctx, dev_handle);
      return 1;
    }
    pf_ax_send(dev_handle, argv[2]);
  } else if(memcmp(argv[1],"recv",5)==0) {
    if(argc<2) {
      fprintf(stderr,"recv needs a recipient name as param :/\nabort\n");
      pf_close(ctx, dev_handle);
      return 1;
    }
    pf_ax_recv(dev_handle, argv[2]);
  } else if(memcmp(argv[1],"kex",4)==0) {
    pf_kex_start(dev_handle);
  } else if(memcmp(argv[1],"respond",8)==0) {
    if(argc<2) {
      fprintf(stderr,"respond needs a recipient name as param :/\nabort\n");
      pf_close(ctx, dev_handle);
      return 1;
    }
    pf_kex_respond(dev_handle, argv[2]);
  } else if(memcmp(argv[1],"end",4)==0) {
    if(argc<2) {
      fprintf(stderr,"end needs a recipient name as param :/\nabort\n");
      pf_close(ctx, dev_handle);
      return 1;
    }
    pf_kex_end(dev_handle, argv[2]);
  } else if(memcmp(argv[1],"pqsign",7)==0) {
    pf_pqsign(dev_handle);
  } else if(memcmp(argv[1],"pqverify",9)==0) {
    pf_pqverify();
  } else if(memcmp(argv[1],"sign",5)==0) {
    pf_sign(dev_handle);
  } else if(memcmp(argv[1],"verify",7)==0) {
    if(argc<2) {
      fprintf(stderr,"verify needs a signers name as param :/\nabort\n");
      pf_close(ctx, dev_handle);
      return 1;
    }
    pf_verify(dev_handle, argv[2]);
  } else if(memcmp(argv[1],"list",5)==0) {
    if(argc<3) {
      fprintf(stderr,"list needs a type as param :/\nabort\n");
      pf_close(ctx, dev_handle);
      return 1;
    }
    uint8_t *peer=NULL;
    if(argc>3) {
      peer=argv[3];
    }
    if(memcmp(argv[2],"axolotl",8)==0) {
      pf_list(dev_handle, PF_KEY_AXOLOTL, peer);
    } else if(memcmp(argv[2],"sphincs",8)==0) {
      pf_list(dev_handle, PF_KEY_SPHINCS, peer);
    } else if(memcmp(argv[2],"shared",7)==0) {
      pf_list(dev_handle, PF_KEY_SHARED, peer);
    } else if(memcmp(argv[2],"prekey",7)==0) {
      pf_list(dev_handle, PF_KEY_PREKEY, peer);
    } else if(memcmp(argv[2],"pub",4)==0) {
      pf_list(dev_handle, PF_KEY_PUBCURVE, peer);
    } else if(memcmp(argv[2],"longterm",10)==0) {
      // tocheck, fails:
      pf_list(dev_handle, PF_KEY_LONGTERM, peer);
    }
  } else if(memcmp(argv[1],"getpub",7)==0) {
    if(argc>2) {
      if(memcmp(argv[2],"sphincs",8)==0) {
        pf_get_pub(dev_handle, 1);
      } else {
        fprintf(stderr,"getpub only accepts sphincs as param, curve25519 is default\nabort :/\n");
        pf_close(ctx, dev_handle);
        return 1;
      }
    } else pf_get_pub(dev_handle, 0);
  } else {
    usage(argv,1);
  }

  // todo enable, only for debugging
  //return pf_close(ctx, dev_handle);
  return 0;
}
