from responses import ProtocolError
import infilter

class ParameterError(ProtocolError):
    def __init__(self, code):
        self.args = 'Unknown event parameter type, can\'t handle!', code

_paramtypes = ('num', 'xy', 'pointer', 'data', 'kbd')

kmods = {
    'lshift':	0x0001,
    'rshift':	0x0002,
    'shift':	0x0003,
    'lctrl':	0x0040,
    'rctrl':	0x0080,
    'ctrl':	0x00c0,
    'lalt':	0x0100,
    'ralt':	0x0200,
    'alt':	0x0300,
    'lmeta':	0x0400,
    'rmeta':	0x0800,
    'meta':	0x0c00,
    'num':	0x1000,
    'caps':	0x2000,
    'mode':	0x4000,
}

typenames = {
    # Widget events
    0x001:	'activate',	# Button has been clicked/selected  
    0x002:	'deactivate',	# Sent when the user clicks outside the active popup 
    0x003:	'close',	# A top-level widget has closed 
    0x004:	'focus',	# Sent when a button is focused, only if it has PG_EXEV_FOCUS. The field widget always sends this. 
    0x204:	'pntr down',	# The "mouse" button is now down 
    0x205:	'pntr up',	# The "mouse" button is now up 
    0x206:	'pntr release',	# The "mouse" button was released outside the widget 
    0x306:	'data',		# Widget is streaming data to the app 
    0x107:	'resize',	# For terminal widgets 
    0x108:	'build',	# Sent from a canvas, clients can rebuild groplist 
    0x109:      'scrollwheel',  # Scroll wheel event with x,y deltas for the scroll wheel
    0x209:	'pntr move',	# The "mouse" moved 
    0x40A:	'kbd char',	# A focused keyboard character recieved 
    0x40B:	'kbd keyup',	# A focused raw keyup event 
    0x40C:	'kbd keydown',	# A focused raw keydown event 
    0x301:	'appmsg',	# Messages from another application 

    # Non-widget events
    0x1001:	'theme inserted',	# This notifies all clients when a theme
                                        # is inserted, so they can reevaluate
                                        # theme properties they're using. The
                                        # parameter passed with this event is a
                                        # handle to the theme.
    0x1002:	'theme removed',	# This notifies all clients when a theme
                                        # is removed, so they can reevaluate
                                        # theme properties they're using. The
                                        # parameter passed with this event is a
                                        # handle to the theme. Note that at the
                                        # time this event is sent, the handle
                                        # will be invalid, but it is provided for
                                        # comparison purposes if needed.
    0x1302:	'infilter',		# Carries an event from pgserver to a
                                        # client-side input filter.
}

class Event(object):
    """A PicoGUI Event.
    """
    def __init__(self, evtype, widget_id, param):
        paramtype = (evtype & 0x0f00) >> 8
        try:
            self.paramtype = _paramtypes[paramtype]
        except IndexError:
            raise ParameterError(paramtype)
        self.type = evtype
        self.name = typenames.get(self.type)
        self.widget_id = widget_id or None
        if self.paramtype == 'data':
            self.additional_data_wanted = param
        else:
            self.setData(param)
        if self.name == 'infilter':
            self.paramtype = 'trigger'

    def setData(self, data):
        """Set the parameter data (decode it)
        """
        handler = getattr(self, '_decode_' + self.paramtype)
        handler(data)

    def _decode_num(self, data):
        self.data = data	# d'oh

    def _decode_xy(self, data):
        self.x = data >> 16
        self.y = data & 0xffff
        # Convert from unsigned to signed
        if self.x & 0x8000:
            self.x -= 0x10000
        if self.y & 0x8000:
            self.y -= 0x10000

    def _decode_pointer(self, data):
        self.x = data & 0xfff
        self.y = (data >> 12) & 0xfff
        self.buttons = (data >> 24) & 0xf
        self.chbuttons = (data >> 28) & 0xf

    def _decode_data(self, data):
        # not really - we don't know what this data means, so we just store it
        self.data = data

    def _decode_kbd(self, data):
        # char, or PGKEY constant - never mind, in python we'll deal with them all the same way
        self.data = (data & 0xffff)
        if self.name == 'kbd_char':
            self.char = unichr(data)
        elif self.data < 256:
            # it's a key, but it represents a character, so why not have it
            self.char = chr(self.data)
        else:
            self.char = ''
        self.mods = data >> 16

    def _decode_trigger(self, data):
        self.trigger = infilter.Trigger(data)

    def hasMod(self, mod):
        """Check for a keyboard modifier"""
        if hasattr(self, 'mods'):
            return self.mods & kmods.get(mod.lower(), 0)

    def __repr__(self):
        r = '<PicoGUI %s event object at %s' % (self.name, id(self))
        if hasattr(self, 'char') and self.char:
            r = '%s (%r)' % (r, self.char)
        if hasattr(self, 'mods') and self.mods:
            r = '%s %s, keyboard mods %s' % (r, self.data, self.mods)
        if hasattr(self, 'x'):
            r = '%s x=%s, y=%s' % (r, self.x, self.y)
            if hasattr(self, 'buttons'):
                r = '%s, buttons %s' % (r, self.buttons)
        return r + '>'

    def __str__(self):
        return self.name
