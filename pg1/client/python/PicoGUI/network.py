import socket, struct, os

__all__ = 'Error', 'ProtocolError', 'sock', 'poll_for'

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

try:
    import select
except ImportError:
    select = None

try:
    poll = select.poll
except AttributeError:
    poll = None

if poll:
    # A wrapper around select.poll that lets us use strings rather than bitmasks for register(),
    # so the user of this class doesn't have to know about the select module. This can't be a
    # subclass, since select.poll is a builtin.
    class poll_for:
        def __init__(self, connection):
            self._p = poll()
            self.register(connection, 'r')

        def register(self, file, modes):
            eventmask = 0
            for char in modes:
                if char == 'w':
                    eventmask |= select.POLLOUT
                elif char == 'r':
                    eventmask |= select.POLLIN
                elif char == 'p':
                    eventmask |= select.POLLPRI
            self._p.register(file, eventmask)

        def unregister(self, file):
            self._p.unregister(file)

        def poll(self, timeout=None):
            result = self._p.poll(timeout)
            if result == None:
                return None
            for i in range(0, len(result)):
                (fd,eventmask) = result[i]
                modes = ''
                if eventmask & select.POLLOUT:
                    modes += 'w'
                if eventmask & select.POLLIN:
                    modes += 'r'
                if eventmask & select.POLLPRI:
                    modes += 'p'
                result[i] = (fd, modes)
            return result
else:
    #FIXME: define a class based on select
    def poll_for(*a):
        return None
