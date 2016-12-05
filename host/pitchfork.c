#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "pitchfork.h"

#define PQCRYPTO_PUBLICKEYBYTES 1056

int open_pitchfork(libusb_context **ctx, libusb_device_handle **dev_handle) {
	int r; //for return values
	r = libusb_init(ctx); //initialize the library for the session we just declared
	if(r < 0) {
     fprintf(stderr,"Init Error %d\n",r); //there was an error
     return 1;
	}
	libusb_set_debug(*ctx, 3); //set verbosity level to 3, as suggested in the documentation

	*dev_handle = libusb_open_device_with_vid_pid(*ctx, 0x0483, 0x5740);
	if(*dev_handle == NULL) {
     fprintf(stderr,"Cannot open device\n");
      return -1;
   }

   if(libusb_kernel_driver_active(*dev_handle, 0) == 1) { //find out if kernel driver is attached
     fprintf(stderr,"Kernel Driver Active\n");
     if(libusb_detach_kernel_driver(*dev_handle, 0) == 0) //detach it
       fprintf(stderr,"Kernel Driver Detached!\n");
   }
   //claim interface 0 (the first) of device (mine had jsut 1)
   if(libusb_claim_interface(*dev_handle, 0) < 0) {
     fprintf(stderr,"Cannot Claim Interface\n");
     return -1;
   }

   fprintf(stderr,"You're wielding a PITCHFORK\n");

   return 0;
}

int pf_stop(libusb_device_handle *dev_handle) {
  unsigned char pkt[65];
  pkt[0]=PITCHFORK_CMD_STOP;
  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1, &len, 0);
  if(ret != 0 || len != 1) {
    fprintf(stderr,"meh\n");
    return -1;
  }
  //fprintf(stderr,"stopped\n");
  while(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 10)==0) {
    pkt[len]=0;
    fprintf(stderr,"resp: %s\n",pkt);
    if(len<64) break;
  }
  //fprintf(stderr,"end response\n");
  return 0;
}

void pf_flush(libusb_device_handle *dev_handle, int endpoint) {
  //fprintf(stderr,"flushing %d\n", endpoint);
  unsigned char pkt[64];
  int len;
  while(libusb_bulk_transfer(dev_handle, endpoint, pkt, 64, &len, 10)==0) {
    if(len<64) break;
  }
}

void pf_reset(libusb_device_handle *dev_handle) {
  pf_stop(dev_handle);
  pf_flush(dev_handle, USB_CRYPTO_EP_DATA_OUT);
  pf_flush(dev_handle, USB_CRYPTO_EP_CTRL_OUT);
}

int pf_rng(libusb_device_handle *dev_handle, int size) {
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_RNG;
  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1, &len, 0);
  if(ret != 0 || len != 1) {
    fprintf(stderr,"meh\n");
    return -1;
  }
  fprintf(stderr, "rng on\n");
  int read=0;
  uint8_t buf[32768];
  while(read<size) {
    len = 64*((((size-read)>32768?32768:(size-read))/64)+1);
    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, len, &len, 0)==0) {
      fwrite(buf, 1, len, stdout);
      fflush(stdout);
      read+=len;
      if(len%64) break;
    }
  }
  pf_reset(dev_handle);
  fprintf(stderr, "rng off\n");
  return 0;
}

int pf_close(libusb_context *ctx, libusb_device_handle *dev_handle) {
  int ret;
  ret = libusb_release_interface(dev_handle, 0); //release the claimed interface
  if(ret!=0) {
    fprintf(stderr,"Fail Release Interface\n");
    return -1;
  }
  libusb_close(dev_handle);
  libusb_exit(ctx);
}

static int pf_perm(libusb_device_handle *dev_handle, uint8_t tok[2]) {
  unsigned char pkt[64];
  int len;
  fprintf(stderr, "wait for permission to show PITCHFORK\n");
  if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 0)) {
    if(len!=2 || memcmp(pkt,tok, 2)!=0) {
      fprintf(stderr, "op rejected on PITCHFORK '%s'\n", pkt);
      return -1;
    }
  }
  fprintf(stderr, "permission granted\n");
  return 0;
}

