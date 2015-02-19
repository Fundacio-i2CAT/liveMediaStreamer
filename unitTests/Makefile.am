AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = connectionTest sinkManagerTest filterTest workerTest

dasherTest_SOURCES = modules/dasher/DasherTest.cpp ../src/modules/dasher/Dasher.cpp ../src/modules/dasher/DashVideoSegmenter.cpp ../src/modules/dasher/DashAudioSegmenter.cpp ../src/modules/dasher/i2libisoff.c ../src/modules/dasher/i2libdash.c  ../src/AVFramedQueue.cpp ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp
dasherTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
dasherTest_CXXFLAGS = -std=c++11
dasherTest_LDFLAGS = -L../src -lcppunit

dashAudioSegmenterTest_SOURCES = modules/dasher/DashAudioSegmenterTest.cpp ../src/modules/dasher/Dasher.cpp ../src/modules/dasher/DashVideoSegmenter.cpp ../src/modules/dasher/DashAudioSegmenter.cpp ../src/modules/dasher/i2libisoff.c ../src/modules/dasher/i2libdash.c ../src/AVFramedQueue.cpp ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp
dashAudioSegmenterTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
dashAudioSegmenterTest_CXXFLAGS = -std=c++11
dashAudioSegmenterTest_LDFLAGS = -L../src -lcppunit

dashVideoSegmenterTest_SOURCES = modules/dasher/DashVideoSegmenterTest.cpp ../src/modules/dasher/Dasher.cpp ../src/modules/dasher/DashVideoSegmenter.cpp ../src/modules/dasher/DashAudioSegmenter.cpp ../src/modules/dasher/i2libisoff.c ../src/modules/dasher/i2libdash.c ../src/AVFramedQueue.cpp ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp
dashVideoSegmenterTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
dashVideoSegmenterTest_CXXFLAGS = -std=c++11
dashVideoSegmenterTest_LDFLAGS = -L../src -lcppunit

connectionTest_SOURCES = modules/liveMediaOutput/ConnectionTest.cpp ../src/modules/liveMediaOutput/Connection.cpp ../src/modules/liveMediaOutput/MPEGTSQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/QueueServerMediaSubsession.cpp ../src/IOInterface.cpp ../src/Utils.cpp ../src/modules/liveMediaOutput/UltraGridAudioRTPSink.cpp ../src/modules/liveMediaOutput/H264StartCodeInjector.cpp ../src/modules/liveMediaOutput/H264VideoStreamSampler.cpp ../src/modules/liveMediaOutput/UltraGridVideoRTPSink.cpp ../src/modules/liveMediaOutput/VP8QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/H264QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/AudioQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/SPSparser/h264_stream.c
connectionTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
connectionTest_CXXFLAGS = -std=c++11
connectionTest_LDFLAGS = -L../src -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale
connectionTest_DEPENDENCIES = ../src/liblivemediastreamer.la

sinkManagerTest_SOURCES = modules/liveMediaOutput/SinkManagerTest.cpp ../src/modules/liveMediaOutput/SPSparser/h264_stream.c  ../src/modules/liveMediaOutput/AudioQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/Connection.cpp ../src/modules/liveMediaOutput/H264QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/H264QueueSource.cpp ../src/modules/liveMediaOutput/H264StartCodeInjector.cpp ../src/modules/liveMediaOutput/H264VideoStreamSampler.cpp ../src/modules/liveMediaOutput/JPEGQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/MPEGTSQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/QueueSource.cpp ../src/modules/liveMediaOutput/SinkManager.cpp ../src/modules/liveMediaOutput/UltraGridAudioRTPSink.cpp ../src/modules/liveMediaOutput/UltraGridVideoRTPSink.cpp ../src/modules/liveMediaOutput/VP8QueueServerMediaSubsession.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Runnable.cpp ../src/AVFramedQueue.cpp ../src/Utils.cpp ../src/Jzon.cpp ../src/Filter.cpp FilterMockup.hh ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp
sinkManagerTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src/
sinkManagerTest_CXXFLAGS = -std=c++11
sinkManagerTest_LDFLAGS = -L../src -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock

filterTest_SOURCES = FilterTest.cpp FilterMockup.hh ../src/AVFramedQueue.cpp ../src/FrameQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp  ../src/VideoFrame.cpp ../src/Runnable.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/Worker.cpp
filterTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
filterTest_CXXFLAGS = -std=c++11
filterTest_LDFLAGS = -llog4cplus -lcppunit

workerTest_SOURCES = ../src/Utils.cpp WorkerTest.cpp RunnableMockup.hh ../src/Worker.cpp ../src/Jzon.cpp ../src/Runnable.cpp
workerTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
workerTest_CXXFLAGS = -std=c++11
workerTest_LDFLAGS = -llog4cplus -lcppunit
