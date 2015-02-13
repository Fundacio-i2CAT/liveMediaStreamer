AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = workerTest #filterTest sinkManagerTest connectionTest 

filterTest_SOURCES = FilterTest.cpp FilterMockup.hh ../src/AVFramedQueue.cpp ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp  ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp
filterTest_CPPFLAGS = -std=c++11 -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
filterTest_LDFLAGS = -llog4cplus -lcppunit

workerTest_SOURCES = WorkerTest.cpp RunnableMockup.hh ../src/Utils.cpp ../src/Worker.cpp ../src/Jzon.cpp
workerTest_CPPFLAGS = -std=c++11 -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
workerTest_LDFLAGS = -llog4cplus -lcppunit

connectionTest_SOURCES = modules/liveMediaOutput/ConnectionTest.cpp
connectionTest_CPPFLAGS = -std=c++11 -g -Wall g -D__STDC_CONSTANT_MACROS -I../src/ -I.
connectionTest_LDFLAGS = -llivemediastreamer -llog4cplus -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale -L../src -L.
connectionTest_DEPENDENCIES = ../src/liblivemediastreamer.la

sinkManagerTest_SOURCES = modules/liveMediaOutput/SinkManagerTest.cpp
sinkManagerTest_CPPFLAGS = -std=c++11 -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src/ -I.
sinkManagerTest_LDFLAGS = -llivemediastreamer -llog4cplus -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale -L../src -L.
sinkManagerTest_DEPENDENCIES = ../src/liblivemediastreamer.la