int pf_list(libusb_device_handle *dev_handle, uint8_t type, uint8_t *peer) {
  pf_reset(dev_handle);
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_LIST_KEYS;
  pkt[1]=type;
  int len=2, sent;
  if(peer!=NULL) {
    int tmp=strnlen(peer,32);
    memcpy(pkt+2,peer,tmp);
    len+=tmp;
  }
  int ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, len, &sent, 0);
  if(ret != 0 || sent != len) {
    fprintf(stderr,"meh\n");
    return -1;
  }
  fprintf(stderr, "listing keys\n");
  if(0!=pf_perm(dev_handle, "ok")) return -1;
  // alternatingly poll ctrl/data for any response
  uint8_t buf[65536];
  while(0!=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, sizeof(buf), &len, 1)) {
    // try ctrl for errors
    if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 1)) {
      if(memcmp(pkt,"err:", 4)==0) {
        pkt[len]=0;
        fprintf(stderr,"%s\n", pkt);
        return -1;
      }
    }
  }
  do {
    fwrite(buf, 1, len, stdout);
    fflush(stdout);
  } while(len==64 && libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, sizeof(buf), &len, 0)==0);

  uint8_t *ptr = buf;
  while(ptr-buf<len) {
    switch(ptr[0]) {
    case 0: { // known user
      int len=ptr[1];
      if(len>32) {
        fprintf(stderr, "wrong len\n");
        return -1;
      }
      uint8_t name[len+1];
      memcpy(name, ptr+2, len);
      name[len]=0;
      fprintf(stderr, "[u] %s\n", name);
      ptr+=2+len;
      break;
    }
    case 1: { // unknown user
      int len=ptr[1];
      if(len>32) {
        fprintf(stderr, "wrong len\n");
        return -1;
      }
      uint8_t name[len+1];
      memcpy(name, ptr+2, len);
      name[len]=0;
      fprintf(stderr, "[?] %s\n", name);
      ptr+=2+len;
      break;
    }
    case 2: { // correct keyid
      fprintf(stderr, "\t+ ");
      int i;
      for(i=0;i<16;i++) fprintf(stderr, "%02x", ptr[1+i]);
      fprintf(stderr, "\n");
      ptr+=1+16;
      break;
    }
    case 3: { // corrupt keyid
      fprintf(stderr, "\t- ");
      int i;
      for(i=0;i<16;i++) fprintf(stderr, "%02x", ptr[1+i]);
      fprintf(stderr, "\n");
      ptr+=1+16;
      break;
    }
    default: {
      fprintf(stderr, "zomg wrong type: %02x\n", ptr[0]);
      return -1;
    }
    }
  }

  fprintf(stderr, "end listing\n");
  return 0;
}

int pf_encrypt(libusb_device_handle *dev_handle, uint8_t *peer) {
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_ENCRYPT;
  int peerlen=strlen(peer);
  if(peerlen>32 || peerlen<1) return -1;
  memcpy(pkt+1,peer,peerlen);

  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1+peerlen, &len, 0);
  if(ret != 0 || len != 1+peerlen) {
    fprintf(stderr,"write meh\n");
    return -1;
  }
  if(0!=pf_perm(dev_handle, "ok")) return -1;

  // get ekid
  if(0!=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 0) || len!=EKID_SIZE) {
    fprintf(stderr,"[?] ekid oops?\n");
    return -1;
  }
  //fprintf(stderr,"got ekid\n");
  // dump it
  //fprintf(stderr, "wrote %d bytes\n", (int) fwrite(pkt, 1, EKID_SIZE, stdout));
  fwrite(pkt, 1, EKID_SIZE, stdout);
  fflush(stdout);

  // get nonce
  if(0!=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, pkt, 64, &len, 0) || len!=24) {
    fprintf(stderr,"[?] nonce oops?\n");
    return -1;
  }
  //fprintf(stderr,"got nonce\n");
  // dump it
  //fprintf(stderr, "wrote %d bytes\n", (int) fwrite(pkt, 1, 24, stdout));
  fwrite(pkt, 1, 24, stdout);
  fflush(stdout);

  // start sending stdin
  uint8_t buf[32768+16];
  int size;
  while(!feof(stdin)) {
    size=fread(buf, 1, 32768, stdin);

    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, size, &len, 0)!=0 && len!=size) {
      fprintf(stderr,"[!] bad write\n");
      return -1;
    }
    if(size<32768 && !(size&0x3f)) { // todo test without this, is this needed?
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0
    }

    // read response
    int ret;
    if((ret=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, size+16, &len, 0))!=0 || len!=size+16) {
      fprintf(stderr,"[!] bad read %d %d\n", len, size+16, ret);
      return -1;
    }
    fwrite(buf, 1, len, stdout);
    fflush(stdout);
  }
  if(size==32768)
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0

  pf_reset(dev_handle);
  fprintf(stderr, "pitchfork off\n");
  return 0;
}

