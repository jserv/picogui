# Server class

import network, requests, responses, constants

class Request(object):
    def __init__(self, server, name):
        self.server = server
        self.name = name
        self.handler = getattr(requests, name)
        self.ns = constants.resolve(name)[1]

    def __call__(self, *args, **kw):
        timeout = kw.get('timeout', None)
        args_resolved = []
        ns = self.ns
        for a in args:
            r, ns = constants.resolve(a, ns, self.server)
            args_resolved.append(r)
        return self.server.send_and_wait(self.handler, args_resolved, timeout)

def noop(*a, **kw):
    pass

class Server(object):
    def __init__(self, address=None, display=None, stream=None, stream_read=0, poll=None):
        if stream:
            self._connection = stream
            try:
                self._write = self._connection.send
            except AttributeError:
                self._write = self._connection.write
            self.close_connection = noop
            self._wait = stream_read
            self._poll = poll
        else:
            self._connection = network.sock(address, display)
            self._write = self._connection.send
            self.close_connection = self._connection.close
            self._wait = 1
            self._poll = network.poll_for(self._connection)
        self._strings = {}
        self._fonts = {}
        self._bitmaps = {}
        self._counter = 0
        self.lost_and_found = []
        self.event_queue = []

    def _mkrequest(self, handler, args, id=None):
        return handler(*args, **{'id': id})

    def get_response(self, req_id):
        for resp_id, resp in self.lost_and_found:
            if resp_id == req_id:
                self.lost_and_found.remove((resp_id, resp))
                return resp
        resp_id, resp = responses.get(self._connection)
        if isinstance(resp, Exception):
            raise resp
        if resp_id == req_id:
            return resp
        if resp_id is None:
            # unexpected event
            self.event_queue.append(resp)
        self.lost_and_found.append((resp_id, resp))
        return None

    def _do_send_and_wait(self, req, req_id, timeout):
        self._write(req)
        if not self._wait:
            # our "connection" doesn't answer - wtfile, for example
            return
        if timeout is not None and self._poll:
            if self._poll.poll(timeout):
                return self.get_response(req_id)
            else:
                return None
        resp = None
        while resp is None:
            resp = self.get_response(req_id)
        return resp

    def send_and_wait(self, handler, args=(), timeout=None):
        req_id = self._counter
        self._counter += 1
        return self._do_send_and_wait(self._mkrequest(handler, args, req_id), req_id, timeout)

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

    def checkevent(self):
        server_evts = self.send_and_wait(requests.checkevent) or 0
        return server_evts + len(self.event_queue)

    def wait(self, timeout=None):
        # the wait request is a special case, since events don't have an id
        if self.event_queue:
            return self.event_queue.pop(0)
        return self._do_send_and_wait(requests.wait(), None, timeout)

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
