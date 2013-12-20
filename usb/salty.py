#!/usr/bin/env python

import os
#os.environ['PYUSB_DEBUG_LEVEL'] = 'debug'
#os.environ['PYUSB_DEBUG'] = 'debug'
os.environ['PYUSB_DEBUG_LEVEL'] = 'warning'
os.environ['PYUSB_DEBUG'] = 'warning'
import usb.core
import usb.util
import pysodium as nacl
import sys, time
from binascii import hexlify

USB_CRYPTO_EP_CTRL_IN = 0x01
USB_CRYPTO_EP_DATA_IN = 0x02
USB_CRYPTO_EP_CTRL_OUT = 0x81
USB_CRYPTO_EP_DATA_OUT = 0x82

eps={}
dev = usb.core.find(idVendor=0x0483, idProduct=0x5740)
cfg = dev.get_active_configuration()
interface_number = cfg[(0,0)].bInterfaceNumber
intf = usb.util.find_descriptor(cfg, bInterfaceNumber = interface_number)
for ep in intf:
    eps[ep.bEndpointAddress]=ep

# flush
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

def encrypt(size):
    print 'encrypt msg size:',size >> 10, 'kB',
    plain = 'a'*size
    resp=[]

    start = time.time()
    flush(USB_CRYPTO_EP_DATA_OUT, True)
    eps[USB_CRYPTO_EP_CTRL_IN].write(chr(0))
    size=0
    for pkt in split_by_n(plain,32768):
        wrote = eps[USB_CRYPTO_EP_DATA_IN].write(pkt)
        size += wrote
        if (wrote<32768 and not (wrote&0x3f)):
            eps[USB_CRYPTO_EP_DATA_IN].write(None)
        resp.append(eps[USB_CRYPTO_EP_DATA_OUT].read(wrote+40))
    if(len(pkt)==32768):
        eps[USB_CRYPTO_EP_DATA_IN].write(None)
    end = time.time()

    print 'bytes sent:',size >> 10, 'kB', \
          'time:', end - start, 's', \
          'speed:', ((size+40*((size>>15)+1))/(end - start)) / (1<<10), 'kB/s'

    for buf in resp:
        buf=''.join([chr(x) for x in buf])
        nacl.crypto_secretbox_open(buf[nacl.crypto_secretbox_NONCEBYTES:],buf[:nacl.crypto_secretbox_NONCEBYTES],'\0'*nacl.crypto_secretbox_KEYBYTES)
    reset()

def rng(size):
    print 'rng bytes to read:',size >> 10, 'kB',
    read = 0
    start = time.time()
    eps[USB_CRYPTO_EP_CTRL_IN].write(chr(4))
    while(read<size):
        read += len(eps[USB_CRYPTO_EP_DATA_OUT].read(32768 if size -read > 32768 else size -read))
    eps[USB_CRYPTO_EP_CTRL_IN].write(chr(5))
    end = time.time()
    #read_ctrl()
    print 'bytes read:',read >> 10, 'kB', \
          'time:', end - start, 's', \
          'speed:', ((read)/(end - start)) / 1024, 'kB/s'
    flush(USB_CRYPTO_EP_DATA_OUT, True)
    reset()

def sign(size):
    reset()
    print 'sign msg size:',size >> 10, 'kB',
    plain = ('a'*size)
    written=0
    start = time.time()
    eps[USB_CRYPTO_EP_CTRL_IN].write(chr(2))
    for pkt in split_by_n(plain,32768):
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
    end = time.time()
    #read_ctrl()
    print 'bytes sent:',written >> 10, 'kB', \
          'time:', end - start, 's', \
          'speed:', (size/(end - start)) / (1<<10), 'kB/s'
    res=''.join([chr(x) for x in res])
    #print 'signature:', hexlify(res)
    flush(USB_CRYPTO_EP_DATA_OUT)
    return res

def verify(sign, size):
    reset()
    print 'verify msg size:',size >> 10, 'kB',
    plain = ('a'*size)
    start = time.time()
    flush(USB_CRYPTO_EP_DATA_OUT, True)
    eps[USB_CRYPTO_EP_CTRL_IN].write("%s%s" % (chr(3),sign))
    written=0
    for pkt in split_by_n(plain,32768):
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
    end = time.time()
    #read_ctrl()
    print 'bytes read:',written >> 10, 'kB', \
          'time:', end - start, 's', \
          'speed:', (size/(end - start)) / (1<<10), 'kB/s'
    print 'signature:', 'matches' if res[0] else "no match"
    flush(USB_CRYPTO_EP_DATA_OUT)

def reset():
    eps[USB_CRYPTO_EP_CTRL_IN].write(chr(5))
    flush(USB_CRYPTO_EP_DATA_OUT)
    flush(USB_CRYPTO_EP_CTRL_OUT)

reset()
rng(1<<18)
verify(sign(16), 16)
verify(sign(512), 512)
verify(sign(1024*128), 1024*128)
verify(sign(1024*1024), 1024*1024)
verify(sign(8*1024*1024), 8*1024*1024)
encrypt(1<<10)  # 1MB/32kB
verify(sign(64*512*512), 64*512*512) # 16MB
rng(1<<18)
encrypt(1<<20)  # 1MB/32kB
rng(1<<18)
encrypt(8*(1<<20))  # 16MB/32kB
rng(1<<18)
encrypt(16*(1<<20))  # 16MB/32kB
verify(sign(64*512*512), 64*512*512) # 16MB
rng(1<<18)
encrypt(16*(1<<20))  # 16MB/32kB
verify(sign(64*512*512), 64*512*512) # 16MB
rng(1<<18)
encrypt(16*(1<<20))  # 16MB/32kB
verify(sign(64*512*512), 64*512*512) # 16MB
rng(1<<18)
encrypt(16*(1<<20))  # 16MB/32kB
verify(sign(64*512*512), 64*512*512) # 16MB
rng(1<<18)
encrypt(16*(1<<20))  # 16MB/32kB
verify(sign(64*512*512), 64*512*512) # 16MB
rng(1<<18)
encrypt(16*(1<<20))  # 16MB/32kB
verify(sign(64*512*512), 64*512*512) # 16MB
rng(1<<18)
encrypt(16*(1<<20))  # 16MB/32kB
verify(sign(64*512*512), 64*512*512) # 16MB
rng(1<<18)
encrypt(16*(1<<20))  # 16MB/32kB
verify(sign(64*512*512), 64*512*512) # 16MB
rng(1<<18)
encrypt(16*(1<<20))  # 16MB/32kB
verify(sign(64*512*512), 64*512*512) # 16MB
