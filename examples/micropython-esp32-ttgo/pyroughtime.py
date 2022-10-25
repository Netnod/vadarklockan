import ued25519
import hashlib
import socket
import struct
import time
import os

class BadSignatureError(Exception):
    pass

class VerifyingKey(object):
    def __init__(self, vk_s):
        assert isinstance(vk_s, bytes)
        assert len(vk_s) == 32
        self.vk_s = vk_s

    def verify(self, sig, msg):
        assert isinstance(sig, bytes)
        assert isinstance(msg, bytes)
        assert len(sig) == 64
        sig_R = sig[:32]
        sig_S = sig[32:]
        sig_and_msg = sig_R + sig_S + msg
        # this might raise BadSignatureError
        msg2 = ued25519.open(sig_and_msg, self.vk_s)
        if msg2 is None:
            raise BadSignatureError
        assert msg2 == msg

class RoughtimeError(Exception):
    'Represents an error that has occured in the Roughtime client.'
    def __init__(self, message):
        super(RoughtimeError, self).__init__(message)

class RoughtimeClient:
    CERTIFICATE_CONTEXT = b'RoughTime v1 delegation signature--\x00'
    SIGNED_RESPONSE_CONTEXT = b'RoughTime v1 response signature\x00'

    '''
    Queries Roughtime servers for the current time and authenticates the
    replies.

    Args:
        max_history_len (int): The number of previous replies to keep.
    '''
    def __init__(self, max_history_len=100):
        self.prev_replies = []
        self.max_history_len = max_history_len

    @staticmethod
    def midp_to_datetime(midp):
        if midp == 0xffffffffffffffff:
            return None
        if midp < 30000000000000000:
            return midp / 1E6
        mjd = midp >> 40
        seconds = (midp & 0xffffffffff) / 1E6
        ret = (mjd - 40587) * 86400 + seconds
        return ret

    @staticmethod
    def __udp_query(address, port, packet, timeout):
        for family, type_, proto, canonname, sockaddr in \
                socket.getaddrinfo(address, port):
            sock = socket.socket(family, socket.SOCK_DGRAM)
            sock.settimeout(0.100)
            start_time = time.time()
            try:
                sock.sendto(packet, (sockaddr[0], sockaddr[1]))
            except Exception as ex:
                # Try next IP on failure.
                sock.close()
                continue

            # Wait for reply
            while True:
                try:
                    data, repl = sock.recvfrom(1500)
                    repl_addr = repl[0]
                    repl_port = repl[1]
                except OSError as e:
                    if not 'ETIMEDOUT' in str(e):
                        raise
                    if time.time() - start_time < timeout:
                        continue
                    raise RoughtimeError('Timeout while waiting for reply.')

                # TODO do an early uid check

                if repl_addr == sockaddr[0] and repl_port == sockaddr[1]:
                    break

            recv_time = time.time()
            rtt = recv_time - start_time
            sock.close()
            if rtt >= timeout:
                # Try next IP on timeout.
                continue
            # Break out of loop if successful.
            break

        reply = RoughtimePacket(packet=data)

        return reply, start_time, rtt, data

    def query(self, address, port, pubkey, timeout=2, newver=True,
            protocol='udp'):
        '''
        Sends a time query to the server and waits for a reply.

        Args:
            address (str): The server address.
            port (int): The server port.
            pubkey (str): The server's public key in base64 format.
            timeout (float): Time to wait for a reply from the server.
            newver (boolean): True if the server follows the most recent IETF
                    draft specification. Set to false for compatibility with
                    pre-IETF specifications.
            protocol (str): Either 'udp' or 'tcp'.

        Raises:
            RoughtimeError: On any error. The message will describe the
                    specific error that occurred.

        Returns:
            ret (dict): A dictionary with the following members:
                    midp       - midpoint (MIDP) in microseconds,
                    radi       - accuracy (RADI) in microseconds,
                    datetime   - a datetime object representing the returned
                                 midpoint,
                    prettytime - a string representing the returned time.
                    mint       - a datetime object representing the start of
                                 validity for the delegate key.
                    maxt       - a datetime object representing the end of
                                 validity for the delegate key.
                    pathlen    - the length of the Merkle tree path sent in
                                 the server's reply (0 <= pathlen <= 32).
        '''

        if protocol != 'udp' and protocol != 'tcp':
            raise RoughtimeError('Illegal protocol type.')

        pubkey = VerifyingKey(pubkey)

        # Generate nonce.
        blind = os.urandom(64)
        ha = hashlib.sha512()
        if len(self.prev_replies) > 0:
            ha.update(self.prev_replies[-1][2])
        ha.update(blind)
        nonce = ha.digest()
        if newver:
            nonce = nonce[:32]

        # Create query packet.
        packet = RoughtimePacket()
        if newver:
            packet.add_tag(RoughtimeTag('VER', RoughtimeTag.uint32_to_bytes(0x80000003)))
        packet.add_tag(RoughtimeTag('NONC', nonce))
        if protocol == 'udp':
            packet.add_padding()
        packet = packet.get_value_bytes(packet_header=newver)

        print("Trying %s:%s" % (address, port))

        if protocol == 'udp':
            reply, start_time, rtt, data = self.__udp_query(address, port, packet, timeout)
        else:
            reply, start_time, rtt, data = self.__tcp_query(address, port, packet, timeout)

        # Get reply tags.
        srep = reply.get_tag('SREP')
        cert = reply.get_tag('CERT')
        if srep == None or cert == None:
            raise RoughtimeError('Missing tag in server reply.')
        dele = cert.get_tag('DELE')
        if dele == None:
            raise RoughtimeError('Missing tag in server reply.')
        nonc = reply.get_tag('NONC')
        if newver:
            if nonc == None:
                raise RoughtimeError('Missing tag in server reply.')
            if nonc.get_value_bytes() != nonce:
                raise RoughtimeError('Bad NONC in server reply.')

        try:
            dsig = cert.get_tag('SIG').get_value_bytes()
            dtai = srep.get_tag('DTAI')
            leap = srep.get_tag('LEAP')
            midp = srep.get_tag('MIDP').to_int()
            radi = srep.get_tag('RADI').to_int()
            root = srep.get_tag('ROOT').get_value_bytes()
            sig = reply.get_tag('SIG').get_value_bytes()
            indx = reply.get_tag('INDX').to_int()
            path = reply.get_tag('PATH').get_value_bytes()
            pubk = dele.get_tag('PUBK').get_value_bytes()
            mint = dele.get_tag('MINT').to_int()
            maxt = dele.get_tag('MAXT').to_int()

        except:
            raise RoughtimeError('Missing tag in server reply or parse error.')

        if dtai != None:
            dtai = dtai.to_int()

        if leap != None:
            leapbytes = leap.get_value_bytes()
            leap = []
            while len(leapbytes) > 0:
                leap.append(struct.unpack('<I', leapbytes[:4])[0] & 0x7fffffff)
                leapbytes = leapbytes[4:]

        # Verify signature of DELE with long term certificate.
        try:
            pubkey.verify(dsig, self.CERTIFICATE_CONTEXT
                    + dele.get_received())
        except Exception as e:
            print("signature verification failed", e)
            raise RoughtimeError('Verification of long term certificate '
                    + 'signature failed.')

        # Verify that DELE timestamps are consistent with MIDP value.
        if mint > midp or maxt < midp:
            raise RoughtimeError('MIDP outside delegated key validity time.')

        if newver:
            node_size = 32
        else:
            node_size = 64

        # Ensure that Merkle tree is correct and includes nonce.
        curr_hash = hashlib.sha512(b'\x00' + nonce).digest()[:node_size]
        if len(path) % node_size != 0:
            raise RoughtimeError('PATH length not a multiple of %d.' \
                    % node_size)
        pathlen = len(path) // node_size
        if pathlen > 32:
            raise RoughtimeError('Too many paths in Merkle tree.')

        while len(path) > 0:
            if indx & 1 == 0:
                curr_hash = hashlib.sha512(b'\x01' + curr_hash
                        + path[:node_size]).digest()
            else:
                curr_hash = hashlib.sha512(b'\x01' + path[:node_size]
                        + curr_hash).digest()
            curr_hash = curr_hash[:node_size]
            path = path[node_size:]
            indx >>= 1

        if indx != 0:
            raise RoughtimeError('INDX not zero after traversing PATH.')
        if curr_hash != root:
            raise RoughtimeError('Final Merkle tree value not equal to ROOT.')

        # Verify that DELE signature of SREP is valid.
        delekey = VerifyingKey(pubk)
        try:
            delekey.verify(sig, self.SIGNED_RESPONSE_CONTEXT
                    + srep.get_received())
        except:
            raise RoughtimeError('Bad DELE key signature.')

        self.prev_replies.append((nonce, blind, data))
        while len(self.prev_replies) > self.max_history_len:
            self.prev_replies = self.prev_replies[1:]

        # Return results.
        ret = dict()
        ret['midp'] = midp
        ret['radi'] = radi
        ret['timestamp'] = RoughtimeClient.midp_to_datetime(midp)
        # timestr = ret['datetime'].strftime('%Y-%m-%d %H:%M:%S.%f')
        timestr = repr(time.gmtime(int(ret['timestamp'])))
        if radi < 10000:
            ret['prettytime'] = "%s UTC (+/- %.3f ms)" % (timestr, radi / 1E3)
        else:
            ret['prettytime'] = "%s UTC (+/- %.3f  s)" % (timestr, radi / 1E6)
        ret['start_time'] = start_time
        ret['rtt'] = rtt
        ret['mint'] = RoughtimeClient.midp_to_datetime(mint)
        ret['maxt'] = RoughtimeClient.midp_to_datetime(maxt)
        ret['pathlen'] = pathlen
        if dtai != None:
            ret['dtai'] = dtai
        if leap != None:
            ret['leap'] = leap
        return ret

