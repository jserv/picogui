# Server class

# constant maps

constants = {
    'attachwidget': {
        'after':	(1, {}),
        'inside':	(2, {}),
        'before':	(3, {}),
    },
    'createwidget': {
            'toolbar':		(0, {}),
            'label':		(1, {}),
            'scroll':		(2, {}),
            'indicator':	(3, {}),
            'bitmap':		(4, {}),
            'button':		(5, {}),
            #'panel':		(6, {}),	# internal use only!
            #'popup':		(7, {}),	# internal use only!
            'box':		(8, {}),
            'field':		(9, {}),
            #'background':	(10, {}),	# internal use only!
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
    },
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
    'set': {
        'size':				(1, {
        }),
        'side':				(2, {
            'top':	(1<<3, {}),	# stick to the top edge
            'botton':	(1<<4, {}),	# stick to the bottom edge
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
        }),
        'text':				(7, {
        }),
        'font':				(8, {
        }),
        'transparent':			(9, {
        }),
        'bordercolor':			(10, {
        }),
        'bitmap':			(12, {
        }),
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
        'name':				(26, {	# a widget's name (for named containers, etc)
        }),
        'publicbox':			(27, {	# set to 1 to allow other apps to make widgets in this container
        }),
        'disabled':			(28, {	# for buttons, grays out text and prevents clicking
        }),
        'margin':			(29, {	# for boxes, overrides the default margin
        }),
        'textformat':			(30, {	# for the textbox, defines a format for text. fourcc format,
                                                # with optional preceeding '+' to prevent erasing existing data,
                                                # just append at the cursor position/
        }),
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
        }),
        'password':			(50, {
        }),
        'hotkey flags':			(51, {	# keyboard event flags for the hotkey
        }),
        'hotkey consume':		(52, {	# flag indicating whether to consume the key event when a hotkey comes in
        }),
    },
    'traversewidget': {
        'children':	(1, {}),	# starting with first child, traverse forward
        'forward':	(2, {}),
        'backward':	(3, {}),	# much slower than forward, avoid it
        'container':	(4, {}),	# 'count' is thenumber of container levels to traverse up
    }
}

def resolve_constant(name, namespace=constants):
    if type(namespace) == type(()):
        # for cases where the argument is really supposed to be a string, yet there are contstants after this
        return name, namespace[0]
    if type(name) in (type(()), type([])):
        # multiple names are or'ed together; namespace returned is the last one
        value = 0
        ns = namespace
        for n in name:
            v, ns = resolve_constant(n, namespace)
            value |= v
        return value, ns
    try:
        name = name.lower()
    except AttributeError:
        # never mind, not a string
        return name, namespace
    r = namespace.get(name, (name, namespace))
    if len(r) == 2:
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
            r, ns = resolve_constant(a, ns)
            args_resolved.append(r)
        return self.server.send_and_wait(self.handler,args_resolved)

# the class itself

class Server(object):
    def __init__(self, address='localhost', display='0'):
        self._connection = network.sock(address, display)
        self._strings = {}
        self._fonts = {}
        self._bitmaps = {}

    def send_and_wait(self, handler, args):
        # this is the only method that needs to be touched to add thread-safety.
        # It can send a self-incrementing id, or a thread id, as request id,
        # then wait for a matching reply and dispatch others somehow.
        # However, responses that don't have ids would break this, so it would be
        # a waste of time to implement it now.
        #print 'calling %s%s' % (handler.__name__, args)
        self._connection.send(handler(*args))
        return responses.next(self._connection)

    def getString(self, text):
        if not self._strings.has_key(text):
            self._strings[text] = self.mkstring(text)
        return self._strings[text]

    def getBitmap(self, image):
        # problem: how to calculate X and Y?
        # what format is the image data in?
        raise "Sorry, not implemented"

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

    def __getattr__(self, name):
        # this resolves all undefined names - in our case, requests
        name = name.lower()
        if name in dir(requests):
            return Request(self, name)
        else:
            raise AttributeError(name)