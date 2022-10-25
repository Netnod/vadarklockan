# C implementation of "vad är klockan"

src/c/overlap_algorithm.[ch] is a port of the Python implementation in
python/overlap.py to C.

The roughtime implementation is based on a project called "vroughtime"
(https://github.com/oreparaz/vroughtime/).  vroughtime in turn uses
some code from another project called "craggy"
(https://github.com/nahojkap/craggy).

The cryptographic primitives (Ed25516, SHA-512/256) used by vroughtime
come from the library "tweetnacl" (https://tweetnacl.cr.yp.to/).
Tweetnacl is in the public domain (see LICENSE-tweetnacl.txt)

The remaining files in the C implementation of "vad är klockan",
vroughtime and craggy are all licensed using the Apache License 2"
(see LICENSE.txt).

For more information about how to use the C implemenation, see the
documentation.
