MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = $(configfiles)

configfiles = \
	emotion.plugin.src \
	lms.parsers.src \
	lms.charsets.src

filesdir = $(sysconfdir)
files_DATA = \
	enjoy.conf

#enjoy.conf: $(config_sources)
#	eet -e \
#	$(top_builddir)/config/$@ /enjoy/$(<:.src=) \
#	$< 0

enjoy.conf: $(configfiles:.src=.stamp)
%.stamp: %.src
	eet -e $(top_builddir)/config/enjoy.conf \
	/enjoy/`echo $(<:.src=) | sed -e "s:\.:/:g"` \
	$< 0
	touch $@

clean-local:
	rm -rf *.conf *~ *.stamp
