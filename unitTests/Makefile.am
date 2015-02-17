AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = connectionTest sinkManagerTest workerTest filterTest

#TODO: get rid off livemediastreamer dependency
connectionTest_SOURCES = modules/liveMediaOutput/ConnectionTest.cpp
connectionTest_CPPFLAGS = -std=c++11 -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
connectionTest_LDFLAGS = -L../src -llivemediastreamer -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale
connectionTest_DEPENDENCIES = ../src/liblivemediastreamer.la

#TODO: get rid off livemediastreamer dependency
sinkManagerTest_SOURCES = modules/liveMediaOutput/SinkManagerTest.cpp
sinkManagerTest_CPPFLAGS = -std=c++11 -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src/
sinkManagerTest_LDFLAGS = -L../src -llivemediastreamer -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale
sinkManagerTest_DEPENDENCIES = ../src/liblivemediastreamer.la

#TODO: get rid of liveMedia dependency
filterTest_SOURCES = FilterTest.cpp FilterMockup.hh ../src/AVFramedQueue.cpp ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp  ../src/VideoFrame.cpp ../src/Runnable.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/Worker.cpp
filterTest_CPPFLAGS = -std=c++11 -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
filterTest_LDFLAGS = -llog4cplus -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock

workerTest_SOURCES = ../src/Utils.cpp WorkerTest.cpp RunnableMockup.hh ../src/Worker.cpp ../src/Jzon.cpp ../src/Runnable.cpp
workerTest_CPPFLAGS = -std=c++11 -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
workerTest_LDFLAGS = -llog4cplus -lcppunit
