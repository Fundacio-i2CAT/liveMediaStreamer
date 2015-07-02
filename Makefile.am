ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src unitTests

bin_PROGRAMS = livemediastreamer testtranscoder teststreamer fakelive testvideomix testaudiomix

livemediastreamer_SOURCES = tests/liveMediaStreamer.cpp
livemediastreamer_CPPFLAGS = -Isrc/ -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
livemediastreamer_LDFLAGS = -Lsrc -llivemediastreamer -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale
livemediastreamer_DEPENDENCIES = src/liblivemediastreamer.la

testtranscoder_SOURCES = tests/testTranscoder.cpp
testtranscoder_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
testtranscoder_LDFLAGS = -Lsrc -llivemediastreamer -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale
testtranscoder_DEPENDENCIES = src/liblivemediastreamer.la

testvideomix_SOURCES = tests/testVideoMix.cpp
testvideomix_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
testvideomix_LDFLAGS = -Lsrc -llivemediastreamer -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale
testvideomix_DEPENDENCIES = src/liblivemediastreamer.la

testaudiomix_SOURCES = tests/testAudioMix.cpp
testaudiomix_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
testaudiomix_LDFLAGS = -Lsrc -llivemediastreamer -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale
testaudiomix_DEPENDENCIES = src/liblivemediastreamer.la

teststreamer_SOURCES = tests/H264LoopVideoFileServerMediaSubsession.cpp tests/LoopByteStreamFileSource.cpp tests/testStreamer.cpp
teststreamer_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
teststreamer_LDFLAGS = -Lsrc -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock

fakelive_SOURCES = tests/H264LoopVideoFileServerMediaSubsession.cpp tests/LoopByteStreamFileSource.cpp tests/fakeLive.cpp
fakelive_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
fakelive_LDFLAGS = -Lsrc -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock
