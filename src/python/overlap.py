#! /usr/bin/python3

class OverlapAlgorithm(object):
    """Optimized algoritm to find an overlapping range.

    Ranges are first added using :py:func:`add()`.  To find the
    overlapping range, call :py:func:`find`:

    >>> from overlap import OverlapAlgorithm
    >>> algo = OverlapAlgorithm()
    >>> algo.add(1, 5)
    >>> algo.add(3, 9)
    >>> count, lo, hi = algo.find()
    >>> print((count, lo, hi))
    (2, 3, 5)

    The first value returned by the method, count, is the number of
    overlaps.  The second and third, lo and hi, are the common overlap
    of all ranges.

    It's possible to iteratively add additional ranges and call
    :py:func:`find` again:

    >>> algo.add(4, 5)
    >>> print(algo.find())
    (3, 4, 5)

    If there are multiple overlapping ranges the one with the highest
    number of overlaps will be returned:

    >>> algo.add(10, 20)
    >>> algo.add(12, 18)
    >>> print(algo.find())
    (3, 4, 5)

    If there are multiple overlapping ranges with the same number of
    overlaps, the boundaries of both ranges will be returned,

    >>> algo.add(13, 17)
    >>> print(algo.find())
    (3, 4, 17)

    It's up to the user to interpret this properly.  One userfule
    requirement is that a majority of the ranges have to overlap.

    >>> ranges = [ (1,5), (3,9), (4,5), (10,20), (12,18) ]
    >>> algo = OverlapAlgorithm(ranges)
    >>> count, lo, hi = algo.find()
    >>> print(len(ranges))
    5
    >>> print((count, lo, hi))
    (3, 4, 5)
    >>> assert count > len(ranges) // 2

    This algorithm is based on the selection & clustering algorithm
    from RFC5905 Appendix A.5.5.1.

    """

    def __init__(self, ranges = []):
        """Create a new OverlapAlgorithm object.

        Args:
            ranges (list): Optional list of "ranges" that should be
                added.  Each range is a tuple on the form (lo, hi)
        """

        self._edges = []
        self._wanted = 0

        for lo, hi in ranges:
            self.add(lo, hi)

    def add(self, lo, hi):
        """Add a range.

        Args:
            lo: low value for the range

            hi: high value for the range

        Raises:
            ValueError: if lo >= hi
        """

        if lo > hi:
            raise ValueError("lo must not be higher than hi")

        # Add the edges to our list of edges and sort the list
        self._edges.append((lo, -1))
        self._edges.append((hi, +1))
        self._edges.sort()

        # Increase the number of possible overlaps
        self._wanted += 1

    def find(self):
        """Find overlap between ranges.

        Returns:
            A tuple with the following values:
                count: number of ranges in returned overlap

                lo: low value for overlap

                hi: high value for overlap
        """

        if not self._wanted:
            return 0, None, None

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

        return self._wanted, lo, hi
