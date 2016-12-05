#include "usb.h"
#include "pf_store.h"
#include "crypto_secretbox.h"
#include "randombytes_pitchfork.h"
#include <crypto_generichash.h>
#include <string.h>
#include "master.h"
#include "pitchfork.h"
#include "stfs.h"
#include "axolotl.h"
#include "xeddsa_keygen.h"
#include <utils.h>

static int hexreduce(int b) {
  if(b>='a') return b-'a'+10;
  if(b>='A') return b-'A'+10;
  return b - '0';
}

int unhex(uint8_t *out, const uint8_t *hex, const int hexlen) {
  int i, h,l;
  for(i=0;i<hexlen;i+=2) {
    h=hexreduce(hex[i]);
    l=hexreduce(hex[i+1]);
    if(h>15 || h<0 || l>15 || l<0) return -1; //invalid hexdigit
    out[i/2] = (h<<4) | l;
  }
  return 0;
}

void stohex(uint8_t* d, const uint8_t *s, const uint32_t len) {
  if(d==NULL) return;
  if(s==NULL) return;

  uint32_t i, rc;
  for(i=0;i<len;i++) {
    rc=((*s)>>4)&0xF;
    if(rc>9) rc+=87; else rc+=0x30;
    *(d++)=rc;
    rc=(*s++)&0xF;
    if(rc>9) rc+=87; else rc+=0x30;
    *(d++)=rc;
  }
  *d=0;
}

int topeerid(uint8_t *peerid, const uint8_t *peer, const int len) {
  uint8_t userbuf[sizeof(UserRecord)+PEER_NAME_MAX];
  UserRecord *userdata=(UserRecord*) userbuf;
  if(get_user(userdata)==-1) return -1;

  //topeerid(peer, len, (unsigned char*) peerid);
  crypto_generichash(peerid, STORAGE_ID_LEN,              // output
                     peer, len,                           // input
                     userdata->salt,                      // salt
                     USER_SALT_LEN);                      // salt len
  return 0;
}

/**
  * @brief  combine: concats two strings
  * @param  a: pointer to 1st buffer
  * @param  alen: length of 1st buffer
  * @param  b: pointer to 2nd buffer
  * @param  blen: length of 2nd buffer
  * @param  dst: pointer to output buffer
  * @param  dst_len: length of output buffer
  * @retval length of result
  */
static unsigned int combine(unsigned char* a, unsigned char alen, unsigned char* b, unsigned char blen, unsigned char* dst, unsigned char dst_len) {
  if(alen+blen+2>dst_len)
    return 0;
  memcpy(dst, a, alen);
  dst[alen]=0;
  memcpy(dst+alen+1, b, blen);
  dst[alen+blen+1]=0;
  return alen+blen+2;
}

// closes fd
// plain needs to be crypto_secretbox_ZEROBYTES padded, len not!
// uses stack to store file! use only on small files
int cwrite(int fd, uint8_t *plain, uint32_t len, uint8_t clear) {
  // nonce for encryption, for efficiency stored at beginning of output buffer
  uint8_t out[crypto_secretbox_NONCEBYTES+len+crypto_secretbox_MACBYTES];
  // todo factor ut out or outtmp, write directly to file
  randombytes_buf((void *) out, crypto_secretbox_NONCEBYTES);
  // padded output buffer
  uint8_t outtmp[crypto_secretbox_ZEROBYTES+len];
  // zeroed out
  memset(plain,0, crypto_secretbox_ZEROBYTES);
  crypto_secretbox(outtmp,                         // ciphertext output
                   (uint8_t*) plain,               // plaintext input
                   len+crypto_secretbox_ZEROBYTES, // plain length
                   out,                            // nonce
                   get_master_key("store key"));   // key

  // clear plaintext seed in RAM
  if(clear) memset(plain,0, len+crypto_secretbox_ZEROBYTES);
  memcpy(out+crypto_secretbox_NONCEBYTES, outtmp+crypto_secretbox_BOXZEROBYTES, len+crypto_secretbox_MACBYTES);
  int ret;
  if((ret=stfs_write(fd, out, crypto_secretbox_NONCEBYTES+len+crypto_secretbox_MACBYTES)) !=
                              crypto_secretbox_NONCEBYTES+len+crypto_secretbox_MACBYTES) {
    stfs_close(fd);
    //todo stfs_unlink(file);
    return -1;
  }
  if(stfs_close(fd)==-1) {
    //LOG(1,"[x] failed to close keyfile, err: %d\n", stfs_geterrno());
    //todo stfs_unlink(file);
    return -1;
  }

  return 0;
}

