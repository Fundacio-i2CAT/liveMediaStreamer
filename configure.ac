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

LDFLAGS="$LDFLAGS -L/usr/local/lib"

###############
#Live555 staff#
###############
AC_ARG_WITH(livemedia,  [  --with-livemedia=PATH   liveMedia path for static linking])
if test -n "${with_livemedia}"; then
    AC_MSG_CHECKING(for liveMedia libraries in ${with_livemedia})
    custom_livemedia="`cd ${with_livemedia} 2>/dev/null && pwd`"
    if test -z "${custom_livemedia}"; then
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([cannot cd to ${with_livemedia}])
    fi
    if test -f "${custom_livemedia}/lib/libliveMedia.so"; then
        AC_MSG_RESULT(${custom_livemedia}/lib/libliveMedia.so)
        LDFLAGS="-L${custom_livemedia}/lib $LDFLAGS"
        AC_CHECK_FILE(${custom_livemedia}/include/liveMedia/liveMedia.hh,
                [CPPFLAGS="$CPPFLAGS -I${custom_livemedia}/include/liveMedia";break;])
        AC_CHECK_FILE(${custom_livemedia}/include/BasicUsageEnvironment/BasicUsageEnvironment.hh,
                [CPPFLAGS="$CPPFLAGS -I${custom_livemedia}/include/BasicUsageEnvironment"; break;])
        AC_CHECK_FILE(${custom_livemedia}/include/UsageEnvironment/UsageEnvironment.hh,
            [CPPFLAGS="$CPPFLAGS -I${custom_livemedia}/include/UsageEnvironment"; break;])
        AC_CHECK_FILE(${custom_livemedia}/include/groupsock/Groupsock.hh, 
            [CPPFLAGS="$CPPFLAGS -I${custom_livemedia}/include/groupsock"; break;])
        AC_CHECK_HEADERS([liveMedia.hh  BasicUsageEnvironment.hh UsageEnvironment.hh Groupsock.hh])
    else
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([cannot find ${custom_livemedia}/lib/libliveMedia.so, make sure you compiled
                liveMedia in ${with_livemedia}])
    fi
else
   AC_CHECK_FILE(/usr/local/include/liveMedia/liveMedia.hh, [CPPFLAGS="$CPPFLAGS -I/usr/local/include/liveMedia"; break;],
           [AC_CHECK_FILE(/usr/include/liveMedia/liveMedia.hh, [CPPFLAGS="$CPPFLAGS -I/usr/include/liveMedia"; break;],
               [AC_MSG_ERROR([cannot find libliveMedia.so])])])
   AC_CHECK_FILE(/usr/local/include/BasicUsageEnvironment/BasicUsageEnvironment.hh, [CPPFLAGS="$CPPFLAGS -I/usr/local/include/BasicUsageEnvironment"; break;],
           [AC_CHECK_FILE(/usr/include/BasicUsageEnvironment/BasicUsageEnvironment.hh, [CPPFLAGS="$CPPFLAGS -I/usr/include/BasicUsageEnvironment"; break;],
               [AC_MSG_ERROR([cannot find libBasicUsageEnvironment.so])])])
   AC_CHECK_FILE(/usr/local/include/UsageEnvironment/UsageEnvironment.hh, [CPPFLAGS="$CPPFLAGS -I/usr/local/include/UsageEnvironment"; break;],
           [AC_CHECK_FILE(/usr/include/UsageEnvironment/UsageEnvironment.hh, [CPPFLAGS="$CPPFLAGS -I/usr/include/UsageEnvironment"; break;],
               [AC_MSG_ERROR([cannot find libUsageEnvironment.so])])])
   AC_CHECK_FILE(/usr/local/include/groupsock/Groupsock.hh, [CPPFLAGS="$CPPFLAGS -I/usr/local/include/groupsock"; break;],
           [AC_CHECK_FILE(/usr/include/groupsock/Groupsock.hh, [CPPFLAGS="$CPPFLAGS -I/usr/include/groupsock"; break;],
               [AC_MSG_ERROR([cannot find libgroupsock.so])])])
   AC_CHECK_HEADERS([liveMedia.hh  BasicUsageEnvironment.hh UsageEnvironment.hh Groupsock.hh])
