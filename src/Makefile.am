AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = liblivemediastreamer.la
liblivemediastreamer_la_SOURCES = modules/audioDecoder/AudioDecoderLibav.cpp \
                                  modules/audioEncoder/AudioEncoderLibav.cpp \
                                  modules/audioMixer/AudioMixer.cpp \
                                  modules/videoDecoder/VideoDecoderLibav.cpp \
                                  modules/videoEncoder/VideoEncoderX264.cpp \
                                  modules/videoEncoder/VideoEncoderX265.cpp \
                                  modules/videoEncoder/VideoEncoderX264or5.cpp \
                                  modules/videoMixer/VideoMixer.cpp \
                                  modules/videoSplitter/VideoSplitter.cpp \
                                  modules/videoResampler/VideoResampler.cpp \
                                  modules/decoderPi/DecoderPi.cpp \
				  modules/dasher/Dasher.cpp \
                                  modules/dasher/DashVideoSegmenter.cpp \
                                  modules/dasher/DashVideoSegmenterAVC.cpp \
                                  modules/dasher/DashVideoSegmenterHEVC.cpp \
                                  modules/dasher/DashAudioSegmenter.cpp \
                                  modules/dasher/MpdManager.cpp \
                                  modules/dasher/i2libdash.c \
                                  modules/dasher/i2libisoff.c \
                                  modules/receiver/ExtendedRTSPClient.cpp \
                                  modules/receiver/Handlers.cpp \
                                  modules/receiver/QueueSink.cpp \
                                  modules/receiver/SourceManager.cpp \
                                  modules/receiver/H264VideoSdpParser.cpp \
                                  modules/transmitter/AudioQueueServerMediaSubsession.cpp \
                                  modules/transmitter/H264or5QueueServerMediaSubsession.cpp \
                                  modules/transmitter/H264QueueServerMediaSubsession.cpp \
                                  modules/transmitter/H265QueueServerMediaSubsession.cpp \
                                  modules/transmitter/H264or5QueueSource.cpp \
                                  modules/transmitter/MPEGTSQueueServerMediaSubsession.cpp \
                                  modules/transmitter/QueueServerMediaSubsession.cpp \
                                  modules/transmitter/QueueSource.cpp \
                                  modules/transmitter/SinkManager.cpp \
                                  modules/transmitter/VP8QueueServerMediaSubsession.cpp \
                                  modules/transmitter/Connection.cpp \
                                  modules/transmitter/H264VideoStreamSampler.cpp \
                                  modules/transmitter/H264or5StartCodeInjector.cpp \
                                  modules/transmitter/UltraGridAudioRTPSink.cpp \
                                  modules/transmitter/UltraGridVideoRTPSink.cpp \
                                  modules/transmitter/ADTSQueueServerMediaSubsession.cpp \
                                  modules/transmitter/ADTSStreamParser.cpp \
                                  modules/transmitter/CustomMPEG4GenericRTPSink.cpp \
                                  modules/transmitter/SPSparser/h264_stream.c \
                                  modules/sharedMemory/SharedMemory.cpp \
                                  modules/headDemuxer/HeadDemuxerLibav.cpp \
                                  AVFramedQueue.cpp \
                                  AudioCircularBuffer.cpp \
                                  SlicedVideoFrameQueue.cpp \
                                  AudioFrame.cpp \
                                  Controller.cpp \
                                  Event.cpp \
                                  Filter.cpp \
                                  Frame.cpp \
                                  IOInterface.cpp \
                                  Jzon.cpp \
                                  Path.cpp \
                                  PipelineManager.cpp \
                                  Utils.cpp \
                                  VideoFrame.cpp \
                                  Runnable.cpp \
                                  WorkersPool.cpp 

liblivemediastreamer_la_CXXFLAGS = -std=c++11 

liblivemediastreamer_la_CFLAGS = -g -D__STDC_CONSTANT_MACROS -Wall -O0

liblivemediastreamer_la_CPPFLAGS = -g -D__STDC_CONSTANT_MACROS -Wall -O0

liblivemediastreamer_la_LDFLAGS = -shared -fPIC -pthread -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lavformat -lavutil -lswresample -lswscale -llog4cplus -lopencv_core -lopencv_imgproc -lx264 -lx265 -L/opt/vc/lib -lbcm_host -lvcos -lvchiq_arm -lpthread -lEGL -lGLESv2 -lopenmaxil -lilclient
