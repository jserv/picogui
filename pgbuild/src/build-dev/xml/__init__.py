"""Core XML support for Python.

This package contains three sub-packages:

dom -- The W3C Document Object Model.  This supports DOM Level 1 +
       Namespaces.

parsers -- Python wrappers for XML parsers (currently only supports Expat).

sax -- The Simple API for XML, developed by XML-Dev, led by David
       Megginson and ported to Python by Lars Marius Garshol.  This
       supports the SAX 2 API.
"""


__all__ = ["dom", "parsers", "sax"]

# When being checked-out without options, this has the form
# "<dollar>Revision: x.y </dollar>"
# When exported using -kv, it is "x.y".
__version__ = "$Revision: 1.11 $".split()[-2:][0]


_MINIMUM_XMLPLUS_VERSION = (0, 6, 1)


### xmlplus hack removed -- Micah
