#!/usr/bin/env python

# Author: David Goodger
# Contact: goodger@users.sourceforge.net
# Revision: $Revision: 1.8 $
# Date: $Date: 2002/10/18 04:55:21 $
# Copyright: This module has been placed in the public domain.

"""
A minimal front end to the Docutils Publisher, producing HTML.
"""

import locale
try:
    locale.setlocale(locale.LC_ALL, '')
except:
    pass

from docutils.core import Publisher, default_usage, default_description

description = ('PicoGUI book builder' + default_description)

source_path = 'pg1.txt'
options = {'html': {'stylesheet_path':'pgbook.css'},
           'latex': {'stylesheet':'style.tex'},
           }
extensions = {'html': 'html', 'latex':'tex'}

def publish(writer_name):

    reader=None
    reader_name='standalone'
    parser=None
    parser_name='restructuredtext'
    writer=None
    settings=None
    settings_spec=None
    settings_overrides=options[writer_name]
    config_section=None
    enable_exit=1
    argv=[]
    usage=default_usage

    pub = Publisher(reader, parser, writer, settings=settings)
    pub.set_components(reader_name, parser_name, writer_name)
    settings = pub.get_settings(settings_spec=settings_spec,
                                config_section=config_section)
    if settings_overrides:
        settings._update(settings_overrides, 'loose')
    source = file(source_path)
    pub.set_source(source, source_path)
    destination_path = 'pg1.' + extensions[writer_name]
    destination = file(destination_path, 'w')
    pub.set_destination(destination, destination_path)
    pub.publish(argv, usage, description, settings_spec, settings_overrides,
                config_section=config_section, enable_exit=enable_exit)


if __name__ == '__main__':
    import sys, os
    do_dvi = do_pdf = False

    formats = sys.argv[1:] or ('html',)
    if 'pdf' in formats:
        do_dvi = do_pdf = True
        formats.remove('pdf')
    if 'dvi' in formats:
        do_pdf = True
        formats.remove('dvi')
    if do_dvi and ('latex' not in formats):
        formats.append('latex')

    sys.argv = [sys.argv[0]]

    for format in formats:
        print 'building', format
        try:
            publish(format)
        except ImportError:
            print 'unknown format %r - skipping' % format

    if do_dvi:
        print 'building dvi'
        if os.system('latex pg1.tex'):
            sys.exit(2)
        # twice, because it may have changed the .aux file
        if os.system('latex pg1.tex'):
            sys.exit(2)

    if do_pdf:
        print 'building pdf'
        if os.system('dvipdf pg1.dvi'):
            sys.exit(2)