int pf_decrypt(libusb_device_handle *dev_handle) {
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_DECRYPT;
  if(fread(pkt+1,EKID_SIZE,1,stdin)!=1) {
    fprintf(stderr,"read ekid meh\n");
    return -1;
  }
  if(fread(pkt+1+EKID_SIZE,24,1,stdin)!=1) {
    fprintf(stderr,"read nonce meh\n");
    return -1;
  }

  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1+24+EKID_SIZE, &len, 0);
  if(ret != 0 || len != 1+24+EKID_SIZE) {
    fprintf(stderr,"write meh\n");
    return -1;
  }
  if(0!=pf_perm(dev_handle, "ok")) return -1;

  // wait for 2nd go
  if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 0)) {
    if(len!=2 || memcmp(pkt,"go", 2)!=0) {
      fprintf(stderr, "op rejected on PITCHFORK '%s'\n", pkt);
      return -1;
    }
  }
  // start sending stdin
  uint8_t buf[32768+16];
  int size;
  while(!feof(stdin)) {
    size=fread(buf, 1, 32768+16, stdin);

    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, size, &len, 10)!=0 && len!=size) {
      fprintf(stderr,"[!] bad write\n");
      return -1;
    }
    fprintf(stderr,"sent %d and wrote %d bytes\n", size, len);

    if(size<32768+16 && !(size&0x3f)) { // todo test without this, is this needed?
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0
    }

    // read response
    int ret;
    if((ret=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, size-16, &len, 0))!=0 || len!=size-16) {
      fprintf(stderr,"[!] bad read %d %d %d\n", len, size-16, ret);
      return -1;
    }
    fwrite(buf, 1, size-16, stdout);
    fflush(stdout);
  }
  if(size==32768+16)
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0

  pf_reset(dev_handle);
  fprintf(stderr, "pitchfork off\n");
  return 0;
}

int pf_ax_send(libusb_device_handle *dev_handle, uint8_t *peer) {
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_AX_SEND;
  int peerlen=strlen(peer);
  if(peerlen>32 || peerlen<1) return -1;
  memcpy(pkt+1,peer,peerlen);

  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1+peerlen, &len, 0);
  if(ret != 0 || len != 1+peerlen) {
    fprintf(stderr,"write meh\n");
    return -1;
  }
  if(0!=pf_perm(dev_handle, "ok")) return -1;

  uint8_t hbuf[88];
  // get hnonce+header
  while(1) {
    if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 1)) {
      fprintf(stderr, "%s\n", pkt);
      return -1;
    }
    if((ret=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, hbuf, 88, &len, 1))==0) {
      if(len==88) break;
      fprintf(stderr,"unexpected %d bytes, %s\n", len, pkt);
    }
  }
  //fprintf(stderr,"got hnonce+header\n");
  // dump it
  //fprintf(stderr, "wrote %d bytes\n", (int) fwrite(hbuf, 1, 88, stdout));
  fwrite(hbuf, 1, 88, stdout);
  fflush(stdout);

  // get nonce
  while(1) {
    if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 1)) {
      fprintf(stderr, "%s\n", pkt);
      return -1;
    }
    if((ret=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, pkt, 64, &len, 1))==0) {
      if(len==24) break;
      fprintf(stderr,"unexpected %d bytes, %s\n", len, pkt);
    }
  }
  //fprintf(stderr,"got nonce\n");
  // dump it
  //fprintf(stderr, "wrote %d bytes\n", (int) fwrite(pkt, 1, 24, stdout));
  fwrite(pkt, 1, 24, stdout);
  fflush(stdout);

  // start sending stdin
  uint8_t buf[32768+16];
  int size;
  while(!feof(stdin)) {
    size=fread(buf, 1, 32768, stdin);

    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, size, &len, 0)!=0 && len!=size) {
      fprintf(stderr,"[!] bad write\n");
      return -1;
    }
    if(size<32768 && !(size&0x3f)) { // todo test without this, is this needed?
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0
    }

    // read response
    int ret;
    if((ret=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, size+16, &len, 0))!=0 || len!=size+16) {
      fprintf(stderr,"[!] bad read %d %d %d\n", len, size+16, ret);
      return -1;
    }
    fwrite(buf, 1, len, stdout);
    fflush(stdout);
  }
  if(size==32768)
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0

  pf_reset(dev_handle);
  fprintf(stderr, "pitchfork off\n");
  return 0;
}

