ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src 

bin_PROGRAMS = livemediastreamer testtranscoder h264toisoff testtranscoderh264toisoff testtranscoderaactoisoff

livemediastreamer_SOURCES = tests/liveMediaStreamer.cpp
livemediastreamer_CPPFLAGS = -Isrc/ -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I/usr/local/include/opencv -I/usr/local/include/groupsock -I/usr/local/include/liveMedia -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -Wall
livemediastreamer_LDFLAGS = -Lsrc -llivemediastreamer
livemediastreamer_DEPENDENCIES = src/liblivemediastreamer.la

testtranscoder_SOURCES = tests/testTranscoder.cpp
testtranscoder_CPPFLAGS = -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I/usr/local/include/opencv -I/usr/local/include/groupsock -I/usr/local/include/liveMedia -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -Wall
testtranscoder_LDFLAGS = -Lsrc -llivemediastreamer
testtranscoder_DEPENDENCIES = src/liblivemediastreamer.la

testtranscoderh264toisoff_SOURCES = tests/testTranscoderH264ToISOFF.cpp
testtranscoderh264toisoff_CPPFLAGS = -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I/usr/local/include/opencv -I/usr/local/include/groupsock -I/usr/local/include/liveMedia -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -Wall
testtranscoderh264toisoff_LDFLAGS = -Lsrc -llivemediastreamer
testtranscoderh264toisoff_DEPENDENCIES = src/liblivemediastreamer.la

testtranscoderaactoisoff_SOURCES = tests/testTranscoderAACToISOFF.cpp
testtranscoderaactoisoff_CPPFLAGS = -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I/usr/local/include/opencv -I/usr/local/include/groupsock -I/usr/local/include/liveMedia -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -Wall
testtranscoderaactoisoff_LDFLAGS = -Lsrc -llivemediastreamer
testtranscoderaactoisoff_DEPENDENCIES = src/liblivemediastreamer.la

h264toisoff_SOURCES = tests/testH264ToISOFF.cpp
h264toisoff_CPPFLAGS = -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I/usr/local/include/opencv -I/usr/local/include/groupsock -I/usr/local/include/liveMedia -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -Wall
h264toisoff_LDFLAGS = -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -Lsrc -llivemediastreamer
h264toisoff_DEPENDENCIES = src/liblivemediastreamer.la
