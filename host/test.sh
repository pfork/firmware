#!/bin/ash

echo "testing pq-x3dh, start"
echo "put in Alice pitchfork, press enter"; read
./pitchfork kex >/tmp/kex
echo "put in Bob pitchfork, press enter"; read
./pitchfork respond Alice </tmp/kex >/tmp/response
echo "testing xeddsa signing"
echo "sign this" | ./pitchfork sign >/tmp/signature
echo "put in Alice pitchfork, press enter"; read
./pitchfork end Bob </tmp/response
echo "testing xeddsa signature verification"
{cat /tmp/signature; echo "sign me" } | ./pitchfork verify Bob
echo "testing xeddsa signature verification failing"
{cat /tmp/signature; echo "sign it" } | ./pitchfork verify Bob

echo "testing shared key encryption"
echo 'PITCHFORK!!5!' | ./pitchfork encrypt Bob >/tmp/cipher && ./pitchfork decrypt Bob </tmp/cipher

echo "testing axolotl protocol"
echo '1<3u' | ./pitchfork send Bob >/tmp/ciphertext
echo "put in Bob pitchfork, press enter"; read
./pitchfork recv Alice </tmp/ciphertext
