#include "stfs.h"
#include "oled.h"
#include "delay.h"
#include "keys.h"
#include "widgets.h"
#include "nrf.h"
#include "pitchfork.h"
#include "pf_store.h"
#include "qrcode.h"
#include "master.h"
#include <stdint.h>
#include <string.h>
#include "pgpwords.h"

#include <crypto_generichash.h>
#include "xeddsa_keygen.h"
#include "pqcrypto_sign.h"

// todo peer pub verify/show
// set new password on peer

static char *keymenu[]={"Show", "Broadcast", "New keyphrase", "Delete", "Verify"};
typedef enum {BrNone = 0, BrAx, BrLt, BrSphincs, BrPub, BrCfg, BrKeys, BrPeers, BrPrekeys} Dirmode;

/**
 * @brief maps path to dirmode enum
 * @param path to map to dirmode enum
 * @retval dirmode enum
 */
static Dirmode dirmode(const uint8_t *path) {
  if(memcmp(path,"/ax",4)==0) return BrAx;
  if(memcmp(path,"/lt",4)==0) return BrLt;
  if(memcmp(path,"/sph",5)==0) return BrSphincs;
  if(memcmp(path,"/pub",5)==0) return BrPub;
  if(memcmp(path,"/cfg",5)==0) return BrCfg;
  if(memcmp(path,"/keys",6)==0) return BrKeys;
  if(memcmp(path,"/peers",7)==0) return BrPeers;
  if(memcmp(path,"/prekeys",9)==0) return BrPrekeys;
  return BrNone;
}

/**
 * @brief builds an array of the entries in a directory for the menu from widget.c
 * @param path to get dents of
 * @retval -1 on error, 0 on success
 */
static int getdents(uint8_t *path) {
  int plen=strlen((char*)path)+1;
  MenuCtx *ctx=((void*) outbuf)+plen;
  char *menulen=((void*) ctx)+sizeof(MenuCtx);
  uint8_t **menuptrs=((void*) menulen)+sizeof(int);
  uint8_t **menuptr=menuptrs;
  uint8_t *menuitem=((void*) menuptrs)+33*255;
  const Inode_t *inode;
  ReaddirCTX dctx;

  if(0!=stfs_opendir(path, &dctx)) return -1;

  memcpy(outbuf,path,plen);
  ctx->idx=0;
  ctx->top=0;
  *menulen=0;

  while((inode=stfs_readdir(&dctx))!=0 && *menulen!=255) {
    if(inode->name_len>32 || inode->name_len<1) {
      // invalid name
      memcpy(menuitem,"invalid name",13);
      menuitem+=13;
      continue;
    }
    *menuptr++=menuitem;
    (*menulen)++;
    switch(dirmode(path)) {
    case BrPeers: // /peers/peerid
    case BrAx: // /*/peerid/keyid
    case BrPub: // /pub/peerid
    case BrKeys: { // /*/peerid/keyid
      // resolve peer
      uint8_t peerpath[]="/peers/                                ";
      memcpy(peerpath+7,inode->name, inode->name_len);
      int retries=3, len=0;
      uint8_t cpeer[PEER_NAME_MAX];
      while((len=cread(peerpath, cpeer, sizeof(cpeer)))==-2 && retries-->=0) {
        erase_master_key();
        get_master_key("bad key");
      }
      if(len<1) {
        len=inode->name_len;
        memcpy(menuitem,inode->name,inode->name_len);
      } else {
        memcpy(menuitem,cpeer,len); // username
      }
      menuitem[len]=0;
      menuitem+=len+1;
      break;
    }
    case BrLt: // */keyid
    case BrSphincs: // */keyid
    case BrPrekeys: // */keyid
    case BrCfg:
    case BrNone: {
        memcpy(menuitem, inode->name, inode->name_len);
        menuitem[inode->name_len]=0;
        menuitem+=inode->name_len+1;;
        break;
    }
    }
  }
  return 0;
}