int pf_ax_recv(libusb_device_handle *dev_handle, uint8_t *peer) {
  uint8_t buf[32768+64];
  buf[0]=PITCHFORK_CMD_AX_RECEIVE;
  if(fread(buf+1,24+64+24,1,stdin)!=1) {
    fprintf(stderr,"read hnonce,headers,mnonce meh\n");
    return -1;
  }
  int peerlen=strlen(peer);
  if(peerlen>32 || peerlen<1) return -1;
  memcpy(buf+1+24+64+24,peer,peerlen);

  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, buf, 1+24+64+24+peerlen, &len, 0);
  if(ret != 0 || len != 1+24+64+24+peerlen) {
    fprintf(stderr,"write meh\n");
    return -1;
  }
  if(0!=pf_perm(dev_handle, "ok")) return -1;

  // wait for 2nd go
  if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, buf, 64, &len, 0)) {
    if(len!=2 || memcmp(buf,"go", 2)!=0) {
      fprintf(stderr, "op rejected on PITCHFORK '%s'\n", buf);
      return -1;
    }
  }
  // start sending stdin
  int first=1;
  int size;
  while(!feof(stdin)) {
    size=fread(buf, 1, 32768+16, stdin);

    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, size, &len, 0)!=0 || len!=size) {
      fprintf(stderr,"[!] bad write\n");
      return -1;
    }
    //fprintf(stderr,"sent %d and wrote %d bytes\n", size, len);

    if(size<32768+16 && !(size&0x3f)) { // todo test without this, is this needed?
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0
    }

    if(first) { // only do this in the first block
      first=0;
      // wait for 3rd go
      if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, buf, 64, &len, 0)) {
        if(len!=2 || memcmp(buf,"tx", 2)!=0) {
          fprintf(stderr, "%s\n", buf);
          return -1;
        }
      }
    }
    // read response
    int ret;
    if(0!=(ret=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, 64*(((size-16)/64)+1), &len, 0)) || len!=size-16) {
      buf[len]=0;
      fprintf(stderr, "[#] len: %d, size: %d - %s\n%s\n", (int) len, size-16, libusb_strerror(ret), buf);
      return -1;
    }
    fwrite(buf, 1, size-16, stdout);
    fflush(stdout);
  }
  if(size==32768+16)
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0

  pf_reset(dev_handle);
  fprintf(stderr, "pitchfork off\n");
  return 0;
}

int pf_kex_start(libusb_device_handle *dev_handle) {
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_KEX_START;
  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1, &len, 0);
  if(ret != 0 || len != 1) {
    fprintf(stderr,"meh\n");
    return -1;
  }
  if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 0)) {
    if(len!=2 || memcmp(pkt,"tx", 2)!=0) {
      fprintf(stderr, "%s\n", pkt);
      return -1;
    }
  }
  fprintf(stderr, "processing kex start\n");
  uint8_t buf[((2208/64)+1)*64];
  if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, sizeof(buf), &len, 0)!=0 || len!=2208) {
    fprintf(stderr, "[x] failed to receive prekey\n");
  }
  fwrite(buf, 1, len, stdout);
  fflush(stdout);
  pf_reset(dev_handle);
  fprintf(stderr, "kex start done\n");
  return 0;
}

