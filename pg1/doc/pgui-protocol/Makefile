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
	db2html --nochunks $(DOCNAME).sgml
	mv $(DOCNAME)/$(DOCNAME).html .
	rm -rf $(DOCNAME) $(DOCNAME).junk

html: $(DOCNAME).sgml
	db2html $(DOCNAME).sgml
	rm -rf $(DOCNAME).junk

clean:
	rm -rf *.pdf *.dvi *.rtf *.ps *.html $(DOCNAME) $(DOCNAME).junk
