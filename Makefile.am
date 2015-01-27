ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src 

bin_PROGRAMS = livemediastreamer testtranscoder testaudio h264toisoff testtranscoderh264toisoff testtranscoderaactoisoff

livemediastreamer_SOURCES = tests/liveMediaStreamer.cpp
livemediastreamer_CPPFLAGS = -Isrc/ -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
livemediastreamer_LDFLAGS = -Lsrc -llivemediastreamer
livemediastreamer_DEPENDENCIES = src/liblivemediastreamer.la

testtranscoder_SOURCES = tests/testTranscoder.cpp
testtranscoder_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
testtranscoder_LDFLAGS = -Lsrc -llivemediastreamer
testtranscoder_DEPENDENCIES = src/liblivemediastreamer.la

testaudio_SOURCES = tests/testAudio.cpp
testaudio_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
testaudio_LDFLAGS = -Lsrc -llivemediastreamer
testaudio_DEPENDENCIES = src/liblivemediastreamer.la

testtranscoderh264toisoff_SOURCES = tests/testTranscoderH264ToISOFF.cpp
testtranscoderh264toisoff_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
testtranscoderh264toisoff_LDFLAGS = -Lsrc -llivemediastreamer
testtranscoderh264toisoff_DEPENDENCIES = src/liblivemediastreamer.la

testtranscoderaactoisoff_SOURCES = tests/testTranscoderAACToISOFF.cpp
testtranscoderaactoisoff_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
testtranscoderaactoisoff_LDFLAGS = -Lsrc -llivemediastreamer
testtranscoderaactoisoff_DEPENDENCIES = src/liblivemediastreamer.la

h264toisoff_SOURCES = tests/testH264ToISOFF.cpp
h264toisoff_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS
h264toisoff_LDFLAGS = -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -Lsrc -llivemediastreamer
h264toisoff_DEPENDENCIES = src/liblivemediastreamer.la
