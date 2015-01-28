#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([liveMediaStreamer], [0.1])

# non-verbose output
AM_SILENT_RULES([yes])

LT_INIT
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O

AC_LANG_PUSH([C++])
AX_CXX_COMPILE_STDCXX_11

# Checks for libraries.
AC_CHECK_LIB([avcodec], [av_fast_malloc], [])
AC_CHECK_LIB([avformat], [avformat_version], [])
AC_CHECK_LIB([swscale], [sws_scale], [])
AC_CHECK_LIB([opencv_core], [main], [])
AC_CHECK_LIB([opencv_imgproc], [main], [])
AC_CHECK_LIB([x264], [x264_encoder_encode], [])
AC_CHECK_LIB([i2dash], [main], [])
AC_CHECK_LIB([cppunit], [main], [])

AC_CHECK_FILE(/usr/local/include/opencv/cv.h, [CPPFLAGS="$CPPFLAGS -I/usr/local/include/opencv"; break;],[
    AC_CHECK_FILE(/usr/include/opencv/cv.h, [CPPFLAGS="$CPPFLAGS -I/usr/include/opencv"; break;],[])])
    
AC_CHECK_FILE(/usr/local/include/liveMedia/liveMedia.hh, [CPPFLAGS="$CPPFLAGS -I/usr/local/include/liveMedia"; break;],[
    AC_CHECK_FILE(/usr/include/liveMedia/liveMedia.hh, [CPPFLAGS="$CPPFLAGS -I/usr/include/liveMedia"; break;],[])])
    
AC_CHECK_FILE(/usr/local/include/BasicUsageEnvironment/BasicUsageEnvironment.hh, [CPPFLAGS="$CPPFLAGS -I/usr/local/include/BasicUsageEnvironment"; break;],[
    AC_CHECK_FILE(/usr/include/BasicUsageEnvironment/BasicUsageEnvironment.hh, [CPPFLAGS="$CPPFLAGS -I/usr/include/BasicUsageEnvironment"; break;],[])])
    
AC_CHECK_FILE(/usr/local/include/UsageEnvironment/UsageEnvironment.hh, [CPPFLAGS="$CPPFLAGS -I/usr/local/include/UsageEnvironment"; break;],[
    AC_CHECK_FILE(/usr/include/UsageEnvironment/UsageEnvironment.hh, [CPPFLAGS="$CPPFLAGS -I/usr/include/UsageEnvironment"; break;],[])])
    
AC_CHECK_FILE(/usr/local/include/groupsock/Groupsock.hh, [CPPFLAGS="$CPPFLAGS -I/usr/local/include/groupsock"; break;],[
    AC_CHECK_FILE(/usr/include/groupsock/Groupsock.hh, [CPPFLAGS="$CPPFLAGS -I/usr/include/groupsock"; break;],[])])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor gettimeofday])

AC_OUTPUT([Makefile src/Makefile])
