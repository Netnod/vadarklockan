Python Implementation
=====================

The Python implementation of "vad är klockan" can be found in the
directory "src/python".

The roughtime implementation is based on Marcus Dansarie's Python
implementation of roughtime called pyroughtime
(https://github.com/dansarie/pyroughtime).  Pyroughtime is licensed
using the "GNU GENERAL PUBLIC LICENSE Version 3".

Pyroughtime uses an ed25519 implementation written by Brian Warner
(https://github.com/warner/python-ed25519).  Python-ed25516 is in the
public domain.

All the remaining code of the Python implementation of "vad är
klockan" is licensed under the "BSD 3-Clause License".

Python API
----------

.. toctree::
   :maxdepth: 2

   python_overlap
   pyroughtime

Python on Linux
---------------

The Python 3 implementation for Linux can be found in examples/python.

This version should run on most modern Linux distributions with Python 3.

.. todo:: Add instructions on how to run the Python code on Linux.
          pip install commands for requirements etc

MicroPython on ESP32
--------------------

The Python implementation for TTGO ESP32 can be found in examples/micropython-esp32-ttgo.

This is mostly a "toy implementation" to show that it is possible to
run the Python code on a tiny platform.  In production it would be
better to port the C implementation of "vad är klockan" to the
Micropython environment and create a Python wrapper for it.

MicroPython on the TTGO ESP32 has very little free memory available
and alse differs quite a bit much from mainline Python.  In order to
accommodate this "pyroughtime" has had to be slimmed down and modified
considerably.

MicroPython for ESP32 has been extended with support for those
encryption primitives needed for Roughtime (Ed25519, SHA-512) and to
be able to set the clock from Python code (settimeofday).

.. todo:: Add instructions on how to run the Python code on ESP32
          including on where to get a modified MicroPython and pointer
          and how to use "run.py"

.. todo:: Currently the MicroPython implementation on ESP32 only
          supports roughtime draft version 06.  To make it work with
          roughtime draft version 07 support for SHA-512/256 would
          have to be added to MicroPython first.

Unit testing
------------

Some of the python classes contain unit tests.  The tests are based on
the Python "unittest" framework.  To perform those tests run:

.. highlight:: bash

::

    cd src/python
    python3 -m unittest test*.py

The Python code also contains some code examples in docstrings.  That
code is also used as test cases using the Sphinx doctest target.  See
:ref:`doctest` for more information.
