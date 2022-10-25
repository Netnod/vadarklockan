#! /usr/bin/python3
"""Test case for the C implementation of tweetnacl

"""

import os
import sys
import unittest
import cffi
import binascii
import random

from Crypto.Hash import SHA512

def run(cmd):
    print(cmd)
    ec = os.system(cmd)
    if ec:
        sys.exit(ec)

# Build a library with the C code we want to test
run('gcc -Wall -g -shared -o libtweetnacl.so tweetnacl.c')

# Create a CFFI interface to the library
ffi = cffi.FFI()
ffi.cdef(os.popen('gcc -E tweetnacl.h').read())
lib = ffi.dlopen('./libtweetnacl.so')

class TestTweetNaCl(unittest.TestCase):
    def t(self, f, msg, expect):
        pout = ffi.new('unsigned char [%u]' % len(expect))
        pmsg = ffi.new('unsigned char []', msg)
        r = f(pout, pmsg, len(msg))
        out = bytes(pout)
        self.assertEqual(out[:len(expect)], expect)

    def test_sha512(self):
        # Test vectors from https://www.di-mgt.com.au/sha_testvectors.html
        for expect, msg in [
                ( 'ddaf35a193617aba cc417349ae204131 12e6fa4e89a97ea2 0a9eeee64b55d39a 2192992a274fc1a8 36ba3c23a3feebbd 454d4423643ce80e 2a9ac94fa54ca49f', b'abc' ),
                ( 'cf83e1357eefb8bd f1542850d66d8007 d620e4050b5715dc 83f4a921d36ce9ce 47d0d13c5d85f2b0 ff8318d2877eec2f 63b931bd47417a81 a538327af927da3e', b'' ),
                ( '204a8fc6dda82f0a 0ced7beb8e08a416 57c16ef468b228a8 279be331a703c335 96fd15c13b1b07f9 aa1d3bea57789ca0 31ad85c7a71dd703 54ec631238ca3445', b'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq' ),
                ( '8e959b75dae313da 8cf4f72814fc143f 8f7779c6eb9f7fa1 7299aeadb6889018 501d289e4900f7e4 331b99dec4b5433a c7d329eeb6dd2654 5e96e55b874be909', b'abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu' ),
                ( 'e718483d0ce76964 4e2e42c7bc15b463 8e1f98b13b204428 5632a803afa973eb de0ff244877ea60a 4cb0432ce577c31b eb009c5c2c49aa2e 4eadb217ad8cc09b', b'a' * 1000000),
                ]:
            self.t(lib.crypto_hash_sha512_tweet, msg, binascii.unhexlify(''.join(expect.split())))

    def test_sha512_random(self):
        # Compare random hashes with python implementation
        for i in range(100):
            n = random.randrange(1000)
            msg = random.randbytes(n)
            h = SHA512.new()
            h.update(msg)
            expect = h.digest()
            self.t(lib.crypto_hash_sha512_tweet, msg, expect)

    def test_sha512256(self):
        for expect, msg in [
                ( '53048e2681941ef9 9b2e29b76b4c7dab e4c2d0c634fc6d46 e0e2f13107e7af23', b'abc' ),
                ( 'c672b8d1ef56ed28 ab87c3622c511406 9bdd3ad7b8f97374 98d0c01ecef0967a', b'' ),
                ( 'bde8e1f9f19bb9fd 3406c90ec6bc47bd 36d8ada9f11880db c8a22a7078b6a461', b'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq' ),
                ( '3928e184fb8690f8 40da3988121d31be 65cb9d3ef83ee614 6feac861e19b563a', b'abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu' ),
                ( '9a59a052930187a9 7038cae692f30708 aa6491923ef51943 94dc68d56c74fb21', b'a' * 1000000),
                ]:
            self.t(lib.crypto_hash_sha512256, msg, binascii.unhexlify(''.join(expect.split())))

    def test_sha512256_random(self):
        for i in range(100):
            n = random.randrange(1000)
            msg = random.randbytes(n)
            h = SHA512.new(truncate = '256')
            h.update(msg)
            expect = h.digest()
            self.t(lib.crypto_hash_sha512256, msg, expect)

def main():
    unittest.main(verbosity = 2, exit = False)

if __name__ == '__main__':
    print()

    main()