static void parentdir() {
  // go back to parent dir
  int plen=strlen((char*) outbuf);
  if(plen<2) {
    oled_clear();
    oled_print(0,0, "strange parent", Font_8x8);
    oled_print(0,9, "pls reboot", Font_8x8);
    while(1);
  }
  uint8_t *ptr;
  ptr=outbuf+plen;
  if(*ptr=='/') {
    oled_clear();
    oled_print(0,0, "strange file", Font_8x8);
    oled_print(0,9, "pls reboot", Font_8x8);
    while(1);
  }
  for(;ptr>outbuf;ptr--) {
    if(*ptr!='/') continue;
    *ptr=0;
    break;
  }
  getdents(outbuf);
}

/**
 * @brief sets up the menu for a specific key
 * @param dmode controls which ops to allow
 */
static void setup_keymenu(Dirmode dmode) {
  int plen=strlen((char*)outbuf);
  MenuCtx *ctx=((void*) outbuf)+plen+1;
  char *menulen=((void*) ctx)+sizeof(MenuCtx);
  uint8_t **menuptrs=((void*) menulen)+sizeof(int);
  ctx->top=0;ctx->idx=0;
  switch(dmode) {
  case BrPeers: {
    menuptrs[0]=(uint8_t*) keymenu[2]; // new keyphrase
    menuptrs[1]=(uint8_t*) keymenu[3]; // delete
    // todo check if there is a pubkey available
    //menuptrs[2]=(uint8_t*) keymenu[4]; // verify pubkey
    //menuptrs[3]=(uint8_t*) keymenu[0]; // show pubkey
    //*menulen=5;
    *menulen=3;
    break;
  }
  case BrAx: {
    menuptrs[0]=(uint8_t*) keymenu[3]; // delete
    *menulen=1;
    break;
  }
  case BrPub: {
    menuptrs[0]=(uint8_t*) keymenu[0]; // show pk
    menuptrs[1]=(uint8_t*) keymenu[4]; // verify pk
    menuptrs[2]=(uint8_t*) keymenu[1]; // broadcast pk
    menuptrs[3]=(uint8_t*) keymenu[3]; // delete
    *menulen=4;
    break;
  }
  case BrKeys: {
    menuptrs[0]=(uint8_t*) keymenu[4]; // verify seed
    menuptrs[1]=(uint8_t*) keymenu[3]; // delete
    *menulen=2;
    break;
  }
  case BrLt: {
    menuptrs[0]=(uint8_t*) keymenu[0]; // show pk
    menuptrs[1]=(uint8_t*) keymenu[4]; // verify pk
    menuptrs[2]=(uint8_t*) keymenu[1]; // broadcast pk
    menuptrs[3]=(uint8_t*) keymenu[3]; // delete
    *menulen=4;
    break;
  }
  case BrSphincs: {
    menuptrs[0]=(uint8_t*) keymenu[4]; // verify pk
    menuptrs[1]=(uint8_t*) keymenu[1]; // broadcast pk
    menuptrs[2]=(uint8_t*) keymenu[3]; // delete
    *menulen=3;
    break;
  }
  case BrPrekeys: {
    menuptrs[0]=(uint8_t*) keymenu[3]; // delete
    *menulen=1;
    break;
  }
  case BrCfg: {
    menuptrs[0]=(uint8_t*) keymenu[3]; // delete
    *menulen=1;
    break;
  }
  case BrNone: {
    menuptrs[0]=(uint8_t*) keymenu[3]; // delete
    *menulen=1;
    break;
  }
  }
}

static void unimplemented(void) {
  oled_clear();
  oled_print(0,23,"Unimplemented", Font_8x8);
  oled_print(7*8,33,":/", Font_8x8);
}

/**
 * @brief sends a buffer over the nrf to dst
 * @param dst nrf destination address
 * @param buf the buffer to send
 * @param size the size of the buffer
 */
static int sendbuf(uint8_t *dst, uint8_t *buf, uint32_t size) {
  // try to send 1st pkt
  int i, retries;
  oled_clear();
  oled_print(0,23,"sending",Font_8x8);
  for(retries=0;retries<5;retries++) {
    if(nrf_send(dst,buf,size<=32?size:32)==0) {
      mDelay(10);
      continue;
    }
    // send the rest of the pkts
    int len;
    for(i=32;i<size;i+=len) {
      len = (size-i)>=32?32:(size-i);
      int retries1, ret=0;
      for(retries1=0;retries1<5 && ret==0;retries1++) ret=nrf_send(dst,buf+i,len);
      if(retries1>=5) break;
    }
    if(i==size) break;
    mDelay(10);
  }
  return retries<5;
}

