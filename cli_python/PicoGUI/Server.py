# Server class

import string

# constant maps

def _getString(str, server):
    # check if it's already a handle, too
    if not server:
        return str, _getString
    try:
        str.upper()
    except:
        return str, _getString
    return server.getString(str), _getString

def _getFont(str, server):
    if not server:
        return str, _getFont
    try:
        str.upper()
    except:
        return str, _getFont
    return server.getFont(str), _getFont

def _getBitmap(str, server):
    return server.mkbitmap(str), _getBitmap

def _getSize(s, server):
    # This converts fractions of the form "numerator/denominator"
    # into pgserver 8:8 bit fractions if necessary.
    fraction = string.split(str(s),'/')
    if len(fraction) > 1:
        return (int(fraction[0])<<8) | int(fraction[1]), _getSize
    return int(s), _getSize


_wtype_consts = {
            'toolbar':		(0, {}),
            'label':		(1, {}),
            'scroll':		(2, {}),
            'indicator':	(3, {}),
            'bitmap':		(4, {}),
            'button':		(5, {}),
            'panel':		(6, {}),
            'popup':		(7, {}),
            'box':		(8, {}),
            'field':		(9, {}),
            'background':	(10, {}),
            'menuitem':		(11, {}),	# a variation on button
            'terminal':		(12, {}),	# a full terminal emulator
            'canvas':		(13, {}),
            'checkbox':		(14, {}),	# another variation of button
            'flatbutton':	(15, {}),	# yet another customized button
            'listitem':		(16, {}),	# still yet another...
            'submenuitem':	(17, {}),	# menuitem with a submenu arrow
            'radiobutton':	(18, {}),	# like a check box, but exclusive
            'textbox':		(19, {}),	# client-side text layout
            'panelbar':		(20, {}),	# draggable bar and container
            'simplemenu':	(21, {}),	# create a simple menu from a string or array
            'dialogbox':        (22, {}),       # a type of popup that is always autosized and has a title
            'messagedialog':    (23, {}),       # a type of dialogbox that displays a message and gets feedback
            'scrollbox':        (24, {}),       # box widget with built-in horizontal and/or vertical scrollbars
    }

