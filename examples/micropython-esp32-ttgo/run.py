#! /usr/bin/python3

"""
cd /export/MicroPython_ESP32_psRAM_LoBo/MicroPython_BUILD/components/mpy_cross_build/mpy-cross
make

pip3 install adafruit-ampy
"""

import ast
import binascii
import platform
import textwrap
import hashlib
import sys
import os

import ampy.files as files
import ampy.pyboard as pyboard
from ampy.pyboard import PyboardError

########################################################################
# Configuration

MPY_CROSS = '/export/MicroPython_ESP32_psRAM_LoBo/MicroPython_BUILD/components/mpy_cross_build/mpy-cross/mpy-cross'
ESP_PORT = '/dev/ttyACM0'
ESP_BAUD = 115200
ESP_DELAY = 0

# Top python code to run on the ESP32
TOP = 'algotest.py'

# Python modules and other dependencies that should be uploaded to the
# file system on the ESP32.
#
# The tool calculates the SHA256 hash of each file on the host and on
# the ESP32 and only uploads them if the file is missing or the hash
# differs.  This speeds up things significantly if the files are large.
#
# If any filenames end with ".py" or ".mpy" are modified, the tool
# resets the ESP32 (need_reset) since there does not seem to be
# possible to reload a module from within micropython.
#
# If a filename ends with ".mpy" the tool will try to run MPY_CROSS on
# the corresponding ".py" file first.  This will speed up loading of
# modules slightly and can also allow larger python modules since
# micropython often will run out of memory with plain ".py" modules.
DEPENDENCIES = [ 'pyroughtime.mpy', 'overlap.py', 'wifi_config.py', 'ecosystem.json' ]

########################################################################
# The following code is based on code from ampy.  See the MIT license
# in the ampy library for more information.

global _board
# On Windows fix the COM port path name for ports above 9 (see comment in
# windows_full_port_name function).
if platform.system() == "Windows":
    port = windows_full_port_name(port)
_board = pyboard.Pyboard(ESP_PORT, baudrate = ESP_BAUD, rawdelay = ESP_DELAY)

class RunHelper(object):
    BUFFER_SIZE = 256  # Amount of data to read or write to the serial port at a time.
    # This is kept small because small chips and USB to serial

    # bridges usually have very small buffers.

    def __init__(self, pyboard):
        self._pyboard = pyboard

    def get_hashes(self, fns):
        command = textwrap.dedent('''\
            def _(fns):
                try:
                    import os
                except ImportError:
                    import uos as os
                from hashlib import sha256
                import binascii

                result = {}
                for fn in fns:
                    try:
                        hf = sha256()
                        with open(fn, 'rb') as f:
                            while 1:
                                data = f.read(%u)
                                if not data:
                                    break
                                hf.update(data)
                        h = hf.digest()
                    except:
                         h = None
                    result[fn] = h
                return result
            print(_(%s))
        ''') % (1024, repr(fns))
        self._pyboard.enter_raw_repl()
        try:
            # print(command)
            out = self._pyboard.exec_(command)
        except PyboardError as ex:
            # Check if this is an OSError #2, i.e. directory doesn't exist and
            # rethrow it as something more descriptive.
            if ex.args[2].decode("utf-8").find("OSError: [Errno 2] ENOENT") != -1:
                raise RuntimeError("No such directory: {0}".format(directory))
            else:
                raise ex
        finally:
            self._pyboard.exit_raw_repl()
        # Parse the result list and return it.
        return eval(out.decode("utf-8"))

    def put(self, filename, data):
        """Create or update the specified file with the provided data.
        """
        # Open the file for writing on the board and write chunks of data.
        self._pyboard.enter_raw_repl()
        self._pyboard.exec_("f = open(%s, 'wb')" % repr(filename))
        size = len(data)

        # Loop through and write a buffer size chunk of data at a time.
        for i in range(0, size, self.BUFFER_SIZE):
            chunk_size = min(self.BUFFER_SIZE, size - i)
            chunk = repr(data[i : i + chunk_size])

            # Make sure to send explicit byte strings (handles python 2 compatibility).
            if not chunk.startswith("b"):
                chunk = "b" + chunk
            self._pyboard.exec_("f.write({0})".format(chunk))
        self._pyboard.exec_("f.close()")
        self._pyboard.exit_raw_repl()

    def reset(self):
        self._pyboard.enter_raw_repl()
        self._pyboard.exec_raw_no_follow('import machine\nmachine.reset()') # , stream_output = True)

    def run(self, fn):
        self._pyboard.enter_raw_repl()
        self._pyboard.execfile(fn, stream_output = True)

########################################################################

def main():
    print()

    helper = RunHelper(_board)

    hashes = helper.get_hashes(DEPENDENCIES)
    need_reset = False

    for fn in DEPENDENCIES:
        if fn.endswith('.mpy'):
            py_fn = fn[:-4] + '.py'
            mpy_fn = fn
            compile = False
            if os.path.exists(fn):
                mpy_st = os.stat(fn)
                py_st = os.stat(py_fn)
                if mpy_st.st_mtime < py_st.st_mtime:
                    compile = True
            else:
                compile = True
            if compile:
                print("compiling %s -> %s" % (py_fn, mpy_fn))
                s = os.system('%s \'%s\'' % (MPY_CROSS, py_fn))
                if s:
                    raise ValueError("%s: failed to compile" % py_fn)

        with open(fn, 'rb') as f:
            data = f.read()
        h = hashlib.sha256(data).digest()
        if h != hashes.get(fn):
            print("%s: modified" % fn)
            if fn.endswith('.py') or fn.endswith('.mpy'):
                need_reset = True
            helper.put(fn, data)
        else:
            print("%s: no change" % fn)

    if need_reset:
        print("resetting")
        helper.reset()
    print("run %s" % TOP)
    helper.run(TOP)

if __name__ == '__main__':
    main()
