# $Id: test_pattern.py,v 1.1 1999/04/11 16:09:15 dieter Exp dieter $
"""Test for PyXPath."""

from xml.dom.sax_builder import SaxBuilder
from xml.sax.saxexts import make_parser
from xml.sax.saxutils import ErrorPrinter
from dmutil.xsl.xpath import makeParser, Env
from dmutil.xsl.DomFactory import IdDecl
import dmutil.xsl 

##############################################################################
# build a DOM tree
dh= SaxBuilder()
p= make_parser(); p.setDocumentHandler(dh); p.setErrorHandler(ErrorPrinter())
p.parse('%s/test_pattern.xml' % dmutil.xsl.__path__[0]); p.close()
domtree= dh.document

##############################################################################
# create an idmap provider
#   this is only necessary when "id" patterns are used
idmapper= IdDecl({'product':'id'})
# apply it to "domtree"
#   this gives "domtree" the infrastructure for "id" pattern handling
idmapper(domtree)
d= domtree

##############################################################################
# create Parser and variable environment
p= makeParser(); e=Env(); e.setVariable('x','Hallo')

# child::para
s= p('child::xsa').eval(d,[d],e)
assert len(s) == 1
d= s[0]

# child::*
s= p('child::*').eval(d,[d],e)
assert len(s) == 4

# child::text()
s= p('child::text()').eval(d,[d],e)

# child::node()
s= p('child::node()').eval(d,[d],e)

# attribute::name
d= p('child::product').eval(d,[d],e)[0]
s= p('attribute::id').eval(d,[d],e)
assert len(s) == 1
s= p('attribute::*').eval(d,[d],e)
assert len(s) == 1

# descendant
d= domtree.firstChild
s= p('descendant::product').eval(d,[d],e)
assert len(s) == 3

# descendant-or-self
s= p('descendant-or-self::product').eval(d,[d],e)
assert len(s) == 3
s= p('descendant-or-self::xsa').eval(d,[d],e)
assert len(s) == 1

# self
s= p('self::xsa').eval(d,[d],e)
assert len(s) == 1

# child::chapter/descendant::para
s= p('child::xsa/descendant::version').eval(domtree,[domtree],e)
assert len(s) == 3

# child::*/child::para
s= p('child::xsa/child::product').eval(domtree,[domtree],e)
assert len(s) == 3

# root
s= p('/').eval(d,[d],e)
assert len(s) == 1 and s[0]._node == domtree._node

# /descendant::para
s= p('/descendant::product').eval(domtree,[domtree],e)
assert len(s) == 3


# /descendant::olist/child::item
s=p('/descendant::product/child::version').eval(d,[d],e)
assert len(s) == 3

# child::para[position()=1]
s=p('child::product[position()=1]').eval(d,[d],e)
assert len(s) == 1 and s[0].getAttribute('id') == 'weakdict'

# child::para[position()=last()]
s=p('child::product[position()=last()]').eval(d,[d],e)
assert len(s) == 1 and s[0].getAttribute('id') == 'xslpattern'

# child::para[position()=last()-1]
s=p('child::product[position()=last()-1]').eval(d,[d],e)
assert len(s) == 1 and s[0].getAttribute('id') == 'addcontenttable'

# following-sibling::chapter[position()=1]
s=p('following-sibling::product[position()=1]').eval(d.firstChild,[d.firstChild],e)
assert len(s) == 1 and s[0].getAttribute('id') == 'weakdict'

# preceding-sibling::chapter[position()=1]
s=p('preceding-sibling::product[position()=1]').eval(d.lastChild,[d.lastChild],e)
assert len(s) == 1 and s[0].getAttribute('id') == 'xslpattern'

# /descendant::figure[position()=42]
s=p('/descendant::product[position()=2]').eval(d,[d],e)
assert len(s) == 1 and s[0].getAttribute('id') == 'addcontenttable'

# /child::doc/child::chapter[position()=5]/child::section[position()=2]
s=p('/child::xsa/child::product[position()=2]/child::version[position()=1]').eval(d,[d],e)
assert len(s) == 1

# child::para[attribute::type="warning"]
s=p('child::product[attribute::id="weakdict"]').eval(d,[d],e)
assert len(s) == 1

# child::para[attribute::type="warning"][position()=5]
s=p('child::product[attribute::id="weakdict"][position()=1]').eval(d,[d],e)
assert len(s) == 1

# child::para[position()=5][attribute::type="warning"]
s=p('child::product[position()=2][attribute::id="addcontenttable"]').eval(d,[d],e)
assert len(s) == 1

# child::chapter[child::title="Introduction"]
s=p('child::product[child::version="0.02"]').eval(d,[d],e)
assert len(s) == 2

# child::chapter[child::title]
s=p('child::product[child::version]').eval(d,[d],e)
assert len(s) == 3

# child::*[self::chapter or self::appendix]
s=p('child::*[self::vendor or self::product]').eval(d,[d],e)
assert len(s) == 4

# child::*[self::chapter or self::appendix][position()=last()]
s=p('child::*[self::vendor or self::product][position()=last()]').eval(d,[d],e)
assert len(s) == 1

