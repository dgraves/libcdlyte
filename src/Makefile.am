## Process this file with automake to produce Makefile.in

noinst_HEADERS = \
        compat.h

lib_LTLIBRARIES = libcdlyte.la

libcdlyte_la_SOURCES = \
	cd.c       \
	irix_cd.c  \
	win32_cd.c  \
	cddb.c          \
	changer.c	\
        extra_objs.c

libcdlyte_la_LDFLAGS = -version-info 1:0:0

include_HEADERS = \
	cdlyte.h