int pf_kex_respond(libusb_device_handle *dev_handle, uint8_t *peer) {
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_KEX_RESPOND;
  int plen=strnlen(peer,32);
  memcpy(pkt+1,peer,plen);

  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1+plen, &len, 0);
  if(ret != 0 || len != 1+plen) {
    fprintf(stderr,"meh\n");
    return -1;
  }

  if(0!=pf_perm(dev_handle, "ok")) return -1;
  fprintf(stderr, "processing kex response\n");

  uint8_t buf[2208];
  if(1!=fread(buf, 2208, 1, stdin)) {
    fprintf(stderr, "[x] bad prekey\n");
    return -1;
  }

  // wait for go
  pf_perm(dev_handle, "go");
  //if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 0)) {
  //  if(len!=2 || memcmp(pkt,"go", 2)!=0) {
  //    fprintf(stderr, "%s\n", pkt);
  //    return -1;
  //  }
  //}

  if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 2208, &len, 0)!=0 || len!=2208) {
    fprintf(stderr,"[!] bad write\n");
    return -1;
  }
  fprintf(stderr,"wrote %d bytes\n", len);

  //uint8_t resp[(((2208+16)/64)+1)*64];
  uint8_t resp[65336];
  if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, resp, 65536, &len, 0)!=0 || len!=2208+16) {
    fprintf(stderr, "[x] wrong response: %d %d\n", len, 2208+16);
    return -1;
  }
  fwrite(resp, 1, len, stdout);
  fflush(stdout);
  pf_reset(dev_handle);
  fprintf(stderr, "kex start done\n");
  return 0;
}

int pf_kex_end(libusb_device_handle *dev_handle, uint8_t *peer) {
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_KEX_END;
  int plen=strnlen(peer,32);
  memcpy(pkt+1,peer,plen);

  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1+plen, &len, 0);
  if(ret != 0 || len != 1+plen) {
    fprintf(stderr,"meh\n");
    return -1;
  }

  if(0!=pf_perm(dev_handle, "ok")) return -1;
  fprintf(stderr, "processing kex end\n");

  uint8_t buf[2208+16];
  if(1!=fread(buf, 2208+16, 1, stdin)) {
    fprintf(stderr, "[x] bad prekey\n");
    return -1;
  }

  // wait for go
  pf_perm(dev_handle, "go");
  //if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 0)) {
  //  if(len!=2 || memcmp(pkt,"go", 2)!=0) {
  //    fprintf(stderr, "%s\n", pkt);
  //    return -1;
  //  }
  //}

  if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, sizeof(buf), &len, 0)!=0 || len!=sizeof(buf)) {
    fprintf(stderr,"[!] bad write\n");
    return -1;
  }
  fprintf(stderr,"wrote %d bytes\n", len);

  // wait for ok
  if(0==libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_OUT, pkt, 64, &len, 0)) {
    if(len!=2 || memcmp(pkt,"OK", 2)!=0) {
      fprintf(stderr, "%s\n", pkt);
      return -1;
    }
  }

  pf_reset(dev_handle);
  fprintf(stderr, "kex end done\n");
  return 0;
}

int pf_pqsign(libusb_device_handle *dev_handle) {
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_PQSIGN;

  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1, &len, 0);
  if(ret != 0 || len != 1) {
    fprintf(stderr,"write meh\n");
    return -1;
  }
  pf_perm(dev_handle, "ok");

  pf_perm(dev_handle, "go");
  fprintf(stderr, "pq magic takes its time...\n");
  // start sending stdin
  uint8_t buf[65536];
  int size;
  while(!feof(stdin)) {
    size=fread(buf, 1, 32768, stdin);

    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, size, &len, 10)!=0 && len!=size) {
      fprintf(stderr,"[!] bad write\n");
      return -1;
    }
    if(size<32768 && !(size&0x3f)) { // todo test without this, is this needed?
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0
    }

  }
  // read response
  if((ret=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, sizeof(buf), &len, 0))!=0 || len!=41000) {
    fprintf(stderr,"[!] bad read %d %d\n", len, size+16, ret);
    return -1;
  }
  fwrite(buf, 1, len, stdout);
  fflush(stdout);
  if(size==32768)
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0

  pf_reset(dev_handle);
  fprintf(stderr, "pitchfork off\n");
  return 0;
}

