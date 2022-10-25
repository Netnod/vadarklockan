#from __future__ import division, print_function, unicode_literals

import machine, time, network
import binascii
import json
import sys
import json
import random
import os
import gc

from pyroughtime import RoughtimeClient

from overlap import OverlapAlgorithm, OptimizedOverlapAlgorithm

class Server(object):
    """Glue betweeen roughtime and measurements handed to the algorithm"""

    def __init__(self, cl, j):
        self.cl = cl
        self.proto = j['addresses'][0]['protocol']
        assert j['publicKeyType'] == 'ed25519'
        assert self.proto in [ 'udp', 'tcp' ]
        self.newver = j.get('newver', False)
        self.addr, self.port = j['addresses'][0]['address'].split(':')
        self.port = int(self.port)
        self.pubkey = binascii.a2b_base64(j['publicKey'])

    def __repr__(self):
        return "%s:%s" % (self.addr, self.port)

    def query(self):
        repl = self.cl.query(self.addr, self.port, self.pubkey, 2, self.newver, self.proto)

        self.transmit_time = repl['start_time']
        self.receive_time = self.transmit_time + repl['rtt']
        self.remote_time = repl['timestamp']
        # TODO check that radi >= 0
        self.radi = repl['radi']*1E-6

    def get_meas(self):
        # Round trip time
        rtt = self.receive_time - self.transmit_time

        # Local clock midway between transmit_time and receive_time
        local_time = self.transmit_time + rtt / 2

        # Calculate a local clock adjustment
        adjustment = self.remote_time - local_time

        # The uncertainty is half the round trip time plus the server uncertainty (radi)
        uncertainty = rtt / 2 + self.radi

        return local_time, rtt, adjustment, uncertainty

    def get_adjustment_range(self):
        """Return an adjustment range to overlap algorithm"""
        local_time, rtt, adjustment, uncertainty = self.get_meas()

        # Return lowest and highest adjustments
        return adjustment - uncertainty, adjustment + uncertainty

def random_shuffle(seq):
    l = len(seq)
    for i in range(l):
        j = random.randrange(l)
        seq[i], seq[j] = seq[j], seq[i]

def format_time(t):
    tint = int(t)
    tfrac = t - tint
    gm = time.gmtime(tint)
    return "%04u-%02u-%02u %02u:%02u:%06.3f" % (
        gm[0], gm[1], gm[2], gm[3], gm[4], gm[5] + tfrac)

def main():
    # Query a list of servers in a JSON file.
    with open('ecosystem.json') as f:
        servers = json.load(f)['servers']

    cl = RoughtimeClient()

    servers = [ Server(cl, _) for _ in servers ]

    # Try all servers a couple of times simulating that we have more
    # The first 7 servers should be good, the rest are falsetickers,
    # so use the good ones more often so that we will have a majority
    # of good ones.
    servers = servers[:7] + servers[7:12]

    random_shuffle(servers)

    # We want a majority where at least this many servers agree
    wanted = 5

    algorithm = OptimizedOverlapAlgorithm()

    responses = 0
    last_res = 0

    for server in servers:
        # Force garbage collection to run
        gc.collect()
        print("memory free", gc.mem_free())

        if 0 and server.addr.startswith('false'):
            continue

        if 0 and 'netnod' not in server.addr:
            continue

        try:
            server.query()

            local_time, rtt, adjustment, uncertainty = server.get_meas()

            print('%-38s local %s remote %s adj %9.0f us rtt %7.0f us radi %7.0f us' % (
                server, format_time(local_time), format_time(server.remote_time),
                adjustment * 1E6, rtt * 1E6, server.radi * 1E6))

        except Exception as e:
            print('%-38s Exception: %s' % (server, e))
            print(e.__class__)
            if 0:
                raise
            continue

        responses += 1
        lo, hi = server.get_adjustment_range()
        res = algorithm.handle(lo, hi)
        if res >= responses // 2 and res >= wanted:
            adjustment = (algorithm.hi + algorithm.lo) / 2
            uncertainty = (algorithm.hi - algorithm.lo) / 2

            print("success with %u/%u clocks adjust %+.0f us uncertainty %.0f us" % (
                res, responses,
                adjustment * 1E6,
                uncertainty * 1E6))
            print()

            if 1:
                # This is a good place to set the clock and break out of the loop
                t0 = time.time()
                tn = t0 + adjustment
                time.settime(tn)
                t1 = time.time()
                print("adjust %+.0f us" % (adjustment * 1E6))
                print("t0", format_time(t0), t0)
                print("tn", format_time(tn), tn)
                print("t1", format_time(t1), t1)
                break

            # Getting more responses should always give more overlaps, never fewer
            assert last_res <= res
            last_res = res

            # assert algorithm.adjustment < 1E-3

# Set up WIFI on ESP32

import wifi_config

sta = network.WLAN(network.STA_IF)
if not sta.active():
    sta.active(True)

    # print(sta.scan())
    sta.connect(wifi_config.WLAN_SSID, wifi_config.WLAN_PASSWORD)

while not sta.isconnected():
    pass

print(sta.ifconfig())

main()
