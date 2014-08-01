AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = liblivemediastreamer.la
liblivemediastreamer_la_SOURCES = modules/audioDecoder/AudioDecoderLibav.cpp \
                                  modules/audioEncoder/AudioEncoderLibav.cpp \
                                  modules/audioMixer/AudioMixer.cpp \
                                  modules/videoDecoder/VideoDecoderLibav.cpp \
                                  modules/videoEncoder/VideoEncoderX264.cpp \
                                  modules/videoMixer/VideoMixer.cpp \
                                  modules/videoResampler/VideoResampler.cpp \
                                  modules/liveMediaInput/ExtendedRTSPClient.cpp \
                                  modules/liveMediaInput/H264QueueSink.cpp \
                                  modules/liveMediaInput/Handlers.cpp \
                                  modules/liveMediaInput/QueueSink.cpp \
                                  modules/liveMediaInput/SourceManager.cpp \
                                  modules/liveMediaOutput/AudioQueueServerMediaSubsession.cpp \
                                  modules/liveMediaOutput/H264QueueServerMediaSubsession.cpp \
                                  modules/liveMediaOutput/H264QueueSource.cpp \
                                  modules/liveMediaOutput/JPEGQueueServerMediaSubsession.cpp \
                                  modules/liveMediaOutput/QueueServerMediaSubsession.cpp \
                                  modules/liveMediaOutput/QueueSource.cpp \
                                  modules/liveMediaOutput/SinkManager.cpp \
                                  modules/liveMediaOutput/VP8QueueServerMediaSubsession.cpp \
                                  AVFramedQueue.cpp \
                                  AudioCircularBuffer.cpp \
                                  AudioFrame.cpp \
                                  Controller.cpp \
                                  Event.cpp \
                                  Filter.cpp \
                                  Frame.cpp \
                                  FrameQueue.cpp \
                                  IOInterface.cpp \
                                  Jzon.cpp \
                                  Path.cpp \
                                  PipelineManager.cpp \
                                  Utils.cpp \
                                  VideoFrame.cpp \
                                  Worker.cpp \
                                  X264VideoCircularBuffer.cpp \
                                  X264VideoFrame.cpp 

liblivemediastreamer_la_CPPFLAGS = -std=c++11 -g -D__STDC_CONSTANT_MACROS -I/usr/local/include/opencv -I/usr/local/include/groupsock -I/usr/local/include/liveMedia -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -Wall

liblivemediastreamer_la_LDFLAGS = -shared -fPIC -pthread -lBasicUsageEnvironment -lUsageEnvironment -lliveMedia -lgroupsock -lavcodec -lswresample -lswscale -llog4cplus -lopencv_core -lopencv_imgproc -lx264
