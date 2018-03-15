#include <decaf.h>
#include <stdint.h>
#include "stfs.h"
#include "pf_store.h"
#include "usb.h"
#include "randombytes_pitchfork.h"

static int sphinx(const uint8_t *challenge, const uint8_t *secret, uint8_t *result) {
  // deserialize challenge into C
  decaf_255_point_t C, R;
  if(DECAF_SUCCESS!=decaf_255_point_decode(C, challenge, DECAF_FALSE)) return -1;

  // peer contributes their own secret: R=Cy
  decaf_255_scalar_t key;
  decaf_255_scalar_decode_long(key, secret, DECAF_255_SCALAR_BYTES);
  decaf_255_point_scalarmul(R, C, key);

  // serialize R into resp
  decaf_255_point_encode(result, R);

  return 0;
}

int pf_sphinx_respond(const uint8_t *id, const uint8_t *challenge) {
  // open file with key
  uint8_t fname[]="/sphinx/                                ";
  stohex(fname+8, id, 16);
  int fd;
  if((fd=stfs_open(fname, 0))==-1) {
    goto error;
  }
  uint8_t key[DECAF_255_SCALAR_BYTES];
  if(stfs_read(fd,key,DECAF_255_SCALAR_BYTES)!=DECAF_255_SCALAR_BYTES) {
    stfs_close(fd);
    goto error;
  }
  if(stfs_close(fd)==-1) {
    //LOG(1,"[x] failed to close file, err: %d\n", stfs_geterrno());
    goto error;
  }
  unsigned char resp[DECAF_255_SER_BYTES];
  if(sphinx(challenge, key, resp)==-1) {
    goto error;
  }

  // send response back over usb
  usb_write(resp, sizeof(resp), 32,USB_CRYPTO_EP_DATA_OUT);
  return 0;

error:
  usb_write((uint8_t*)"fail", 5, 32,USB_CRYPTO_EP_DATA_OUT);
  return -1;
}

static int new_sphinx_key(uint8_t *fname, const uint8_t *challenge) {
  // open file for key
  int fd;
  if((fd=stfs_open(fname, O_CREAT))==-1) {
    goto error;
  }
  uint8_t key[DECAF_255_SCALAR_BYTES];
  randombytes_buf(key,sizeof(key));
  if(stfs_write(fd,key,DECAF_255_SCALAR_BYTES)!=DECAF_255_SCALAR_BYTES) {
    stfs_close(fd);
    goto error;
  }
  if(stfs_close(fd)==-1) {
    goto error;
  }
  unsigned char resp[DECAF_255_SER_BYTES];
  if(sphinx(challenge, key, resp)==-1) {
    goto error;
  }

  // send response back over usb
  usb_write(resp, sizeof(resp), 32,USB_CRYPTO_EP_DATA_OUT);
  return 0;

error:
  usb_write((uint8_t*)"fail", 5, 32,USB_CRYPTO_EP_DATA_OUT);
  return -1;
}

int pf_sphinx_create(const uint8_t *id, const uint8_t *challenge) {
  // open file with key
  uint8_t fname[]="/sphinx/                                ";
  stohex(fname+8, id, 16);
  return new_sphinx_key(fname, challenge);
}

int pf_sphinx_change(const uint8_t *id, const uint8_t *challenge) {
  // open file with key
  uint8_t fname[]="/sphinx/                                ";
  stohex(fname+8, id, 16);
  fname[39]='~';
  return new_sphinx_key(fname, challenge);
}

int pf_sphinx_commit(const uint8_t *id) {
  // reads new key from file /sphinx/<hex-id-lastdigit-is-tilde>
  // writes new key to /sphinx/<hex-id>
  // unlinks /sphinx/<hex-id-lastdigit-is-tilde>
  // sends back 'ok' if all ok, else 'fail' over usb
  uint8_t fname[]="/sphinx/                                ";
  uint8_t last_digit;
  stohex(fname+8, id, 16);
  // preserve last digit for later
  last_digit=fname[39];
  // last digit is ~ for the new key
  fname[39]='~';

  // read new key
  int fd;
  if((fd=stfs_open(fname, 0))==-1) {
    goto error;
  }
  uint8_t key[DECAF_255_SCALAR_BYTES];
  if(stfs_read(fd,key,DECAF_255_SCALAR_BYTES)!=DECAF_255_SCALAR_BYTES) {
    stfs_close(fd);
    goto error;
  }
  if(stfs_close(fd)==-1) {
    goto error;
  }

  //write new key
  // restore last digit
  fname[39]=last_digit;
  if((fd=stfs_open(fname, 0))==-1) {
    goto error;
  }
  if(stfs_write(fd,key,DECAF_255_SCALAR_BYTES)!=DECAF_255_SCALAR_BYTES) {
    stfs_close(fd);
    goto error; // catastropic? can we recover?
  }
  if(stfs_close(fd)==-1) {
    goto error; // really signal error here? this is a bad situation
  }

  // unlink new key file
  fname[39]='~';
  if(-1==stfs_unlink(fname)) {
    goto error; // todo really signal error here?
  }

  usb_write((uint8_t*)"ok", 3, 32,USB_CRYPTO_EP_DATA_OUT);
  return 0;
error:
  usb_write((uint8_t*)"fail", 5, 32,USB_CRYPTO_EP_DATA_OUT);
  return -1;
}

int pf_sphinx_delete(const uint8_t *id) {
  // open file with key
  uint8_t fname[]="/sphinx/                                ";
  stohex(fname+8, id, 16);
  if(-1==stfs_unlink(fname)) {
    usb_write((uint8_t*)"fail", 5, 32,USB_CRYPTO_EP_DATA_OUT);
    return -1;
  }
  usb_write((uint8_t*)"ok", 3, 32,USB_CRYPTO_EP_DATA_OUT);
  return 0;
}