fi

##############
#ffmpeg staff#
##############
AC_ARG_WITH(ffmpeg,  [  --with-ffmpeg=PATH   ffmpeg path for static linking])
if test -n "${with_ffmpeg}"; then
    AC_MSG_CHECKING(for ffmpeg libraries in ${with_ffmpeg})
    custom_ffmpeg="`cd ${with_ffmpeg} 2>/dev/null && pwd`"
    if test -z "${custom_ffmpeg}"; then
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([cannot cd to ${with_ffmpeg}])
    fi
    if test -f "${custom_ffmpeg}/lib/libavcodec.so"; then
        AC_MSG_RESULT(${custom_ffmpeg}/lib/libavcodec.so)
        LDFLAGS="-L${custom_ffmpeg}/lib $LDFLAGS"
        AC_CHECK_FILE(${custom_ffmpeg}/include/libavcodec/avcodec.h, 
                [CPPFLAGS="$CPPFLAGS -I${custom_ffmpeg}/include"; break;])
        AC_CHECK_FILE(${custom_ffmpeg}/include/libavformat/avformat.h,
                [CPPFLAGS="$CPPFLAGS -I${custom_ffmpeg}/include"; break;])
        AC_CHECK_FILE(${custom_ffmpeg}/include/libavutil/avutil.h, 
                [CPPFLAGS="$CPPFLAGS -I${custom_ffmpeg}/include"; break;])
        AC_CHECK_FILE(${custom_ffmpeg}/include/libswresample/swresample.h,
                [CPPFLAGS="$CPPFLAGS -I${custom_ffmpeg}/include"; break;])
        AC_CHECK_FILE(${custom_ffmpeg}/include/libswscale/swscale.h,
                [CPPFLAGS="$CPPFLAGS -I${custom_ffmpeg}/include"; break;])
        AC_CHECK_HEADERS([libavcodec/avcodec.h libavformat/avformat.h libavutil/avutil.h libswresample/swresample.h libswscale/swscale.h])
    else
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([cannot find ${custom_ffmpeg}/lib/libavcodec.so, make sure you
                compiled ffmpeg in ${with_ffmpeg}])
    fi
else
    AC_CHECK_LIB([avcodec], [avcodec_find_encoder], [], AC_MSG_ERROR([cannot find avcodec]))
    AC_CHECK_LIB([avformat], [avformat_new_stream], [], AC_MSG_ERROR([cannot find avformat]))
    AC_CHECK_LIB([swscale], [sws_scale], [], AC_MSG_ERROR([cannot find swscale]))
    AC_CHECK_LIB([avutil], [av_gettime], [], AC_MSG_ERROR([cannot find avutil]))
    AC_CHECK_LIB([swresample], [swr_convert], [], AC_MSG_ERROR([cannot find swresample]))
fi

# Checks for libraries.
AC_CHECK_LIB([opencv_core], [main], [], AC_MSG_ERROR([cannot find opencv_core]))
AC_CHECK_LIB([opencv_imgproc], [main], [], AC_MSG_ERROR([cannot find opencv_imgproc]))
AC_CHECK_LIB([x264], [x264_encoder_encode], [], AC_MSG_ERROR([cannot find x264]))
AC_CHECK_LIB([log4cplus], [main], [], AC_MSG_ERROR([cannot find log4cplus]))
AC_CHECK_LIB([tinyxml2], [main], [], AC_MSG_ERROR([cannot find tinyxml2]))

AC_CHECK_FILE(/usr/local/include/opencv/cv.h, [CPPFLAGS="$CPPFLAGS -I/usr/local/include/opencv"; break;],[
    AC_CHECK_FILE(/usr/include/opencv/cv.h, [CPPFLAGS="$CPPFLAGS -I/usr/include/opencv"; break;],[])])
    
# Checks for header files.
AC_CHECK_HEADERS([stdint.h string.h sys/time.h cv.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor gettimeofday])

AC_OUTPUT([Makefile src/Makefile unitTests/Makefile])
