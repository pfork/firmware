#!/usr/bin/env python

DEBUG = True

if DEBUG:
    import os
    os.environ['PYUSB_DEBUG_LEVEL'] = 'warning' # 'debug'
    os.environ['PYUSB_DEBUG'] = 'warning' # 'debug'
    import time

import usb.core
import usb.util
import pysodium as nacl

USB_CRYPTO_EP_CTRL_IN = 0x01
USB_CRYPTO_EP_DATA_IN = 0x02
USB_CRYPTO_EP_CTRL_OUT = 0x81
USB_CRYPTO_EP_DATA_OUT = 0x82

USB_CRYPTO_CMD_ENCRYPT = chr(0)
USB_CRYPTO_CMD_DECRYPT = chr(1)
USB_CRYPTO_CMD_SIGN = chr(2)
USB_CRYPTO_CMD_VERIFY = chr(3)
USB_CRYPTO_CMD_ECDH_START = chr(4)
USB_CRYPTO_CMD_ECDH_RESPOND = chr(5)
USB_CRYPTO_CMD_ECDH_END = chr(6)
USB_CRYPTO_CMD_RNG = chr(7)
USB_CRYPTO_CMD_STOP = chr(8)
USB_CRYPTO_CMD_STORAGE = chr(9)

eps={}

def init():
    dev = usb.core.find(idVendor=0x0483, idProduct=0x5740)
    cfg = dev.get_active_configuration()
    interface_number = cfg[(0,0)].bInterfaceNumber
    intf = usb.util.find_descriptor(cfg, bInterfaceNumber = interface_number)
    for ep in intf:
        eps[ep.bEndpointAddress]=ep

def flush(ep, quiet=False):
    while(True):
        try:
            tmp = eps[ep].read(64, timeout=10)
        except usb.core.USBError:
            break
        if not quiet: print '>', len(tmp), repr(''.join([chr(x) for x in tmp]))

def read_ctrl():
    try:
        tmp = eps[USB_CRYPTO_EP_CTRL_OUT].read(32768, timeout=10)
    except usb.core.USBError:
        pass
    else:
        print ''.join([chr(x) for x in tmp])

def split_by_n( seq, n ):
    """A generator to divide a sequence into chunks of n units.
       src: http://stackoverflow.com/questions/9475241/split-python-string-every-nth-character"""
    while seq:
        yield seq[:n]
        seq = seq[n:]

def encrypt(plain):
    size = len(plain)
    if DEBUG: print 'encrypt msg size:',size >> 10, 'kB',
    resp=[]

    if DEBUG: start = time.time()
    flush(USB_CRYPTO_EP_DATA_OUT, True)
    eps[USB_CRYPTO_EP_CTRL_IN].write(USB_CRYPTO_CMD_ENCRYPT)
    size=0
    for pkt in split_by_n(plain,32768):
        wrote = eps[USB_CRYPTO_EP_DATA_IN].write(pkt)
        size += wrote
        if (wrote<32768 and not (wrote&0x3f)):
            eps[USB_CRYPTO_EP_DATA_IN].write(None)
        resp.append(eps[USB_CRYPTO_EP_DATA_OUT].read(wrote+40))
    if(len(pkt)==32768):
        eps[USB_CRYPTO_EP_DATA_IN].write(None)
    if DEBUG:
        end = time.time()
        print 'bytes sent:',size >> 10, 'kB', \
              'time:', end - start, 's', \
              'speed:', ((size+40*((size>>15)+1))/(end - start)) / (1<<10), 'kB/s'

    ctext = []
    for buf in resp:
        buf=''.join([chr(x) for x in buf])
        ctext.append(buf)
        nacl.crypto_secretbox_open(buf[nacl.crypto_secretbox_NONCEBYTES:],buf[:nacl.crypto_secretbox_NONCEBYTES],'\0'*nacl.crypto_secretbox_KEYBYTES)
    reset()
    return ''.join(ctext)

def decrypt(ctext):
    if DEBUG: print 'decrypt msg size:',len(ctext) >> 10, 'kB',
    resp=[]

    if DEBUG: start = time.time()
    flush(USB_CRYPTO_EP_DATA_OUT, True)
    eps[USB_CRYPTO_EP_CTRL_IN].write(USB_CRYPTO_CMD_DECRYPT)
    size=0
    for pkt in split_by_n(ctext,32808):
        wrote = eps[USB_CRYPTO_EP_DATA_IN].write(pkt)
        size += wrote
        if (wrote<32808 and not (wrote&0x3f)):
            eps[USB_CRYPTO_EP_DATA_IN].write(None)
        resp.append(eps[USB_CRYPTO_EP_DATA_OUT].read(wrote-40))
    if(len(pkt)==32808):
        eps[USB_CRYPTO_EP_DATA_IN].write(None)
    if DEBUG:
        end = time.time()
        print 'bytes sent:',size >> 10, 'kB', \
              'time:', end - start, 's', \
              'speed:', (len(ctext)/(end - start)) / (1<<10), 'kB/s'

    reset()
    return ''.join([''.join([chr(x) for x in buf]) for buf in resp])

