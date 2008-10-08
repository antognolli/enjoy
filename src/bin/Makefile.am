MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(top_srcdir)/src/bin/principal \
	-I$(top_srcdir)/src/bin/panel \
	-I$(top_srcdir)/src/bin/playlist \
	-I$(top_srcdir)/src/bin/conf \
	@LMS_CFLAGS@ \
	@GUARANA_CFLAGS@ @GUARANA_WIDGETS_CFLAGS@ @GUARANA_UTILS_CFLAGS@ \
	@EVAS_CFLAGS@ @ECORE_CFLAGS@ @EDJE_CFLAGS@ @EET_CFLAGS@ \
	@ECORE_EVAS_CFLAGS@ @EMOTION_CFLAGS@

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

enjoy_headers = \
	$(top_srcdir)/src/bin/principal/enjoy_principal.h \
	$(top_srcdir)/src/bin/panel/enjoy_panel.h \
	$(top_srcdir)/src/bin/playlist/enjoy_playlist.h \
	$(top_srcdir)/src/bin/conf/eet_loader.h

enjoy_src = \
	enjoy.c \
	auxdbg.c \
	$(top_srcdir)/src/bin/principal/principal_controller.c \
	$(top_srcdir)/src/bin/principal/principal_screen.c \
	$(top_srcdir)/src/bin/principal/principal_model.c \
	$(top_srcdir)/src/bin/panel/panel_controller.c \
	$(top_srcdir)/src/bin/panel/panel_screen.c \
	$(top_srcdir)/src/bin/playlist/playlist_controller.c \
	$(top_srcdir)/src/bin/playlist/playlist_list.c \
	$(top_srcdir)/src/bin/playlist/playlist_screen.c \
	$(top_srcdir)/src/bin/playlist/playlist_model.c \
	$(top_srcdir)/src/bin/conf/eet_loader.c

bin_PROGRAMS = enjoy

enjoy_SOURCES = $(enjoy_src) \
	$(enjoy_headers)
enjoy_DEPENDENCIES =
enjoy_LDADD = \
	@LMS_LIBS@ \
	@GUARANA_LIBS@ @GUARANA_WIDGETS_LIBS@ @GUARANA_UTILS_LIBS@ \
	@EVAS_LIBS@ @ECORE_LIBS@ @EDJE_LIBS@ @EET_LIBS@ \
	@ECORE_EVAS_LIBS@ @EMOTION_LIBS@