class RoughtimeTag:
    '''
    Represents a Roughtime tag in a Roughtime message.

    Args:
        key (str): A Roughtime key. Must me less than or equal to four ASCII
                characters. Values shorter than four characters are padded with
                NULL characters.
        value (bytes): The tag's value.
    '''
    def __init__(self, key, value=b''):
        if len(key) > 4:
            raise ValueError
        while len(key) < 4:
            key += '\x00'
        self.key = key
        assert len(value) % 4 == 0
        self.value = value

    def __repr__(self):
        'Generates a string representation of the tag.'
        tag_uint32 = struct.unpack('<I', RoughtimeTag.tag_str_to_uint32(self.key))[0]
        ret = 'Tag: %s (0x%08x)\n' % (self.get_tag_str(), tag_uint32)
        if self.get_value_len() == 4 or self.get_value_len() == 8:
            ret += "Value: %d\n" % self.to_int()
        ret += "Value bytes:\n"
        num = 0
        for b in self.get_value_bytes():
            ret += '%02x' % b
            num += 1
            if num % 16 == 0:
                ret += '\n'
            else:
                ret += ' '
        if ret[-1] == '\n':
            pass
        elif ret[-1] == ' ':
            ret = ret[:-1] + '\n'
        else:
            ret += '\n'
        return ret

    def get_tag_str(self):
        'Returns the tag key string.'
        return self.key

    def get_tag_bytes(self):
        'Returns the tag as an encoded uint32.'
        assert len(self.key) == 4
        return RoughtimeTag.tag_str_to_uint32(self.key)

    def get_value_len(self):
        'Returns the number of bytes in the tag\'s value.'
        return len(self.get_value_bytes())

    def get_value_bytes(self):
        'Returns the bytes representing the tag\'s value.'
        assert len(self.value) % 4 == 0
        return self.value

    def set_value_bytes(self, val):
        assert len(val) % 4 == 0
        self.value = val

    def set_value_uint32(self, val):
        self.value = struct.pack('<I', val)

    def set_value_uint64(self, val):
        self.value = struct.pack('<Q', val)

    def to_int(self):
        '''
        Converts the tag's value to an integer, either uint32 or uint64.

        Raises:
            ValueError: If the value length isn't exactly four or eight bytes.
        '''
        vlen = len(self.get_value_bytes())
        if vlen == 4:
            (val,) = struct.unpack('<I', self.value)
        elif vlen == 8:
            (val,) = struct.unpack('<Q', self.value)
        else:
            raise ValueError
        return val

    @staticmethod
    def tag_str_to_uint32(tag):
        'Converts a tag string to its uint32 representation.'
        return struct.pack('BBBB', ord(tag[0]), ord(tag[1]), ord(tag[2]),
                ord(tag[3]))

    @staticmethod
    def tag_uint32_to_str(tag):
        'Converts a tag uint32 to it\'s string representation.'
        return chr(tag & 0xff) + chr((tag >> 8) & 0xff) \
                + chr((tag >> 16) & 0xff) + chr(tag >> 24)

    @staticmethod
    def uint32_to_bytes(val):
        return struct.pack('<I', val)

    @staticmethod
    def uint64_to_bytes(val):
        return struct.pack('<Q', val)

