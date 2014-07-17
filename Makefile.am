ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src 

bin_PROGRAMS = livemediastreamer testtranscoder

livemediastreamer_SOURCES = tests/liveMediaStreamer.cpp
livemediastreamer_CPPFLAGS = -Isrc/ -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I/usr/local/include/groupsock -I/usr/local/include/liveMedia -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -Wall
livemediastreamer_LDFLAGS = -Lsrc -llivemediastreamer
livemediastreamer_DEPENDENCIES = src/liblivemediastreamer.la

testtranscoder_SOURCES = tests/testTranscoder.cpp
testtranscoder_CPPFLAGS = -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I/usr/local/include/groupsock -I/usr/local/include/liveMedia -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -Wall
testtranscoder_LDFLAGS = -Lsrc -llivemediastreamer
testtranscoder_DEPENDENCIES = src/liblivemediastreamer.la