import sys
from Nifty import Frame, FileBuffer, ScratchBuffer

frame = Frame("This here is my text editor. Is it not nifty? Worship the text editor!")
if len(sys.argv) == 1:
    frame.open(ScratchBuffer('__scratch__', 'This buffer is for experimenting with Nifty'))
else:
    for name in sys.argv[1:]:
        frame.open(FileBuffer(name))

sys.exit(frame.run())
