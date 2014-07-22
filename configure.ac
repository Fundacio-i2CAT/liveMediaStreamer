#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([liveMediaStreamer], [0.1])

# non-verbose output
AM_SILENT_RULES([yes])

LT_INIT
AM_INIT_AUTOMAKE([subdir-objects])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O

AC_LANG_PUSH([C++])
AX_CXX_COMPILE_STDCXX_11

# Checks for libraries.
AC_CHECK_LIB([avcodec], [av_fast_malloc])
AC_CHECK_LIB([avformat], [avformat_version])
AC_CHECK_LIB([swscale], [sws_scale])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor gettimeofday])

AC_OUTPUT([Makefile src/Makefile])