int cread(uint8_t *fname, uint8_t *buf, uint32_t len) {
  int fd;
  if((fd=stfs_open(fname, 0))==-1) {
    //LOG(1,"[x] failed to open %s, err: %d\n", fname, stfs_geterrno());
    return -1;
  }

  unsigned char cipher[len + crypto_secretbox_ZEROBYTES];
  memset(cipher,0, crypto_secretbox_BOXZEROBYTES);

  uint8_t nonce[crypto_secretbox_NONCEBYTES];
  if(stfs_read(fd,nonce,crypto_secretbox_NONCEBYTES)!=crypto_secretbox_NONCEBYTES) {
    //LOG(1,"[x] failed reading nonce from file '%s' err: %d\n", fname, stfs_geterrno());
    return -1;
    if(stfs_close(fd)==-1) {
      //LOG(1,"[x] failed to close file, err: %d\n", stfs_geterrno());
      return -1;
    }
  }

  int size=stfs_read(fd,cipher+crypto_secretbox_BOXZEROBYTES,len + crypto_secretbox_MACBYTES);
  if(stfs_close(fd)==-1) {
    //LOG(1,"[x] failed to close file, err: %d\n", stfs_geterrno());
    return -1;
  }
  if(size<=crypto_secretbox_MACBYTES) {
    //LOG(1,"[x] file '%s' is to short: %d, expected: %d\n", fname, size, len + crypto_secretbox_MACBYTES);
    return -1;
  }
  unsigned char plain[size + crypto_secretbox_ZEROBYTES];
  // decrypt
  if(crypto_secretbox_open(plain, cipher, size+crypto_secretbox_BOXZEROBYTES, nonce, get_master_key("load key")) == -1) {
    return -2;
  }
  if(buf!=NULL) {
    memcpy(buf, plain+crypto_secretbox_ZEROBYTES,size-crypto_secretbox_MACBYTES);
  }
  memset(plain,0,sizeof(plain));

  return size-crypto_secretbox_MACBYTES;
}

int save_peer(uint8_t *peer, const uint8_t len) {
  uint8_t peerid[STORAGE_ID_LEN];
  if(topeerid(peerid, peer, len)!=0) return -1;

  uint8_t peerpath[]="/peers/                                ";
  stohex(peerpath+7,peerid, sizeof(peerid));
  int fd=stfs_open(peerpath,64);
  if(fd<0) {
    if(stfs_geterrno() != E_EXISTS) {
      return -1;
    }
    // already exists
    return 0;
  }

  // pad peer for cwrite
  uint8_t paddedpeer[crypto_secretbox_ZEROBYTES+len],
    *ppeer=paddedpeer+crypto_secretbox_ZEROBYTES;
  memset(paddedpeer,0,crypto_secretbox_ZEROBYTES);
  memcpy(ppeer, peer, len);

  // store new peer
  return cwrite(fd, paddedpeer, len, 0);
}

int write_enc(uint8_t *path, const uint8_t *key, const int keylen) {
  // open file
  int fd;
  if((fd=stfs_open(path, 64))==-1) {
    // exists, overwrite contents
    if((fd=stfs_open(path, 0))==-1) {
      // failed to open file
      return -1;
    }
  }

  uint8_t plain[crypto_secretbox_ZEROBYTES+keylen];
  memset(plain,0,crypto_secretbox_ZEROBYTES);
  memcpy(plain+crypto_secretbox_ZEROBYTES, key, keylen);

  if(cwrite(fd, plain, keylen, 1)!=0) {
    //LOG(1, "[x] failed to store ctx '%s'\n", axolotldir);
    return -1;
  }
  // todo maybe stfs_truncate(path, keylen);
  return 0;
}

