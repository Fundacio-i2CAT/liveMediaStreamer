AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = connectionTest sinkManagerTest filterTest dasherTest dashAudioSegmenterTest dashVideoSegmenterTest mpdManagerTest encodingDecodingTest sharedMemoryTest slicedVideoFrameQueueTest audioCircularBufferTest videoMixerTest videoMixerFunctionalTest audioMixerFunctionalTest headDemuxerTest headDemuxerFunctionalTest workersPoolTest

videoMixerTest_SOURCES = modules/videoMixer/VideoMixerTest.cpp ../src/modules/videoMixer/VideoMixer.cpp ../src/AVFramedQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/Runnable.cpp
videoMixerTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
videoMixerTest_CXXFLAGS = -std=c++11
videoMixerTest_LDFLAGS = -L../src -lcppunit 

audioCircularBufferTest_SOURCES = AudioCircularBufferTest.cpp ../src/AudioCircularBuffer.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Frame.cpp ../src/AudioFrame.cpp
audioCircularBufferTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
audioCircularBufferTest_CXXFLAGS = -std=c++11
audioCircularBufferTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -lfdk-aac

slicedVideoFrameQueueTest_SOURCES = SlicedVideoFrameQueueTest.cpp ../src/SlicedVideoFrameQueue.cpp ../src/VideoFrame.cpp ../src/AVFramedQueue.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Frame.cpp ../src/AudioFrame.cpp
slicedVideoFrameQueueTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
slicedVideoFrameQueueTest_CXXFLAGS = -std=c++11
slicedVideoFrameQueueTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -lfdk-aac

audioMixerFunctionalTest_SOURCES = AudioMixerFunctionalTest.cpp ../src/modules/audioMixer/AudioMixer.cpp ../src/AVFramedQueue.cpp ../src/AudioCircularBuffer.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/AudioFrame.cpp ../src/SlicedVideoFrameQueue.cpp
audioMixerFunctionalTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
audioMixerFunctionalTest_CXXFLAGS = -std=c++11
audioMixerFunctionalTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -lfdk-aac

videoMixerFunctionalTest_SOURCES = VideoMixerFunctionalTest.cpp ../src/modules/videoMixer/VideoMixer.cpp ../src/AVFramedQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/AudioFrame.cpp ../src/SlicedVideoFrameQueue.cpp ../src/Runnable.cpp
videoMixerFunctionalTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
videoMixerFunctionalTest_CXXFLAGS = -std=c++11
videoMixerFunctionalTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -lfdk-aac

encodingDecodingTest_SOURCES = EncodingDecodingTest.cpp ../src/modules/videoEncoder/VideoEncoderX264or5.cpp ../src/modules/videoEncoder/VideoEncoderX264.cpp ../src/modules/videoEncoder/VideoEncoderX265.cpp ../src/modules/videoDecoder/VideoDecoderLibav.cpp ../src/AVFramedQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/AudioFrame.cpp ../src/SlicedVideoFrameQueue.cpp ../src/Runnable.cpp
encodingDecodingTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
encodingDecodingTest_CXXFLAGS = -std=c++11
encodingDecodingTest_LDFLAGS = -L../src -lcppunit -lavutil -lavcodec -lavformat -lswresample -lfdk-aac

sharedMemoryTest_SOURCES = modules/sharedMemory/SharedMemoryTest.cpp ../src/modules/sharedMemory/SharedMemory.cpp modules/sharedMemory/SharedMemoryDummyReader.cpp ../src/modules/videoEncoder/VideoEncoderX264or5.cpp ../src/modules/videoEncoder/VideoEncoderX264.cpp ../src/modules/videoEncoder/VideoEncoderX265.cpp ../src/modules/videoDecoder/VideoDecoderLibav.cpp ../src/AVFramedQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/SlicedVideoFrameQueue.cpp ../src/AudioFrame.cpp ../src/Runnable.cpp
sharedMemoryTest_CPPFLAGS = -std=c++11 -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
sharedMemoryTest_LDFLAGS =  -L../src -lcppunit -lpthread -lavutil -lavcodec -lavformat -lswresample -lfdk-aac

