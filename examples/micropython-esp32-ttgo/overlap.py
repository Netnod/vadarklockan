class OverlapAlgorithm(object):
    """Algoritm from RFC5905 A.5.5.1 to find an overlapping range.

    This is an iterative process, add a new range and return the
    number of overlaps.  If the number of overlaps is >0 the members
    .lo and .hi will contain be the overlap.
    """

    def __init__(self):
        self._edges = []
        self._responses = 0

    def handle(self, lo, hi):
        # Add the edges to our list of edges and sort the list
        self._edges.append((lo, -1))
        self._edges.append((hi, +1))
        self._edges.sort()

        self._responses += 1

        for allow in range(self._responses):
            wanted = self._responses - allow

            chime = 0
            lo = None
            for e in self._edges:
                chime -= e[1]
                if chime >= wanted:
                    lo = e[0]
                    break

            chime = 0
            hi = None
            for e in reversed(self._edges):
                chime += e[1]
                if chime >= wanted:
                    hi = e[0]
                    break

            if lo is not None and hi is not None and lo <= hi:
                break

        else:
            return 0

        self.lo = lo
        self.hi = hi

        return wanted

class OptimizedOverlapAlgorithm(object):
    """Algoritm from RFC5905 A.5.5.1 to find an overlapping range.

    This is an iterative process, add a new range and return the
    number of overlaps.  If the number of overlaps is >0 the members
    .lo and .hi will contain be the overlap.

    This is an optimized version which relies on the fact that adding
    one more measurement will never increase the number of overlaps by
    more than one.

    """

    def __init__(self):
        self._edges = []
        self._wanted = 0

    def handle(self, lo, hi):
        # Add the edges to our list of edges and sort the list
        self._edges.append((lo, -1))
        self._edges.append((hi, +1))
        self._edges.sort()

        # Increase the number of wanted overlaps
        self._wanted += 1

        while self._wanted:
            chime = 0
            lo = None
            for e in self._edges:
                chime -= e[1]
                if chime >= self._wanted:
                    lo = e[0]
                    break

            chime = 0
            hi = None
            for e in reversed(self._edges):
                chime += e[1]
                if chime >= self._wanted:
                    hi = e[0]
                    break

            if lo is not None and hi is not None and lo <= hi:
                break

            self._wanted -= 1
        else:
            return self._wanted

        self.lo = lo
        self.hi = hi
        return self._wanted

if __name__ == '__main__':
    import unittest
    import random

    class TestAlgo(unittest.TestCase):
        def process(self, ranges, overlap, n):
            algs = [ OverlapAlgorithm(), OptimizedOverlapAlgorithm() ]

            # Make sure that all algorithms agree at each step
            for lo, hi in ranges:
                prev = None
                for alg in algs:
                    res = alg.handle(lo, hi)

                    curr = (res, (alg.lo, alg.hi))
                    # print(alg.__class__.__name__, curr)
                    if prev:
                        self.assertEqual(prev, curr)
                    prev = curr

            # print("final", curr)
            # print("expect", (n, overlap))

            # Make sure that the final result is as expected
            self.assertEqual(curr, (res, overlap))

        def process2(self, ranges, overlap, n):
            # Try with ranges in specified order
            self.process(ranges, overlap, n)

            # Try with ranges in reversed order
            self.process(reversed(ranges), overlap, n)

            # Try with ranges in random order
            random.shuffle(ranges)
            self.process(ranges, overlap, n)

        def test_single(self):
            # Single range
            self.process2([ (1,2) ],(1,2), 1)

        def test_two_same(self):
            # Two ranges that are the same
            self.process2([ (1,2), (1,2) ],(1,2), 2)

        def test_two_nested(self):
            # One range nested within in a second range
            self.process2([ (1,4), (2,3) ],(2,3), 2)

        def test_two_partial(self):
            # Two ranges that overlap on one side
            self.process2([ (1,3), (2,4) ],(2,3), 2)

        def test_two_non_overlapping(self):
            # Two ranges without overlap.  Note that this will return
            # the hull and say that there is one overlap.
            #
            # This could be interpreted as not being a majority
            # agreement or as that it is at least within these bounds.
            self.process2([ (1,2), (3,4) ], (1,4), 1)

        def test_three_non_overlapping(self):
            self.process2([ (1,2), (3,4), (5,6) ], (1,6), 1)

        def test_two_non_overlapping_nested_in_overlapping(self):
            # More corner cases, two non-overlapping ranges which are
            # both within one larger range.  This will say that there
            # are two ovelaps, and return the hull of the two
            # non-overlapping ranges within the larger range.
            self.process2([ (1,6), (2,3), (4,5) ], (2,5), 2)

        def test_two_nested_plus_one_outside(self):
            # One range contained in a second range,plus a non-overlap
            self.process2([ (1,4), (2,3), (5,6) ], (2,3), 2)

        def test_two_partial_plus_one_outside(self):
            # Ranges that overlap on one side, plus a non-overlap
            self.process2([ (1,3), (2,4), (5,6) ], (2,3), 2)

        def test_two_nested_plus_two_non_overlapping(self):
            # One range contained in a second range, plus a two non-overlaps
            self.process2([ (1,4), (2,3), (5,6), (7,8) ], (2,3), 2)

        def test_two_partial_plus_two_non_overlapping(self):
            # Ranges that overlap on one side,plus a non-overlap
            self.process2([ (1,3), (2,4), (5,6), (7,8) ], (2,3), 2)

        def test_two_nested_plus_two_nested(self):
            # Two nested ranges plus two more nested ranges with no
            # overlap between the two nested ranges.  Both are equally
            # valid which gives the hull with two overlaps
            self.process2([ (1,4), (2,3), (5,8), (6,7) ], (2,7), 2)

        def test_two_partial_plus_two_nested(self):
            self.process2([ (1,3), (2,4), (5,8), (6,7) ], (2,7), 2)

        def test_two_nested_plus_three_non_overlapping(self):
            self.process2([ (1,4), (2,3), (5,6), (7,8), (9,10) ], (2,3), 2)

        def test_two_nested_plus_three_non_overlapping(self):
            self.process2([ (1,4), (2,3), (5,10), (6,7), (8,9) ], (2,9), 2)

        def test_two_nested_plus_three_nested(self):
            self.process2([ (1,4), (2,3), (5,10), (6,9), (7,8) ], (7,8), 3)

    print()
    unittest.main(verbosity = 2)
