#
# Base class for informational CGI scripts on navi. Provides a nice XHTML+CSS2
# interface that lets you display several collapsable sections of information,
# and optionally auto-refresh the page.
#
# -- Micah Dowty <micah@picogui.org>
#

import cgitb
cgitb.enable()
import cgi, time, os, re
from StringIO import StringIO

def collapseWhitespace(text):
    return re.sub("\s+", " ", text)
        
def foldText(text, lineLength=120):
    # This code collapses multiple whitespaces, then packs the
    # resulting tokens into reasonably long lines.
    line = ''
    maxLineWidth = 120
    output = ''
    for token in text.split(" "):
        if len(line) + len(token) > maxLineWidth:
            output += line + '\n'
            line = ''
        if line:
            line = line + ' ' + token
        else:
            line = token
    output += line + '\n'
    return output

class NaviPage:
    plainTitle = "Title"
    htmlTitle = None
    subtitle = "Subtitle"
    allSections = []
    defaultSections = None
    parameters = [
        ('sections', lambda x: x.split(" ")),
        'css',
        ('refresh', int),
        ]
    css = "http://navi.picogui.org/svn/picogui/trunk/tools/css/navi_cgi.css"
    refresh = None
    footer = '<a href="/"><img src="/images/web/navi64.png" width="64" height="39" alt="Navi"/></a>'
            
    def __init__(self, form=None):
        # Defaults
        if self.defaultSections is None:
            self.defaultSections = self.allSections
        if self.htmlTitle is None:
            self.htmlTitle = self.plainTitle
        self.sections = self.defaultSections
        
        # Allow them to be overridden by CGI parameters
        if form is None:
            form = cgi.FieldStorage()
        self.form = form
        self.cgiInit()

    def cgiInit(self):
        """Set the output content-type, and parse form arguments"""
        print "Content-type: text/html\n"        
        for parameter in self.parameters:
            try:
                if type(parameter) == str:
                    name = parameter
                    translator = lambda x: x
                else:
                    name = parameter[0]
                    translator = parameter[1]
                setattr(self, name, translator(self.form[name].value))
            except KeyError:
                pass

    def linkURL(self, formKeys={}, useExistingForm=True):
        """Create a link to ourselves, including possibly-modified form values"""
        # Copy form attributes to a dictionary we can modify
        mutableForm = {}
        if useExistingForm:
            for key in self.form.keys():
                mutableForm[key] = self.form[key].value
        mutableForm.update(formKeys)

        # Figure out the name of this script so we can link to ourselves
        try:
            scriptName = os.getenv("REQUEST_URI").split("?")[0].split("/")[-1]
        except AttributeError:
            # We're not running as a CGI? Take a guess...
            try:
                scriptName = __file__
            except NameError:
                scriptName = __name__

        # Stick together a new URL
        attributes = []
        for key in mutableForm:
            value = mutableForm[key]
            if value is not None:
                attributes.append("%s=%s" % (key, mutableForm[key]))
        if attributes:
            return scriptName + "?" + "&".join(attributes)
        else:
            return scriptName

    def run(self):
        doc = StringIO()
        for section in ['header'] + self.sections + ['footer']:
            getattr(self, 'section_%s' % section)(doc.write)

        # It's hard to properly pretty-print HTML like this generates,
        # so for now we'll just strip out all extra whitespace. If the
        # source is going to look bad either way, it might as well save
        # a little bandwidth.
        print foldText(collapseWhitespace(doc.getvalue()))

    def begin_section(self, write, section, title, contentClass="row"):
        write('<div><span class="section">%s</span>' % title)

        # If this isn't already the only section, give some links to hide
        # this section and to view only this section
        if len(self.sections) > 1:
            sectionHideList = []
            for listedSection in self.sections:
                if listedSection != section:
                    sectionHideList.append(listedSection)
            sectionHideLink = self.linkURL({'sections': "+".join(sectionHideList)})
            sectionOnlyLink = self.linkURL({'sections': section})
            write('<a class="section" href="%s">hide section</a>' % sectionHideLink)
            write('<a class="section" href="%s">this section only</a>' % sectionOnlyLink)

        write('</div><div class="section"><div class="sectionTop"></div>')
        write('<div class="%s">' % contentClass)

    def end_section(self, write):
        write("</div></div>")

    def section_header(self, write):
        write("""<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
                                       "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
           <html>
           <head>
              <title>%s</title>
              <style type="text/css" media="all">
                 @import url(%s);
              </style>
           """ % (self.plainTitle, self.css))
        if self.refresh is not None:
            write('<meta http-equiv="refresh" content="%d">' % self.refresh)
        write("""
           </head>
           <body>
           <div class="heading">
              <div class="dateBox">%s</div>
              <div class="title">%s</div>
              <div class="subtitle">%s</div>
              <div class="headingTabs">
              """ % (time.strftime("%c"), self.htmlTitle, self.subtitle))

        if self.sections != self.allSections: 
            write('<a class="headingTab" href="%s">all sections</a>' % self.linkURL({'sections': None}))
        if self.form.keys():
            write('<a class="headingTab" href="%s">defaults</a>' % self.linkURL({}, False))
        if self.refresh:
            write('<a class="headingTab" href="%s">refresh off</a>' % self.linkURL({'refresh': None}))
        else:
            write('<a class="headingTab" href="%s">refresh on</a>' % self.linkURL({'refresh': 30}))

        write("</div></div>")

    def section_footer(self, write):
        write('<div class="footer">%s</div></body></html>' % self.footer)