/**
 * @brief broadcasts a pubkey over nrf and displays a verifier code for it.
 */
static void broadcast() {
  // sphincs(v), lt, pub
  // get pk and send it off
  int retries=3;
  uint8_t verifier[16];
  int len;

  int i, plen=strlen((char*) outbuf);
  for(i=1;i<plen;i++) {
    if(outbuf[i]=='/') {
      outbuf[i]=0;
      break;
    }
  }
  Dirmode dmode=dirmode(outbuf);
  if(i<plen) outbuf[i]='/';
  else {
    oled_clear();
    oled_print(0,9, "Something went", Font_8x8);
    oled_print(0,18, "horribly wrong", Font_8x8);
    oled_print(0,36, "pls reboot", Font_8x8);
    while(1); }; // how did we get here anyway?

  switch(dmode) {
  case BrSphincs: {
    uint8_t ssk[PQCRYPTO_SECRETKEYBYTES];
    while((len=cread(outbuf, ssk, sizeof(ssk)))==-2 && retries-->=0) {
      erase_master_key();
      get_master_key("bad key");
    }
    gui_refresh=1;
    if(retries<0 || len!=PQCRYPTO_SECRETKEYBYTES) {
      //fail
      oled_clear();
      oled_print(0,23,"load key fail", Font_8x8);
      mDelay(200);
      return;
    }

    uint8_t spk[PQCRYPTO_PUBLICKEYBYTES];
    pqcrypto_sign_public_key(spk, ssk);
    memset(ssk,0,sizeof(ssk));
    sendbuf((uint8_t*)"PFORK",spk,PQCRYPTO_PUBLICKEYBYTES);
    crypto_generichash(verifier, sizeof(verifier),                   // output
                       spk, sizeof(spk),                               // msg
                       (uint8_t*) "PITCHFORK!!5! Key Verifier", 26); // "MK")
    break;
  }
  case BrLt: {
    uint8_t lsk[crypto_scalarmult_curve25519_BYTES];
    while((len=cread(outbuf, lsk, crypto_scalarmult_curve25519_BYTES))==-2 && retries-->=0) {
      erase_master_key();
      get_master_key("bad key");
    }

    gui_refresh=1;
    if(retries<0 || len!=crypto_scalarmult_curve25519_BYTES) {
      //fail
      oled_clear();
      oled_print(0,23,"load key fail", Font_8x8);
      mDelay(200);
      return;
    }

    uint8_t lpk[crypto_scalarmult_curve25519_BYTES];
    sc_clamp(lsk);
    curve25519_keygen(lpk,lsk);
    memset(lsk,0,crypto_scalarmult_curve25519_BYTES);
    sendbuf((uint8_t*)"PFORK",lpk,PQCRYPTO_PUBLICKEYBYTES);
    crypto_generichash(verifier, sizeof(verifier),                   // output
                       lpk, sizeof(lpk),                               // msg
                       (uint8_t*) "PITCHFORK!!5! Key Verifier", 26); // "MK")
    break;
  }
  case BrKeys:
  case BrPub: {
    uint8_t ppk[crypto_scalarmult_curve25519_BYTES];
    while((len=cread(outbuf, ppk, sizeof(ppk)))==-2 && retries-->=0) {
      erase_master_key();
      get_master_key("bad key");
    }
    gui_refresh=1;
    if(retries<0 || len!=crypto_scalarmult_curve25519_BYTES) {
      //fail
      oled_clear();
      oled_print(0,23,"load key fail", Font_8x8);
      mDelay(200);
      return;
    }
    sendbuf((uint8_t*)"PFORK",ppk,PQCRYPTO_PUBLICKEYBYTES);
    crypto_generichash(verifier, sizeof(verifier),                   // output
                       ppk, sizeof(ppk),                               // msg
                       (uint8_t*) "PITCHFORK!!5! Key Verifier", 26); // "MK")
    break;
  }
  case BrPeers: {
    unimplemented();
    return;
  }
  default: {
    // fail - wrong file type to operate a verify op on
    oled_clear();
    oled_print(0,0,"how did you", Font_8x8);
    oled_print(0,9,"get here?", Font_8x8);
    oled_print(0,18,"how will you", Font_8x8);
    oled_print(0,27,"get out of here?", Font_8x8);
    mDelay(5000);
    return;
  }
  }
  MenuCtx *ctx=((void*) outbuf)+plen+1;
  char *menulen=((void*) ctx)+sizeof(MenuCtx);
  uint8_t **menuptrs=((void*) menulen)+sizeof(int);
  uint8_t *words = ((void*) menuptrs) + sizeof(void*)*16;
  to_pgpwords(menuptrs, words, verifier, sizeof(verifier));
  *menulen=16;
  ctx->idx=0; ctx->top=0;
  gui_refresh=1;
}

