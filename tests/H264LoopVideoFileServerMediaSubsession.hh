#ifndef _H264_LOOP_VIDEO_FILE_SERVER_MEDIA_SUBSESSION_HH
#define _H264_LOOP_VIDEO_FILE_SERVER_MEDIA_SUBSESSION_HH

#include <liveMedia.hh>


class H264LoopVideoFileServerMediaSubsession : public H264VideoFileServerMediaSubsession {
public:
  static H264LoopVideoFileServerMediaSubsession*
  createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource);

protected:
  H264LoopVideoFileServerMediaSubsession(UsageEnvironment& env,
                                      char const* fileName, Boolean reuseFirstSource);
  
  virtual ~H264LoopVideoFileServerMediaSubsession(){};

protected: // redefined virtual functions
  FramedSource* createNewStreamSource(unsigned clientSessionId,
                                              unsigned& estBitrate);
};

#endif