class RoughtimePacket(RoughtimeTag):
    '''
    Represents a Roughtime packet.

    Args:
        key (str): The tag key value of this packet. Used if it was contained
                in another Roughtime packet.
        packet (bytes): Bytes received from a Roughtime server that should be
                parsed. Set to None to create an empty packet.

    Raises:
        RoughtimeError: On any error. The message will describe the specific
                error that occurred.
    '''
    def __init__(self, key='\x00\x00\x00\x00', packet=None):
        self.tags = []
        self.key = key
        self.packet = None

        # Return if there is no packet to parse.
        if packet == None:
            return

        self.packet = packet

        if len(packet) % 4 != 0:
            raise RoughtimeError('Packet size is not a multiple of four.')

        if RoughtimePacket.unpack_uint64(packet, 0) == 0x4d49544847554f52:
            if len(packet) - 12 != RoughtimePacket.unpack_uint32(packet, 8):
                raise RoughtimeError('Bad packet size.')
            packet = packet[12:]

        num_tags = RoughtimePacket.unpack_uint32(packet, 0)
        headerlen = 8 * num_tags
        if headerlen > len(packet):
            raise RoughtimeError('Bad packet size.')
        # Iterate over the tags.
        for i in range(num_tags):
            # Tag value offset.
            if i == 0:
                offset = headerlen
            else:
                offset = RoughtimePacket.unpack_uint32(packet, i * 4) \
                        + headerlen
            if offset > len(packet):
                raise RoughtimeError('Bad packet size.')

            # Tag value end.
            if i == num_tags - 1:
                end = len(packet)
            else:
                end = RoughtimePacket.unpack_uint32(packet, (i + 1) * 4) \
                        + headerlen
            if end > len(packet):
                raise RoughtimeError('Bad packet size.')

            # Tag key string.
            key = RoughtimeTag.tag_uint32_to_str(
                    RoughtimePacket.unpack_uint32(packet, (num_tags + i) * 4))

            value = packet[offset:end]

            leaf_tags = ['SIG\x00', 'INDX', 'PATH', 'ROOT', 'MIDP', 'RADI',
                    'PAD\x00', 'PAD\xff', 'NONC', 'MINT', 'MAXT', 'PUBK',
                    'VER\x00', 'DTAI', 'DUT1', 'LEAP']
            parent_tags = ['SREP', 'CERT', 'DELE']
            if self.contains_tag(key):
                raise RoughtimeError('Encountered duplicate tag: %s' % key)
            if key in leaf_tags:
                self.add_tag(RoughtimeTag(key, packet[offset:end]))
            elif key in parent_tags:
                # Unpack parent tags recursively.
                self.add_tag(RoughtimePacket(key, packet[offset:end]))
            else:
                raise RoughtimeError('Encountered unknown tag: %s' % key)

    def add_tag(self, tag):
        '''
        Adds a tag to the packet:

        Args:
            tag (RoughtimeTag): the tag to add.

        Raises:
            RoughtimeError: If a tag with the same key already exists in the
                    packet.
        '''
        for t in self.tags:
            if t.get_tag_str() == tag.get_tag_str():
                raise RoughtimeError('Attempted to add two tags with same key '
                        + 'to RoughtimePacket.')
        self.tags.append(tag)
        self.tags.sort(key=lambda x: struct.unpack('<I', x.get_tag_bytes()))

    def verify_replies(self):
        '''
        Verifies replies from servers that have been received by the instance.

        Returns:
            ret (list): A list of pairs containing the indexes of any invalid
                    pairs. An empty list indicates that no replies appear to
                    violate causality.
        '''
        invalid_pairs = []
        for i in range(len(self.prev_replies)):
            packet_i = RoughtimePacket(packet=self.prev_replies[i][2])
            midp_i = RoughtimeClient.midp_to_datetime(\
                    packet_i.get_tag('SREP').get_tag('MIDP').to_int())
            radi_i = datetime.timedelta(microseconds=packet_i.get_tag('SREP')\
                    .get_tag('RADI').to_int())
            for k in range(i + 1, len(self.prev_replies)):
                packet_k = RoughtimePacket(packet=self.prev_replies[k][2])
                midp_k = RoughtimeClient.midp_to_datetime(\
                        packet_k.get_tag('SREP').get_tag('MIDP').to_int())
                radi_k = datetime.timedelta(microseconds=\
                        packet_k.get_tag('SREP').get_tag('RADI').to_int())
                if midp_i - radi_i > midp_k + radi_k:
                    invalid_pairs.append((i, k))
        return invalid_pairs

    def contains_tag(self, tag):
        '''
        Checks if the packet contains a tag.

        Args:
            tag (str): The tag to check for.

        Returns:
            boolean
        '''
        if len(tag) > 4:
            raise ValueError
        while len(tag) < 4:
            tag += '\x00'
        for t in self.tags:
            if t.get_tag_str() == tag:
                return True
        return False

    def get_tag(self, tag):
        '''
        Gets a tag from the packet.

        Args:
            tag (str): The tag to get.

        Return:
            RoughtimeTag or None.
        '''
        if len(tag) > 4:
            raise RoughtimeError('Invalid tag key length.')
        while len(tag) < 4:
            tag += '\x00'
        for t in self.tags:
            if t.get_tag_str() == tag:
                return t
        return None

    def get_tags(self):
        'Returns a list of all tag keys in the packet.'
        return [x.get_tag_str() for x in self.tags]

    def get_num_tags(self):
        'Returns the number of keys in the packet.'
        return len(self.tags)

    def get_value_bytes(self, packet_header=False):
        'Returns the raw byte string representing the value of the tag.'
        packet = struct.pack('<I', len(self.tags))
        offset = 0
        for tag in self.tags[:-1]:
            offset += tag.get_value_len()
            packet += struct.pack('<I', offset)
        for tag in self.tags:
            packet += tag.get_tag_bytes()
        for tag in self.tags:
            packet += tag.get_value_bytes()
        assert len(packet) % 4 == 0
        if packet_header:
            packet = struct.pack('<QI', 0x4d49544847554f52, len(packet)) + packet
        return packet

    def get_received(self):
        return self.packet

    def add_padding(self):
        '''
        Adds a padding tag to ensure that the packet is larger than 1024 bytes,
        if necessary. This method should be called before sending a request
        packet to a Roughtime server.
        '''
        packetlen = len(self.get_value_bytes())
        if packetlen >= 1024:
            return
        padlen = 1016 - packetlen
        # Transmit "PAD\xff" instead of "PAD" for compatibility with older
        # servers that do not properly ignore unknown tags in queries.
        self.add_tag(RoughtimeTag('PAD\xff', b'\x00' * padlen))

    @staticmethod
    def unpack_uint32(buf, offset):
        'Utility function for parsing server replies.'
        return struct.unpack('<I', buf[offset:offset + 4])[0]

    @staticmethod
    def unpack_uint64(buf, offset):
        'Utility function for parsing server replies.'
        return struct.unpack('<Q', buf[offset:offset + 8])[0]
