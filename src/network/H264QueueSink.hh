#ifndef _H264_QUEUE_SINK_HH
#define _H264_QUEUE_SINK_HH

#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#ifndef _QUEUE_SINK_HH
#include "QueueSink.hh"
#endif


class H264QueueSink: public QueueSink {
    
public:
    static H264QueueSink* createNew(UsageEnvironment& env, FrameQueue* queue, 
                                    char const* sPropParameterSetsStr);
    
protected:
    H264QueueSink(UsageEnvironment& env, FrameQueue* queue, 
                  char const* sPropParameterSetsStr);
    virtual Boolean continuePlaying();
    
    virtual void afterGettingFrame(unsigned frameSize, struct timeval presentationTime);
    
private:
   
    char const* fSPropParameterSetsStr;
    Boolean fHaveWrittenFirstFrame;
};

#endif