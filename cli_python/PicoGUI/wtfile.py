# Provides support to write widget template files
# obviously, not all requests will work, so you can't run a full app
# on these fake "servers"

import struct
idsize = struct.calcsize('!L')
# 2 below is for PG_RESPONSE_RET
null_response = struct.pack('!HxxLL', 2, 0, 0)

class stream(object):
  def __init__(self):
    self.s = ''
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
    response = struct.pack('!HxxLL', 2, id, handle + 0x800000L)
    self.pending_responses += response

  def send(self, request):
    id = struct.unpack('!L', request[:idsize])
    if id == 0xFFFFFFFFL:
      self.null_response()
    else:
      id = self.make_response(id)
      request = struct.pack('!L', id) + request[idsize:]
    # FIXME: how to tell global from instance requests?
    self.num_global += 1
    self.s += request

  def recv(self, size):
    if size > len(self.pending_responses):
      size = len(self.pending_responses)
    r = self.pending_responses[:size]
    self.pending_responses = self.pending_responses[size:]
    return r

  def dump(self):
    # dump to a string in PGwt format
    s = PGwt
    # FIXME: len and checksum are for the whole file?
    flen = 0
    checksum = 0
    version = 1
    s += struct.pack('!LLHHHH', 0, 0, 1,
                     self.num_global, self.num_instance, self.next_handle)
    s += self.s
    return s
