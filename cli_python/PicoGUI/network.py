import socket, struct, os

base_port = 30450
magic = 0x31415926L
proto_version_min = 13
default_pgserver = 'localhost:0'


class Error(Exception): pass
class ProtocolError(Error): pass

def sock(address=None, display=None):
        if address is None:
                pgserver = os.environ.get('PGSERVER', default_pgserver).split(':')
                if len(pgserver) < 2:
                        # just the address
                        address = pgserver[0]
                        display = 0
                else:
                        address, display = pgserver
                        if not address:
                                # just the display
                                address = default_pgserver.split(':')[0]
                display = int(display)
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((address, base_port + display))
	hello = struct.unpack('!LHxx', s.recv(8))
	if hello[0] != magic:
		raise ProtocolError('Wrong magic!')
	if hello[1] < proto_version_min:
		raise ProtocolError('Possibly incompatible protocol')
	#s.setblocking(0)
	return s
