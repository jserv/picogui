import PicoGUI, PicoGUI.keys, struct, gc, os
from PicoGUI.infilter import Trigger

def shift(c):
    try:
        return c.upper()
    except:
        return c

def ctrl():
    pass

tables = [
    # alpha
    (
        # mode 0 (alpha)
        (
            ('a', 's', 'k', 'm', None, 'q'),
            (1, 't', 2),
            ('x', 'f', 'n', None, 'r', None, 'p'),
            ('h', None, None, 'backspace', None, 'c'),
            (None, None, 'u', None, 'space', None, None, 'y'),
            ('v', None, None, 'w', None, 'o', 'g', 'z'),
            (None, None, None, None, None, '\n', 'e', ctrl),
            (None, None, 'j', None, 'l', 'b', 'd', 'i'),
        ),
        # mode 1 (symbols)
        (
            ('`', '^', 'k', '\\', None, 'q'),
            (1, '!', 2),
            ('x', '"', "'", None, '|', None, 'p'),
            ('h', None, None, '\x08', None, 'c'),
            (None, None, 'u', None, ' ', None, None, 'y'),
            ('v', None, None, '&', None, '@', '~', 'z'),
            (None, None, None, None, None, '\n', '?', ctrl),
            (None, None, 'j', None, ':', 'b', '_', ';'),
        ),
    ),
    # numeric
    (
        # mode 0 (numbers)
        (
            ('1', '(', '[', '%', None, '$'),
            (1, '2', 2),
            (']', ')', '3', None, '/', None, '*'),
            ('4', None, None, '\x08', None, '6'),
            (None, None, '5', None, ' ', None, None, '7'),
            ('+', None, None, '-', None, '8', '=', '<'),
            (None, None, None, None, None, '\n', '9', ctrl),
            (None, None, '#', None, ',', '>', '.', '0'),
        ),
        # mode 1 (symbols)
        (
            ('¬', '{', '«', '÷', None, '¢'),
            (1, '¡', 2),
            ('»', '}', 'µ', None, '×', None, '¤'),
            ('4', None, None, '\x08', None, '6'),
            (None, None, '5', None, ' ', None, None, '7'),
            ('euro', None, None, '£', None, '©', '¶', 'º'),
            (None, None, None, None, None, '\n', '¿', ctrl),
            (None, None, '|', None, '¥', 'ª', '§', '®'),
        ),
    ),
]
# shift alpha
tables[0] += (tuple([tuple([shift(c) for c in t]) for t in tables[0][0]]),)
tables = tuple(tables)