def rng(size):
    if DEBUG: print 'rng bytes to read:',size >> 10, 'kB',
    read = 0
    if DEBUG: start = time.time()
    eps[USB_CRYPTO_EP_CTRL_IN].write(USB_CRYPTO_CMD_RNG)
    while(read<size):
        read += len(eps[USB_CRYPTO_EP_DATA_OUT].read(32768 if size -read > 32768 else size -read))
    eps[USB_CRYPTO_EP_CTRL_IN].write(USB_CRYPTO_CMD_STOP)
    #read_ctrl()
    if DEBUG:
        end = time.time()
        print 'bytes read:',read >> 10, 'kB', \
              'time:', end - start, 's', \
              'speed:', ((read)/(end - start)) / 1024, 'kB/s'
    flush(USB_CRYPTO_EP_DATA_OUT, True)
    reset()

def sign(msg):
    reset()
    size = len(msg)
    if DEBUG: print 'sign msg size:',size >> 10, 'kB',
    written=0
    if DEBUG: start = time.time()
    eps[USB_CRYPTO_EP_CTRL_IN].write(USB_CRYPTO_CMD_SIGN)
    for pkt in split_by_n(msg,32768):
        while(True):
            try:
                written+=eps[USB_CRYPTO_EP_DATA_IN].write(pkt)
            except usb.core.USBError:
                continue
            break
    if(size%64==0):
        eps[USB_CRYPTO_EP_DATA_IN].write(None)
    #read_ctrl()
    while(True):
        try:
            res = eps[USB_CRYPTO_EP_DATA_OUT].read(32)
        except usb.core.USBError, e:
            print e
            continue
        break
    if DEBUG:
        end = time.time()
        print 'bytes sent:',written >> 10, 'kB', \
              'time:', end - start, 's', \
              'speed:', (size/(end - start)) / (1<<10), 'kB/s'
    #read_ctrl()
    res=''.join([chr(x) for x in res])
    flush(USB_CRYPTO_EP_DATA_OUT)
    return res

def verify(sign, msg):
    reset()
    size = len(msg)
    print 'verify msg size:',size >> 10, 'kB',
    if DEBUG: start = time.time()
    flush(USB_CRYPTO_EP_DATA_OUT, True)
    eps[USB_CRYPTO_EP_CTRL_IN].write("%s%s" % (USB_CRYPTO_CMD_VERIFY,sign))
    written=0
    for pkt in split_by_n(msg,32768):
        while(True):
            try:
                written+=eps[USB_CRYPTO_EP_DATA_IN].write(pkt)
            except usb.core.USBError:
                continue
            break
    eps[USB_CRYPTO_EP_DATA_IN].write(None)
    while(True):
        try:
            res = eps[USB_CRYPTO_EP_DATA_OUT].read(1)
            #read_ctrl()
        except usb.core.USBError, e:
            print e
            continue
        break
    if DEBUG:
        end = time.time()
        print 'bytes read:',written >> 10, 'kB', \
              'time:', end - start, 's', \
              'speed:', (size/(end - start)) / (1<<10), 'kB/s'
    #read_ctrl()
    flush(USB_CRYPTO_EP_DATA_OUT)
    return res[0]

def start_ecdh(name):
    if DEBUG: start = time.time()
    flush(USB_CRYPTO_EP_DATA_OUT, True)
    eps[USB_CRYPTO_EP_CTRL_IN].write(USB_CRYPTO_CMD_ECDH_START+name)
    while(True):
        try:
            resp=eps[USB_CRYPTO_EP_DATA_OUT].read(64)
            break
        except usb.core.USBError:
            continue
    reset()
    resp = ''.join([chr(x) for x in resp])
    return (resp[:16], resp[16:])

