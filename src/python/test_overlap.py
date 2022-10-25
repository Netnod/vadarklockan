#! /usr/bin/python3
"""Test cases for the Python implementation of the overlap
algorithm."""

import unittest
import random

from overlap import OverlapAlgorithm
from overlap_ref import ReferenceOverlapAlgorithm
from overlap_multi import MultiOverlapAlgorithm

Algo =  MultiOverlapAlgorithm
MultiOverlapAlgorithm.ALGOS = [ OverlapAlgorithm, ReferenceOverlapAlgorithm ]

RANDOM_COUNT = 1000

class TestOverlapAlgo(unittest.TestCase):
    def process_one(self, ranges, expected):
        expect_valuerror = False

        # Verify that it's possible to add all ranges in one go and then calling find at the end
        algo = Algo()
        for lo, hi in ranges:
            if lo <= hi:
                algo.add(lo, hi)
            else:
                self.assertRaises(ValueError, algo.add, lo, hi)
                expect_valuerror = True
        res_single = algo.find()

        # Make sure that the final result is as expected
        if expected is not None:
            self.assertAlmostEqual(res_single, expected)

        # Verify that it's possible to pass ranges as a parameter to the constructor
        if expect_valuerror:
            self.assertRaises(ValueError, Algo, ranges)
        else:
            algo = Algo(ranges)
            res_constructor = algo.find()

            # Make sure results agree
            if expected is not None:
                self.assertAlmostEqual(res_constructor, res_single)

        # Verify that it's possible to add ranges incrementally and calling find after each one
        if len(ranges) > 1:
            algo = Algo()
            for lo, hi in ranges:
                if lo <= hi:
                    algo.add(lo, hi)
                else:
                    self.assertRaises(ValueError, algo.add, lo, hi)
                res_incremental = algo.find()

            # Make sure results agree
            self.assertAlmostEqual(res_incremental, res_single)

    def process(self, ranges, overlap, n):
        expected = (n, overlap[0], overlap[1])

        # Try with ranges in specified order
        self.process_one(ranges, expected)

        if len(ranges) > 1:
            # Try with ranges in reversed order
            self.process_one(list(reversed(ranges)), expected)

            # Try with ranges in random order a bunch of times
            for i in range(RANDOM_COUNT):
                random.shuffle(ranges)
                self.process_one(ranges, expected)

    def test_empty(self):
        # Empty range
        self.process([ ], (None,None), 0)

    def test_invalid(self):
        # Invalid range, lo is larger than hi
        self.process([ (2,1) ], (None,None), 0)

    def test_single(self):
        # Single range
        self.process([ (1,2) ],(1,2), 1)

    def test_two_same(self):
        # Two ranges that are the same
        self.process([ (1,2), (1,2) ],(1,2), 2)

    def test_two_nested(self):
        # One range nested within in a second range
        self.process([ (1,4), (2,3) ],(2,3), 2)

    def test_two_partial(self):
        # Two ranges that overlap on one side
        self.process([ (1,3), (2,4) ],(2,3), 2)

    def test_two_non_overlapping(self):
        # Two ranges without overlap.  Note that this will return
        # the hull and say that there is one overlap.
        #
        # This could be interpreted as not being a majority
        # agreement or as that it is at least within these bounds.
        self.process([ (1,2), (3,4) ], (1,4), 1)

    def test_three_non_overlapping(self):
        self.process([ (1,2), (3,4), (5,6) ], (1,6), 1)

    def test_two_non_overlapping_nested_in_overlapping(self):
        # More corner cases, two non-overlapping ranges which are
        # both within one larger range.  This will say that there
        # are two ovelaps, and return the hull of the two
        # non-overlapping ranges within the larger range.
        self.process([ (1,6), (2,3), (4,5) ], (2,5), 2)

    def test_two_nested_plus_one_outside(self):
        # One range contained in a second range,plus a non-overlap
        self.process([ (1,4), (2,3), (5,6) ], (2,3), 2)

    def test_two_partial_plus_one_outside(self):
        # Ranges that overlap on one side, plus a non-overlap
        self.process([ (1,3), (2,4), (5,6) ], (2,3), 2)

    def test_two_nested_plus_two_non_overlapping(self):
        # One range contained in a second range, plus a two non-overlaps
        self.process([ (1,4), (2,3), (5,6), (7,8) ], (2,3), 2)

    def test_two_partial_plus_two_non_overlapping(self):
        # Ranges that overlap on one side,plus a non-overlap
        self.process([ (1,3), (2,4), (5,6), (7,8) ], (2,3), 2)

    def test_two_nested_plus_two_nested(self):
        # Two nested ranges plus two more nested ranges with no
        # overlap between the two nested ranges.  Both are equally
        # valid which gives the hull with two overlaps
        self.process([ (1,4), (2,3), (5,8), (6,7) ], (2,7), 2)

    def test_two_partial_plus_two_nested(self):
        self.process([ (1,3), (2,4), (5,8), (6,7) ], (2,7), 2)

    def test_two_nested_plus_three_non_overlapping(self):
        self.process([ (1,4), (2,3), (5,6), (7,8), (9,10) ], (2,3), 2)

    def test_two_nested_plus_three_non_overlapping(self):
        self.process([ (1,4), (2,3), (5,10), (6,7), (8,9) ], (2,9), 2)

    def test_two_nested_plus_three_nested(self):
        self.process([ (1,4), (2,3), (5,10), (6,9), (7,8) ], (7,8), 3)

    def test_random(self):
        # Random testing just to see that nothing breaks
        # expected is None since we don't know what the result ought to be
        N = 30
        for n in range(random.randint(1, RANDOM_COUNT // N)):
            ranges = []
            for i in range(random.randint(1, N)):
                ranges.append((random.random(), random.random()))
            self.process_one(ranges, None)

def main():
    unittest.main(verbosity = 2)

if __name__ == '__main__':
    print()
    main()
