## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src
LDADD = $(top_builddir)/src/libcdplayer.la

noinst_PROGRAMS = \
        cddb_id   \
        cddb_cddbp_read \
        cddb_cddbp_sites \
	cddb_http_read \
        cddb_http_sites \
        cddb_file_write \
        cddb_file_read \
        cddb_file_erase \
        cddb_submit_http \
        cddb_submit_smtp \
        http_emulator \
        smtp_emulator

cddb_id_SOURCES			= cddb_id.c
cddb_cddbp_read_SOURCES		= cddb_cddbp_read.c
cddb_cddbp_sites_SOURCES	= cddb_cddbp_sites.c
cddb_http_read_SOURCES		= cddb_http_read.c
cddb_http_sites_SOURCES		= cddb_http_sites.c
cddb_file_write_SOURCES		= cddb_file_write.c
cddb_file_read_SOURCES		= cddb_file_read.c
cddb_file_erase_SOURCES		= cddb_file_erase.c
cddb_submit_http_SOURCES	= cddb_submit_http.c
cddb_submit_smtp_SOURCES	= cddb_submit_smtp.c
http_emulator_SOURCES		= http_emulator.c
smtp_emulator_SOURCES		= smtp_emulator.c