// invoke like
// if(0!=store_key(key, sizeof(key), "/peer/", prekey->ephemeralkey, peer, peer_len)) {
int store_key(const uint8_t* key, const int keylen, const char *type, const uint8_t *keyid, uint8_t* peer, const uint8_t peer_len) {
  // try to save peer
  if(0!=save_peer(peer, peer_len)) {
    return -1;
  }

  //new keyid
  int typelen=strlen((char*) type);
  if(typelen<1 || typelen>MAX_DIR_SIZE) {
    return -1;
  }

  // construct path
  uint8_t path[typelen+33*(!!peer)+33*(!!keyid)];
  memcpy(path,type,typelen);
  if(!peer && keyid) {
    stohex(path+typelen, keyid, 16);
    path[typelen+32]=0;
  } else if(peer) {
    if(peer_len<1 || peer_len>PEER_NAME_MAX) {
      return -1;
    }
    uint8_t peerid[STORAGE_ID_LEN];
    topeerid((unsigned char*) peerid, peer, peer_len);
    stohex(path+typelen, peerid, 16);

    if(keyid) {
      path[typelen+32]=0;
      stfs_mkdir(path); // needed one time
      path[typelen+32]='/';
      stohex(path+typelen+33, keyid, 16);
      path[typelen+65]=0;
    } else {
      path[typelen+32]=0;
    }
  }
  // write out encrypted key to path
  if(0!=write_enc(path, key, keylen)) {
    return -1;
  }
  return 0;
}

/*
  * @brief  save_seed: stores a seed related to a peer
  * @param  seed: pointer to seed
  * @param  peer: pointer to peers name
  * @param  len: length of peers name
  * @retval 0 or -1 on error
  *
  * if doesn't exist creates /peers/<peerid> with encrypted name in it.
  * checks if /keys/<keyid> doesn't exist - if so it fails!
  * stores enrypted key in /keys/<keyid>
  */
int save_seed(unsigned char *seed, unsigned char* peer, unsigned char len) {
  unsigned char peers[2+PEER_NAME_MAX*2];
  uint8_t keyid[STORAGE_ID_LEN];
  uint8_t userbuf[sizeof(UserRecord)+PEER_NAME_MAX];
  UserRecord *userdata=(UserRecord*) userbuf;
  if(get_user(userdata)==-1) return -1;
  // set keyid
  switch(memcmp(peer, userdata->name, MIN(len,userdata->len))) {
  case -1:
    combine(peer, len, userdata->name, userdata->len, peers, 2+PEER_NAME_MAX*2);
    break;
  case 1:
    combine(userdata->name, userdata->len, peer, len, peers, 2+PEER_NAME_MAX*2);
    break;
  case 0:
    if(len<userdata->len)
      combine(peer, len, userdata->name, userdata->len, peers, 2+PEER_NAME_MAX*2);
    else
      combine(userdata->name, userdata->len, peer, len, peers, 2+PEER_NAME_MAX*2);
    break;
  }
  crypto_generichash((unsigned char*) keyid, STORAGE_ID_LEN,         // output
                     peers, len+userdata->len+2,                     // input
                     seed, crypto_secretbox_KEYBYTES);               // key
  return store_key(seed, crypto_secretbox_KEYBYTES, "/keys/", keyid, peer, len);
}

// todo tba: maybe also add ctx->rk into the verifier?
void calc_verifier(uint8_t *out, int outlen, uint8_t *k1, uint8_t *k2) {
  if(memcmp(k1,k2,crypto_scalarmult_curve25519_BYTES)<=0) {
    crypto_generichash(out, outlen, k1, 32, k2, 32);
  } else {
    crypto_generichash(out, outlen, k2, 32, k1, 32);
  }
}

