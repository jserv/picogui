DOCNAME = pgui-protocol

.PHONY: clean all dvi pdf html ps rtf onehtml
all: dvi pdf ps rtf onehtml html

dvi: $(DOCNAME).dvi
pdf: $(DOCNAME).pdf
ps: $(DOCNAME).ps
rtf: $(DOCNAME).rtf
onehtml: $(DOCNAME).html

$(DOCNAME).dvi: $(DOCNAME).sgml
	db2dvi $(DOCNAME).sgml

$(DOCNAME).pdf: $(DOCNAME).sgml
	db2pdf $(DOCNAME).sgml

$(DOCNAME).ps: $(DOCNAME).sgml
	db2ps $(DOCNAME).sgml

$(DOCNAME).rtf: $(DOCNAME).sgml
	db2rtf $(DOCNAME).sgml

$(DOCNAME).html: $(DOCNAME).sgml
	db2html --nochunks $(DOCNAME).sgml | grep -v "output is pgui-protocol" | egrep -v "^Using catalogs:" | egrep -v "^Using stylesheet:" | egrep -v "Working on:" > $(DOCNAME).html
	rm -rf $(DOCNAME).junk

html: $(DOCNAME).sgml
	db2html $(DOCNAME).sgml
	rm -rf $(DOCNAME).junk

clean:
	rm -rf *.pdf *.dvi *.rtf *.ps *.html $(DOCNAME)
