from Application import ToolbarApp

FIXME = '''
- there should be a set of buttons in the left-hand side with some common
  operations (directions, Enter, modifiers, backspace, tab).
- the default remaps are incomplete.
- if you move too fast, right trough a sector before pgserver gets a chance
  to post enter/leave events (easiest if you try to move diagonally),
  the remaps get out of sync.
- the canvases are not updating, I must be forgetting something obvious.
- of course, once the proof of concept stage is satisfactory,
  the app should post keyboard triggers to the server.
'''

initial_map = (
    # 4 strings: initial values
    '`258-',
    'qrup]',
    'afj;\\',
    u'zvm/¤',
    )
remaps = {
    # remaps: up, right, down, left
    '`': ('', '', '~', ''),
    '1': ('', '2', '!', ''),
    '2': ('', '3', '@', '1'),
    '3': ('', '', '#', '2'),
    '4': ('', '5', '$', ''),
    '5': ('', '6', '%', '4'),
    '6': ('', '', '^', '5'),
    '7': ('', '8', '&', ''),
    '8': ('', '9', '*', '7'),
    '9': ('', '', '(', '8'),
    '0': ('', '-', ')', ''),
    '-': ('', '', '_', '0'),

    'q': ('Q', 'w', '', ''),
    'r': ('R', 't', '', 'e'),
    'u': ('U', 'i', '', 'y'),
    'p': ('P', '', '', 'o'),
    ']': ('}', '', '', '['),

    'a': ('A', 's', '', ''),
    'f': ('F', 'g', '', 'd'),
    'j': ('J', 'k', '', 'h'),
    ';': (':', "'", '', 'l'),
    '\\': ('|', '', '', ''),

    'z': ('Z', 'x', '', ''),
    'v': ('V', 'b', '', 'c'),
    'm': ('M', ',', '', 'n'),
    '/': ('?', '', '', '.'),
    u'¤': (u'£', '', '', u'¥'),
    }
default_mapping = initial_map, remaps

class SectorBar(ToolbarApp):
    def __init__(self, server=None, boxsize=15, mapping=default_mapping):
        super(SectorBar, self).__init__(title='Sector Input', server=server)
        self.__initial_map, self.__remaps = mapping
        self.__sectors = []
        self.__remapped = []
        self.__boxsize = boxsize
        self.side = 'bottom'
        self.sizemode = 'pixel'
        self.size = boxsize * 4
        box = self.addWidget('box')
        box.side = 'right'
        box.sizemode = 'pixel'
        box.size = boxsize * 5
        box.margin = 0
        for y in range(4):
            if y:
                box = box.addWidget('box', 'after')
            else:
                box = box.addWidget('box', 'inside')
            box.margin = 0
            box.default_relationship = 'inside'
            box.side = 'top'
            box.sizemode = 'pixel'
            box.size = boxsize
            sector = box
            self.__sectors.append([])
            for x in range(5):
                sector = sector.addWidget('canvas')
                self.__sectors[-1].append(sector)
                sector.__which = (x, y)
                sector.sizemode = 'pixel'
                sector.size = boxsize
                sector.side = 'left'
                sector.triggermask |= (1<<11 | 1<<12)
                self._draw(sector)
                self.link(self._enter, sector, 'pntr enter')
                self.link(self._enter, sector, 'pntr down')
                self.link(self._release, sector, 'pntr up')
                self.link(self._release, sector, 'pntr release')
        self.__selected = None

    def _draw(self, sector, key=None):
        if key is None:
            (x, y) = sector.__which
            key = self.__initial_map[y][x]
        else:
            sector.__key = key
        print 'draw:', sector.__which, repr(key)
        if key:
            sector.grop.setcolor(0xffffff)
        else:
            sector.grop.setcolor(0)
        sector.grop.rect(0xffffff, 0, self.__boxsize, self.__boxsize)
        if key:
            sector.grop.setcolor(0)
            sector.grop.text(0, 0, 1, 1, key)
        self.server.updatepart(sector.handle)

    def _reset(self):
        for sector in self.__remapped:
            self._draw(sector)
        self.__remapped = []

    def _enter(self, ev):
        if not ev.buttons:
            return
        print 'enter:', ev.widget.__which
        (x, y) = ev.widget.__which
        sector = self.__sectors[y][x]
        if sector in self.__remapped:
            key = sector.__key
            remapped = True
        else:
            key = self.__initial_map[y][x]
            remapped = False
        # do remapping
        self._reset()
        if remapped:
            self._draw(sector, key)
            self.__remapped = [sector]
        self.__selected = sector
        if key in self.__remaps:
            map = self.__remaps[key]
            print 'remapping:', repr(key), map
            if map[0] and y: # up
                sector = self.__sectors[y-1][x]
                self.__remapped.append(sector)
                self._draw(sector, map[0])
            if map[1] and (x < 4): # right
                sector = self.__sectors[y][x+1]
                self.__remapped.append(sector)
                self._draw(sector, map[1])
            if map[2] and (y < 3): # down
                sector = self.__sectors[y+1][x]
                self.__remapped.append(sector)
                self._draw(sector, map[2])
            if map[3] and x: # left
                sector = self.__sectors[y][x-1]
                self.__remapped.append(sector)
                self._draw(sector, map[3])

    def _release(self, ev):
        if self.__selected is None:
            return
        if self.__selected in self.__remapped:
            key = self.__selected.__key
        else:
            (x, y) = self.__selected.__which
            key = self.__initial_map[y][x]
        print 'release:', self.__selected.__which, repr(key)
        self._reset()
        self.__selected = None


if __name__ == '__main__':
    s = SectorBar()
    try:
        s.run()
    except KeyboardInterrupt:
        pass
    s.shutdown()
