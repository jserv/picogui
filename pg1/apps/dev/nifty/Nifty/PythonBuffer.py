from FileBuffer import FileBuffer
from Frame import file_detectors
import os

unindenters = (
    'return',
    'break',
    'continue',
    'raise',
    'pass',
)

class PythonBuffer(FileBuffer):
    "Buffer class for Python source code"
    # TODO: try to integrate BicycleRepairMan

    def __init__(self, path='', module=None):
        if module is not None:
            # open a module from the path
            pass
        FileBuffer.__init__(self, path)
        self.indent_step = 4
        self.autoindent = True

    def _get_indentation(self, lines, index, unindent=False):
        if index == 0:
            return 0
        prevl = lines[index-1].expandtabs()
        previous = len(prevl) - len(prevl.lstrip())
        if unindent:
            return max(0, previous - self.indent_step)
        prevl = prevl.strip()
        for word in unindenters:
            if prevl.startswith(word):
                return max(0, previous - self.indent_step)
        if prevl and prevl[-1] == ':':
            return previous + self.indent_step
        return previous

def detector(path):
    if path.endswith('.py'):
        return PythonBuffer(path)
    if not os.path.exists(path):
        return
    try:
        f = file(path)
    except:
        return
    text = f.readlines()
    f.close()
    print repr(text)
    if text[0].startswith('#!') and (text[0].find('python') >= 0):
        return PythonBuffer(path)
    header = ''.join(text[:2])
    if header.find('-*-') >= 0:
        if (header.find('-*- python -*-' >= 0) or header.find('mode: python') >= 0):
            return PythonBuffer(path)

file_detectors.insert(0, detector)
