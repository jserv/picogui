# pseudo-constant magic

#################################################################
# utility functions

def _getString(str, server, reverse=0):
    if reverse:
        if str == 0:
            return ''
        else:
            return server.getstring(str).data.replace('\0','')
    else:
        # check if it's already a handle, too
        if not server:
            return str, _getString
        try:
            str.upper()
        except:
            return str, _getString
        return server.getString(str), _getString

def _getFont(str, server, reverse=0):
    if reverse:
        return str
    else:
        if not server:
            return str, _getFont
        try:
            str.upper()
        except:
            return str, _getFont
        return server.getFont(str), _getFont

def _getBitmap(str, server, reverse=0):
    if reverse:
        return str
    else:
        return server.mkbitmap(str), _getBitmap

def _getSize(s, server, reverse=0):
    if reverse:
        return s
    else:
        # This converts fractions of the form "numerator/denominator"
        # into pgserver 8:8 bit fractions if necessary.
        # Also, allow "None" to indicate automatic sizing,
        # represented in the server by -1.
        if s == None:
            return 0xFFFFFFFFL, _getSize
        fraction = str(s).split('/')
        if len(fraction) > 1:
            return (int(fraction[0])<<8) | int(fraction[1]), _getSize
        return int(s), _getSize

def _pairAt16bits(p, server, reverse=0):
    if reverse:
        return p >> 16, p & 0xffff
    else:
        return (p[0] << 16) + p[1], {}


def _gropseq(seq, server):
    # not reversable
    try:
        seq = seq.all()
    except:
        pass
    return tuple(seq), _stop

#################################################################
# unique values

_stop = ((),)
_default = -1

#################################################################
# tables

_color_consts = {
            'black':            (0x000000, {}),
            'green':            (0x008000, {}),
            'silver':           (0xC0C0C0, {}),
            'lime':             (0x00FF00, {}),
            'gray':             (0x808080, {}),
            'olive':            (0x808000, {}),
            'white':            (0xFFFFFF, {}),
            'yellow':           (0xFFFF00, {}),
            'maroon':           (0x800000, {}),
            'navy':             (0x000080, {}),
            'red':              (0xFF0000, {}),
            'blue':             (0x0000FF, {}),
            'purple':           (0x800080, {}),
            'teal':             (0x008080, {}),
            'fuchsia':          (0xFF00FF, {}),
            'aqua':             (0x00FFFF, {}),
    }