constants = {
    'attachwidget': {
        'after':	(1, {}),
        'inside':	(2, {}),
        'before':	(3, {}),
    },
    'createwidget': _wtype_consts,
    'mkwidget': {
        'after':	(1, _wtype_consts),
        'inside':	(2, _wtype_consts),
        'before':	(3, _wtype_consts),
    },
    'get': 'set',
    'mkfont': ( # first argument (family) is string and should pass trough
        {
            'fixed':			((1<<0), {}),	# fixed width
            'default':			((1<<1), {}),	# the default font in its category, fixed or proportional.
            'symbol':			((1<<2), {}),	# font contains nonstandard chars and will not be chosen
                                                        # unless specifically requested
            'subset':			((1<<3), {}),	# font does not contain all the ascii chars before 127, and
                                                        # shouldn't be used unless requested
            'encoding isolatin1':	((1<<4), {}),	# iso latin-1 encoding
            'encoding ibm':		((1<<5), {}),	# ibm-pc extended characters
            'doublespace':		((1<<7), {}),	# add extra space between lines
            'bold':			((1<<8), {}),	# use or simulate a bold version of the font
            'italic':			((1<<9), {}),	# use or simulate an italic version of the font
            'underline':		((1<<10), {}),	# underlined text
            'strikeout':		((1<<11), {}),	# strikeout, a line through the middle of the text
            'flush':			((1<<14), {}),	# disable the margin that picogui puts around text
            'doublewidth':		((1<<15), {}),	# add extra space between characters
            'italic2':			((1<<16), {}),	# twice the slant of the default italic
            'encoding unicode':		((1<<17), {}),	# unicode encoding
    },),
    'register': _getString,
    'getresource': {
        'default font':			(0, {}),
        'string ok':			(1, {}),
        'string cancel':		(2, {}),
        'string yes':			(3, {}),
        'string no':			(4, {}),
        'string segfault':		(5, {}),
        'string matherr':		(6, {}),
        'string pguierr':		(7, {}),
        'string pguiwarn':		(8, {}),
        'string pguierrdlg':		(9, {}),
        'string pguicompat':		(10, {}),
        'default textcolors':		(11, {}),
        'infilter key normalize':	(12, {}),
        'infilter pntr normalize':	(13, {}),
        'infilter touchscreen':		(14, {}),
        'infilter key preprocess':	(15, {}),
        'infilter pntr preprocess':	(16, {}),
        'infilter key magic':		(17, {}),
        'infilter key dispatch':	(18, {}),
        'infilter pntr dispatch':	(19, {}),
    },
    'set': {
        'size':				(1, _getSize),
        'side':				(2, {
            'top':	(1<<3, {}),	# stick to the top edge
            'bottom':	(1<<4, {}),	# stick to the bottom edge
            'left':	(1<<5, {}),	# stick to the left edge
            'right':	(1<<6, {}),	# stick to the right edge
            'all':	(1<<11, {}),	# occupy all available space
        }),
        'align':			(3, {
            'center':	(0, {}),	# center in the available space
            'top':	(1, {}),	# stick to the top-center of the available space
            'left':	(2, {}),	# stick to the left-center of the available space
            'bottom':	(3, {}),	# stick to the bottom-center of the available space
            'right':	(4, {}),	# stick to the right-center of the available space
            'nw':	(5, {}),	# stick to the northwest corner
            'sw':	(6, {}),	# stick to the southwest corner
            'ne':	(7, {}),	# stick to the northeast corner
            'se':	(8, {}),	# stick to the southeast corner
            'all':	(9, {}),	# occupy all available space (good for tiled bitmaps)
        }),
        'bgcolor':			(4, {
        }),
        'color':			(5, {
        }),
        'sizemode':			(6, {
            'pixel':	(0, {}),
            'percent':	(1<<2, {}),
            'cntfrac':	(1<<15, {}),               # Container fraction
            'container fraction':  (1<<15, {}),    # Less stupidly named version of the same
        }),
        'text':				(7, _getString),
        'font':				(8, _getFont),
        'transparent':			(9, {
        }),
        'bordercolor':			(10, {
        }),
        'bitmap':			(12, _getBitmap),
        'lgop':				(13, {
        }),
        'value':			(14, {
        }),
        'bitmask':			(15, {
        }),
        'bind':				(16, {
        }),
        'scroll x':			(17, {	# horizontal and vertical scrolling amount
        }),
        'scroll y':			(18, {
        }),
        'scroll':			(17, {	# for backwards compatibility
        }),
        'hotkey':			(19, {
        }),
        'extdevents':			(20, {	# for buttons, a mask of extra events to send
        }),
        'direction':			(21, {
            'horizontal':	(0, {}),
            'vertical':		(90, {}),
            'antihorizontal':	(180, {}),
        }),
        'absolutex':			(22, {	# read-only, relative to screen
        }),
        'absolutey':			(23, {
        }),
        'on':				(24, {	# on-off state of button/checkbox/etc
        }),
        'state':			(25, {	# deprecated! use thobj instead
        }),
        'thobj':			(25, {	# set a widget's theme object
        }),
        'name':				(26, _getString), # a widget's name (for named containers, etc)
        'publicbox':			(27, {	# set to 1 to allow other apps to make widgets in this container
        }),
        'disabled':			(28, {	# for buttons, grays out text and prevents clicking
        }),
        'margin':			(29, {	# for boxes, overrides the default margin
        }),
        'textformat':			(30, 	# for the textbox, defines a format for text. fourcc format,
                                                # with optional preceeding '+' to prevent erasing existing data,
                                                # just append at the cursor position/
        _getString),
        'triggermask':			(31, {	# mask of extra triggers accepted (self->trigger_mask)
        }),
        'hilighted':			(32, {	# widget property to hilight a widget and all it's children
        }),
        'selected':			(33, {	# list property to select a row.
        }),
        'selected handle':		(34, {	# list property to return a handle to the selected row
        }),
        'autoscroll':			(35, {	# for the textbox or terminal, scroll to any new text that's inserted
        }),
        'lines':			(36, {	# height, in lines
        }),
        'preferred w':			(37, {	# read only (for now) properties to get any widget's preferred size
        }),
        'preferred h':			(38, {
        }),
        'panelbar':			(39, {	# read-only property for panels returns a handle to its embedded
                                                # panelbar widget
        }),
        'auto orientation':		(40, {	# automatically reorient child widgets when side changes
        }),
        'thobj button':			(41, {	# these four theme properties set the theme objects used for the
                                              	# three possible states of the button widget.
        }),
        'thobj button hilight':		(42, {
        }),
        'thobj button on':		(43, {
        }),
        'thobj button on nohilight':	(44, {
        }),
        'panelbar label':		(45, {	# more read-only panelbar properties to get the built-in panelbar widgets
        }),
        'panelbar close':		(46, {
        }),
        'panelbar rotate':		(47, {
        }),
        'panelbar zoom':		(48, {
        }),
        'bitmapside':			(49, {
            'top':	(1<<3, {}),	# stick to the top edge
            'bottom':	(1<<4, {}),	# stick to the bottom edge
            'left':	(1<<5, {}),	# stick to the left edge
            'right':	(1<<6, {}),	# stick to the right edge
            'all':	(1<<11, {}),	# occupy all available space
        }),
        'password':			(50, {
        }),
        'hotkey flags':			(51, {	# keyboard event flags for the hotkey
        }),
        'hotkey consume':		(52, {	# flag indicating whether to consume the key event when a hotkey comes in
        }),
        'width':			(53, {
        }),
        'height':			(54, {
        }),
        'spacing':			(55, {
        }),
        'minimum':			(56, {
        }),
        'multiline':			(57, {
        }),
        'selection':			(58, _getString),
        'readonly':			(59, {
        }),
        'insertmode':			(60, {
        }),
        'type':				(61, {
        }),
    },
    'traversewidget': {
        'children':	(1, {}),	# starting with first child, traverse forward
        'forward':	(2, {}),
        'backward':	(3, {}),	# much slower than forward, avoid it
        'container':	(4, {}),	# 'count' is thenumber of container levels to traverse up
    }
}

