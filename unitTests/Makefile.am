AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = connectionTest sinkManagerTest filterTest dasherTest dashAudioSegmenterTest \
               dashVideoSegmenterTest mpdManagerTest encodingDecodingTest sharedMemoryTest \
               slicedVideoFrameQueueTest audioCircularBufferTest videoMixerTest videoMixerFunctionalTest \
               audioMixerFunctionalTest headDemuxerTest headDemuxerFunctionalTest workersPoolTest \
               avFramedQueueTest pipelineManagerTest IOInterfaceTest videoSplitterTest videoSplitterFunctionalTest

videoMixerTest_SOURCES = modules/videoMixer/VideoMixerTest.cpp 
videoMixerTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
videoMixerTest_CXXFLAGS = -std=c++11
videoMixerTest_LDFLAGS = -L../src -lcppunit -llivemediastreamer
videoMixerTest_DEPENDENCIES = ../src/liblivemediastreamer.la

videoSplitterTest_SOURCES = modules/videoSplitter/VideoSplitterTest.cpp 
videoSplitterTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
videoSplitterTest_CXXFLAGS = -std=c++11
videoSplitterTest_LDFLAGS = -L../src -lcppunit -llivemediastreamer
videoSplitterTest_DEPENDENCIES = ../src/liblivemediastreamer.la

avFramedQueueTest_SOURCES = AVFramedQueueTest.cpp
avFramedQueueTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
avFramedQueueTest_CXXFLAGS = -std=c++11
avFramedQueueTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -llivemediastreamer
avFramedQueueTest_DEPENDENCIES = ../src/liblivemediastreamer.la

audioCircularBufferTest_SOURCES = AudioCircularBufferTest.cpp 
audioCircularBufferTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
audioCircularBufferTest_CXXFLAGS = -std=c++11
audioCircularBufferTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -llivemediastreamer
audioCircularBufferTest_DEPENDENCIES = ../src/liblivemediastreamer.la

slicedVideoFrameQueueTest_SOURCES = SlicedVideoFrameQueueTest.cpp
slicedVideoFrameQueueTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
slicedVideoFrameQueueTest_CXXFLAGS = -std=c++11
slicedVideoFrameQueueTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -llivemediastreamer
slicedVideoFrameQueueTest_DEPENDENCIES = ../src/liblivemediastreamer.la

audioMixerFunctionalTest_SOURCES = AudioMixerFunctionalTest.cpp
audioMixerFunctionalTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
audioMixerFunctionalTest_CXXFLAGS = -std=c++11
audioMixerFunctionalTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -llivemediastreamer
audioMixerFunctionalTest_DEPENDENCIES = ../src/liblivemediastreamer.la

videoMixerFunctionalTest_SOURCES = VideoMixerFunctionalTest.cpp
videoMixerFunctionalTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
videoMixerFunctionalTest_CXXFLAGS = -std=c++11
videoMixerFunctionalTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -llivemediastreamer
videoMixerFunctionalTest_DEPENDENCIES = ../src/liblivemediastreamer.la

videoSplitterFunctionalTest_SOURCES = VideoSplitterFunctionalTest.cpp
videoSplitterFunctionalTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
videoSplitterFunctionalTest_CXXFLAGS = -std=c++11
videoSplitterFunctionalTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -llivemediastreamer
videoSplitterFunctionalTest_DEPENDENCIES = ../src/liblivemediastreamer.la

encodingDecodingTest_SOURCES = EncodingDecodingTest.cpp 
encodingDecodingTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
encodingDecodingTest_CXXFLAGS = -std=c++11
encodingDecodingTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -llivemediastreamer
encodingDecodingTest_DEPENDENCIES = ../src/liblivemediastreamer.la

sharedMemoryTest_SOURCES = modules/sharedMemory/SharedMemoryTest.cpp modules/sharedMemory/SharedMemoryDummyReader.cpp
sharedMemoryTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src/ -I.
sharedMemoryTest_CXXFLAGS = -std=c++11
sharedMemoryTest_LDFLAGS =  -L../src -lcppunit -lpthread -lavutil -lavcodec -lavformat -lswresample -llivemediastreamer 
sharedMemoryTest_DEPENDENCIES = ../src/liblivemediastreamer.la

