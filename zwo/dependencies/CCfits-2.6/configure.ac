dnl Process this file with autoconf to produce a configure script.
dnl
dnl Author: Paul_Kunz@slac.stanford.edu

AC_INIT([CCfits], [2.6], [], CCfits)
AC_CANONICAL_TARGET
AC_CONFIG_SRCDIR([FITS.cxx])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE

AC_LANG([C++])

dnl Checks for programs.
AC_PROG_CXX

dnl Force libtool to use C++ compiler when checking for PIC 
CC=$CXX

dnl Depending on target, we may need extra libraries in link command
dnl also set LD before doing libtool macros.
case "$target" in
  *-*-solaris*)
dnl Use the C++ compiler for the linker
    LD=$CXX
    LIBS="-lm -lsocket -lgen -lnsl";;
  *) ;;
esac

AC_DISABLE_STATIC([])
LT_INIT

dnl Get the path to the standard C++ library
PFK_CXX_LIB_PATH

dnl Turn on -Wall if compiling with gcc
AC_COMPILE_WARNINGS

dnl Turn on -ansi if compiling with gcc
#AC_COMPILE_ANSI

#   AX_CXX_COMPILE_STDCXX(VERSION, [ext|noext], [mandatory|optional])
AX_CXX_COMPILE_STDCXX(14,[],[])

dnl Check for gmake and use it
AC_CHECK_PROG(GMAKE, gmake, gmake, make)
AC_SUBST(GMAKE)

dnl check STD C++ library
AC_CHECK_LIB(stdc++, main, LIBSTDCPP="lstdc++", LIBSTDCPP=)
AC_SUBST(LIBSTDCPP)

dnl check where the cfitsio library and include files are at.
dnl NOTE: do this before checking sstream, else it fails.
AC_CHECK_PACKAGE(cfitsio, ffpss, cfitsio, fitsio.h, 
   AC_MSG_RESULT([cfitsio found.]), 
   AC_MSG_ERROR([cfitsio NOT found]) )

dnl check for valarray which was not available with older compilers
AX_CXX_HAVE_VALARRAY
if test "x$ax_cv_cxx_have_valarray" != xyes; then
  AC_MSG_ERROR( [valarray<T> is missing and required.] )
fi

dnl Use standard <sstream> if found; otherwise it uses the older <strstream>.
AC_CHECK_HEADER(sstream,
 [],
 [AC_CHECK_HEADER(strstream,
   AC_DEFINE(SSTREAM_DEFECT, 1,
    Define to 1 if you have the <strstream> header file instead of <sstream>),
   AC_MSG_ERROR(You have neither the <sstream> or <strstream> header file)
 )])

dnl Enables testing of alternate code dealing with MS VC++ workarounds.
AC_ARG_ENABLE(msvc-code,
[  --enable-msvc-code      compile alternate code dealing ]
[                          with MS VC++ workarounds],
AC_DEFINE(ITERATORBASE_DEFECT, 1,
Define if IteratorBase does not work with some STL functions.) 
AC_DEFINE(SPEC_TEMPLATE_DECL_DEFECT, 1,
Define partical template specializtion can not be declared.)
AC_DEFINE(TEMPLATE_AMBIG7_DEFECT, 1,
Define if compile can not resolve ambiguity in overloaded template functions.)
)

dnl check where the STLport library and include files are at.
dnl AC_ARG_WITH(stlport, --with-stlport=DIR (prefix to stlport),
dnl STLPORT=$withval, STLPORT=$prefix/include/stlport)
dnl AC_MSG_RESULT( using stlport at $STLPORT)

AC_SUBST(STLPORT)

AC_CONFIG_FILES([Makefile Doxyfile CCfits.pc vs.net/Makefile])
AC_OUTPUT

AC_MSG_RESULT([CCFits project now configured.])
