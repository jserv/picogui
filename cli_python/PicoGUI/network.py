import socket, struct

base_port = 30450
magic = 0x31415926L
proto_version_min = 13

class Error(Exception): pass
class ProtocolError(Error): pass

def sock(address='localhost', display=0):
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((address, base_port + display))
	hello = struct.unpack('!LHxx', s.recv(8))
	if hello[0] != magic:
		raise ProtocolError('Wrong magic!')
	if hello[1] < proto_version_min:
		raise ProtocolError('Possibly incompatible protocol')
	#s.setblocking(0)
	return s
