# support for input filters
import struct

trigger_base_format = '!' + ('L' * 16)

trigger_types = {
  1<<0:		'timer',	# Timer event from install_timer 
  1<<1:		'pntr relative',# Specify relative mouse motion and the current
                                # button status 
  1<<3:		'activate',	# Sent when it receives focus 
  1<<4:		'deactivate',	# Losing focus 
  1<<5:		'key up',	# Ignores autorepeat, etc. Raw key codes
  1<<6:		'key down',	# Ditto. 
  1<<7:		'release',	# Mouse up (This is when the mouse was pressed
                                # inside the widget, then released elsewhere.)
  1<<8:		'up',		# Mouse up in specified divnode 
  1<<9:		'down',		# Mouse down in divnode 
  1<<10:	'move',		# Triggers on any mouse movement in node 
  1<<11:	'enter',	# Mouse moves inside widget 
  1<<12:	'leave',	# Mouse moves outside widget 
  1<<13:	'drag',		# Mouse move when captured 
  1<<14:	'char',		# A processed ASCII/Unicode character 
  1<<15:	'stream',	# Incoming packet (from WRITETO) 
  1<<16:	'key start',	# Sent at the beginning of key propagation 
  1<<17:	'nontoolbar',	# Not really a trigger, but widgets can put this
                                # in their trigger mask to request placement in
                                # the nontoolbar area when applicable 
  1<<18:	'pntr status',	# A driver can send this trigger with the current
                                # status of the mouse to have the input filters
                                # automatically extrapolate other events. 
  1<<19:	'key',		# A driver can send this with a key code when the
                                # exact state of the key is unknown, to have
                                # KEYUP, KEYDOWN, and CHAR events generated. 
  1<<20:	'scrollwheel',	# The x,y coordinates passed with this are signed
                                # scroll wheel values 
  1<<21:	'touchscreen',	# A touchscreen event to be processed by
                                # infilter_touchscreen 
  1<<22:	'ts calibrate',	# Store the touchscreen calibration given in this event
  1<<23:	'close',	# Sent by drivers to a managed window when it's externally closed
  1<<24:	'motiontracker',# Data from a motion tracker sensor
}

mouse_triggers = ('pntr relative', 'up', 'down', 'move', 'drag', 'pntr status',
                  'scrollwheel', 'release', 'touchscreen', 'ts calibrate')

kbd_triggers =   ('key up', 'key down', 'char', 'key start', 'key')

mouse = 'mouse'
kbd = 'kbd'
unknown = 'unknown'

class Trigger(object):
    def __init__(self, data='\0'*64, name=None, sender=None):
        self._data = list(struct.unpack(trigger_base_format, data))
        if sender:
            self.sender = sender
        else:
            self.sender = self._data[0]
        self.name = name or trigger_types.get(self._data[1], self._data[1])
        if self.name in mouse_triggers:
            self.dev = mouse
        elif self.name in kbd_triggers:
            self.dev = kbd
        else:
            self.dev = unknown
        format = getattr(self, 'format_' + self.dev, None)
        if format:
            if callable(format):
                format(self._data)
            else:
                for index, name in format:
                    setattr(self, name, self._data[index])

# the first member in each structure below is index 2
##       struct {
## 	u32 x,y,btn,pressure;    /* Current status */
## 	u32 chbtn;               /* Changed buttons */
## 	u32 cursor_handle;
## 	u32 is_logical;          /* Nonzero if events are in logical coordinates */
## 	u32 ts_calibration;      /* Handle of a calibration string for the touchscreen */
##       } mouse;

##       struct {
## 	u32 key;                 /* PGKEY_* constant */
## 	u32 mods;                /* PGMOD_* constant */
## 	u32 flags;               /* PG_KF_* constants */
## 	u32 consume;             /* Consume event during widget propagation */
##       } kbd;

    format_mouse = (
        (2, 'x'),
        (3, 'y'),
        (4, 'buttons'),
        (5, 'pressure'),
        (6, 'changed_buttons'),
        (7, 'cursor_handle'),
        (8, 'is_logical'),
        (9, 'ts_calibration'),
    )

    format_kbd = (
        (2, 'key'),
        (3, 'mods'),
        (4, 'flags'),
        (5, 'consume'),
    )

    def pack(self):
        if hasattr(self.sender, 'handle'):
            self._data[0] = self.sender.handle
        elif type(self.sender) in (int, long):
            self._data[0] = self.sender
        for code, name in trigger_types.items():
            if name == self.name:
                self._data[1] = code
                break
        format = getattr(self, 'format_' + self.dev, None)
        if format:
            if callable(format):
                for index, value in format():
                    self._data[index] = long(value)
            else:
                for index, name in format:
                    self._data[index] = long(getattr(self, name))
        return struct.pack(trigger_base_format, *self._data)


def _make_trigger_mask(seq):
    if not seq:
        return 0
    if seq == '*':
        return 0xFFFFFFFFL
    result = 0
    for value, name in trigger_types.iteritems():
        if name in seq:
            result |= value
    return result

class Infilter(object):
    def __init__(self, app, after=0, accept='*', absorb=None):
        accept = _make_trigger_mask(accept)
        absorb = _make_trigger_mask(absorb)
        self.handle = app.server.mkinfilter(after, accept, absorb)
        self.app = app
        self.send = app.server.infiltersend

# limit "from infilter import *"
__all__ = 'Trigger mouse kbd unknown Infilter'.split()