/**
 * @brief displays a verifier for a pubkey as pgp words
 */
static void verify() {
  // sphincs(v), lt, pub, keys(verifier)
  // get pk and display pgpwords
  int retries=3;
  uint8_t verifier[16];
  int len;

  int i, plen=strlen((char*) outbuf);
  for(i=1;i<plen;i++) {
    if(outbuf[i]=='/') {
      outbuf[i]=0;
      break;
    }
  }
  Dirmode dmode=dirmode(outbuf);
  if(i<plen) outbuf[i]='/';
  else {
    oled_clear();
    oled_print(0,9, "Something went", Font_8x8);
    oled_print(0,18, "horribly wrong", Font_8x8);
    oled_print(0,36, "pls reboot", Font_8x8);
    while(1); }; // how did we get here anyway?

  switch(dmode) {
  case BrSphincs: {
    uint8_t ssk[PQCRYPTO_SECRETKEYBYTES];
    while((len=cread(outbuf, ssk, sizeof(ssk)))==-2 && retries-->=0) {
      erase_master_key();
      get_master_key("bad key");
    }
    gui_refresh=1;
    if(retries<0 || len!=PQCRYPTO_SECRETKEYBYTES) {
      //fail
      oled_clear();
      oled_print(0,23,"load key fail", Font_8x8);
      mDelay(200);
      return;
    }

    uint8_t spk[PQCRYPTO_PUBLICKEYBYTES];
    pqcrypto_sign_public_key(spk, ssk);
    memset(ssk,0,sizeof(ssk));
    crypto_generichash(verifier, sizeof(verifier),                   // output
                       spk, sizeof(spk),                               // msg
                       (uint8_t*) "PITCHFORK!!5! Key Verifier", 26); // "MK")
    break;
  }
  case BrLt: {
    uint8_t lsk[crypto_scalarmult_curve25519_BYTES];
    while((len=cread(outbuf, lsk, crypto_scalarmult_curve25519_BYTES))==-2 && retries-->=0) {
      erase_master_key();
      get_master_key("bad key");
    }

    gui_refresh=1;
    if(retries<0 || len!=crypto_scalarmult_curve25519_BYTES) {
      //fail
      oled_clear();
      oled_print(0,23,"load key fail", Font_8x8);
      mDelay(200);
      return;
    }

    uint8_t lpk[crypto_scalarmult_curve25519_BYTES];
    sc_clamp(lsk);
    curve25519_keygen(lpk,lsk);
    memset(lsk,0,crypto_scalarmult_curve25519_BYTES);
    crypto_generichash(verifier, sizeof(verifier),                   // output
                       lpk, sizeof(lpk),                               // msg
                       (uint8_t*) "PITCHFORK!!5! Key Verifier", 26); // "MK")
    break;
  }
  case BrKeys:
  case BrPub: {
    uint8_t ppk[crypto_scalarmult_curve25519_BYTES];
    while((len=cread(outbuf, ppk, sizeof(ppk)))==-2 && retries-->=0) {
      erase_master_key();
      get_master_key("bad key");
    }
    gui_refresh=1;
    if(retries<0 || len!=crypto_scalarmult_curve25519_BYTES) {
      //fail
      oled_clear();
      oled_print(0,23,"load key fail", Font_8x8);
      mDelay(200);
      return;
    }
    crypto_generichash(verifier, sizeof(verifier),                   // output
                       ppk, sizeof(ppk),                               // msg
                       (uint8_t*) "PITCHFORK!!5! Key Verifier", 26); // "MK")
    break;
  }
  case BrPeers: {
    unimplemented();
    return;
  }
  default: {
    // fail - wrong file type to operate a verify op on
    oled_clear();
    oled_print(0,0,"how did you", Font_8x8);
    oled_print(0,9,"get here?", Font_8x8);
    oled_print(0,18,"how will you", Font_8x8);
    oled_print(0,27,"get out of here?", Font_8x8);
    mDelay(5000);
    return;
  }
  }
  MenuCtx *ctx=((void*) outbuf)+plen+1;
  char *menulen=((void*) ctx)+sizeof(MenuCtx);
  uint8_t **menuptrs=((void*) menulen)+sizeof(int);
  uint8_t *words = ((void*) menuptrs) + sizeof(void*)*16;
  to_pgpwords(menuptrs, words, verifier, sizeof(verifier));
  *menulen=16;
  ctx->idx=0; ctx->top=0;
  gui_refresh=1;
}

