C Implementation
================

The C implementation of "vad är klockan" can be found in the directory
"src/c".  The same C source code is used both on Linux and on ESP32.

src/c/overlap_algorithm.[ch] is a port of the Python implementation in
python/overlap.py to C.

The roughtime implementation is based on a project called "vroughtime"
(https://github.com/oreparaz/vroughtime/).  vroughtime in turn uses
some code from another project called "craggy"
(https://github.com/nahojkap/craggy/).  "vroughtime" did not initially
support newer draft versions of roughtime so the code was modified to
also support draft version 07 and 06 of Roughtime.

The cryptographic primitives (Ed25516, SHA-512/256) used by vroughtime
come from the library "tweetnacl" (https://tweetnacl.cr.yp.to/).
Tweetnacl is in the public domain.

The remaingin files in the C implementation of "vad är klockan",
vroughtime and craggy are all licensed using the Apache License 2".

C API
-----

.. toctree::
   :maxdepth: 2

   c_overlap
   vrt
   tweetnacl

C on Linux
----------

A an example C implementation for Linux can be found in examples/c.
This implementation may also work on other Unix variants such as
MacOS.

Some glue code needed to be written to run on Linux and for UDP
communication via BSD sockets API.  The code supports both IPv4 and
IPv6.

.. todo:: Add instructions on how to compile and run the C code on Linux

C on ESP32
----------

The C implementation for the ESP32 can be found in examples/esp32.

Some glue code needed to be written to use WiFi and UDP on the ESP32.
The code so far only supports IPv4.

Using the esp32-c example

Install the Arduino development environment.  It is availabe on a
modern Debian distribution:

::
    sudo apt-get -y install arduino git

Start the Arduino environment, either by finding "Development" /
"Arduino IDE" in the start menus or by running the following from a
command line:

::
    arduino

Select "File" / "Preferences" and the following URL to "Additional
Boards Manager URLs":

https://dl.espressif.com/dl/package_esp32_index.json

Select "Tools" / "Board" / "Board Manager".  Search for "esp32" and
press "Install" on the one which says "by Expressif Systems".  Wait
for the download to finish.

Select "Tools" / "Board" / "ESP32 Arduino" / "ESP32 Dev Module".

Configure the correct serial port for your ESP32 device (usally
/dev/ttyACM0) using "Tools" / "Port".

Select "File" / "Open" and open "examples/esp32-c/esp32-c.ino".

Modify "login.h" with the name and password for your WiFi network.

Select "Sketch" / "Upload" to upload the sketch.

Connect to the serial port using terminal program to see the output
from the application running in the ESP32.

Using the ttgo-esp32-c example

Follow the instructions for the esp32-c example above on how to
install and configure the Arduino environment.

You will also have to add the TTGO display driver driver to your
Arduino environment.  First find your Arduino directory; on Linux it's
usually the "Arduino" directory in your home directory.  Check out a
git repository with the display driver and create a link to it.

::
    cd ~/Arduino
    git clone https://github.com/Xinyuan-LilyGO/TTGO-T-Display.git

    cd libraries
    ln -s ../TTGO-T-Display/TFT_eSPI .

Select "File" / "Open" and open
"examples/ttgo-esp32-c/ttgo-esp32-c.ino".

Modify "login.h" with the name and password for your WiFi network.

Select "Sketch" / "Upload" to upload the sketch.

Unit testing
------------

Some of the c modules contain unit tests.

The test for overlap_algorithm reuses the test cases from the Python
implementation.  The C code is built into a dynamic library which is
wrapped with a class which implements the same API as the Python
implementation.  The results from the Python and C implementations are
compared to see that they perform identically.  Additionally, some C
code to replay the same test caes is generated to overlap_replay.c
which is then compiled and run through valgrind to catch errors such
as buffer overruns or memory leaks.

To run the test cases, install the following packages on a Debian or
Ubuntu system:

.. highlight:: bash

::

    sudo apt-get install -y build-essential python3-cffi valgrind

Then run the test cases with:

::
    cd src/c
    python3 test_overlap_c.py
