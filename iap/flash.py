#!/usr/bin/env python

# #!#!#!#!#!#!#!#!#!#!#!#!#!#!#
# need to call init()
# before using
# #!#!#!#!#!#!#!#!#!#!#!#!#!#!#

import sys
import usb.core
import usb.util
from binascii import hexlify
from struct import unpack
from pysodium import crypto_sign_detached, crypto_generichash, crypto_sign_verify_detached

idVendor=0x0483
idProduct=0x5740
DEBUG = False

if DEBUG:
    import os
    os.environ['PYUSB_DEBUG_LEVEL'] = 'warning' # 'debug'
    os.environ['PYUSB_DEBUG'] = 'warning' # 'debug'

USB_CRYPTO_EP_CTRL_IN = 0x01
USB_CRYPTO_EP_DATA_IN = 0x02
USB_CRYPTO_EP_CTRL_OUT = 0x81
USB_CRYPTO_EP_DATA_OUT = 0x82

# USB endpoint cache
eps={}

def flash(binary):
    # in the first round the PITCHFORK wants to see that we have a valid firmware
    with open(binary,'r') as fd:
        i=0
        sector = fd.read(32*1024)
        while sector and i<8:
            # send sector
            print "sending sector", i
            wrote = eps[USB_CRYPTO_EP_DATA_IN].write(sector)
            # next sector
            i+=1
            sector = fd.read(32*1024)

        # resend the the whole thing
        i=0
        fd.seek(0)
        sector = fd.read(32*1024)
        while sector and i<8:
            print "sleeping a bit"
            eps[USB_CRYPTO_EP_DATA_OUT].read(64, timeout=10000)
            # send sector
            print "re-sending sector", i
            wrote = eps[USB_CRYPTO_EP_DATA_IN].write(sector, timeout=10000)
            # next sector
            i+=1
            sector = fd.read(32*1024)
        print

#####  support ops  #####
def init():
    dev = usb.core.find(idVendor=idVendor, idProduct=idProduct)
    cfg = dev.get_active_configuration()
    interface_number = cfg[(0,0)].bInterfaceNumber
    intf = usb.util.find_descriptor(cfg, bInterfaceNumber = interface_number)
    for ep in intf:
        eps[ep.bEndpointAddress]=ep

def reset():
    flush(USB_CRYPTO_EP_DATA_OUT)
    flush(USB_CRYPTO_EP_CTRL_OUT)

def flush(ep, quiet=True):
    while(True):
        try:
            tmp = eps[ep].read(64, timeout=10)
        except usb.core.USBError:
            break
        if not quiet: print '>', len(tmp), repr(''.join([chr(x) for x in tmp]))

init()

if __name__ == '__main__':
    #print flush(USB_CRYPTO_EP_CTRL_IN, False)
    #print flush(USB_CRYPTO_EP_CTRL_OUT, False)
    #print flush(USB_CRYPTO_EP_DAT_IN, False)
    #print flush(USB_CRYPTO_EP_DATA_OUT, False)
    #print repr(read_ctrl())
    flash(sys.argv[1])
