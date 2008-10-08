MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = images fonts

EDJE_CC = edje_cc
EDJE_FLAGS = -v \
	-id $(top_srcdir)/data/themes/enjoy/images \
	-fd $(top_srcdir)/data/themes/enjoy/fonts

filesdir = $(themesdir)
files_DATA = default.edj

EXTRA_DIST = \
	main.edc \
	principal.edc \
	panel.edc \
	playlist.edc

default.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
		$(top_srcdir)/data/themes/enjoy/main.edc \
		$(top_builddir)/data/themes/enjoy/default.edj

clean-local:
	rm -f default.edj