mpdManagerTest_SOURCES = modules/dasher/MpdManagerTest.cpp
mpdManagerTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
mpdManagerTest_CXXFLAGS = -std=c++11
mpdManagerTest_LDFLAGS = -L../src -lcppunit -llivemediastreamer
mpdManagerTest_DEPENDENCIES = ../src/liblivemediastreamer.la

dasherTest_SOURCES = modules/dasher/DasherTest.cpp 
dasherTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/ 
dasherTest_CXXFLAGS = -std=c++11
dasherTest_LDFLAGS = -L../src -lcppunit -llivemediastreamer
dasherTest_DEPENDENCIES = ../src/liblivemediastreamer.la

dashAudioSegmenterTest_SOURCES = modules/dasher/DashAudioSegmenterTest.cpp 
dashAudioSegmenterTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
dashAudioSegmenterTest_CXXFLAGS = -std=c++11
dashAudioSegmenterTest_LDFLAGS = -L../src -lcppunit -llivemediastreamer
dashAudioSegmenterTest_DEPENDENCIES = ../src/liblivemediastreamer.la

dashVideoSegmenterTest_SOURCES = modules/dasher/DashVideoSegmenterTest.cpp 
dashVideoSegmenterTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
dashVideoSegmenterTest_CXXFLAGS = -std=c++11
dashVideoSegmenterTest_LDFLAGS = -L../src -lcppunit -llivemediastreamer
dashVideoSegmenterTest_DEPENDENCIES = ../src/liblivemediastreamer.la

connectionTest_SOURCES = modules/transmitter/ConnectionTest.cpp 
connectionTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
connectionTest_CXXFLAGS = -std=c++11
connectionTest_LDFLAGS = -L../src -llivemediastreamer -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale -lilclient
connectionTest_DEPENDENCIES = ../src/liblivemediastreamer.la

sinkManagerTest_SOURCES = modules/transmitter/SinkManagerTest.cpp 
sinkManagerTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src/
sinkManagerTest_CXXFLAGS = -std=c++11
sinkManagerTest_LDFLAGS = -L../src -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -llivemediastreamer
sinkManagerTest_DEPENDENCIES = ../src/liblivemediastreamer.la

filterTest_SOURCES = FilterTest.cpp
filterTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
filterTest_CXXFLAGS = -std=c++11
filterTest_LDFLAGS = -llog4cplus -lcppunit -lpthread -L../src -llivemediastreamer
filterTest_DEPENDENCIES = ../src/liblivemediastreamer.la

workersPoolTest_SOURCES = WorkersPoolTest.cpp
workersPoolTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
workersPoolTest_CXXFLAGS = -std=c++11
workersPoolTest_LDFLAGS = -llog4cplus -lcppunit -lpthread -L../src -llivemediastreamer
workersPoolTest_DEPENDENCIES = ../src/liblivemediastreamer.la

headDemuxerTest_SOURCES = modules/headDemuxer/HeadDemuxerTest.cpp
headDemuxerTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
headDemuxerTest_CXXFLAGS = -std=c++11
headDemuxerTest_LDFLAGS = -llog4cplus -lcppunit -lpthread -lavcodec -lavformat -lavutil -L../src -llivemediastreamer
headDemuxerTest_DEPENDENCIES = ../src/liblivemediastreamer.la

headDemuxerFunctionalTest_SOURCES = HeadDemuxerFunctionalTest.cpp 
headDemuxerFunctionalTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
headDemuxerFunctionalTest_CXXFLAGS = -std=c++11
headDemuxerFunctionalTest_LDFLAGS = -llog4cplus -lcppunit -lpthread -lavcodec -lavformat -lavutil -lswresample -L../src -llivemediastreamer
headDemuxerFunctionalTest_DEPENDENCIES = ../src/liblivemediastreamer.la

pipelineManagerTest_SOURCES =  PipelineManagerTest.cpp
pipelineManagerTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
pipelineManagerTest_CXXFLAGS = -std=c++11
pipelineManagerTest_LDFLAGS = -llog4cplus -lcppunit -lpthread -llivemediastreamer -L../src
pipelineManagerTest_DEPENDENCIES = ../src/liblivemediastreamer.la

IOInterfaceTest_SOURCES =  IOInterfaceTest.cpp
IOInterfaceTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
IOInterfaceTest_CXXFLAGS = -std=c++11
IOInterfaceTest_LDFLAGS = -llog4cplus -lcppunit -lpthread -L../src -llivemediastreamer
IOInterfaceTest_DEPENDENCIES = ../src/liblivemediastreamer.la