/**
 * @brief displays a qr code for a long-term key
 */
static void ltqr(uint8_t *verifier) {
  int retries=3;
  int len;
  uint8_t sk[crypto_scalarmult_curve25519_BYTES];
  while((len=cread(outbuf, sk, crypto_scalarmult_curve25519_BYTES))==-2 && retries-->=0) {
    erase_master_key();
    get_master_key("bad key");
  }

  gui_refresh=1;
  if(retries<0 || len!=crypto_scalarmult_curve25519_BYTES) {
    //fail
    oled_clear();
    oled_print(0,23,"load key fail", Font_8x8);
    mDelay(200);
    return;
  }

  uint8_t pk[crypto_scalarmult_curve25519_BYTES];
  sc_clamp(sk);
  curve25519_keygen(pk,sk);
  memset(sk,0,crypto_scalarmult_curve25519_BYTES);
  crypto_generichash(verifier, 16,                                 // output
                     pk, sizeof(pk),                               // msg
                     (uint8_t*) "PITCHFORK!!5! Key Verifier", 26); // "MK")

  // it's our own key, so prepend our own name to the key.
  uint8_t userbuf[sizeof(UserRecord)+PEER_NAME_MAX];
  UserRecord *userdata=(UserRecord*) userbuf;
  if(get_user(userdata)==-1) return;
  uint8_t lmsg[userdata->len+1+crypto_scalarmult_curve25519_BYTES];
  memcpy(lmsg,userdata->name, userdata->len);
  lmsg[userdata->len]='\n';
  memcpy(lmsg+userdata->len+1,pk,crypto_scalarmult_curve25519_BYTES);
  qrcode(lmsg, userdata->len+1+crypto_scalarmult_curve25519_BYTES);
}

/**
 * @brief displays a qr code verifier for a pubkey
 */
static void pubqr(uint8_t *verifier) {
  int retries=3;
  int len;
  uint8_t ppk[crypto_scalarmult_curve25519_BYTES];
  while((len=cread(outbuf, ppk, sizeof(ppk)))==-2 && retries-->=0) {
    erase_master_key();
    get_master_key("bad key");
  }
  gui_refresh=1;
  if(retries<0 || len!=crypto_scalarmult_curve25519_BYTES) {
    //fail
    oled_clear();
    oled_print(0,23,"load key fail", Font_8x8);
    mDelay(200);
    return;
  }
  crypto_generichash(verifier, 16,                                 // output
                     ppk, sizeof(ppk),                             // msg
                     (uint8_t*) "PITCHFORK!!5! Key Verifier", 26); // "MK")

  // resolve peer name
  uint8_t peerpath[]="/peers/                                ";
  memcpy(peerpath+7,outbuf+5, 32);
  uint8_t cpeer[PEER_NAME_MAX];
  while((len=cread(peerpath, cpeer, sizeof(cpeer)))==-2 && retries-->=0) {
    erase_master_key();
    get_master_key("bad key");
  }
  if(len<1) {
    oled_clear();
    oled_print(0,0,"No name found", Font_8x8);
    mDelay(500);
    return;
  }
  uint8_t msg[crypto_scalarmult_curve25519_BYTES+1+len];
  memcpy(msg,cpeer,len); // username
  msg[len]='\n';
  memcpy(msg+len+1,ppk,sizeof(ppk));
  qrcode(msg, len+1+crypto_scalarmult_curve25519_BYTES);
}

