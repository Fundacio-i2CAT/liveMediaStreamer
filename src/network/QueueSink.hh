#ifndef _QUEUE_SINK_HH
#define _QUEUE_SINK_HH

#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#ifndef _FRAME_QUEUE_HH
#include "../FrameQueue.hh"
#endif

#ifndef _FRAME_HH
#include "../Frame.hh"
#endif

class QueueSink: public MediaSink {

public:
    static QueueSink* createNew(UsageEnvironment& env, FrameQueue* queue);

protected:
    QueueSink(UsageEnvironment& env, FrameQueue* queue);


protected: 
    virtual Boolean continuePlaying();

protected:
    static void afterGettingFrame(void* clientData, unsigned frameSize,
                unsigned numTruncatedBytes,
                struct timeval presentationTime,
                unsigned durationInMicroseconds);
    virtual void afterGettingFrame(unsigned frameSize, struct timeval presentationTime);

    void updateFrame();
    
    FrameQueue* queue;
    Frame *frame;
};

#endif