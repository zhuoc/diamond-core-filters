FILTERS  += searches/shingling/fil_shingling
SEARCHES += searches/shingling/shingling.search

searches_shingling_fil_shingling_SOURCES = \
	searches/shingling/fil_shingling.c \
	searches/shingling/rabin.c \
	searches/shingling/rabin.h

EXTRA_DIST += searches/shingling/genpoly.py