static void show(void) {
  // pub & lt
  // get pk and display it as qrcode
  uint8_t verifier[16];

  int i, plen=strlen((char*) outbuf);
  for(i=1;i<plen;i++) {
    if(outbuf[i]=='/') {
      outbuf[i]=0;
      break;
    }
  }
  Dirmode dmode=dirmode(outbuf);
  if(i<plen) outbuf[i]='/';
  else {
    oled_clear();
    oled_print(0,9, "Something went", Font_8x8);
    oled_print(0,18, "horribly wrong", Font_8x8);
    oled_print(0,36, "pls reboot", Font_8x8);
    while(1); }; // how did we get here anyway?

  switch(dmode) {
  case BrLt: {ltqr(verifier); break;}
  case BrPub: { pubqr(verifier); break;}
  case BrPeers: {unimplemented(); return;}
  default: {
    // fail - wrong file type to operate a verify op on
    oled_clear();
    oled_print(0,0,"how did you", Font_8x8);
    oled_print(0,9,"get here?", Font_8x8);
    oled_print(0,18,"how will you", Font_8x8);
    oled_print(0,27,"get out of here?", Font_8x8);
    mDelay(5000);
    return;
  }
  }

  oled_cmd(0x81);//--set contrast control register
  oled_cmd(0xff);
  while(keys_pressed()==0);
  while(keys_pressed()!=0);
  oled_cmd(0x81);//--set contrast control register
  oled_cmd(0x7f);

  MenuCtx *ctx=((void*) outbuf)+plen+1;
  char *menulen=((void*) ctx)+sizeof(MenuCtx);
  uint8_t **menuptrs=((void*) menulen)+sizeof(int);
  uint8_t *words = ((void*) menuptrs) + sizeof(void*)*16;
  to_pgpwords(menuptrs, words, verifier, 16);
  *menulen=16;
  ctx->idx=0; ctx->top=0;
  gui_refresh=1;
}

// just for /peers
static void changepass() {
  // find all keys in /ax, /keys, /pub
  // and reencode them with the new password
  unimplemented();
  int retries=3;
  int len;
  uint8_t peer[PEER_NAME_MAX];
  while((len=cread(outbuf, peer, sizeof(peer)))==-2 && retries-->=0) {
    erase_master_key();
    get_master_key("bad key");
  }
  gui_refresh=1;
  if(retries<0 || len>PEER_NAME_MAX || len<1) {
    //fail
    oled_clear();
    oled_print(0,23,"load peer fail", Font_8x8);
    mDelay(200);
    return;
  }
}

static void delete() {
  oled_clear();
  if(0!=stfs_unlink(outbuf)) {
    oled_print(0,23,"delete failed", Font_8x8);
    mDelay(200);
  }

  // go back from file to parent dir
  parentdir();

  // check if directory is empty
  int plen=strlen((char*)outbuf)+1;
  MenuCtx *ctx=((void*) outbuf)+plen;
  char *menulen=((void*) ctx)+sizeof(MenuCtx);
  if(*menulen==0) { // also clear parent dir (peerid)
    stfs_rmdir(outbuf);
    parentdir();
  }
  gui_refresh=1;
}

