#include <libopencm3/usb/usbd.h>
#include "stm32f.h"
#include "nrf.h"
#include "pitchfork.h"
#include "itoa.h"
#include <string.h>
#include <stddef.h>
#include "delay.h"
#include "display.h"
#include "widgets.h"
#include "buttons.h"
#include "randombytes_pitchfork.h"
#include "axolotl.h"
#include "crypto_scalarmult_curve25519.h"
#include <crypto_generichash.h>
#include "user.h"
#include "pf_store.h"
#include "pgpwords.h"

// implements a post-quantum axolotl extension of the x3dh
// protocol. it omits the AD(pkidA||pkidB) construct. instead it mixes
// in a newhope post-quantum component and compares the fingerprints
// in one go on the display

// todo use AD construct
// todo send sphincs pubkey?

#define VERIFIER_SIZE 16

extern unsigned char outbuf[crypto_secretbox_ZEROBYTES+BUF_SIZE];
extern MenuCtx appctx;

static size_t peers=0;
//static uint8_t *msgs;
static uint8_t **menuitems=NULL;
static bool show_verifier=0;
static int ii=0;
static uint8_t peer[33];
static int peer_len=0;
static int prevpeers=0;
static uint8_t peerpub[crypto_scalarmult_curve25519_BYTES];
static Axolotl_ctx ctx;

typedef struct {
  unsigned char addr[5];
  unsigned char name[32-5+1];
  unsigned int ctr;
} __attribute((packed)) PeerCandidate;

static PeerCandidate *msgs;

static int find_peer(uint8_t* msg) {
  size_t i;
  for(i=0;i<peers;i++) {
    if(memcmp((uint8_t*) msgs[i].name, (void*) msg+5, 32-5)==0) return i;
  }
  return -1;
}

static void accept_secret(char ignored) {
  if(peer_len<1)
    getstr("pls name key", peer, &peer_len);

  if(0!=save_ax(&ctx, peerpub, peer, peer_len)) {
    disp_clear();
    disp_print(0,16,"ctx store fail");
    show_verifier=0;
    return;
  }

  show_verifier=0;
  gui_refresh=1;
}

static int show_pgpwords() {
  int ret = menu(&appctx, (const uint8_t**) menuitems,VERIFIER_SIZE,accept_secret);
  if(ret == 0) {
    //memset(key,0, sizeof(key));
    show_verifier=0;
    gui_refresh=1;
  }
  return ret;
}

