# Configure paths for libcdlyte
#
# Derived from libcdaudio.m4 (Tony Arcierei) - changes from original for library name only 
# Derived from glib.m4 (Owen Taylor 97-11-3)
#

dnl AM_PATH_LIBCDLYTE([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]])
dnl Test for libcdlyte, and define LIBCDLYTE_CFLAGS, LIBCDLYTE_LIBS and
dnl LIBCDLYTE_LDADD
dnl
AC_DEFUN(AM_PATH_LIBCDLYTE,
[dnl 
dnl Get the cflags and libraries from the libcdlyte-config script
dnl
AC_ARG_WITH(libcdlyte-prefix,[  --with-libcdlyte-prefix=PFX   Prefix where libcdlyte is installed (optional)],
            libcdlyte_config_prefix="$withval", libcdlyte_config_prefix="")
AC_ARG_WITH(libcdlyte-exec-prefix,[  --with-libcdlyte-exec-prefix=PFX Exec prefix where libcdlyte is installed (optional)],
            libcdlyte_config_exec_prefix="$withval", libcdlyte_config_exec_prefix="")
AC_ARG_ENABLE(libcdlytetest, [  --disable-libcdlytetest       Do not try to compile and run a test libcdlyte program],
		    , enable_libcdlytetest=yes)

  if test x$libcdlyte_config_exec_prefix != x ; then
     libcdlyte_config_args="$libcdlyte_config_args --exec-prefix=$libcdlyte_config_exec_prefix"
     if test x${LIBCDLYTE_CONFIG+set} != xset ; then
        LIBCDLYTE_CONFIG=$libcdlyte_config_exec_prefix/bin/libcdlyte-config
     fi
  fi
  if test x$libcdlyte_config_prefix != x ; then
     libcdlyte_config_args="$libcdlyte_config_args --prefix=$libcdlyte_config_prefix"
     if test x${LIBCDLYTE_CONFIG+set} != xset ; then
        LIBCDLYTE_CONFIG=$libcdlyte_config_prefix/bin/libcdlyte-config
     fi
  fi

  AC_PATH_PROG(LIBCDLYTE_CONFIG, libcdlyte-config, no)
  min_libcdlyte_version=ifelse([$1], ,0.99.0,$1)
  AC_MSG_CHECKING(for libcdlyte - version >= $min_libcdlyte_version)
  no_libcdlyte=""
  if test "$LIBCDLYTE_CONFIG" = "no" ; then
    no_libcdlyte=yes
  else
    LIBCDLYTE_CFLAGS=`$LIBCDLYTE_CONFIG $libcdlyte_config_args --cflags`
    LIBCDLYTE_LIBS=`$LIBCDLYTE_CONFIG $libcdlyte_config_args --libs`
    LIBCDLYTE_LDADD=`$LIBCDLYTE_CONFIG $libcdlyte_config_args --ldadd`
    libcdlyte_config_major_version=`$LIBCDLYTE_CONFIG $libcdlyte_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\1/'`
    libcdlyte_config_minor_version=`$LIBCDLYTE_CONFIG $libcdlyte_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\2/'`
    libcdlyte_config_micro_version=`$LIBCDLYTE_CONFIG $libcdlyte_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\3/'`
    if test "x$enable_libcdlytetest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $LIBCDLYTE_CFLAGS $LIBCDLYTE_LDADD"
      LIBS="$LIBCDLYTE_LIBS $LIBS"
dnl
dnl Now check if the installed libcdlyte is sufficiently new. (Also sanity
dnl checks the results of libcdlyte-config to some extent
dnl
      rm -f conf.cdlytetest
      AC_TRY_RUN([
#include <cdlyte.h>
#include <stdio.h>
#include <stdlib.h>

char* my_strdup (char *str)
{
  char *new_str;

  if (str) {
    new_str = malloc ((strlen (str) + 1) * sizeof(char));
    strcpy (new_str, str);
  } else
    new_str = NULL;

  return new_str;
}

int main()
{
  int major,minor,micro;
  int libcdlyte_major_version,libcdlyte_minor_version,libcdlyte_micro_version;
  char *tmp_version;

  system ("touch conf.cdlytetest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_libcdlyte_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_libcdlyte_version");
     exit(1);
   }

  libcdlyte_major_version=(cd_getversion()>>16)&255;
  libcdlyte_minor_version=(cd_getversion()>> 8)&255;
  libcdlyte_micro_version=(cd_getversion()    )&255;

  if ((libcdlyte_major_version != $libcdlyte_config_major_version) ||
      (libcdlyte_minor_version != $libcdlyte_config_minor_version) ||
      (libcdlyte_micro_version != $libcdlyte_config_micro_version))
    {
      printf("\n*** 'libcdlyte-config --version' returned %d.%d.%d, but libcdlyte (%d.%d.%d)\n", 
             $libcdlyte_config_major_version, $libcdlyte_config_minor_version, $libcdlyte_config_micro_version,
             libcdlyte_major_version, libcdlyte_minor_version, libcdlyte_micro_version);
      printf ("*** was found! If libcdlyte-config was correct, then it is best\n");
      printf ("*** to remove the old version of libcdlyte. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If libcdlyte-config was wrong, set the environment variable LIBCDLYTE_CONFIG\n");
      printf("*** to point to the correct copy of libcdlyte-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    } 
  else if ((libcdlyte_major_version != LIBCDLYTE_VERSION_MAJOR) ||
	   (libcdlyte_minor_version != LIBCDLYTE_VERSION_MINOR) ||
           (libcdlyte_micro_version != LIBCDLYTE_VERSION_MICRO))
    {
      printf("*** libcdlyte header files (version %d.%d.%d) do not match\n",
	     LIBCDLYTE_VERSION_MAJOR, LIBCDLYTE_VERSION_MINOR, LIBCDLYTE_VERSION_MICRO);
      printf("*** library (version %d.%d.%d)\n",
	     libcdlyte_major_version, libcdlyte_minor_version, libcdlyte_micro_version);
    }
  else
    {
      if ((libcdlyte_major_version > major) ||
        ((libcdlyte_major_version == major) && (libcdlyte_minor_version > minor)) ||
        ((libcdlyte_major_version == major) && (libcdlyte_minor_version == minor) && (libcdlyte_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of libcdlyte (%d.%d.%d) was found.\n",
               libcdlyte_major_version, libcdlyte_minor_version, libcdlyte_micro_version);
        printf("*** You need a version of libcdlyte newer than %d.%d.%d.\n",
	       major, minor, micro);
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the libcdlyte-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of libcdlyte, but you can also set the LIBCDLYTE_CONFIG environment to point to the\n");
        printf("*** correct copy of libcdlyte-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_libcdlyte=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_libcdlyte" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$LIBCDLYTE_CONFIG" = "no" ; then
       echo "*** The libcdlyte-config script installed by libcdlyte could not be found"
       echo "*** If libcdlyte was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the LIBCDLYTE_CONFIG environment variable to the"
       echo "*** full path to libcdlyte-config."
     else
       if test -f conf.cdlytetest ; then
        :
       else
          echo "*** Could not run libcdlyte test program, checking why..."
          CFLAGS="$CFLAGS $LIBCDLYTE_CFLAGS"
          LIBS="$LIBS $LIBCDLYTE_LIBS"
          AC_TRY_LINK([
#include <cdlyte.h>
#include <stdio.h>
],      [ return (cd_getversion()!=0); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding libcdlyte or finding the wrong"
          echo "*** version of libcdlyte. If it is not finding libcdlyte, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location. Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means libcdlyte was incorrectly installed"
          echo "*** or that you have moved libcdlyte since it was installed. In the latter case, you"
          echo "*** may want to edit the libcdlyte-config script: $LIBCDLYTE_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     LIBCDLYTE_CFLAGS=""
     LIBCDLYTE_LIBS=""
     LIBCDLYTE_LDADD=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(LIBCDLYTE_CFLAGS)
  AC_SUBST(LIBCDLYTE_LIBS)
  AC_SUBST(LIBCDLYTE_LDADD)
  rm -f conf.cdlytetest
])
