## Process this file with automake to produce Makefile.in

noinst_HEADERS = \
        compat.h

lib_LTLIBRARIES = libcdlyte.la

libcdlyte_la_SOURCES = \
	cd.c           \
	irix_cd.c      \
	win32_cd.c     \
	cddb.c         \
	changer.c      \
        extra_objs.c

libcdlyte_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

include_HEADERS = \
        cdver.h   \
	cdlyte.h
