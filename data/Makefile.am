MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = themes

ICONS = \
	enjoy_256x256.png \
	enjoy_128x128.png \
	enjoy_64x64.png \
	enjoy_48x48.png \
	enjoy_32x32.png

EXTRA_DIST = \
	enjoy.desktop \
	enjoy.desktop.in \
	enjoy-icon.svg \
	enjoy-mockup.svg \
	$(ICONS)

desktop_DATA = enjoy.desktop
desktopdir = $(datadir)/applications

enjoy_%.png: enjoy-icon.svg
	inkscape --file=$< \
		--export-id=$(patsubst %.png,%,$@) \
		--export-id-only --export-png=$@

iconsdir = $(datadir)/icons/hicolor

install-data-local: $(ICONS)
	for icon in $(ICONS); do \
		s=`echo $$icon | sed 's/enjoy_\([0-9]\+x[0-9]\+\).png/\1/g'`; \
		$(INSTALL_DATA) -D $$icon $(iconsdir)/$$s/apps/enjoy.png; \
	done

uninstall-local:
	for icon in $(ICONS); do \
		s=`echo $$icon | sed 's/enjoy_\([0-9]\+x[0-9]\+\).png/\1/g'`; \
		$(RM) -f $(iconsdir)/$$s/apps/enjoy.png; \
	done

