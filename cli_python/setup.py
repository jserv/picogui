# patch distutils if it can't cope with the "classifiers" keyword
import sys
if sys.version < '2.2.3':
    from distutils.dist import DistributionMetadata
    DistributionMetadata.classifiers = None

from distutils.core import setup
setup (name = "PicoGUI",
       version = "0.44",
       description = "PicoGUI python client library",
       maintainer = "Lalo Martins",
       maintainer_email = "lalo@laranja.org",
       url = "http://www.picogui.org/",
       long_description = "The Python client library for the PicoGUI user interface system",
       keywords = "picogui",
       classifiers = [
           "Development Status :: 3 - Alpha",
           "Intended Audience :: Developers",
           "License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)",
       ],
       license = "LGPL",
       packages = ['PicoGUI']
       )
