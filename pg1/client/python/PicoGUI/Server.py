# Server class

import network, requests, responses, constants, os
try:
    import thread
except:
    thread = None

debug_threads = 0

PGDEBUG = os.environ.get('PGDEBUG', '')
if PGDEBUG.find('thread') >= 0:
    debug_threads = 1

def PlatformIncompatibility(exception):
    pass

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
            if type(r) is tuple:
                args_resolved.extend(r)
            else:
                args_resolved.append(r)
        return self.server.send_and_wait(self.handler, args_resolved, timeout)

def noop(*a, **kw):
    pass

class verbose_lock(object):
    # verbose wrapper for thread locks - useful for debugging
    def __init__(self):
        self._real_lock = thread.allocate_lock()

    def acquire(self):
        print 'trying to acquire lock', id(self), 'from thread', thread.get_ident()
        self._real_lock.acquire()
        print 'acquired lock', id(self), 'from thread', thread.get_ident()

    def release(self):
        print 'releasing lock', id(self), 'from thread', thread.get_ident()
        self._real_lock.release()

    def locked(self):
        print 'checking lock', id(self), 'from thread', thread.get_ident()
        return self._real_lock.locked()

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
        self._poll_handlers = {}
        self.lost_and_found = []
        self.event_queue = []
        if thread is not None:
            if debug_threads:
                self._write_lock = verbose_lock()
            else:
                self._write_lock = thread.allocate_lock()

    def _mkrequest(self, handler, args, id=None):
        return handler(*args, **{'id': id})

    def get_response(self, req_id):
        for resp_id, resp in self.lost_and_found:
            if resp_id == req_id:
                self.lost_and_found.remove((resp_id, resp))
                return resp
        try:
            resp_id, resp = responses.get(self._connection)
        except responses.Dead, corpse:
            # pgserver exited, or network error
            import sys
            print >> sys.stderr, corpse
            raise SystemExit
        if isinstance(resp, Exception):
            raise resp
        if resp_id == req_id:
            return resp
        if resp_id is None:
            # unexpected event
            self.event_queue.append(resp)
        self.lost_and_found.append((resp_id, resp))
        return None

    def poll(self, handler, fd, mask='r'):
        if not self._poll:
            raise PlatformIncompatibility, 'poll not available'
        try:
            fd = fd.fileno()
        except:
            pass
        if handler == None:
            self._poll.unregister(fd)
            del self._poll_handlers[fd]
        else:
            self._poll.register(fd, mask)
            self._poll_handlers[fd] = handler

    def _do_send_and_wait(self, req, req_id, timeout, is_interruptable=False):
        try:
            if thread is not None:
                # XXX sub-optimal!
                self._write_lock.acquire()
            self._write(req)
            if not self._wait:
                # our "connection" doesn't answer - wtfile, for example
                if thread is not None:
                    self._write_lock.release()
                return
            resp = None

            # Convert the connection to a file descriptor if it supports that
            try:
                connection_fd = self._connection.fileno()
            except:
                connection_fd = self._connection

            while resp == None:
                if self._poll:
                    pollResults = self._poll.poll(timeout)
                    if pollResults:
                        for (fd, event) in pollResults:
                            if fd == connection_fd:
                                resp = self.get_response(req_id)

                            elif is_interruptable:
                                # The C client library goes through a lot of trouble
                                # to avoid a race condition in which an event arrives
                                # before a request response, but since cli_python's
                                # get_response is a lot smarter, this shouldn't be
                                # necessary here.
                                self._poll_handlers[fd](fd, event)
                                self.update()
                                self._write(req)
                    else:
                        if thread is not None:
                            self._write_lock.release()
                        return None  # Timeout
                else:
                    # No poll function, we have to block
                    resp = self.get_response(req_id)
            if thread is not None:
                self._write_lock.release()
            return resp
        finally:
            if thread is not None and self._write_lock.locked():
                self._write_lock.release()

    def send_and_wait(self, handler, args=(), timeout=None):
        req_id = self._counter
        self._counter += 1
        if PGDEBUG:
            print 'PGDEBUG:', handler, args
        return self._do_send_and_wait(self._mkrequest(handler, args, req_id), req_id, timeout)

    def getString(self, text):
        if type(text) is unicode:
            text = text.encode('utf8')
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
        # the wait request is a special case, since events don't have an id,
        # and wait requests are interruptable.
        if self.event_queue:
            return self.event_queue.pop(0)
        return self._do_send_and_wait(requests.wait(), None, timeout, True)

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
