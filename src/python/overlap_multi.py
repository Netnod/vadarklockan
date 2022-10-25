#! /usr/bin/python3

class MultiOverlapAlgorithm(object):
    """An overlap algorithm which tries multiple algorithms and verifies
    that they all produce the same result.

    """

    def __init__(self, ranges = []):
        self.algos = [ algo(ranges) for algo in self.ALGOS ]

    def add(self, lo, hi):
        result = None
        for algo in self.algos:
            try:
                algo.add(lo, hi)
            except Exception as e:
                if result:
                    assert result.__class__ == e.__class__
                result = e
        if result:
            raise result

    def find(self):
        result = None
        for algo in self.algos:
            try:
                r = algo.find()
                if result:
                    assert result[0] == r[0]
                    assert result[1] == r[1] or abs(result[1] - r[1]) < 1E-9
                    assert result[2] == r[2] or abs(result[2] - r[2]) < 1E-9
                result = r

            except Exception as e:
                if result:
                    assert result.__class__ == e.__class__
                result = e
        if isinstance(result, Exception):
            raise result
        return result
