import socket, struct, os

base_port = 30450
magic = 0x31415926L
proto_version_min = 21
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
    if display is None:
        display = 0
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    except:
        pass
    s.connect((address, base_port + display))
    hello = struct.unpack('!LHxx', s.recv(8))
    if hello[0] != magic:
        raise ProtocolError('Wrong magic!')
    if hello[1] < proto_version_min:
        raise ProtocolError('Possibly incompatible protocol')
    #s.setblocking(0)
    return s

#FIXME: make this conditional; in case of import error, define a class based on select
import select

def poll_for(connection):
    p = select.poll()
    p.register(connection, select.POLLIN)
    return p