# para
s=p('product').eval(d,[d],e)
assert len(s) == 3

# *
s=p('*').eval(d,[d],e)
assert len(s) == 4

# text()
s=p('text()').eval(d,[d],e)
assert s

# @name
s=p('@id').eval(d.lastChild.previousSibling,[d.lastChild.previousSibling],e)
assert len(s) == 1

# @*
s=p('@*').eval(d.lastChild.previousSibling,[d.lastChild.previousSibling],e)
assert len(s) == 1

# para[1]
s=p('product[1]').eval(d,[d],e)
assert len(s) == 1 and s[0].getAttribute('id') == 'weakdict'

# para[last()]
s=p('product[last()]').eval(d,[d],e)
assert len(s) == 1 and s[0].getAttribute('id') == 'xslpattern'

# */para
s=p('*/version').eval(d,[d],e)
assert len(s) == 3

# /doc/chapter[5]/section[2]
s=p('/xsa/product[2]/version[1]').eval(d,[d],e)
assert len(s) == 1

# chapter//para
s=p('*//product').eval(domtree,[domtree],e)
assert len(s) == 3

# //para
s=p('//version').eval(d,[d],e)
assert len(s) == 3

# //olist/item
s=p('//product/version').eval(d,[d],e)
assert len(s) == 3

# .
s=p('.').eval(d,[d],e)
assert len(s) == 1

# .//para
s=p('.//vendor').eval(d,[d],e)
assert len(s) == 1

# ..
s=p('..').eval(d,[d],e)
assert len(s) == 1

# ../@lang
x=p('//version').eval(d,[d],e)[0]
s=p('../@id').eval(x,[x],e)
assert len(s) == 1 and s[0].nodeValue == 'weakdict'

# para[@type="warning"]
s=p('product[@id="weakdict"]').eval(d,[d],e)
assert len(s) == 1

# para[@type="warning"][5]
s=p('product[@id="weakdict"][1]').eval(d,[d],e)
assert len(s) == 1

# para[5][@type="warning"]
s=p('product[1][@id="weakdict"]').eval(d,[d],e)
assert len(s) == 1

# chapter[title="Introduction"]
s=p('product[version="0.02"]').eval(d,[d],e)
assert len(s) == 2

# chapter[title]
s=p('product[version]').eval(d,[d],e)
assert len(s) == 3

# employee[@secretary and @assistant]
s=p('product[version and @id]').eval(d,[d],e)
assert len(s) == 3

# Variable Reference
e.setVariable('x',1)
s=p('$x').eval(d,[d],e)
assert s == 1

# |
s=p('product | vendor').eval(d,[d],e)
assert len(s) ==  4

# /
s= p('(product | vendor) / version').eval(d,[d],e)
assert len(s) == 3

# //
s= p('(product | vendor) // version').eval(d,[d],e)
assert len(s) == 3

# or
s= p('true() or false()').eval(d,[d],e)
assert s == 1
s= p('false() or false()').eval(d,[d],e)
assert s == 0

#and
s= p('true() and false()').eval(d,[d],e)
assert s == 0
s= p('true() and true()').eval(d,[d],e)
assert s == 1

# RelationalExpr
assert p('.//version <= id("addcontenttable")/version').eval(d,[d],e) == 1
assert p('.//version > id("addcontenttable")/version').eval(d,[d],e) == 0
assert p('.//version <= 0.02').eval(d,[d],e) == 1
assert p('-1 < .//version').eval(d,[d],e) == 1
assert p('.//version < "a"').eval(d,[d],e) == 1
assert p('.//version > false()').eval(d,[d],e) == 1
assert p('.//version/@id < true()').eval(d,[d],e) == 1
assert p('"-4.1" > -4.11').eval(d,[d],e) == 1
assert p('"1" >= true()').eval(d,[d],e) == 1
assert p('false() <= 0.1').eval(d,[d],e) == 1

# EqualityExpr
assert p('.//version = id("addcontenttable")/version').eval(d,[d],e) == 1
assert p('.//version != id("addcontenttable")/version').eval(d,[d],e) == 1
assert p('.//version = 0.020').eval(d,[d],e) == 1
assert p('-1 = .//version').eval(d,[d],e) == 0
assert p('.//version = "0.020"').eval(d,[d],e) == 0
assert p('.//version = true()').eval(d,[d],e) == 1
assert p('.//version/@id = false()').eval(d,[d],e) == 1
assert p('"-4.1" = -4.10').eval(d,[d],e) == 1
assert p('"1" = true()').eval(d,[d],e) == 1
assert p('false() = ""').eval(d,[d],e) == 1

# Numbers
assert p('5 mod 2').eval(d,[d],e) == 1
assert p('5 mod -2').eval(d,[d],e) == 1
assert p('-5 mod 2').eval(d,[d],e) == -1
assert p('-5 mod -2').eval(d,[d],e) == -1
assert p('5 div 2').eval(d,[d],e) == 2.5
assert p('5 div -2').eval(d,[d],e) == -2.5
assert p('-5 div 2').eval(d,[d],e) == -2.5
assert p('-5 div -2').eval(d,[d],e) == 2.5
assert p('1.5+1.3').eval(d,[d],e) == 2.8
assert abs(p('1.5-1.3').eval(d,[d],e) - 0.2) < 1e-10
assert abs(p('4.2*3').eval(d,[d],e) - 12.6) < 1e-10
assert p('-1').eval(d,[d],e) == -1

