#! /usr/bin/python3
"""Test case for the C implementation of the overlap algorithm.

Note that we reuse all the Python test cases by building a shared
library with the C implementation which is then wrapped with a Python
class which has the sampe API as the Python implementation.

Additionally, some C code is generated with all test cases that are
run.  That C code is then compiled and run through valgrind to catch
possible memory errors.

"""

import os
import sys
import unittest
import cffi
import atexit

sys.path.append('../python')

import test_overlap
from test_overlap import *

def run(cmd):
    print(cmd)
    ec = os.system(cmd)
    if ec:
        sys.exit(ec)

# Build a library with the C code we want to test
run('gcc -Wall -g -shared -o liboverlap_algo.so overlap_algo.c')

# Create a CFFI interface to the library
ffi = cffi.FFI()
ffi.cdef(os.popen('gcc -E overlap_algo.h').read())
lib = ffi.dlopen('./liboverlap_algo.so')

# Wrap the library with the same API as the Python implementations
class COverlapAlgorithm(object):
    def __init__(self, ranges = []):
        self.algo = lib.overlap_new()
        global fnum
        self.var = 'algo%d' % fnum
        fnum += 1

        fr.write('    struct overlap_algo *%s = overlap_new();\n' % self.var)

        for lo, hi in ranges:
            self.add(lo, hi)

    def add(self, lo, hi):
        fr.write('    overlap_add(%s, %s, %s);\n' % (self.var, lo, hi))

        if not lib.overlap_add(self.algo, lo, hi):
            raise ValueError("invalid parameters to add")

    def find(self):
        lo_p = ffi.new('double [1]')
        hi_p = ffi.new('double [1]')
        r = lib.overlap_find(self.algo, lo_p, hi_p)
        if r:
            res = (r, lo_p[0], hi_p[0])
        else:
            res = (0, None, None)

        fr.write('    overlap_find(%s, &lo, &hi);\n' % (self.var))

        return res

    def __del__(self):
        fr.write('    overlap_del(%s);\n' % (self.var))
        lib.overlap_del(self.algo)

test_overlap.Algo.ALGOS.append(COverlapAlgorithm)
test_overlap.RANDOM_COUNT = 1000

fhead = '''
#include "overlap_algo.h"

int main()
{
    overlap_value_t lo, hi;
'''

ftail = '''
    return 0;
}
'''

def main():
    global fnum
    global fr

    fnum = 0
    fr = open('overlap_replay.c', 'w')
    fr.write(fhead)

    unittest.main(verbosity = 2, exit = False)

    fr.write(ftail)
    fr.close()

    run('gcc -Wall -g -o overlap_replay overlap_replay.c overlap_algo.c')

    run('valgrind -s --leak-check=yes ./overlap_replay')

if __name__ == '__main__':
    print()
    main()
