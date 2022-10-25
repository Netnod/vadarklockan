#! /usr/bin/python3

class ReferenceOverlapAlgorithm(object):
    """Reference algoritm to finds an overlapping range.

    This algorithm is based on the selection & clustering algorithm
    from RFC5905 Appendix A.5.5.1.
    """

    def __init__(self, ranges = []):
        self._edges = []
        self._responses = 0

        for lo, hi in ranges:
            self.add(lo, hi)

    def add(self, lo, hi):
        if lo > hi:
            raise ValueError("lo must not be higher than hi")

        # Add the edges to our list of edges and sort the list
        self._edges.append((lo, -1))
        self._edges.append((hi, +1))
        self._edges.sort()

        self._responses += 1

    def find(self):
        if not self._responses:
            return 0, None, None

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

            if lo is not None and hi is not None:
                break

        return wanted, lo, hi