void browse_cb(char idx) {
  int plen=strlen((char*)outbuf);
  MenuCtx *ctx=((void*) outbuf)+plen+1;
  char *menulen=((void*) ctx)+sizeof(MenuCtx);
  uint8_t **menuptrs=((void*) menulen)+sizeof(int);
  if(*menulen<=idx) {
    // should not happen
    return;
  }

  uint8_t *menuitem=menuptrs[(int) idx];
  // handle keymenu entries
  if(menuitem==(uint8_t*)keymenu[0]) { show(); return; }
  else if(menuitem==(uint8_t*)keymenu[1]) { broadcast(); return; }
  else if(menuitem==(uint8_t*)keymenu[2]) { changepass(); return; }
  else if(menuitem==(uint8_t*)keymenu[3]) { delete(); return; }
  else if(menuitem==(uint8_t*)keymenu[4]) { verify(); return; }

  // assemble new path
  int flen=strlen((char*)menuitem);
  if(flen<1 || flen > 32) {
    // todo fail
    return;
  }
  uint8_t path[plen+32+2];
  memcpy(path,outbuf,plen);
  if(outbuf[0]=='/' && outbuf[1]==0) {
    path[0]='/';
    memcpy(path+1,menuitem,flen+1);
  } else {
    path[plen]='/';
    // if current pathtype is a peername converting one
    // convert the name back to a peerid
    switch(dirmode(outbuf)) {
    case BrPeers: // /peers/peerid
    case BrAx: // /*/peerid/keyid
    case BrPub: // /pub/peerid
    case BrKeys: { // /*/peerid/keyid
      uint8_t peerid[STORAGE_ID_LEN];
      if(topeerid(peerid, menuitem, flen)!=0) return;
      stohex(path+plen+1,peerid, sizeof(peerid));
      path[plen+1+STORAGE_ID_LEN*2]=0;
      break;
    }
    default: {
      memcpy(path+plen+1,menuitem,flen+1);
    }
    }
  }

  ReaddirCTX dummy;
  // is the path a directory?
  if(-1==stfs_opendir(path, &dummy)) {
    if(stfs_geterrno()==E_NOTDIR) {
      // no it's a file
      int i, p2len;
      p2len=strlen((char*) path);
      for(i=1;i<p2len;i++) {
        if(path[i]=='/') {
          path[i]=0;
          break;
        }
      }
      Dirmode d2mode=dirmode(path);
      if(i<p2len) path[i]='/';
      else {
        oled_clear();
        oled_print(0,9, "Something went", Font_8x8);
        oled_print(0,18, "horribly wrong", Font_8x8);
        oled_print(0,36, "pls reboot", Font_8x8);
        while(1); }; // how did we get here anyway?
      memcpy(outbuf,path,p2len+1);
      setup_keymenu(d2mode);
      gui_refresh=1;
      return;
    }
    // todo fail somehow
    oled_clear();
    oled_print(0,23,"bad file",Font_8x8);
    oled_print(0,33,(char*) path+strlen((char*) path)-16,Font_8x8);
    mDelay(200);
    return;
  } else {
    // yup path is a directory
    if(0!=getdents(path)) {
        oled_clear();
        oled_print(0,9, "Something went", Font_8x8);
        oled_print(0,18, "horribly wrong", Font_8x8);
        oled_print(0,36, "pls reboot", Font_8x8);
        oled_print(0,56, "getdents toctou", Font_8x8);
        while(1); // how did we get here anyway?
    }
    gui_refresh=1;
  }
}

int browser_init(void) {
  uint8_t root[]="/";
  gui_refresh=1;
  return getdents(root);
}

int browser(void) {
  int plen=strlen((char*)outbuf)+1;
  MenuCtx *ctx=((void*) outbuf)+plen;
  char *menulen=((void*) ctx)+sizeof(MenuCtx);
  uint8_t **menuptrs=((void*) menulen)+sizeof(int);
  if(0==menu(ctx, (const uint8_t **) menuptrs,*menulen,browse_cb)) {
    // go back one dir
    int plen=strlen((char*) outbuf);
    if(plen==1) return 0; // trying to leave root - gets us back to the main menu
    uint8_t *ptr;
    for(ptr=outbuf+plen;ptr>outbuf;ptr--) {
      if(*ptr!='/') continue;
      *ptr=0;
      break;
    }
    if(ptr==outbuf) {
      outbuf[0]='/';
      outbuf[1]=0;
    }
    getdents(outbuf);
  }
  return 1;
}