int save_ax(Axolotl_ctx *ctx, uint8_t *peerpub, uint8_t *peer, uint8_t peer_len) {
  // calculate session id
  uint8_t keyid[STORAGE_ID_LEN];
  // todo use something else than ctx.hk[sr] for the session id?
  // save ax session from ctx for sessionid=hash(sorted(ctx.hks,ctx.hkr)) into /ax/peerid/sessionid
  calc_verifier(keyid, STORAGE_ID_LEN, ctx->hkr, ctx->hks);

  // store ctx
  if(0!=store_key((uint8_t*) ctx, sizeof(Axolotl_ctx), "/ax/", keyid, peer, peer_len)) {
    return -1;
  }

  // save identitykey of peer from peerpub to /pub/peerid
  // todo keyid not NULL but 1st 16 bytes of pubkey, also change this in verify message handler
  if(0!=store_key(peerpub, crypto_scalarmult_curve25519_BYTES, "/pub/", NULL, peer, peer_len)) {
    // todo tba delete ax if we fail here?
    return -1;
  }

  // also generate a shared secret
  uint8_t seed[crypto_secretbox_KEYBYTES];
  // derive it off ctx.rk
  crypto_generichash(seed,sizeof(seed),ctx->rk,sizeof(ctx->rk),(uint8_t*) "AX derived shared secret", 24);
  if(save_seed(seed, peer, peer_len)!=0) {
    // todo tba delete ax and pubkey if we fail here?
    return -1;
  }
  return 0;
}

int load_key(uint8_t *path, int sep, uint8_t *buf, int buflen) {
  // implement select key, if only one then default
  ReaddirCTX ctx;
  path[sep]=0;
  opendir(path, &ctx);
  path[sep]='/';
  const Inode_t *inode;
  uint32_t cnt=0;
  while((inode=readdir(&ctx))!=0) {
    if(inode->name_len>32 || inode->name_len<1) {
      continue; // todo flag error?
    }
    memcpy(path+sep+1,inode->name, inode->name_len);
    cnt++;
  }
  if(cnt<1) {
    return -1;
  }
  // todo if cnt>1 select key from menu
  if(cread(path, buf, buflen)!= buflen) {
    memset(buf,0,buflen);
    return -1;
  }
  return 0;
}

/**
  * @brief  get_ekid: calculates temp key id
  * @param  keyid: pointer to keyid name
  * @param  nonce: pointer to nonce
  * @param  ekid: pointer to buffer receiving ekid
  * @retval None
  */
static void get_ekid(unsigned char* keyid,
              unsigned char* nonce,
              unsigned char* ekid) {
  randombytes_buf((void *) nonce, (size_t) EKID_NONCE_LEN);
  crypto_generichash(ekid, EKID_LEN,           // output
                     nonce, EKID_NONCE_LEN,    // output
                     keyid, STORAGE_ID_LEN);   // salt
}

/**
  * @brief  peer2seed: retrieves key and
  *         returns keyid for crypt/sign operations via usb data ep
  * @param  key: pointer to hold retrieved key
  * @param  peer: pointer to peer name
  * @param  len: length of peer name
  * @retval 0 on error, 1 on success
  */
