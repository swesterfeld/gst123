dist_man_MANS = gst123.1

DOCS_DIR = ~/public_html/gst123/docs-$(VERSION)
DOXER = ~/src/beast/doxer/doxer.py -D GST123_VERSION $(VERSION)

rebuild-manpage:
	for p in $(dist_man_MANS); do \
          curl -sfS https://testbit.eu/wiki/$$p?action=render >$$p.web; \
	  curl -sfS "https://testbit.eu/wiki.mw/index.php?title=$$p&action=raw" > $$p.wiki; \
	  $(WIKIHTML2MAN) $$p.web >$$p; \
	  groff -mandoc -Thtml < $$p >$$p.html; \
	  rm $$p.web; \
	done

publish-docs:
	mkdir -p $(DOCS_DIR)
	for i in $(dist_man_MANS); do \
	  cp $$i.html $(DOCS_DIR); \
        done
	cd $(DOCS_DIR) && find . -type f -print0 | xargs -0 git add
