## Process this file with automake to produce Makefile.in

noinst_HEADERS = \
	data.h   \
        compat.h

lib_LTLIBRARIES = libcdaudio2.la

libcdaudio2_la_SOURCES = \
	cd.c       \
	irix_cd.c  \
	cddb.c          \
	cdindex.c	\
	data.c          \
	changer.c	\
	base64.c        \
        extra_objs.c

libcdaudio2_la_LDFLAGS = -version-info 1:0:0

include_HEADERS = \
	cdaudio2.h
