#!/usr/bin/env python

import sys

from pysodium import crypto_sign_keypair

def gen_keys(skfile):
    pk, sk = crypto_sign_keypair()
    with open(skfile, 'w') as fd:
        fd.write(sk)
    print "const unsigned char pk[]={%s};" % ', '.join("0x%s" % format(ord(x),"02x") for x in pk)
    print "const unsigned char sk[]={%s};" % ', '.join("0x%s" % format(ord(x),"02x") for x in sk)

if __name__ == '__main__':
    gen_keys(sys.argv[1])