_thobj_consts = {
            'default':			(0, {}),
            'base interactive':		(1, {}),
            'base container':		(2, {}),
            'button':			(3, {}),
            'button hilight':		(4, {}),
            'button on':		(5, {}),
            'toolbar':			(6, {}),
            'scroll':			(7, {}),
            'scroll hilight':		(8, {}),
            'indicator':		(9, {}),
            'panel':			(10, {}),
            'panelbar':			(11, {}),
            'popup':			(12, {}),
            'background':		(13, {}),
            'base display':		(14, {}),
            'base tlcontainer':		(15, {}),
            'themeinfo':		(16, {}),
            'label':			(17, {}),
            'field':			(18, {}),
            'bitmap':			(19, {}),
            'scroll on':		(20, {}),
            'label scroll':		(21, {}),
            'panelbar hilight':		(22, {}),
            'panelbar on':		(23, {}),
            'box':			(24, {}),
            'label dlgtitle':		(25, {}),
            'label dlgtext':		(26, {}),
            'closebtn':			(27, {}),
            'closebtn on':		(28, {}),
            'closebtn hilight':		(29, {}),
            'base panelbtn':		(30, {}),
            'rotatebtn':		(31, {}),
            'rotatebtn on':		(32, {}),
            'rotatebtn hilight':	(33, {}),
            'zoombtn':			(34, {}),
            'zoombtn on':		(35, {}),
            'zoombtn hilight':		(36, {}),
            'popup menu':		(37, {}),
            'popup messagedlg':		(38, {}),
            'menuitem':			(39, {}),
            'menuitem hilight':		(40, {}),
            'checkbox':			(41, {}),
            'checkbox hilight':		(42, {}),
            'checkbox on':		(43, {}),
            'flatbutton':		(44, {}),
            'flatbutton hilight':	(45, {}),
            'flatbutton on':		(46, {}),
            'listitem':			(47, {}),
            'listitem hilight':		(48, {}),
            'listitem on':		(49, {}),
            'checkbox on nohilight':	(50, {}),
            'submenuitem':		(51, {}),
            'submenuitem hilight':	(52, {}),
            'radiobutton':		(53, {}),
            'radiobutton hilight':	(54, {}),
            'radiobutton on':		(55, {}),
            'radiobutton on nohilight':	(56, {}),
            'textbox':			(57, {}),
            'terminal':			(58, {}),
            'menubutton':		(60, {}),
            'menubutton on':		(61, {}),
            'menubutton hilight':	(62, {}),
            'label hilight':		(63, {}),
            'box hilight':		(64, {}),
            'indicator h':		(65, {}),
            'indicator v':		(66, {}),
            'scroll h':			(67, {}),
            'scroll v':			(68, {}),
            'scroll h on':		(69, {}),
            'scroll h hilight':		(70, {}),
            'scroll v on':		(71, {}),
            'scroll v hilight':		(72, {}),
            'panelbar h':		(73, {}),
            'panelbar v':		(74, {}),
            'panelbar h on':		(75, {}),
            'panelbar h hilight':	(76, {}),
            'panelbar v on':		(77, {}),
            'panelbar v hilight':	(78, {}),
            'textedit':			(79, {}),
            'managedwindow':		(80, {}),
            'tab page':			(81, {}),
            'tab bar':			(82, {}),
            'tab':			(83, {}),
            'tab hilight':		(84, {}),
            'tab on':			(85, {}),
            'tab on nohilight':		(86, {}),
    }

_wtype_consts = {
            'toolbar':		(0, {}),
            'label':		(1, {}),
            'scroll':		(2, {}),
            'indicator':	(3, {}),
            'managedwindow':	(4, {}),
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
            'textedit':         (25, {}),
            'tabpage':          (26, {}),
    }

import keys
_key_consts = {}
for name, code in keys.keys.items():
    _key_consts[name] = (code, {})
_modifier_consts = {}
for name, code in keys.mods.items():
    _modifier_consts[name] = (code, {})

