AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = connectionTest sinkManagerTest

connectionTest_SOURCES = modules/liveMediaOutput/ConnectionTest.cpp
connectionTest_CPPFLAGS = -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I../src/
connectionTest_LDFLAGS = -L../src -llivemediastreamer -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale
connectionTest_DEPENDENCIES = ../src/liblivemediastreamer.la

sinkManagerTest_SOURCES = modules/liveMediaOutput/SinkManagerTest.cpp
sinkManagerTest_CPPFLAGS = -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I../src/
sinkManagerTest_LDFLAGS = -L../src -llivemediastreamer -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale
sinkManagerTest_DEPENDENCIES = ../src/liblivemediastreamer.la