# NodeSetFunctions
assert p('last()').eval(d,[1,2,3],e) == 3
assert p('position()').eval(d,[domtree,domtree,d,domtree],e) == 3
assert p('count(//version)').eval(d,[d],e) == 3
s= p('id(//@id)').eval(d,[d],e)
assert len(s) == 3
s= p('id("weakdict addcontenttable")').eval(d,[d],e)
assert len(s) == 2
assert p('local-part()').eval(d,[d],e) == 'xsa'
assert p('local-part(product)').eval(d,[d],e) == 'product'
assert p('name()').eval(d,[d],e) == 'xsa'
assert p('name(product)').eval(d,[d],e) == 'product'

#String Functions
assert p('string("1")').eval(d,[d],e) == "1"
assert p('string(1)').eval(d,[d],e) == "1"
assert p('string(true())').eval(d,[d],e) == "true"
assert p('string(false())').eval(d,[d],e) == "false"
assert p('string(product/version)').eval(d,[d],e) == "0.01"
p('string()').eval(d,[d],e)
assert p('concat("1","2")').eval(d,[d],e) == "12"
assert p('concat("1","2","3")').eval(d,[d],e) == "123"
assert p('starts-with("1234","123")').eval(d,[d],e) == 1
assert p('starts-with("1234","23")').eval(d,[d],e) == 0
assert p('contains("1234","23")').eval(d,[d],e) == 1
assert p('contains("1234","203")').eval(d,[d],e) == 0
assert p('substring-before("1999/04/01","/")').eval(d,[d],e) == '1999'
assert p('substring-before("1999/04/01","*")').eval(d,[d],e) == ''
assert p('substring-after("1999/04/01","/")').eval(d,[d],e) == '04/01'
assert p('substring-after("1999/04/01","*")').eval(d,[d],e) == ''
assert p('substring("12345",2,3)').eval(d,[d],e) == '234'
assert p('substring("12345",2)').eval(d,[d],e) == '2345'
assert p('substring("12345",1.5,2.6)').eval(d,[d],e) == "234"
assert p('substring("12345",0,3)').eval(d,[d],e) == "12"
assert p('string-length("12345")').eval(d,[d],e) == 5
assert p('normalize("   1  2 \r\n 3\r4\n5\r")').eval(d,[d],e) == "1 2 3 4 5"
assert p('translate("bar","abc","ABC")').eval(d,[d],e) == "BAr"
assert p('translate("--aaa--","abc-","ABC")').eval(d,[d],e) == "AAA"
assert p('translate("aaa","aa","xz")').eval(d,[d],e) == "xxx"
assert p('translate("aaa","a","xz")').eval(d,[d],e) == "xxx"

# Boolean Functions
assert p('boolean(true())').eval(d,[d],e) == 1
assert p('boolean(1)').eval(d,[d],e) == 1
assert p('boolean("1")').eval(d,[d],e) == 1
assert p('boolean(product)').eval(d,[d],e) == 1
assert p('boolean(false())').eval(d,[d],e) == 0
assert p('boolean(0)').eval(d,[d],e) == 0
assert p('boolean("")').eval(d,[d],e) == 0
assert p('boolean(version)').eval(d,[d],e) == 0
assert p('true()').eval(d,[d],e) == 1
assert p('false()').eval(d,[d],e) == 0
assert p('not(true())').eval(d,[d],e) == 0
assert p('not(false())').eval(d,[d],e) == 1

# Number Functions
assert p('number("  1234.67  ")').eval(d,[d],e) == 1234.67
assert p('number(true())').eval(d,[d],e) == 1
assert p('number(false())').eval(d,[d],e) == 0
assert p('number(//version)').eval(d,[d],e) == 0.01
x= p('id("weakdict")/version').eval(d,[d],e)[0]
assert p('number()').eval(x,[x],e) == 0.01
assert p('sum(//version)').eval(d,[d],e) == 0.05
assert p('floor(1.5)').eval(d,[d],e) == 1
assert p('floor(1)').eval(d,[d],e) == 1
assert p('floor(-1)').eval(d,[d],e) == -1
assert p('floor(-1.5)').eval(d,[d],e) == -2
assert p('ceiling(1.5)').eval(d,[d],e) == 2
assert p('ceiling(1)').eval(d,[d],e) == 1
assert p('ceiling(-1)').eval(d,[d],e) == -1
assert p('ceiling(-1.5)').eval(d,[d],e) == -1
assert p('round(1.5)').eval(d,[d],e) == 2
assert p('round(1.6)').eval(d,[d],e) == 2
assert p('round(1.4)').eval(d,[d],e) == 1
assert p('round(1)').eval(d,[d],e) == 1
assert p('round(-1)').eval(d,[d],e) == -1
assert p('round(-1.5)').eval(d,[d],e) == -2






