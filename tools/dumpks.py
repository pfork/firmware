#!/usr/bin/env python

import sys
from pyrsp.rsp import CortexM3
from pyrsp.utils import unhex, switch_endian

if __name__ == "__main__":
    try:
        rsp = CortexM3('/dev/ttyACM0', None, verbose=False)
        addr = 0x08040000
        while True:
            print >>sys.stderr, hex(addr)
            b = rsp.dump(256,addr)
            if len(b)!=256:
                print >>sys.stderr, "short read, %d" % len(b)
                print >>sys.stderr
                rsp.port.close()
                sys.exit(1)
            if ord(b[0]) == 255:
                addr += 0x020000
                addr &= 0xffff0000
            elif ord(b[128]) == 255:
                sys.stdout.write(b[:128])
                addr += 0x020000
                addr &= 0xffff0000
            else:
                sys.stdout.write(b)
                addr+=256
            if addr >= 0x8100000:
                print >>sys.stderr
                break
            sys.stdout.flush()
        rsp.port.close()

    # this is for cleaning up in case a keyboard interrupt came
    except KeyboardInterrupt:
        import traceback
        traceback.print_exc()

        res = None
        while not res:
            res = rsp.port.read()
        discards = []
        retries = 20
        while res!='+' and retries>0:
            discards.append(res)
            retries-=1
            res = rsp.port.read()
        if len(discards)>0 and rsp.verbose: print 'send discards', discards

        rsp.port.close()
        sys.exit(1)
