# Canvas class
# A layer around Widget that makes it easy to access the Canvas methods

#
# Consider this interface DEPRECATED.  It will be removed in the future.
#

class Canvas:
    def __init__(self, widget):
        self.widget = widget

    # Canvas commands, as specified in canvas.h

    def nuke(self):
        self.widget.command(1)

    def grop(self, grop, *params):
        self.widget.command(2, grop, *params)

    def execfill(self, thobj, property, x, y, w, h):
        self.widget.command(3, thobj, property, x, y, w, h)

    def findgrop(self, index):
        self.widget.command(4, index)

    def setgrop(self,  *params):
        self.widget.command(5, *params)

    def movegrop(self,  x, y, w, h):
        self.widget.command(6, x, y, w, h)
        
    def mutategrop(self, type):
        self.widget.command(7, type)
        
    def defaultflags(self, flags):
        self.widget.command(8, flags)
        
    def gropflags(self, flags):
        self.widget.command(9, flags)

    def redraw(self):
        self.widget.command(10)

    def redraw(self):
        self.widget.incremental(11)
        
    def scroll(self, x, y):
        self.widget.command(12, x, y)
        
    def inputmapping(self, x, y, w, h, type):
        self.widget.command(13, x, y, w, h, {
            'none': 0,
            'scale': 1,
            'squarescale': 2,
            'center': 3,
            }[type])

    def gridsize(self, x, y):
        self.widget.command(14, x, y)

    # Gropnode wrappers

    def rect(self, x, y, w, h):
        self.grop(0x00, x, y, w, h)

    def frame(self, x, y, w, h):
        self.grop(0x10, x, y, w, h)

    def slab(self, x, y, w):
        self.grop(0x20, x, y, w, 1)

    def bar(self, x, y, h):
        self.grop(0x30, x, y, 1, h)

    def pixel(self, x, y):
        self.grop(0x40, x, y, 1, 1)

    def line(self, x1, y1, x2, y2):
        self.grop(0x50, x1, y1, x2-x1, y2-y1)

    def ellipse(self, x, y, w, h):
        self.grop(0x60, x, y, w, h)

    def fellipse(self, x, y, w, h):
        self.grop(0x70, x, y, w, h)

    def text(self, x, y, handle):
        self.grop(0x04, x, y, 1, 1, handle)

    def bitmap(self, x, y, w, h, handle):
        self.grop(0x14, x, y, w, h, handle)
    
    def tilebitmap(self, x, y, w, h, handle):
        self.grop(0x24, x, y, w, h, handle)

    def fpolygon(self, x, y, handle):
        self.grop(0x34, x, y, handle)

    def blur(self, x, y, w, h, radius):
        self.grop(0x44, x, y, w, h, radius)

    def rotatebitmap(self, x, y, w, h, handle):
        self.grop(0x74, x, y, w, h, handle)

    def resetclip(self):
        self.grop(0x13)

    def setoffset(self, x, y, w, h):
        self.grop(0x01, x, y, w, h)

    def setclip(self, x, y, w, h):
        self.grop(0x11, x, y, w, h)

    def setsrc(self, x, y, w, h):
        self.grop(0x21, x, y, w, h)

    def setmapping(self, x, y, w, h, type):
        self.grop(0x05, x, y, w, h, {
            'none': 0,
            'scale': 1,
            'squarescale': 2,
            'center': 3,
            }[type])

    def setcolor(self, color):
        self.grop(0x07, color)

    def setfont(self, handle):
        self.grop(0x17, handle)

    def setlgop(self, lgop):
        self.grop(0x27, {
            'null': 0,
            'none': 1,
            'or': 2,
            'and': 3,
            'xor': 4,
            'invert': 5,
            'invert or': 6,
            'invert and': 7,
            'invert xor': 8,
            'add': 9,
            'subtract': 10,
            'multiply': 11,            
            'stipple': 12,            
            'alpha': 13,            
            }[lgop])

    def setangle(self, angle):
        self.grop(0x37, angle)



