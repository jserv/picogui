""" PGBuild.CommandLine.Options

Processes command line options using Optik and sorts them out
into the config database.
"""
# 
# PicoGUI Build System
# Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
# 
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#  
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#  
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
# 

import optik
import PGBuild
import PGBuild.Config

class HelpFormatter(optik.IndentedHelpFormatter):
    """Custom help formatting- provides some extra information about
       the program above the 'usage' line.
       """
    def __init__ (self,
                  indent_increment=3,
                  max_help_position=40,
                  width=80,
                  short_first=1):
        optik.IndentedHelpFormatter.__init__(
            self, indent_increment, max_help_position, width, short_first)
        
    def format_usage(self, usage):
        return "%s\n\nusage: %s\n" % (PGBuild.about, usage)
    
class OptionsXML:
    """Convert options from the supplied hash into XML, suitable
       for mounting into the configuration tree.
       """
    def __init__(self, parseResults):
        (self.options, self.args) = parseResults

    def get_contents(self):
        xml = '<pgbuild title="Command Line Options" root="invocation">\n'                
        for option in self.options.__dict__:
            value = getattr(self.options, option)
            if value != None:
                xml += '\t<option name="%s">%s</option>\n' % (option, value)
        for arg in self.args:
            xml += '\t<target name="%s">%s</target>\n' % (arg, self.args[arg])                    
        xml += '</pgbuild>\n'
        return xml


def parse(config, argv):
    """Entry point for PGBuild command line parsing
         config: configuration tree to put results in
           argv: list of command line arguments
    """

    parser = optik.OptionParser(formatter=HelpFormatter(),
                                usage="%prog [options] [targets] ...",
                                version=PGBuild.version)

    configGroup = parser.add_option_group("Configuration Management")
    configGroup.add_option("-t", "--dump-tree", dest="treeDumpFile",
                           help="Dump the configuration tree in XML Build Configuration format to FILE", metavar="FILE")

    config.mount(OptionsXML(parser.parse_args(argv[1:])))


### The End ###
        
    
