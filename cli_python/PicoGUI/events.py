import struct
from responses import ProtocolError

class ParameterError(ProtocolError):
    def __init__(self, code):
        self.args = 'Unknown event parameter type, can\'t handle!', code

_paramtypes = ('num', 'xy', 'pointer', 'data', 'kbd')

kmods = {
    'LSHIFT':	0x0001,
    'RSHIFT':	0x0002,
    'SHIFT':	0x0003,
    'LCTRL':	0x0040,
    'RCTRL':	0x0080,
    'CTRL':	0x00C0,
    'LALT':	0x0100,
    'RALT':	0x0200,
    'ALT':	0x0300,
    'LMETA':	0x0400,
    'RMETA':	0x0800,
    'META':	0x0C00,
    'NUM':	0x1000,
    'CAPS':	0x2000,
    'MODE':	0x4000,
}

typenames = {
    # Widget events
    0x001:	'activate',	# Button has been clicked/selected  
    0x002:	'deactivate',	# Sent when the user clicks outside the active popup 
    0x003:	'close',	# A top-level widget has closed 
    0x004:	'focus',	# Sent when a button is focused, only if it has PG_EXEV_FOCUS. The field widget always sends this. 
    0x204:	'pntr_down',	# The "mouse" button is now down 
    0x205:	'pntr_up',	# The "mouse" button is now up 
    0x206:	'pntr_release',	# The "mouse" button was released outside the widget 
    0x306:	'data',		# Widget is streaming data to the app 
    0x107:	'resize',	# For terminal widgets 
    0x108:	'build',	# Sent from a canvas, clients can rebuild groplist 
    0x209:	'pntr_move',	# The "mouse" moved 
    0x40A:	'kbd_char',	# A focused keyboard character recieved 
    0x40B:	'kbd_keyup',	# A focused raw keyup event 
    0x40C:	'kbd_keydown',	# A focused raw keydown event 
    0x301:	'appmsg',	# Messages from another application 

    # Non-widget events
    0x140A:	'kbd_char',	# These are sent if the client has captured the 
    0x140B:	'kbd_keyup',	# keyboard (or pointing device ) 
    0x140C:	'kbd_keydown',
    0x1209:	'pntr_move',
    0x1205:	'pntr_up',
    0x1204:	'pntr_down',
    0x120D:	'bgclick',	# The user clicked the background widget 
    0x1101:	'pntr_raw',	# Raw coordinates, for tpcal or games 
    0x1301:	'calib_penpos',	# Raw 32-bit coordinates, for tpcal 
}

class Event(object):
    """A PicoGUI Event.
    """
    def __init__(self, evtype, widget_id, param):
        paramtype = (evtype & 0x0f00) >> 8
        try:
            self.paramtype = _paramtypes[paramtype]
        except KeyError:
            raise ParameterError(paramtype)
        self.type = evtype
        self.name = typenames[self.type]
        self.widget_id = widget_id or None
        if self.paramtype == 'data':
            self.additional_data_wanted = param
        else:
            self.setData(param)

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
        elif data < 256:
            # it's a key, but it represents a character, so why not have it
            self.char = chr(data)
        else:
            self.char = ''
        self.mods = data >> 16

    def hasMod(self, mod):
        """Check for a keyboard modifier"""
        if hasattr(self, 'mods'):
            return self.mods & kmods.get(mod.upper(), 0)

    def __repr__(self):
        r = '<PicoGUI %s event object at %s' % (self.name, id(self))
        if hasattr(self, 'char') and self.char:
            r = '%s ("%s")' % (r, self.char)
        if hasattr(self, 'mods') and self.mods:
            r = '%s %s, keyboard mods %s' % (r, self.data, self.mods)
        if hasattr(self, 'x'):
            r = '%s x=%s, y=%s' % (r, self.x, self.y)
            if hasattr(self, 'buttons'):
                r = '%s, buttons %s' % (r, self.buttons)
        return r + '>'

    def __str__(self):
        return self.name