int pf_sign(libusb_device_handle *dev_handle) {
  unsigned char pkt[128];
  pkt[0]=PITCHFORK_CMD_SIGN;

  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1, &len, 0);
  if(ret != 0 || len != 1) {
    fprintf(stderr,"write meh\n");
    return -1;
  }
  if(0!=pf_perm(dev_handle, "ok")) return -1;
  // start sending stdin
  uint8_t buf[65536];
  int size;
  usleep(1000000);
  while(!feof(stdin)) {
    size=fread(buf, 1, 32768, stdin);

    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, size, &len, 0)!=0 && len!=size) {
      fprintf(stderr,"[!] bad write\n");
      return -1;
    }
    if(size<32768 && !(size&0x3f)) { // todo test without this, is this needed?
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0
    }

  }
  fprintf(stderr, "sent\n");
  // read signature
  if((ret=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, pkt, sizeof(pkt), &len, 0))!=0 || len!=64) {
    fprintf(stderr,"[!] bad read %d %d\n", len, ret);
    return -1;
  }
  // echo sig
  fwrite(pkt, 64, 1, stdout);
  fflush(stdout);

  pf_reset(dev_handle);
  fprintf(stderr, "pitchfork off\n");
  return ret;
}

int pf_verify(libusb_device_handle *dev_handle, uint8_t *peer) {
  unsigned char pkt[128];
  pkt[0]=PITCHFORK_CMD_VERIFY;
  if(1!=fread(pkt+1, 64,1, stdin)) {
    fprintf(stderr,"read sig meh\n");
    return -1;
  }
  int plen=strnlen(peer,32);
  memcpy(pkt+64+1,peer,plen);

  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1+64+plen, &len, 0);
  if(ret != 0 || len != 1+64+plen) {
    fprintf(stderr,"write meh\n");
    return -1;
  }
  if(0!=pf_perm(dev_handle, "ok")) return -1;
  // start sending stdin

  uint8_t buf[65536];
  usleep(1000000);
  int size;
  while(!feof(stdin)) {
    size=fread(buf, 1, 32768, stdin);

    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, size, &len, 0)!=0 && len!=size) {
      fprintf(stderr,"[!] bad write\n");
      return -1;
    }
    if(size<32768 && !(size&0x3f)) { // todo test without this, is this needed?
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0
    }
  }
  // read response
  if((ret=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, pkt, sizeof(pkt), &len, 0))!=0 || len!=1) {
    fprintf(stderr,"[!] bad read %d %d\n", len, size+16, ret);
    return -1;
  }
  switch(pkt[0]) {
  case '0': {
    fprintf(stderr,"msg invalid\n");
    ret= 1;
    break;
  }
  case '1': {
    fprintf(stderr,"msg valid\n");
    ret= 0;
    break;
  }
  default: {
    fprintf(stderr,"invalid answer %d\n", pkt[0]);
    ret=1;
  }
  }

  pf_reset(dev_handle);
  fprintf(stderr, "pitchfork off\n");
  return ret;
}

int pf_decrypt_anon(libusb_device_handle *dev_handle) {
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_DECRYPT_ANON;
  if(fread(pkt+1,24,1,stdin)!=1) {
    fprintf(stderr,"read nonce meh\n");
    return -1;
  }
  if(fread(pkt+1+24,32,1,stdin)!=1) {
    fprintf(stderr,"read ephpub meh\n");
    return -1;
  }

  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 1+24+32, &len, 0);
  if(ret != 0 || len != 1+24+32) {
    fprintf(stderr,"write meh\n");
    return -1;
  }
  if(0!=pf_perm(dev_handle, "ok")) return -1;

  // start sending stdin
  uint8_t buf[32768+16];
  int size;
  while(!feof(stdin)) {
    size=fread(buf, 1, 32768+16, stdin);

    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, size, &len, 0)!=0 && len!=size) {
      fprintf(stderr,"[!] bad write\n");
      return -1;
    }
    fprintf(stderr,"sent %d and wrote %d bytes\n", size, len);

    if(size<32768+16 && !(size&0x3f)) { // todo test without this, is this needed?
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0
    }

    // read response
    int ret;
    if((ret=libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, size-16, &len, 0))!=0 || len!=size-16) {
      fprintf(stderr,"[!] bad read %d %d %d\n", len, size-16, ret);
      return -1;
    }
    fwrite(buf, 1, size-16, stdout);
    fflush(stdout);
  }
  if(size==32768+16)
      libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_IN, buf, 0, &len, 0); // zlp for payload%64==0

  pf_reset(dev_handle);
  fprintf(stderr, "pitchfork off\n");
  return 0;
}

#include <sodium/crypto_scalarmult.h>
#include <sodium/crypto_secretbox.h>
#include <sodium/randombytes.h>