def resolve_constant(name, namespace=constants, server=None):
    if type(namespace) == type(()):
        # for cases where the argument is really supposed to be a string, yet there are contstants after this
        return name, namespace[0]
    if callable(namespace):
        # for cases when we need even stranger processing
        return namespace(name, server)
    if type(name) in (type(()), type([])):
        # multiple names are or'ed together; namespace returned is the last one
        value = 0
        ns = namespace
        for n in name:
            v, ns = resolve_constant(n, namespace)
            value |= v
        return value, ns
    try:
        lname = name.lower()
    except AttributeError:
        # never mind, not a string
        return name, namespace
    r = namespace.get(lname, (name, namespace))
    if type(r) == type(()) and len(r) == 2:
        if r[1]:
            return r
        else:
            # stay in the same namespace if we hit a "leaf"
            return r[0], namespace
    try:
        r.lower()
        isstr = 1
    except AttributeError:
        isstr = 0
    if isstr:
        # it's an alias
        return resolve_constant(r, namespace)
    # otherwise, it's just a namespace
    return None, r


# imports

import network, requests, responses

# the Request utility class

class Request(object):
    # IMPORTANT: we can't resolve constants in keyword arguments
    def __init__(self, server, name):
        self.server = server
        self.handler = getattr(requests, name)
        self.ns = resolve_constant(name)[1]

    def __call__(self, *args):
        args_resolved = []
        ns = self.ns
        for a in args:
            r, ns = resolve_constant(a, ns, self.server)
            args_resolved.append(r)
        return self.server.send_and_wait(self.handler,args_resolved)

def noop(*a, **kw):
    pass

# the class itself

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
