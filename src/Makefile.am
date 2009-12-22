## Process this file with automake to produce Makefile.in

noinst_HEADERS = \
        cdver.h  \
        compat.h

lib_LTLIBRARIES = libcdplayer.la

libcdplayer_la_SOURCES = \
	cd.c           \
	irix_cd.c      \
	win32_cd.c     \
	cddb.c         \
	changer.c      \
        extra_objs.c

libcdplayer_la_LDFLAGS = \
	-version-info $(LT_VERSION)

include_HEADERS = \
	cdplayer.h
