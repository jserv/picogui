import __builtin__, keyword

class BaseCompleter(object):
    "Base class for completers"

    def namespaces(self, nslist):
        "return the namespaces for this instance"
        return nslist

    def parse(self, text):
        "find if we must do any delegation"
        raise NotImplemented

    def complete(self, text, *nslist):
        "return the text, filled up to where completion was possible"
        # it is important that all completion exceptions are silently ignored.
        import sys
        print >>sys.stderr, 'attempting completion:', self.__class__.__name__, repr(text)
        try:
            completer, fragment = self.parse(text)
            if completer is None:
                completions = []
                for ns in self.namespaces(nslist):
                    for name in ns.keys():
                        if name.startswith(fragment):
                            completions.append(name)
                if len(completions) == 0:
                    print >>sys.stderr, 'nothing found'
                    return text
                completions.sort()
                print >>sys.stderr, 'found:', completions
                largest = completions[0]
                for name in completions[1:]:
                    if name.startswith(largest):
                        continue
                    if largest.startswith(name):
                        largest = name
                        continue
                    for i in range(len(fragment), min(len(largest), len(name))):
                        if largest[i] != name[i]:
                            largest = largest[:i]
                            break
                print >>sys.stderr, 'largest common prefix:', repr(largest)
                return text[:-len(fragment)] + largest
            else:
                if fragment:
                    text = text[:-len(fragment)]
                return (text + completer.complete(fragment, *nslist))
        except:
            print >>sys.stderr, 'completion error'
            import traceback
            traceback.print_exc()
            return text

import re
_python_identifiers = re.compile('[a-zA-Z_][a-zA-Z0-9_]*')
_marker = []

class PythonAttr(BaseCompleter):
    "completer for attributes of Python objects"

    def __init__(self, name, parent=_marker):
        self.name = name
        if parent is _marker:
            self._self = _marker
        else:
            self._self = getattr(parent, name)

    def self(self, nslist):
        if self._self is not _marker:
            return self._self
        for ns in nslist:
            try:
                self._self = ns[self.name]
            except KeyError:
                continue
            return self._self
        raise NameError, self.name

    def namespaces(self, nslist):
        "return the namespaces for this instance"
        it = self.self(nslist)
        d = {}
        for name in dir(it):
            d[name] = getattr(it, name)
        return (d,)

    def parse(self, text):
        "find if we must do any delegation"
        text = text.lstrip()
        if not text:
            return None, text
        id = _python_identifiers.match(text)
        if (not id) or id.group() == text:
            return None, text
        # things start to get interesting
        import sys
        if text[id.end()] == '.':
            print >>sys.stderr, 'attribute of', id.group()
            return PythonAttr(id.group(), self._self), text[id.end()+1:]
        # something went wrong... must guess
        [id for id in _python_identifiers.finditer(text)]
        if id.end() == len(text):
            return None, id.group()
        else:
            return None, ''

class PythonExpr(BaseCompleter):
    "completer for standalone Python expressions"

    def namespaces(self, nslist):
        "return the namespaces for this instance"
        return nslist + (__builtin__.__dict__,)

    def parse(self, text):
        "find if we must do any delegation"
        text = text.lstrip()
        if not text:
            return None, text
        id = _python_identifiers.match(text)
        if (not id) or id.group() == text:
            return None, text
        # things start to get interesting
        import sys
        if text[id.end()] == '.':
            print >>sys.stderr, 'attribute of', id.group()
            return PythonAttr(id.group()), text[id.end()+1:]
        # something went wrong... must guess
        [id for id in _python_identifiers.finditer(text)]
        if id.end() == len(text):
            return None, id.group()
        else:
            return None, ''

class PythonStatement(PythonExpr):
    "completer for standalone Python statements - eg for the Minibuffer"

    def namespaces(self, nslist):
        "return the namespaces for this instance"
        return PythonExpr.namespaces(self, nslist) + (keyword.kwdict,)

    def parse(self, text):
        "find if we must do any delegation"
        text = text.lstrip()
        if not text:
            return None, text
        cmd = _python_identifiers.match(text)
        if cmd.group() == text:
            return None, text
        if cmd.group() in keyword.kwlist:
            # it would be cool to complete modules... yumm.
            return python_expr, text[cmd.end():]
        return python_expr, text

python_expr = PythonExpr()
python_stmt = PythonStatement()