static void incnonce(uint8_t *nonce) {
  int i;
  uint32_t *ptr = (uint32_t *) nonce;
  for(i=crypto_secretbox_NONCEBYTES/4-1;i>=0;i--) {
    ptr[i]++;
    if(ptr[i]!=0) break;
  }
}

int pf_encrypt_anon(void) {
  uint8_t opk[32];
  if(1!=fread(opk,32,1,stdin)) {
    fprintf(stderr, "short input :/\nabort\n");
    return 1;
  }
  uint8_t nonce[crypto_secretbox_NONCEBYTES];
  randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
  fwrite(nonce, sizeof(nonce), 1, stdout);
  fflush(stdout);

  uint8_t sk[32];
  randombytes_buf(sk, 32);
  uint8_t mpk[32];
  if(0!=crypto_scalarmult_curve25519_base(mpk, sk)) {
    fprintf(stderr, "bad eph key\nabort\n");
    return 1;
  };
  fwrite(mpk, sizeof(mpk), 1, stdout);
  fflush(stdout);
  uint8_t key[32];
  if(0!=crypto_scalarmult_curve25519(key, sk, opk)) {
    fprintf(stderr, "bad pub key\nabort\n");
    return 1;
  }
  memset(sk,0,sizeof(sk));
  uint8_t paddedplain[32+32768],
    *plain=paddedplain+32,
    paddedcipher[32+32768],
    *cipher=paddedcipher+16;
  int size;
  while(!feof(stdin)) {
    size=fread(plain, 1, 32768, stdin);
    crypto_secretbox(paddedcipher, paddedplain, size+32, nonce, key);
    if(1!=fwrite(cipher, size+16, 1, stdout)) {
      fprintf(stderr,"[x] dang couldn't write %d bytes to stdout\nabort\n", size+16);
      return 1;
    }
    fflush(stdout);
    incnonce(nonce);
  }
  memset(key,0,sizeof(key));

  return 0;
}

int pf_get_pub(libusb_device_handle *dev_handle, int type) {
  unsigned char pkt[64];
  pkt[0]=PITCHFORK_CMD_DUMP_PUB;
  pkt[1]=type;
  int len, ret = libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_CTRL_IN, pkt, 2, &len, 0);
  if(ret != 0 || len != 2) {
    fprintf(stderr,"meh\n");
    return -1;
  }
  if(0!=pf_perm(dev_handle, "ok")) return -1;
  fprintf(stderr,"type: %d\n",type);
  if(type==1) {
    uint8_t buf[((PQCRYPTO_PUBLICKEYBYTES/64)+1)*64];
    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, sizeof(buf), &len, 0)!=0 || len!=PQCRYPTO_PUBLICKEYBYTES) {
      fprintf(stderr, "[x] failed to receive sphincs pubkey\n");
      return -1;
    }
    fwrite(buf, 1, len, stdout);
  } else {
    uint8_t buf[64];
    if(libusb_bulk_transfer(dev_handle, USB_CRYPTO_EP_DATA_OUT, buf, sizeof(buf), &len, 0)!=0 || len!=32) {
      fprintf(stderr, "[x] failed to receive curve25519 pubkey\n");
      return -1;
    }
    fwrite(buf, 1, len, stdout);
  }
  fflush(stdout);
  pf_reset(dev_handle);
  fprintf(stderr, "kex start done\n");
  return 0;
}

#include "sphincs/sphincs256.h"
#include "sodium/crypto_generichash.h"

// todo change chacha12 into chacha20
int pf_pqverify(void) {
  uint8_t sig[41000];
  if(1!=fread(sig, 41000, 1, stdin)) {
    fprintf(stderr,"[x] fail sig read\nabort\n");
    return -1;
  }
  crypto_generichash_state hash_state;
  crypto_generichash_init(&hash_state, NULL, 0, 32);
  uint8_t buf[32768];
  int size;
  while(!feof(stdin)) {
    size=fread(buf,1,sizeof(buf), stdin);
    crypto_generichash_update(&hash_state, buf, size);
  }
  uint8_t hash[32];
  crypto_generichash_final(&hash_state, hash, 32);
  if(crypto_sign_open(hash,32,sig)==0) {
    fprintf(stderr,"ok\n");
  } else {
    fprintf(stderr,"invalid\n");
  }
}