mpdManagerTest_SOURCES = modules/dasher/MpdManagerTest.cpp ../src/modules/dasher/MpdManager.cpp
mpdManagerTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
mpdManagerTest_CXXFLAGS = -std=c++11
mpdManagerTest_LDFLAGS = -L../src -lcppunit

dasherTest_SOURCES = modules/dasher/DasherTest.cpp ../src/modules/dasher/Dasher.cpp ../src/modules/dasher/DashVideoSegmenter.cpp ../src/modules/dasher/DashAudioSegmenter.cpp ../src/modules/dasher/MpdManager.cpp ../src/modules/dasher/i2libisoff.c ../src/modules/dasher/i2libdash.c  ../src/AVFramedQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/Runnable.cpp
dasherTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
dasherTest_CXXFLAGS = -std=c++11
dasherTest_LDFLAGS = -L../src -lcppunit

dashAudioSegmenterTest_SOURCES = modules/dasher/DashAudioSegmenterTest.cpp ../src/modules/dasher/Dasher.cpp ../src/modules/dasher/DashVideoSegmenter.cpp ../src/modules/dasher/DashAudioSegmenter.cpp ../src/modules/dasher/MpdManager.cpp ../src/modules/dasher/i2libisoff.c ../src/modules/dasher/i2libdash.c ../src/AVFramedQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/Runnable.cpp
dashAudioSegmenterTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
dashAudioSegmenterTest_CXXFLAGS = -std=c++11
dashAudioSegmenterTest_LDFLAGS = -L../src -lcppunit

dashVideoSegmenterTest_SOURCES = modules/dasher/DashVideoSegmenterTest.cpp ../src/modules/dasher/Dasher.cpp ../src/modules/dasher/DashVideoSegmenter.cpp ../src/modules/dasher/DashAudioSegmenter.cpp ../src/modules/dasher/MpdManager.cpp ../src/modules/dasher/i2libisoff.c ../src/modules/dasher/i2libdash.c ../src/AVFramedQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/Runnable.cpp
dashVideoSegmenterTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
dashVideoSegmenterTest_CXXFLAGS = -std=c++11
dashVideoSegmenterTest_LDFLAGS = -L../src -lcppunit

connectionTest_SOURCES = modules/liveMediaOutput/ConnectionTest.cpp ../src/modules/liveMediaOutput/Connection.cpp ../src/modules/liveMediaOutput/MPEGTSQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/QueueServerMediaSubsession.cpp ../src/IOInterface.cpp ../src/Utils.cpp ../src/modules/liveMediaOutput/UltraGridAudioRTPSink.cpp ../src/modules/liveMediaOutput/H264or5StartCodeInjector.cpp ../src/modules/liveMediaOutput/H264VideoStreamSampler.cpp ../src/modules/liveMediaOutput/UltraGridVideoRTPSink.cpp ../src/modules/liveMediaOutput/VP8QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/H264QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/H265QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/H264or5QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/AudioQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/SPSparser/h264_stream.c ../src/modules/liveMediaOutput/ADTSQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/CustomMPEG4GenericRTPSink.cpp ../src/modules/liveMediaOutput/ADTSStreamParser.cpp ../src/modules/liveMediaOutput/QueueSource.cpp ../src/Jzon.cpp
connectionTest_CPPFLAGS = -g -Wall -D__STDC_CONSTANT_MACROS -I../src/
connectionTest_CXXFLAGS = -std=c++11
connectionTest_LDFLAGS = -L../src -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale
connectionTest_DEPENDENCIES = ../src/liblivemediastreamer.la