def resp_ecdh(pub, name):
    if DEBUG: start = time.time()
    flush(USB_CRYPTO_EP_DATA_OUT, True)
    eps[USB_CRYPTO_EP_CTRL_IN].write(USB_CRYPTO_CMD_ECDH_RESPOND+pub+name)
    while(True):
        try:
            resp=eps[USB_CRYPTO_EP_DATA_OUT].read(64)
            break
        except usb.core.USBError:
            continue
    reset()
    resp = ''.join([chr(x) for x in resp])
    return (resp[:16], resp[16:])

def end_ecdh(pub, keyid):
    if DEBUG: start = time.time()
    flush(USB_CRYPTO_EP_DATA_OUT, True)
    eps[USB_CRYPTO_EP_CTRL_IN].write(USB_CRYPTO_CMD_ECDH_END+pub+keyid)
    while(True):
        try:
            resp=eps[USB_CRYPTO_EP_DATA_OUT].read(64)
            break
        except usb.core.USBError:
            continue
    reset()
    return ''.join([chr(x) for x in resp])

def reset():
    eps[USB_CRYPTO_EP_CTRL_IN].write(USB_CRYPTO_CMD_STOP)
    flush(USB_CRYPTO_EP_DATA_OUT)
    flush(USB_CRYPTO_EP_CTRL_OUT)

def test_crypt(msg):
    res = decrypt(encrypt(msg))
    print 'D(E(p) == p', res == msg

def test_sign(msg):
    print 'signature:', 'matches' if verify(sign(msg), msg) else "no match"

def test():
    reset()
    test_crypt('a'*(1<<20))
    rng(1<<18)
    test_sign('a'*16)
    test_sign('a'*512)
    test_sign('a'*(1<<17)) # 128kB
    test_sign('a'*(1<<20)) # 1MB
    test_sign('a'*(1<<23)) # 8MB
    test_crypt('a'*(1<<10))
    test_sign('a'*(1<<24)) # 16MB
    rng(1<<18)
    test_crypt('a'*(1<<20))
    rng(1<<18)
    test_crypt('a'*(1<<20))
    rng(1<<22)
    while(True):
        test_crypt('a'*(1<<20))
        test_sign('a'*(1<<24)) # 16MB
        rng(1<<22)

init()
test()

#import binascii, sys
#reset()
#keyid1,pub1 = start_ecdh('test user 3')
# print hexlify(start_ecdh('test user 1')
# returned 
#msg=binascii.unhexlify("bd15cf68071fdd9c63eb8b4be1cf8166d8c563918282f1e3a82ac41705e72479d3ff433699f3f3ff15005849c40eb117")
#keyid1=msg[:16]
#pub1=msg[16:]

#print 'sending', repr(keyid1), repr(pub1)
#keyid2, pub2 =  resp_ecdh(pub1, 'test user 3')
#print 'keyid2', binascii.hexlify(keyid2)
#print 'pub2', binascii.hexlify(pub2)
# returned
#keyid2=binascii.unhexlify("c70aeffb82105c2db7d174e7d817b246")
#pub2=binascii.unhexlify("1cf5c8e15217a694e2ad74166abe4db2984f4145292f15a500ec927e131faa73")

#keyid3 = end_ecdh(pub2, keyid1)
#print 'keyid3', binascii.hexlify(keyid3)

#keyid1, pub1 = start_ecdh('test user 9')
#keyid2, pub2 = resp_ecdh(pub1, 'test user a')
#keyid3 = end_ecdh(pub2, keyid1)

# interaction with pbp

# python pbp/main.py -D1
# secret exponent 4dae91f41901addf073738ffc5c353fc2f98fb6af6b90592edcf10e0fb379ff0
# public component c108521213bbe7a7b6aeec0a81b9414d891b907ee79eabbcea810ef893b34140

# pub1 = binascii.unhexlify("c108521213bbe7a7b6aeec0a81b9414d891b907ee79eabbcea810ef893b34140")
# keyid2, pub2 = resp_ecdh(pub1, 'pbp')
# print binascii.hexlify(pub2)

#  python pbp/main.py -D3 -DP 8bcafd74ebfc1a834e93e4a8abbbf855ea2565c2026ba5f4b61b6545d9027d52 -DE 4dae91f41901addf073738ffc5c353fc2f98fb6af6b90592edcf10e0fb379ff0
# shared secret 1db5410eff8465dbd932d860668001cd5587f6dc9c6428e0a8034e05ad0bbf58

# after reading out the seed in memory via jtag it resulted in
# 1db5410eff8465dbd932d860668001cd5587f6dc9c6428e0a8034e05ad0bbf58
# 1db5410eff8465dbd932d860668001cd5587f6dc9c6428e0a8034e05ad0bbf58
# for both pbp and pitchfork

#sys.exit(0)