class QWPad(object):
    def __init__(self, height, app=None):
        self.height = height
        self.app = PicoGUI.ToolbarApp('nqw', parent=app)
        self.app.side = 'bottom'
        self.app.size = self.height+5
        #self.app.margin = 0
        alpha = self.app.addWidget('box')
        alpha.side = 'left'
        alpha.size = self.height
        alpha.sizemode = 'pixel'
        alpha.margin = 0
        right = alpha.addWidget('flatbutton', 'inside')
        right.side = 'right'
        right.sizemode = 'pixel'
        right.size = self.height/3
        right.thobj_button_hilight = 'flatbutton'
        right.font = 'clean:6'
        topright = right.addWidget('flatbutton', 'inside')
        topright.side = 'top'
        topright.sizemode = 'pixel'
        topright.size = self.height/3
        topright.thobj_button_hilight = 'flatbutton'
        topright.font = 'clean:6'
        btright = topright.addWidget('flatbutton')
        btright.side = 'bottom'
        btright.sizemode = 'pixel'
        btright.size = self.height/3
        btright.thobj_button_hilight = 'flatbutton'
        btright.font = 'clean:6'
        left = right.addWidget('flatbutton')
        left.side = 'left'
        left.sizemode = 'pixel'
        left.size = self.height/3
        left.thobj_button_hilight = 'flatbutton'
        left.font = 'clean:6'
        topleft = left.addWidget('flatbutton', 'inside')
        topleft.side = 'top'
        topleft.sizemode = 'pixel'
        topleft.size = self.height/3
        topleft.thobj_button_hilight = 'flatbutton'
        topleft.font = 'clean:6'
        btleft = topleft.addWidget('flatbutton')
        btleft.side = 'bottom'
        btleft.sizemode = 'pixel'
        btleft.size = self.height/3
        btleft.thobj_button_hilight = 'flatbutton'
        btleft.font = 'clean:6'
        top = left.addWidget('flatbutton')
        top.side = 'top'
        top.sizemode = 'pixel'
        top.size = self.height/3
        top.thobj_button_hilight = 'flatbutton'
        top.font = 'clean:6'
        bottom = top.addWidget('flatbutton')
        bottom.side = 'bottom'
        bottom.sizemode = 'pixel'
        bottom.size = self.height/3
        bottom.thobj_button_hilight = 'flatbutton'
        bottom.font = 'clean:6'
        center = bottom.addWidget('canvas')
        center.side = 'all'
        center.sizemode = 'pixel'
        center.size = self.height/3
        center.grop.setcolor(0xff9900)
        center.grop.rect(0, 0, center.width, center.height)
        center.triggermask |= (1<<11 | 1<<12)
        self.drawables = (topleft, top, topright, left, right, btleft, bottom, btright)
        self.app.link(self.enter, center, 'pntr enter')
        self.app.link(self.leave, center, 'pntr leave')
        self.app.link(lambda ev: self.set_mode(0), center, 'pntr up')
        # FIXME: VR3-specific, should also check for apm devices
        if os.path.exists('/dev/adif'):
            self.batt = self.app.addWidget('label')
            self.batt.transparent = False
            self.batt.side = 'right'
            self.batt.text = '???'
        self.primary = None
        self.mode = 0
        self.modelock = False

        self.set_mode()

    def hide(self):
        self.app.size = 0

    def unhide(self):
        self.app.size = self.height

    def toggle_hide(self):
        if self.app.size:
            self.hide()
        else:
            self.unhide()

    def nuke_all(self):
        for button in self.drawables:
            button.text = ''
            self.app.server.updatepart(button.handle)
        try:
            self.batt.text = self.find_batt_voltage()
        except:
            # could be that batt doesn't exist, could be that /dev/adif doesn't exist...
            pass

    def set_mode(self, m=0):
        print '(changing mode to %s)' % m
        if m and self.mode == m:
            if self.modelock:
                m = 0
            self.modelock = not self.modelock
        else:
            self.modelock = False
        self.mode = m
        # draw template?

    def get_quadrant(self, ev):
        if ev.x < self.height/12 or ev.x > 0x800:
            if ev.y < self.height/12 or ev.y > 0x800:
                return 0
            if ev.y > self.height/4:
                return self.height/12
            return 3
        if ev.x > self.height/4:
            if ev.y < self.height/12 or ev.y > 0x800:
                return 2
            if ev.y > self.height/4:
                return 7
            return 4
        if ev.y < self.height/6 or ev.y > 0x800:
            return 1
        return 6

    def post_key(self, key):
        trigger = Trigger(name='key down')
        trigger.key = PicoGUI.keys.keys.get(key, None) or ord(key)
        trigger.mods = 0
        self.app.server.infiltersend(trigger)
        trigger.name = 'char'
        self.app.server.infiltersend(trigger)
        trigger.name = 'key up'
        self.app.server.infiltersend(trigger)

    def enter(self, ev):
        if not ev.buttons:
            return
        if self.primary is None:
            return
        secondary = self.get_quadrant(ev)
        try:
            k = tables[0][self.mode][self.primary][secondary]
        except:
            k = None
        if k is None:
            print 'invalid gesture: %r %r' % (self.primary, secondary)
        elif type(k) is str:
            self.post_key(k)
            if not self.modelock:
                self.set_mode()
        elif type(k) is int:
            self.set_mode(k)
        self.primary = None
        self.nuke_all()

    def leave(self, ev):
        if not ev.buttons:
            return
        self.primary = self.get_quadrant(ev)
        t = tables[0][self.mode][self.primary]
        self.nuke_all()
        for secondary in range(8):
            try:
                k = t[secondary]
            except:
                k = None
            if type(k) is str:
                button = self.drawables[secondary]
                button.text = k
                self.app.server.updatepart(button.handle)

    def find_batt_voltage(self):
        # FIXME: VR3-specific, should also check for apm devices
        dev = file('/dev/adif')
        # for some reason, flbatt tells us to always read 4 registers...
        # probably the device gets confused if we don't
        status = struct.unpack('Hxxxxxx', dev.read(8))[0]
        dev.close()
        return 3.3 * status / 1024

    def run(self):
        self.app.run()
        self.app.shutdown()
        gc.collect()

if __name__ == '__main__':
    import sys
    if 'embedded' in sys.argv:
        print 'testing embedded mode'
        app = PicoGUI.InvisibleApp('nqw test')
        QWpad(120, app)
        app.run()
    else:
        QWPad(120).run()