static int send_buf(uint8_t *dst, uint8_t *buf, uint32_t size) {
  // try to send 1st pkt
  int i, retries;
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

static int recv_fast(uint8_t *buf, uint32_t size) {
  int i, len, retries, res;
  for(i=0;i<size;i+=len) {
    len = (size-i)>=32?32:(size-i);
    for(retries=0;retries<5;) {
      res=nrf_recv(buf+i, len);
      if((res & 0x80) == 0x80) continue; //broadcast
      if((res & 0x7f) == len)  break;
      retries++;
    }
    if(retries>=5) return 0;
  }
  return size;
}

static int recv_buf(uint8_t *src, uint8_t *buf, uint32_t size) {
  nrf_open_rx(src);
  int retries, res, len;
  for(retries=0;retries<5;) {
    len=size<=32?size:32;
    res=nrf_recv(buf, len);
    if((res & 0x80) == 0x80) continue; //broadcast
    if((res & 0x7f) == len) break;
    mDelay(15);
    retries++;
  }
  if(retries>=5) return 0;
  // try to receive rest of the packets
  return recv_fast(buf+32, size-32)==size-32;
}

static void pqx3dh(char menuidx) {
  //if(menuidx<1) return;
  // adjust menuidx to peers with packets hitting thresholds
  int i, j;
  for(i=0,j=-1;j<(int)menuidx;i++) {
    if(msgs[i].ctr>3) {
      j++;
    }
  }
  menuidx = i;

  disp_clear();
  statusline();

  // load lt private key and derive pubkey from it
  Axolotl_KeyPair kp;
  if(load_ltkeypair(&kp)==0) {
    disp_print(0,16, "fail load ltkey");
    return;
  }

  // generate prekey
  uint8_t sendbuf[sizeof(Axolotl_PreKey)+33], resp[sizeof(Axolotl_Resp)+33];
  Axolotl_Resp *o_pk = (Axolotl_Resp*) resp;
  Axolotl_PreKey *my_pk = (Axolotl_PreKey *) sendbuf;
  Axolotl_prekey_private my_sk;
  memset(sendbuf,0,sizeof(sendbuf));
  axolotl_prekey(my_pk, &my_sk, &kp);

  // copy username to the end of prekey, so we can send it off
  if((sendbuf[sizeof(Axolotl_PreKey)]=get_owner(sendbuf+sizeof(Axolotl_PreKey)+1))<1) {
    disp_print(0,16, "no owner");
    return;
    }

  // send out prekey
  while(1) {
    if(button_handler() & BUTTON_LEFT) {
      // user abort
      gui_refresh=1;
      return;
    }
    // send out prekey
    if(send_buf((uint8_t*) (&msgs[(int) menuidx]), sendbuf,sizeof(sendbuf))==0) {
      // fail
      uDelay(10);
      continue;
    }
    break;
  }

  // sleep a bit to allow for computations on peer
  // uDelay(1);
  // sent prekey successfully wait for response
  // listen for incoming prekeys
  while(recv_buf((uint8_t*) (&msgs[(int) menuidx]), resp, sizeof(resp))==0) {
    if(button_handler() & BUTTON_LEFT) {
      // user abort
      gui_refresh=1;
      return;
    }
  }

  int plen=resp[sizeof(Axolotl_Resp)];
  if(plen<1 || plen>32) {
    // fail invalid peername length
    disp_clear();
    disp_print(0,16, "failed: bad name");
    return;
  }
  // compare peer_names
  int pplen=strlen(((char*) (&msgs[(int) menuidx]))+5);
  pplen=pplen>32-5?32-5:pplen;
  if(memcmp(((char*) (&msgs[(int) menuidx]))+5,
            resp+sizeof(Axolotl_Resp)+1,
            pplen)!=0) {
    // prekey
    disp_clear();
    disp_print(0,16, "failed: bad peer");
    return;
  }

  peer_len=plen;
  if(axolotl_handshake_resp(&ctx, o_pk, &my_sk)!=0) {
    disp_clear();
    disp_print(0,16, "failed: bad data");
    return;
  }

  // remember peers pubkey for storing later
  memcpy(peerpub, o_pk->identitykey, crypto_scalarmult_curve25519_BYTES);

  // set peername from response
  memcpy(peer, resp+sizeof(Axolotl_Resp)+1, peer_len);

  uint8_t verifier[VERIFIER_SIZE];
  // we hash(idkey1|idkey2) and have the users compare those.
  // one fingerprint instead of two.
  calc_verifier(verifier, VERIFIER_SIZE, my_pk->identitykey, o_pk->identitykey, ctx.rk);

  to_pgpwords(menuitems, bufs[0].buf, verifier, VERIFIER_SIZE);
  gui_refresh=1;
  appctx.idx=0; appctx.top=0;
  show_verifier=1;
}

static int show_peers() {
  int i, j, ret;
  for(i=0,j=0;i<peers;i++) {
    if(msgs[i].ctr>3) {
      menuitems[j]=msgs[i].name;
      j++;
    }
  }
  if(j>0 && j!=prevpeers) { gui_refresh=1; prevpeers=j; }
  ret = menu(&appctx, (const uint8_t**) menuitems,j,pqx3dh);
  if(ret == 0) {
    show_verifier=0;
    gui_refresh=1;
  }
  return ret;
}

static void discover() {
  char res;
  uint8_t recvbuf[sizeof(Axolotl_PreKey)+33];
  uint8_t *msg = recvbuf;

  // send sometimes
  if(ii++ % 16 == 0) {
    nrf_send(BCAST, (uint8_t*) msgs,32);
    nrf_open_rx((uint8_t*) msgs);
    uDelay(120);
  }

  //receive
  if(((res = nrf_recv(msg, PLOAD_WIDTH)) & 0x7f) < 1) {
    return;
  }

  if(res & 0x80) { // broadcast stuff
    // process msg
    int peeridx=0;
    if((peeridx=find_peer(msg)) == -1 && peers < 64) {
      // insert into list
      peers++;
      memcpy((uint8_t*) (&msgs[peers]),msg,32);
      msgs[peers].name[32-5]=0;
      msgs[peers].ctr=0;
      gui_refresh=1;
    } else {
      if(msgs[peeridx].ctr<(32^2)-1) {
        msgs[peeridx].ctr++;
      }
    }
    return;
  }

  // private stuff received
  if(res!=32) {
    return;
  }

  // receive remaining pkts of pq-axolotl prekey
  if(recv_fast(recvbuf+32,sizeof(recvbuf)-32)==0) {
    // fail
    return;
  }

  uint8_t sendbuf[sizeof(Axolotl_Resp)+33];
  Axolotl_Resp *my_pk = (Axolotl_Resp *) sendbuf;
  Axolotl_PreKey *o_pk = (Axolotl_PreKey *) recvbuf;
  Axolotl_prekey_private my_sk;

  // copy username to the end of sendbuf, so we can send it off with our prekey
  if((sendbuf[sizeof(Axolotl_Resp)]=get_owner(sendbuf+sizeof(Axolotl_Resp)+1))<1) {
    disp_print(0,16, "no owner");
    return;
    }

  // load lt private key and derive pubkey from it
  Axolotl_KeyPair kp;
  if(load_ltkeypair(&kp)==0) {
    disp_clear();
    disp_print(0,16, "fail load ltkey");
    return;
  }

  // init own prekey
  axolotl_kexresp(my_pk, &my_sk, &kp);

  if(axolotl_handshake(&ctx, my_pk, o_pk, &my_sk)!=0) {
    // fail, try again
    //disp_print_inv(0,32,"failed");
    return;
  }

  mDelay(10);
  while(send_buf((uint8_t*) msgs, sendbuf,sizeof(sendbuf))==0) {
    if(button_handler() & BUTTON_LEFT) {
      // fail
      gui_refresh=1;
      return;
    }
  }

  memcpy(peerpub, o_pk->identitykey, crypto_scalarmult_curve25519_BYTES);

  uint8_t verifier[VERIFIER_SIZE];
  // we hash(idkey1|idkey2) and have the users compare those.
  // one fingerprint instead of two.
  calc_verifier(verifier, VERIFIER_SIZE, my_pk->identitykey, o_pk->identitykey, ctx.rk);

  // set peer and peer_len from recvbuf
  peer_len=recvbuf[sizeof(Axolotl_PreKey)];
  if(peer_len<1 || peer_len>32) {
    // fail invalid peername length
    return;
  }
  memcpy(peer, recvbuf+sizeof(Axolotl_PreKey)+1, peer_len);

  to_pgpwords(menuitems, bufs[0].buf, verifier, VERIFIER_SIZE);
  gui_refresh=1;
  appctx.idx=0; appctx.top=0;
  show_verifier=1;
}

int kex_menu_init(void) {
  msgs=(PeerCandidate*) ((((uint32_t) outbuf)+3) & 0xfffffffc);
  menuitems = (uint8_t **) bufs[1].buf;
  prevpeers=0;
  // add self
  uint8_t userbuf[sizeof(UserRecord)+PEER_NAME_MAX];
  UserRecord *userec=(UserRecord*) userbuf;
  if(get_user(userec)==0) {
    size_t namelen = userec->len;
    namelen = (namelen>(32-5))?(32-5):namelen;
    memcpy(msgs[0].name, userec->name, namelen);
    msgs[0].name[namelen]=0;
  } else {
    disp_print(0,16, "uninitialized :/");
    return 0;
  }
  // set random address
  randombytes_buf((void *) msgs, 5);
  peers=1;
  return 1;
}

int kex_menu(void) {
  if(show_verifier==0) {
    discover();
  }
  if(show_verifier==0) {
    return show_peers();
  }
  return show_pgpwords();
}
