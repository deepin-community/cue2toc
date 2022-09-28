AUTOMAKE_OPTIONS = no-dependencies

bin_PROGRAMS = cue2toc
cue2toc_SOURCES = main.c cue2toc.c cue2toc.h timecode.c timecode.h \
	conf.c conf.h error.c error.h convert.c convert.h
man_MANS = cue2toc.1
EXTRA_DIST = $(man_MANS) cue2tocrc

dist-hook:
	rcs2log | sed -e 's/<lal@ddm>/<dermatsch@gmx.de>/g' \
		>$(distdir)/ChangeLog
