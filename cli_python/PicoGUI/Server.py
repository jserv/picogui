# Server class

import network, requests, responses, constants

class Request(object):
    def __init__(self, server, name):
        self.server = server
        self.handler = getattr(requests, name)
        self.ns = constants.resolve(name)[1]

    def __call__(self, *args):
        args_resolved = []
        ns = self.ns
        for a in args:
            r, ns = constants.resolve(a, ns, self.server)
            args_resolved.append(r)
        return self.server.send_and_wait(self.handler,args_resolved)

def noop(*a, **kw):
    pass

class Server(object):
    def __init__(self, address=None, display=None, stream=None, stream_read=0):
        if stream:
            self._connection = stream
            try:
                self._write = self._connection.send
            except AttributeError:
                self._write = self._connection.write
            self.close_connection = noop
            self._wait = stream_read
        else:
            self._connection = network.sock(address, display)
            self._write = self._connection.send
            self.close_connection = self._connection.close
            self._wait = 1
        self._strings = {}
        self._fonts = {}
        self._bitmaps = {}

    def _mkrequest(self, handler, args, id=None):
        return handler(*args)

    def send_and_wait(self, handler, args):
        # this is the only method that needs to be touched to add thread-safety.
        # It can send a self-incrementing id, or a thread id, as request id,
        # then wait for a matching reply and dispatch others somehow.
        # However, responses that don't have ids would break this, so it would be
        # a waste of time to implement it now.
        #print 'calling %s%s' % (handler.__name__, args)
        self._write(self._mkrequest(handler, args))
        if self._wait:
            return responses.next(self._connection)

    def getString(self, text):
        if not self._strings.has_key(text):
            self._strings[text] = self.mkstring(text)
        return self._strings[text]

    def getBitmap(self, image):
        if not self._bitmaps.has_key(image):
            self._bitmaps[image] = self.mkbitmap(image)
        return self._bitmaps[image]

    def getFont(self, spec):
        if not self._fonts.has_key(spec):
            specinfo = spec.split(':')
            if len(specinfo) < 2:
                raise TypeError('Invalid font spec')
            family = specinfo[0]
            size = specinfo[1]
            style = [name for name in specinfo[2:]]
            self._fonts[spec] = self.mkfont(family, style, size)
        return self._fonts[spec]

    def close_connection(self):
        self._connection.close()

    def __getattr__(self, name):
        # this resolves all undefined names - in our case, requests
        name = name.lower()
        if name in dir(requests):
            return Request(self, name)
        else:
            raise AttributeError(name)

# support for writing to widget template files
import wtfile

_requests_to_save = (
    # the requests that return a handle
    requests.createwidget,
    requests.dup,
    requests.findwidget,
    requests.loaddriver,	# why would a WT do this?
    requests.mkbitmap,
    requests.mkfillstyle,
    requests.mkfont,
    requests.mkstring,
    requests.mktheme,		# why?
    requests.mkwidget,		# ditto
    requests.newbitmap,
    requests.register,
    requests.traversewidget,
)

class WTFile(Server):
    def __init__(self):
        stream = wtfile.stream()
        Server.__init__(self, stream=stream, stream_read=1)

    def _mkrequest(self, handler, args, id=None):
        if id is not None:
            pass
        elif handler in _requests_to_save:
            id = 0 # any value suffices
        else:
            id = 0xFFFFFFFFL # discard
        return handler(*args)

    def dump(self):
        return self._connection.dump()
