AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = connectionTest sinkManagerTest dashVideoSegmenterTest dashAudioSegmenterTest dasherTest filterTest workerTest

dasherTest_SOURCES = modules/dasher/DasherTest.cpp ../src/modules/dasher/Dasher.cpp ../src/modules/dasher/DashVideoSegmenter.cpp ../src/modules/dasher/DashAudioSegmenter.cpp ../src/modules/dasher/i2libisoff.c ../src/modules/dasher/i2libdash.c  ../src/AVFramedQueue.cpp ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp
dasherTest_CPPFLAGS = -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I../src/
dasherTest_LDFLAGS = -L../src -lcppunit

dashAudioSegmenterTest_SOURCES = modules/dasher/DashAudioSegmenterTest.cpp ../src/modules/dasher/Dasher.cpp ../src/modules/dasher/DashVideoSegmenter.cpp ../src/modules/dasher/DashAudioSegmenter.cpp ../src/modules/dasher/i2libisoff.c ../src/modules/dasher/i2libdash.c ../src/AVFramedQueue.cpp ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp
dashAudioSegmenterTest_CPPFLAGS = -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I../src/
dashAudioSegmenterTest_LDFLAGS = -L../src -lcppunit

dashVideoSegmenterTest_SOURCES = modules/dasher/DashVideoSegmenterTest.cpp ../src/modules/dasher/Dasher.cpp ../src/modules/dasher/DashVideoSegmenter.cpp ../src/modules/dasher/DashAudioSegmenter.cpp ../src/modules/dasher/i2libisoff.c ../src/modules/dasher/i2libdash.c ../src/AVFramedQueue.cpp ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp
dashVideoSegmenterTest_CPPFLAGS = -std=c++11 -g -Wall -std=c++11 -g -D__STDC_CONSTANT_MACROS -I../src/
dashVideoSegmenterTest_LDFLAGS = -L../src -lcppunit

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

filterTest_SOURCES = FilterTest.cpp FilterMockup.hh ../src/AVFramedQueue.cpp ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp  ../src/VideoFrame.cpp ../src/Runnable.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/Worker.cpp
filterTest_CPPFLAGS = -std=c++11 -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
filterTest_LDFLAGS = -llog4cplus -lcppunit

workerTest_SOURCES = ../src/Utils.cpp WorkerTest.cpp RunnableMockup.hh ../src/Worker.cpp ../src/Jzon.cpp ../src/Runnable.cpp
workerTest_CPPFLAGS = -std=c++11 -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
workerTest_LDFLAGS = -llog4cplus -lcppunit
