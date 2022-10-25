# Vad är klockan example based on pyroughtime

This is an example based on Marcus Dansarie's Python implementation of
roughtime called
[pyroughtime](https://github.com/dansarie/pyroughtime).

This example will work with a modified version of [MicroPython for the
ESP32 TTGO
platform](https://github.com/wingel/MicroPython_ESP32_psRAM_LoBo ).
The modifications add support for ed25519 and sha512 which are
required by roughtime.

pyroughtime.py in this example has been heavily modified, everything
not neccesary for a roughtime client has been stripped out.  Since
MicroPython does not have the datetime module it has been replaced
with unix time_t variants.  The ed25519 and sha512 calls use smaller
variants found in the modified version of MicroPython.

overlap.py is an implementation of the "find overlaps" part of the
selection and clustering algorithm RFC5905.  It is the same as in
../examples/overlap.py.

algotest.py is finally the implementation of "vad är klockan" which
uses rougthime in combination with the find overlaps algorithm to find
out how to adhust a local clock in a reasonably secure way.

# Stuff in swedish

TODO this should be translated to english

Installera lite saker som behövs

```
pip3 install adafruit-ampy esptool python-is-python3
```

Hämta micropython med stöd för ed25519 och sha512

```
git clone https://github.com/wingel/MicroPython_ESP32_psRAM_LoBo
```

Flasha ESP32 med MicroPython

```
esptool.py -p /dev/ttyACM0 -b 921600 erase_flash
cd MicroPython_BUILD/firmware/esp32_psram_all_ed25519_sha512
../flash.sh -p /dev/ttyACM0 -b 921600
```

Bygg mpy_cross

```
cd MicroPython_BUILD/components/mpy_cross_build
make
```

Kopiera "rt_config.py.example" till "rt_config.py" och ändra den filen
så att den har rätt WLAN_SSID och WLAN_PASSWORD, samt ladda upp den
och "ecosystem.json" till filsystemet på ESP32:

```
ampy --port /dev/ttyACM0 --baud 115200 put rt_config.py /flash/rt_config.py
ampy --port /dev/ttyACM0 --baud 115200 put ecosystem.json /flash/ecosystem.json
```

Korskompilera "rt.py" till "rt.mpy" och ladda upp den till filsystemet
på ESP32:

```
MicroPython_BUILD/components/mpy_cross_build/mpy-cross/mpy-cross rt.py
ampy --port /dev/ttyACM0 --baud 115200 put rt.mpy /flash/rt.mpy
```

Nu kan man köra "rt_main.py" via thonny.

Alternativt så kan man använda "run.py" för att både kors-kompilera
och ladda upp filer till ESP32.  Modifiera konfigurationen i början på
run.py så att sökvägarna till /dev/ttyACM0 och mpy-cross stämmer.

```
python3 run.py
```
