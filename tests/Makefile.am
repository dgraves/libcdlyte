## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src
LDADD = $(top_builddir)/src/libcdlyte.la

noinst_PROGRAMS = \
        cddb_id   \
        cddb_cddbp_read \
        cddb_cddbp_sites \
	cddb_http_read \
        cddb_http_sites \
        cddb_file_write

cddb_id_SOURCES			= cddb_id.c
cddb_cddbp_read_SOURCES		= cddb_cddbp_read.c
cddb_cddbp_sites_SOURCES	= cddb_cddbp_sites.c
cddb_http_read_SOURCES		= cddb_http_read.c
cddb_http_sites_SOURCES		= cddb_http_sites.c
cddb_file_write_SOURCES		= cddb_file_write.c
