# Provides support to write widget template files
# obviously, not all requests will work, so you can't run a full app
# on these fake "servers"

import struct
# zlib is not present in some installations, specially embedded
try:
  import zlib
except:
  zlib = None

idsize = struct.calcsize('!L')
# 2 below is for PG_RESPONSE_RET
null_response = struct.pack('!HxxLL', 2, 0, 0)

class stream(object):
  def __init__(self):
    self.instance = ''
    self.globals = ''
    self.pending_responses = ''
    self.next_handle = 0
    self.num_global = self.num_instance = 0

  def null_response(self):
    self.pending_responses += null_response

  def make_response(self, id):
    # remap IDs to make them sequential (to save server memory)
    # and prepare the response
    handle = self.next_handle
    self.next_handle += 1
    # 2 below is for PG_RESPONSE_RET
    response = struct.pack('!HxxLL', 2, handle, handle + 0x80000000L)
    self.pending_responses += response
    return handle

  def send(self, request):
    id = struct.unpack('!L', request[:idsize])
    if id == 0xFFFFFFFFL:
      self.null_response()
    else:
      id = self.make_response(id)
      request = struct.pack('!L', id) + request[idsize:]
    # FIXME: mkstring, mkfont and mkbitmap are global
    # (this requires decoding the packet, sigh
    self.num_instance += 1

    # Pad the data to the next 32-bit boundary
    if (len(request)&3) != 0:
      request += "\0" * (4 - (len(request)&3))

    self.instance += request

  def recv(self, size):
    if size > len(self.pending_responses):
      size = len(self.pending_responses)
    r = self.pending_responses[:size]
    self.pending_responses = self.pending_responses[size:]
    return r

  def dump(self):
    # dump to a string in PGwt format
    s = 'PGwt'
    fmt = '!LLHHHH'
    s1 = s
    version = 1
    if zlib is not None:
      flen = len(s) + len(self.globals) + len(self.instance) + struct.calcsize(fmt)
      s1 += struct.pack(fmt, flen, 0, version,
                        self.num_global, self.num_instance, self.next_handle)
      s1 += self.globals + self.instance
      checksum = zlib.crc32(s1)
    else:
      flen = 0
      checksum = 0
    s += struct.pack(fmt, flen, checksum, version,
                     self.num_global, self.num_instance, self.next_handle)
    s += self.globals + self.instance
    return s
