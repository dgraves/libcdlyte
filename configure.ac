dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/cd.c)

LIBCDLYTE_MAJOR_VERSION=0
LIBCDLYTE_MINOR_VERSION=9
LIBCDLYTE_PATCH_LEVEL=8
AC_SUBST(LIBCDLYTE_MAJOR_VERSION)
AC_SUBST(LIBCDLYTE_MINOR_VERSION)
AC_SUBST(LIBCDLYTE_PATCH_LEVEL)

LIBCDLYTE_VERSION=$LIBCDLYTE_MAJOR_VERSION.$LIBCDLYTE_MINOR_VERSION.$LIBCDLYTE_PATCH_LEVEL
AC_SUBST(LIBCDLYTE_VERSION)

dnl Calculate libtool versioning information
LT_CURRENT=1
LT_REVISION=0
LT_AGE=0
LT_VERSION=$LT_CURRENT:$LT_REVISION:$LT_AGE
AC_SUBST(LT_VERSION)

dnl Automake support
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(libcdplayer, ${LIBCDLYTE_VERSION})

dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_GCC_TRADITIONAL
AC_CANONICAL_HOST
AM_PROG_CC_STDC
AM_PROG_LIBTOOL

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h unistd.h linux/cdrom.h linux/ucdrom.h sys/cdio.h io/cam/cdrom.h stdarg.h mntent.h sys/mntent.h sys/ucred.h sys/mount.h)

dnl Checks for libraries
AC_CHECK_LIB(m,main)

CFLAGS=""

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN
AC_CHECK_SIZEOF(long)

dnl Checks for library functions
AC_FUNC_GETMNTENT
AC_CHECK_FUNCS(gethostbyname gethostbyname_r mkdir socket strerror strstr strtol snprintf getmntinfo)

dnl Check operating system
case "$host_os" in
  irix*)
    AC_DEFINE(IRIX_CDLYTE,1,[Enable IRIX support])
    LIBRARY_LIBS="-lcdaudio -lmediad -lds $LIBS"
    ;;
  *solaris*)
    AC_DEFINE(SOLARIS_GETMNTENT,1,[Enable Solaris 'getmntent' support])
    AC_DEFINE(BROKEN_SOLARIS_LEADOUT,1,[Enable fixes for Solaris leadout issues])
    ;;
esac

dnl Debugging turned on
dnl If using gcc, turn on special debugging mode suitable for gdb
AC_MSG_CHECKING(for debugging)
AC_ARG_ENABLE(debug,[  --enable-debug          compile for debugging])
if test "x$enable_debug" = "xyes" ; then
  AC_MSG_RESULT(yes)
  if test "$GCC" != "" ; then
    CFLAGS="-ggdb3 -DDEBUG"
  else
    CFLAGS="-g -DDEBUG"
  fi
else
  AC_MSG_RESULT(no)
fi

dnl Building for release
AC_MSG_CHECKING(for release build)
AC_ARG_ENABLE(release,[  --enable-release        compile for release])
if test "x$enable_release" = "xyes" ; then
  AC_MSG_RESULT(yes)
  CFLAGS="-O2 -DNDEBUG"
  if test "${GCC}" = "yes" ; then
    CFLAGS="${CFLAGS} -Wuninitialized -ffast-math -finline-functions -fomit-frame-pointer -fexpensive-optimizations"
  fi
else
  AC_MSG_RESULT(no)
fi

dnl Add copious amounts of debugging with gcc, egcs, etc
if test "${GCC}" = "yes" ; then
  CFLAGS="${CFLAGS} -Wall -W -Wmissing-prototypes -Wformat"
fi

dnl Check for threading support
threads=no

AC_ARG_ENABLE(threads,
[  --enable-threads        build a thread-safe version of libcdplayer [default=guessed]],
[if test "$enableval" = "yes"; then
  threads=yes
else
  threads=no
fi])

dnl POSIX.4 threads
if test "$threads" = "yes"; then
  AC_CHECK_LIB(pthread,pthread_create,threads=-lpthread,threads=no)
else
  AC_CHECK_LIB(c_r,pthread_create,threads=-lc_r,threads=no)
fi

if test "$threads" != "no"; then
  AC_DEFINE(HAVE_PTHREAD,1,[Enable support for POSIX threads])
  CFLAGS="$CFLAGS -D_REENTRANT"
  LIBRARY_LIB="$threads $LIBRARY_LIB"
  REENTRANT="-D_REENTRANT"
fi
AC_SUBST(REENTRANT)

AC_SUBST(LIBS)
AC_SUBST(LIB_LDADD)

AC_OUTPUT([
Makefile
src/Makefile
src/cdver.h
tests/Makefile
])