unsigned char peer2seed(unsigned char* key, unsigned char* peer, const unsigned char len) {
  unsigned char ekid[EKID_LEN+EKID_NONCE_LEN];

  if( len == 0 || len >= PEER_NAME_MAX) {
    usb_write((unsigned char*) "err: bad name", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    return 0;
  }
  //SeedRecord* sptr = get_peer_seed(dst, src, len);

  uint8_t peerid[STORAGE_ID_LEN];
  topeerid((unsigned char*) peerid, peer, len);

  uint8_t keypath[]="/keys/                                /                                ";
  uint8_t keyid[STORAGE_ID_LEN];
  stohex(keypath+6,peerid, sizeof(peerid));
  if(load_key(keypath, 5+33, key, crypto_secretbox_KEYBYTES)==-1) {
    return 0;
  }
  unhex(keyid, keypath+39 ,32);

  // calculate ephemeral keyid
  get_ekid(keyid, ekid+EKID_LEN, ekid);
  // and send it back immediately over the ctrl ep
  usb_write(ekid, sizeof(ekid), 32, USB_CRYPTO_EP_CTRL_OUT);
  return 1;
}

int peer2pub(uint8_t *pub, uint8_t *peer, int peerlen) {
  if( peerlen < 1 || peerlen > PEER_NAME_MAX) {
    usb_write((unsigned char*) "err: bad name", 13, 32,USB_CRYPTO_EP_CTRL_OUT);
    return 0;
  }
  uint8_t peerid[STORAGE_ID_LEN];
  topeerid((unsigned char*) peerid, peer, peerlen);

  uint8_t keypath[]="/pub/                                ";
  stohex(keypath+5,peerid, sizeof(peerid));
  if(cread(keypath, pub, crypto_secretbox_KEYBYTES)==-1) {
    return 0;
  }
  return 1;
}

int ekid2key(uint8_t* key, uint8_t *ekid ) {
  //crypto_generichash(ekid, EKID_LEN,                                     // output
  //                   keyid+EKID_LEN, EKID_NONCE_LEN,                     // nonce
  //                   (unsigned char*) seedrec->keyid, STORAGE_ID_LEN);   // key
  //if(sodium_memcmp(ekid,keyid,EKID_LEN) == 0)
  //  return seedrec;
  uint8_t path[]="/keys/                                /                                ";

  ReaddirCTX pctx, kctx;
  path[5]=0;
  if(opendir(path, &pctx)!=0) {
    // fail
    return -1;
  }
  path[5]='/';
  const Inode_t *inode;
  while((inode=readdir(&pctx))!=0) {
    if(inode->name_len>32 || inode->name_len<1) {
      continue; // todo flag error?
    }
    memcpy(path+6,inode->name, inode->name_len);
    path[5+33]=0;
    opendir(path, &kctx);
    path[5+33]='/';
    while((inode=readdir(&kctx))!=0) {
      if(inode->name_len>32 || inode->name_len<1) {
        continue; // todo flag error?
      }
      uint8_t keyid[STORAGE_ID_LEN];
      if(unhex(keyid, inode->name, inode->name_len)==-1) {
        // fail invalid hex digit in filename, ignore and skip
        continue;
      }
      unsigned char _ekid[EKID_LEN+EKID_NONCE_LEN];
      crypto_generichash(_ekid, EKID_LEN,                // output
                         ekid+EKID_LEN, EKID_NONCE_LEN, // nonce
                         keyid, STORAGE_ID_LEN);         // key
      if(sodium_memcmp(_ekid,ekid,EKID_LEN) == 0) {
        // found key
        memcpy(path+5+33+1, inode->name, inode->name_len);
        if(cread(path, key, crypto_secretbox_KEYBYTES)==crypto_secretbox_KEYBYTES) {
          return 0;
        }
      }
    }
  }
  // nothing found
  return -1;
}

int load_ltkeypair(Axolotl_KeyPair *kp) {
  uint8_t path[]="/lt/                                ";
  if(load_key(path, 3, kp->sk, crypto_scalarmult_curve25519_BYTES)==-1) {
    // fail hard
    return 0;
  }
  //crypto_scalarmult_curve25519_base(kp->pk, kp->sk);
  sc_clamp(kp->sk);
  curve25519_keygen(kp->pk,kp->sk);
  return 1;
}

int get_owner(uint8_t *name) {
  uint8_t userbuf[sizeof(UserRecord)+PEER_NAME_MAX];
  UserRecord *userdata=(UserRecord*) userbuf;
  if(get_user(userdata)==-1) {
    return 0;
  }
  if(userdata->len>32) {
    return 0;
  }
  memcpy(name,userdata->name,userdata->len); // cpy peername
  return userdata->len; // length of peername
}

int pf_store_init(void) {
  char *paths[]={"/lt", "/ax", "/sph", "/keys", "/pub", "/prekeys", "/peers"};
  int i;
  ReaddirCTX ctx;
  uint8_t path[32];
  for(i=0;i<7;i++) {
    int pathlen=strlen(paths[i])+1;
    if(pathlen>sizeof(path)) {
      return -1;
    }
    memcpy(path, paths[i], pathlen);
    //path[pathlen-1]=0;
    if(opendir(path, &ctx)!=0) {
      if(0!=stfs_mkdir(path)) return -1;
    }
  }
  return 0;
}