sinkManagerTest_SOURCES = modules/liveMediaOutput/SinkManagerTest.cpp ../src/modules/liveMediaOutput/SPSparser/h264_stream.c  ../src/modules/liveMediaOutput/AudioQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/Connection.cpp ../src/modules/liveMediaOutput/H264QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/H264or5QueueSource.cpp ../src/modules/liveMediaOutput/H264or5StartCodeInjector.cpp ../src/modules/liveMediaOutput/H264VideoStreamSampler.cpp ../src/modules/liveMediaOutput/JPEGQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/MPEGTSQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/QueueSource.cpp ../src/modules/liveMediaOutput/SinkManager.cpp ../src/modules/liveMediaOutput/UltraGridAudioRTPSink.cpp ../src/modules/liveMediaOutput/UltraGridVideoRTPSink.cpp ../src/modules/liveMediaOutput/VP8QueueServerMediaSubsession.cpp ../src/Frame.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Runnable.cpp ../src/AVFramedQueue.cpp ../src/Utils.cpp ../src/Jzon.cpp ../src/Filter.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/modules/liveMediaOutput/H264or5QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/H265QueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/ADTSStreamParser.cpp ../src/modules/liveMediaOutput/ADTSQueueServerMediaSubsession.cpp ../src/modules/liveMediaOutput/CustomMPEG4GenericRTPSink.cpp
sinkManagerTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src/
sinkManagerTest_CXXFLAGS = -std=c++11
sinkManagerTest_LDFLAGS = -L../src -lcppunit -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock

filterTest_SOURCES = FilterTest.cpp ../src/AVFramedQueue.cpp ../src/IOInterface.cpp ../src/Event.cpp ../src/Frame.cpp  ../src/VideoFrame.cpp ../src/Runnable.cpp ../src/AudioFrame.cpp ../src/Jzon.cpp ../src/Utils.cpp ../src/Filter.cpp ../src/WorkersPool.cpp
filterTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
filterTest_CXXFLAGS = -std=c++11
filterTest_LDFLAGS = -llog4cplus -lcppunit -lpthread

workersPoolTest_SOURCES = WorkersPoolTest.cpp ../src/WorkersPool.cpp ../src/Runnable.cpp ../src/Utils.cpp
workersPoolTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
workersPoolTest_CXXFLAGS = -std=c++11
workersPoolTest_LDFLAGS = -llog4cplus -lcppunit -lpthread

headDemuxerTest_SOURCES = ../src/Utils.cpp modules/headDemuxer/HeadDemuxerTest.cpp ../src/modules/headDemuxer/HeadDemuxerLibav.cpp ../src/Jzon.cpp ../src/Filter.cpp ../src/Event.cpp ../src/Frame.cpp ../src/IOInterface.cpp ../src/AVFramedQueue.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/Runnable.cpp
headDemuxerTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
headDemuxerTest_CXXFLAGS = -std=c++11
headDemuxerTest_LDFLAGS = -llog4cplus -lcppunit -lpthread -lavcodec -lavformat -lavutil

headDemuxerFunctionalTest_SOURCES = ../src/Utils.cpp HeadDemuxerFunctionalTest.cpp ../src/modules/headDemuxer/HeadDemuxerLibav.cpp ../src/Jzon.cpp ../src/Filter.cpp ../src/Event.cpp ../src/Frame.cpp ../src/IOInterface.cpp ../src/AVFramedQueue.cpp ../src/VideoFrame.cpp ../src/AudioFrame.cpp ../src/modules/videoDecoder/VideoDecoderLibav.cpp ../src/modules/audioDecoder/AudioDecoderLibav.cpp ../src/AudioCircularBuffer.cpp ../src/Runnable.cpp
headDemuxerFunctionalTest_CPPFLAGS = -g -Wall -g -D__STDC_CONSTANT_MACROS -I../src -I.
headDemuxerFunctionalTest_CXXFLAGS = -std=c++11
headDemuxerFunctionalTest_LDFLAGS = -llog4cplus -lcppunit -lpthread -lavcodec -lavformat -lavutil -lswresample