_constants = {
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
        'infilter touchscreen': 	(12, {}),
        'infilter key preprocess':	(13, {}),
        'infilter pntr preprocess':	(14, {}),
        'infilter magic':		(15, {}),
        'infilter key dispatch':	(16, {}),
        'infilter pntr dispatch':	(17, {}),
        'default cursorbitmap': 	(18, {}),
        'default cursorbitmask': 	(19, {}),
        'background widget':    	(20, {}),
        'infilter hotspot':     	(21, {}),
        'infilter key alpha':    	(22, {}),
        'infilter pntr normalize': 	(23, {}),
    },
    'set': {
        'size':				(1,
            _getSize),
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
        'bgcolor':			(4,
            _color_consts),
        'color':			(5,
            _color_consts),
        'sizemode':			(6, {
            'pixel':	(0, {}),
            'percent':	(1<<2, {}),
            'cntfrac':	(1<<15, {}),               # Container fraction
            'container fraction':  (1<<15, {}),    # Less stupidly named version of the same
        }),
        'text':				(7,
            _getString),
        'font':				(8,
            _getFont),
        'transparent':			(9, {}),
        'bordercolor':			(10,
            _color_consts),
        'image':			(12,
            _getBitmap),
        'bitmap':			'image',
        'lgop':				(13, {
        }),
        'value':			(14, {
        }),
        'bitmask':			(15,
            _getBitmap),
        'bind':				(16, {
        }),
        'scroll x':			(17, {	# horizontal and vertical scrolling amount
        }),
        'scroll y':			(18, {
        }),
        'hotkey':			(19, 
	    _key_consts),
        'extdevents':			(20, {	# mask of extra events to send
            'pointer up':      (0x0001, {}),
            'pointer down':    (0x0002, {}),
            'noclick':         (0x0004, {}),
            'pointer move':    (0x0008, {}),
            'pointer':         (0x0009, {}),
            'key':             (0x0010, {}),
            'char':            (0x0020, {}),
            'kbd':	       (0x0030, {}),
            'toggle':          (0x0040, {}),
            'exclusive':       (0x0080, {}),
            'focus':           (0x0100, {}),
            'no hotspot':      (0x0200, {}),
            'none':	       (0, {}),
            _default:	       (0, {}),
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
        'on':				(24, {  # on-off state of button/checkbox/etc
        }),
        'thobj':			(25,    # set a widget's theme object
            _thobj_consts),
        'name':				(26,    # a widget's name (for named containers, etc)
            _getString),
        'publicbox':			(27, {  # set to 1 to allow other apps to make widgets in this container
        }),
        'disabled':			(28, {  # for buttons, grays out text and prevents clicking
        }),
        'margin':			(29, {	# for boxes, overrides the default margin
        }),
        'textformat':			(30, 	# for the textbox, defines a format for text.
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
        'thobj button':			(41, 	# these four theme properties set the theme objects used for the
                                              	# three possible states of the button widget.
            _thobj_consts),

        'thobj button hilight':		(42,
            _thobj_consts),
        'thobj button on':		(43,
            _thobj_consts),
        'thobj button on nohilight':	(44,
            _thobj_consts),
        'panelbar label':		(45, {	# more read-only panelbar properties to get the built-in panelbar widgets
        }),
        'panelbar close':		(46, {
        }),
        'panelbar rotate':		(47, {
        }),
        'panelbar zoom':		(48, {
        }),
        'imageside':			(49, {
            'top':	(1<<3, {}),	# stick to the top edge
            'bottom':	(1<<4, {}),	# stick to the bottom edge
            'left':	(1<<5, {}),	# stick to the left edge
            'right':	(1<<6, {}),	# stick to the right edge
            'all':	(1<<11, {}),	# occupy all available space
        }),
        'bitmapside':			'imageside',
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
        'selection':			(58,
            _getString),
        'readonly':			(59, {
        }),
        'insertmode':			(60, {
            'overwrite':   (0, {}),	# Replace all text
            'append':	   (1, {}),	# Add to the end
            'prepend':	   (2, {}),	# Add to the beginning
            'atcursor':	   (3, {}),	# Add at the current cursor
        }),
        'type':				(61,
            _wtype_consts),
        'tab':				(62, {
        }),
        'tab bar':			(63, {
        }),
        'popup is menu':		(64, {
        }),
        'popup is submenu':		(65, {
        }),
        'cursor position':		(66,
            _pairAt16bits),
        'hotkey modifiers':		(67,
	    _modifier_consts),    
    },
    'traversewidget': {
        'children':	(1, {}),	# starting with first child, traverse forward
        'forward':	(2, {}),
        'backward':	(3, {}),	# much slower than forward, avoid it
        'container':	(4, {}),	# 'count' is thenumber of container levels to traverse up
    },
    'writedata': _stop,
    'writecmd': ({
        'nuke':			(1, _stop),
        'grop':			(2, {
            'rect':		  (0x00, _stop),
            'frame':		  (0x10, _stop),
            'slab':		  (0x20, _stop),
            'bar':		  (0x30, _stop),
            'pixel':		  (0x40, _stop),
            'line':		  (0x50, _stop),
            'ellipse':		  (0x60, _stop),
            'fellipse':		  (0x70, _stop),
            'text':		  (0x04, ((_getString,),)),
            'bitmap':		  (0x14, _stop),
            'tilebitmap':	  (0x24, _stop),
            'fpolygon':		  (0x34, _stop),
            'blur':		  (0x44, _stop),
            'rotatebitmap':	  (0x74, _stop),
            'arc':		  (0x08, _stop),
            'resetclip':	  (0x13, _stop),
            'setoffset':	  (0x01, _stop),
            'setclip':		  (0x11, _stop),
            'setsrc':		  (0x21, _stop),
            'setmapping':	  (0x05, {
                'none':		    (0, _stop),
                'scale':	    (1, _stop),
                'squarescale':	    (2, _stop),
                'center':	    (3, _stop),
                _default:	    (0, _stop), # so you can use None
            }),
            'setcolor':		  (0x07, _stop),
            'setfont':		  (0x17, _getFont),
            'setlgop':		  (0x27, {
                'null':		    (0, _stop),
                'none':		    (1, _stop),
                'or':		    (2, _stop),
                'and':		    (3, _stop),
                'xor':		    (4, _stop),
                'invert':	    (5, _stop),
                'invert or':	    (6, _stop),
                'invert and':	    (7, _stop),
                'invert xor':	    (8, _stop),
                'add':		    (9, _stop),
                'subtract':	    (10, _stop),
                'multiply':	    (11, _stop),
                'stipple':	    (12, _stop),
                'alpha':	    (13, _stop),
                _default:	    (1, _stop), # so you can use None
            }),
            'setangle':		  (0x37, _stop),
        }),
        'execfill':		(3, _stop),
        'findgrop':		(4, _stop),
        'setgrop':		(5, _stop),
        'movegrop':		(6, _stop),
        'mutategrop':		(7, _stop),
        'defaultflags':		(8, _stop),
        'gropflags':		(9, _stop),
        'redraw':		(10, _stop),
        'incremental':		(11, _stop),
        'scroll':		(12, _stop),
        'inputmapping':		(13, {
            'none':		  (0, _stop),
            'scale':		  (1, _stop),
            'squarescale':	  (2, _stop),
            'center':		  (3, _stop),
        }),
        'gridsize':		(14, _stop),
        'gropseq':		(15, _gropseq),
        _default:       _stop,
    },),
}

#################################################################
# some useful stuff for importing

# names of all properties
propnames = [name for name in _constants['set'].keys() if type(name) is str]
# namespace for property values, to pass to resolve()
def prop_ns(name):
    return _constants['set'][name][1]

# names of all commands
cmdnames = [name for name in _constants['writecmd'][0].keys() if type(name) is str]
# namespace for command arguments, to pass to resolve()
def cmd_ns(name):
    t = _constants['writecmd'][0][name]
    if t is _stop:
        return {}
    return t[1]

def cmd_code(name):
    t = _constants['writecmd'][0][name]
    if t is _stop:
        return None
    return t[0]

#################################################################
# external API

def resolve(name, namespace=_constants, server=None):
    if namespace is _stop:
        return name, namespace
    if type(namespace) is tuple:
        # for cases where the argument is really supposed to be a string, yet there are contstants after this
        return name, namespace[0]
    if callable(namespace):
        # for cases when we need even stranger processing
        return namespace(name, server)
    if type(name) in (tuple, list):
        # multiple names are or'ed together; namespace returned is the last one
        value = 0
        ns = namespace
        for n in name:
            v, ns = resolve(n, namespace)
            value |= v
        return value, ns
    try:
        lname = name.lower()
    except AttributeError:
        # not a string
        r = namespace.get(_default, (name, namespace))
    else:
        try:
            r = namespace[lname]
        except KeyError:
            r = namespace.get(_default, (name, namespace))
    if r is _stop:
        return name, _stop
    if type(r) == tuple and len(r) == 2:
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
        return resolve(r, namespace)
    # otherwise, it's just a namespace
    return None, r

def unresolve(name, namespace=_constants, server=None):
    if callable(namespace):
        # If we need additional processing,
        # call the namespace with the reverse flag on
        return namespace(name, server, 1)
    for n in namespace.keys():
        if namespace[n][0] == name:
            return n
    return name
