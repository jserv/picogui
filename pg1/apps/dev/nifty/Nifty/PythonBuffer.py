from FileBuffer import FileBuffer
from Frame import file_detectors
import os, imp

unindenters = (
    'return',
    'break',
    'continue',
    'raise',
    'pass',
)

_source_suffix = ''
for _info in imp.get_suffixes():
    if _info[2] == imp.PY_SOURCE:
        _source_suffix = _info[0]
del _info

def _find_module(modname, searchpath=None):
    mfile = None
    for partname in modname.split('.'):
        if mfile is not None:
            mfile.close()
        (mfile, filename, (suffix, mode, mtype)) = imp.find_module(partname, searchpath)
        if mtype in (imp.PY_SOURCE, imp.PY_COMPILED):
            break
        module = imp.load_module(partname, mfile, filename, (suffix, mode, mtype))
        try:
            searchpath = module.__path__
        except AttributeError:
            raise ImportError, 'No source for module ' + module.__name__
    if mfile is not None:
        mfile.close()
    if mtype == imp.PY_COMPILED:
        filename = filename[:-len(suffix)] + _source_suffix
        if os.path.exists(filename):
            mtype = imp.PY_SOURCE
        else:
            raise ImportError, 'No source for module ' + partname
    return filename, mtype

class PythonBuffer(FileBuffer):
    "Buffer class for Python source code"
    # TODO: try to integrate BicycleRepairMan

    def __init__(self, path='', module=None):
        if module is not None:
            # open a module from the path
            # XXX Ought to insert current file's directory in front of search path
            (filename, mtype) = _find_module(module)
            # could get NameError, ImportError - we pass these trough
            if mtype != imp.PY_SOURCE:
                raise ImportError, "%s is not a source module (%s)" % (path, mtype)
            path = filename
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
    if text[0].startswith('#!') and (text[0].find('python') >= 0):
        return PythonBuffer(path)
    header = ''.join(text[:2])
    if header.find('-*-') >= 0:
        if (header.find('-*- python -*-' >= 0) or header.find('mode: python') >= 0):
            return PythonBuffer(path)

file_detectors.insert(0, detector)
