dist_man_MANS = gst123.1

DOCS_DIR = ~/public_html/gst123/docs-$(VERSION)
DOXER = ~/src/beast/doxer/doxer.py -D GST123_VERSION $(VERSION)

rebuild-manpage:
	a2x -f manpage gst123.1.txt
	asciidoc -b xhtml11 -d manpage gst123.1.txt

publish-docs:
	mkdir -p $(DOCS_DIR)
	for i in $(dist_man_MANS); do \
	  cp $$i.html $(DOCS_DIR); \
        done
	cd $(DOCS_DIR) && find . -type f -print0 | xargs -0 git add
