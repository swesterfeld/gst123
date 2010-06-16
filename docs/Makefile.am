EXTRA_DIST = gst123.1.doxi gst123defs.doxi

dist_man_MANS = gst123.1

DOCS_DIR = ~/public_html/gst123/docs-$(VERSION)
DOXER = ~/src/beast/doxer/doxer.py -D GST123_VERSION $(VERSION)

rebuild-manpage:
	$(DOXER) doxi2man gst123.1.doxi

publish-docs:
	mkdir -p $(DOCS_DIR)
	for i in $(dist_man_MANS); do \
	  $(DOXER) doxi2html $$i.doxi; \
	  cp $$i.html $(DOCS_DIR); \
          rm $$i.html ; \
        done
	cd $(DOCS_DIR) && find . -type f -print0 | xargs -0 git add
