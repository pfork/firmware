#!/usr/bin/env python
# takes as input a pbm file

import sys

def split_by_n( seq, n ):
    """A generator to divide a sequence into chunks of n units.
       src: http://stackoverflow.com/questions/9475241/split-python-string-every-nth-character"""
    while seq:
        yield seq[:n]
        seq = seq[n:]

dat = ' '.join(x.strip() for x in sys.stdin.readlines()[3:]).split()
rows = list(split_by_n(dat,128))
for i in xrange(8):
    sys.stdout.write('"')
    for c in zip(*rows[i*8:(i+1)*8]):
        sys.stdout.write("\\x%02x" % (int(''.join(reversed(c)),2) ^ 0xff))
    sys.stdout.write('" \\\n')
    sys.stdout.flush()
